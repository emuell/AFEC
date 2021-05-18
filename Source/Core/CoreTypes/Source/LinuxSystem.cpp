#include "CoreTypesPrecompiledHeader.h"

#include <unistd.h>
#include <sched.h>
#include <signal.h> 

#include <sys/unistd.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h> 
#include <sys/wait.h>

#include <dirent.h>
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>

#include <netinet/ether.h>
#include <net/if.h>  

#include <cstring>
#include <cstdio>
#include <cerrno>

#include <string>
#include <vector>
#include <algorithm>

// =================================================================================================

// enable to trace reaping of child process zombies. use for debugging only!
// nothing is safe to do within a signal handler, so this may lock up...
// #define MTraceZombies

// =================================================================================================

int gInMain = 0;
bool gRunningAsExecutable = false;

// =================================================================================================

static std::vector<pid_t> sAutoReapChildPids;
static bool sProcessKilled = false;

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static void SAutoReapChildHook(int)
{
  // NB: don't log or do anything else here. this easily may create deadlocks,
  // especially with malloc and free

  // reap registered child processes from TSystem::LaunchProcess
  for (std::vector<pid_t>::iterator Iter = sAutoReapChildPids.begin();
       Iter != sAutoReapChildPids.end();
       ++Iter)
  {
    if ((::waitpid(*Iter, NULL, WNOHANG)) > 0)
    {
      #if defined(MTraceZombies)
        MTrace1("System: Child process %d terminated. Reaping zombie...", (int)*Iter);
      #endif

      sAutoReapChildPids.erase(Iter);
      return;
    }
  }

  // and ignore all others
  #if defined(MTraceZombies)
    MTrace("System: Ignoring SIGCHLD for a child process");
  #endif
}
    
// =================================================================================================

// Global (and shared library) entry/exit points

// -------------------------------------------------------------------------------------------------

void __attribute__ ((constructor)) gInit()
{
  // set locale to C, to emulate the default Windows behavior
  if (::setlocale(LC_ALL, "C") == NULL)
  {
    MInvalid("setlocale failed");
    ::fprintf(stderr, "warning: could not set default locale to 'C'\n");
  }

  ++gInMain;
  
  // do not try to run API initializers here. This is called 
  // before other base project's statics got initialized.
}

// -------------------------------------------------------------------------------------------------

void __attribute__ ((destructor)) gExit()
{
  // do not try to cleanly release the API here. This is called after the base
  // project's statics got released.
  
  --gInMain;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TString TSystem::GetOsAsString()
{
  FILE *f = fopen ("/proc/version", "r");

  if (f)
  {
    TString Ret;
    
    char line[100];
    line[sizeof(line) - 1] = 0;
    
    while (fgets(line, sizeof(line) - 1, f) != NULL) 
    {
      Ret += TString(line, TString::kPlatformEncoding);
    }
  
    fclose(f);
    
    return Ret;
  }
  
  return TString("Unknown Linux Version ");
}

// -------------------------------------------------------------------------------------------------

bool TSystem::Is64BitOs()
{
  #if defined(MArch_X64)
    return true; // definitely is

  #elif defined(MArch_X86)
    return TSystem::Is32BitProcessIn64BitOs(); // maybe is

  #else
    #error "unknown architecture"
  #endif
}

// -------------------------------------------------------------------------------------------------

bool TSystem::Is32BitProcessIn64BitOs()
{
  #if defined(MArch_X64)
    return false;

  #elif defined(MArch_X86)
    static int s64BitOs = -1;
  
    if (s64BitOs == -1)
    {
      // test arch of '/sbin/init' (the root process) or '/bin/sh'
      TList<TSystem::TArchInfo> ArchInf;
      if (TFile("/sbin/init").Exists())
      {
        ArchInf = ExecutableArchitectures("/sbin/init");
      }
      else if (TFile("/bin/sh").Exists())
      {
        ArchInf = ExecutableArchitectures("/bin/sh");
      }

      if (! ArchInf.IsEmpty())
      {
        if (ArchInf.First().mCpuType == TArchInfo::kX86_64)
        {
          TLog::SLog()->AddLine("System", 
            "Running as 32bit process in a 64bit kernel...");

          s64BitOs = 1;
        }
        else if (ArchInf.First().mCpuType == TArchInfo::kI386)
        {
          TLog::SLog()->AddLine("System", 
            "Running as 32bit process in a 32bit kernel...");

          s64BitOs = 0;
        }
        else
        {
          TLog::SLog()->AddLine("System", 
            "WARNING: failed to find out if we're running in a 64bit "
            "kernel (unexpected '/sbin/init' arch flags)");

          s64BitOs = 0;
        }
      }
      else
      {
        TLog::SLog()->AddLine("System", 
          "WARNING: failed to find out if we're running in a 64bit "
          "kernel (can't access '/sbin/init' nor '/bin/sh')");

        s64BitOs = 0;
      }
    }

    return !!s64BitOs;

  #else
    #error "unknown architecture"
  #endif
}

// -------------------------------------------------------------------------------------------------

bool TSystem::InMain()
{
  if (gRunningAsExecutable)
  {
    // gInit() and main() must be called
    return (gInMain > 1);
  }
  else
  {
    // gInit() will be called only
    return (gInMain > 0);
  }
}

// -------------------------------------------------------------------------------------------------

bool TSystem::RunningInDebugger()
{
  // assuming that we are only debuged when invoked via gdb. 
  // lame, but will work in most cases...
  static int sRunningInDebugger = -1;

  if (sRunningInDebugger == -1)
  {
    sRunningInDebugger = 0;
    
    char CmdLinePath[32];
    snprintf(CmdLinePath, sizeof(CmdLinePath), "/proc/%d/cmdline", ::getppid()); 
  
    FILE* pProcFile = ::fopen(CmdLinePath, "r");

    if (pProcFile != NULL) 
    {
      char Line[1024];
       const char* pLineResult = ::fgets(Line, sizeof(Line), pProcFile);
      
      sRunningInDebugger = (::strcmp(pLineResult, "gdb") == 0 || 
        ::strcmp(pLineResult, "/usr/bin/gdb") == 0);
    
      ::fclose(pProcFile);
    }
  }
   
  return !!sRunningInDebugger;
}

// -------------------------------------------------------------------------------------------------

bool TSystem::RunningAsPlugin()
{
  return !gRunningAsExecutable;
}

// -------------------------------------------------------------------------------------------------

TList<TString> TSystem::RunningProcesses()
{
  TList<TString> Ret;

  DIR* pProcDir = ::opendir("/proc");

  if (!pProcDir)
  {
    MInvalid("Can't open /proc");
    return Ret;
  }

  size_t PathBufferSize = PATH_MAX - 1;
  char* pPath = (char*)::malloc(PathBufferSize);
  
  if (pPath == NULL) 
  {
    throw TOutOfMemoryException();
  }
  
  char* pPath2 = (char*)::malloc(PathBufferSize);
  
  if (pPath2 == NULL) 
  {
    ::free(pPath); 
    throw TOutOfMemoryException();
  }

  struct dirent* pDirEntry;
  
  while ((pDirEntry = ::readdir(pProcDir)) != NULL) 
  {
    if (!::isdigit(*pDirEntry->d_name))
    {
      continue;
    }

    ::snprintf(pPath2, PathBufferSize - 1, "/proc/%s/exe", pDirEntry->d_name);

    pPath[0] = '\0';

    for (MEver) 
    {
      // read the exe link
      ssize_t LinkBuferSize = ::readlink(pPath2, pPath, PathBufferSize - 1);
      
      if (LinkBuferSize == -1) 
      {
        break;
      }

      pPath[LinkBuferSize] = '\0';

      // check whether the symlink's target is also a symlink.
      struct stat StatBuffer;
      
      if (::stat(pPath, &StatBuffer) == -1) 
      {
        // error
        break; 
      }

      if (!S_ISLNK(StatBuffer.st_mode)) 
      {
        // done
        break;
      }

      // path is a symlink. continue to resolve...
      ::strncpy(pPath, pPath2, PathBufferSize - 1);
    }

    if (::strlen(pPath))
    {
      Ret.Append(TString(pPath, TString::kFileSystemEncoding));
    }
  }

  ::free(pPath);
  ::free(pPath2);

  ::closedir(pProcDir);

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TString TSystem::MacAdress()
{
  int sock_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    
  if (sock_fd == -1) 
  { 
    ::perror("Failed to create a socket");
    return TString("00:00:00:00:00:00");
  }
  
  static const char* pEthNames[] = {"eth0", "eth1", "eth2"};
    
  struct ifreq ifr;
  bool IoctlSucceeded = false;
        
  for (size_t i = 0; i < sizeof(pEthNames) / sizeof(pEthNames[0]); ++i)  
  {
    ::strcpy(ifr.ifr_name, pEthNames[i]);
    
    if (::ioctl(sock_fd, SIOCGIFHWADDR, &ifr) == 0) 
    { 
      IoctlSucceeded = true;
      break;
    }
  }
    
  ::close(sock_fd);
    
  if (!IoctlSucceeded)
  {
   ::perror("ioctl SIOCGIFHWADDR failed");
    return TString("00:00:00:00:00:00");
  }
  
  struct __attribute__((__may_alias__)) ether_addr_a {
       u_char ether_addr_octet[ETHER_ADDR_LEN];
  };

  struct ether_addr_a* pAddr = (struct ether_addr_a*)
    ifr.ifr_hwaddr.sa_data;
    
  char MacAdress[1024] = {0};
    
  ::snprintf(MacAdress, sizeof(MacAdress), "%02x:%02x:%02x:%02x:%02x:%02x", 
    pAddr->ether_addr_octet[0], pAddr->ether_addr_octet[1], 
    pAddr->ether_addr_octet[2], pAddr->ether_addr_octet[3], 
    pAddr->ether_addr_octet[4], pAddr->ether_addr_octet[5]);
  
  return TString(MacAdress);
}
  
// -------------------------------------------------------------------------------------------------

unsigned int TSystem::TimeInMsSinceStartup()
{
  struct timeval tv;
  ::gettimeofday(&tv, NULL);
  
  return (unsigned int)tv.tv_usec; 
}

// -------------------------------------------------------------------------------------------------

bool TSystem::SwitchToOtherThread()
{
  return (::sched_yield() == 0);
}

// -------------------------------------------------------------------------------------------------

void TSystem::Sleep(int Ms)
{
  // windows sleep(0) behaviour
  if (Ms == 0)
  {
    ::sched_yield();
  }
  else
  {
    #define MICROS_PER_SECOND 1000000LL
    #define NANOS_PER_MICRO 1000LL
    #define MICROS_PER_MILLI 1000LL

    const long long microseconds = MICROS_PER_MILLI * Ms;

    timespec ts;
    ts.tv_sec = (long)(microseconds / MICROS_PER_SECOND);
    ts.tv_nsec = (long)((microseconds % MICROS_PER_SECOND) * NANOS_PER_MICRO);
    
    ::nanosleep(&ts, NULL); 
  }
}

// -------------------------------------------------------------------------------------------------

bool TSystem::SetProcessPriority(TProcessPriority NewPriority)
{
  switch (NewPriority)
  {
  default: 
    MInvalid("Unknown priority");

  case kPriorityHigh:
  case kPriorityNormal:
  case kPriorityLow:
    MCodeMissing;
    break;
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

int TSystem::CurrentProcessId()
{
  return (int)::getpid();
}

// -------------------------------------------------------------------------------------------------

int TSystem::LaunchProcess(
  const TString&          FileName, 
  const TList<TString>&   Args,
  bool                    WaitTilFinished)
{
  if (!TFile(FileName).Exists())
  {
    MInvalid("Should validate the executable's filename before launching");
    return (WaitTilFinished) ? EXIT_FAILURE : kInvalidProcessId;
  }
  
  // install auto-reap hook
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_handler = SAutoReapChildHook;
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sa, NULL);

  // log
  TString DebugString = "Executing '" + FileName;
  for (int i = 0; i < Args.Size(); ++i)
  {
    DebugString += " " + Args[i];
  }
  DebugString += "'";
  TLog::SLog()->AddLineNoVarArgs("System", DebugString.CString());

  // fork
  int ProcessId = ::fork();

  if (ProcessId == 0)
  {
    // > new child process
    
    // suppress stdout and stderr to avoid that they get redirected to the parent
    const int FdNull = ::open("/dev/null", O_WRONLY);
    ::dup2(FdNull, 1); // stdout
    ::dup2(FdNull, 2); // stderr
    ::close(FdNull);

    // convert TString args to cstrings
    TArray<char> FileChars;
    const char* pFile = FileName.CString(TString::kFileSystemEncoding);
    FileChars.SetSize(::strlen(pFile) + 1);
    ::strcpy(FileChars.FirstWrite(), pFile);

    TList< TArray<char> > ArgCharsList;
    ArgCharsList.PreallocateSpace(Args.Size());
    for (int i = 0; i < Args.Size(); ++i)
    {
      TArray<char> ArgChars;
      const char* pArg = Args[i].CString(TString::kUtf8);
      ArgChars.SetSize(::strlen(pArg) + 1);
      ::strcpy(ArgChars.FirstWrite(), pArg);
      
      ArgCharsList.Append(ArgChars);
    }
    
    TList<char*> ArgCharPtrs;
    ArgCharPtrs.PreallocateSpace(Args.Size() + 2);
    ArgCharPtrs.Append(FileChars.FirstWrite());
    for (int i = 0; i < Args.Size(); ++i)
    {
      ArgCharPtrs.Append(ArgCharsList[i].FirstWrite());
    }
    ArgCharPtrs.Append(NULL);
    
    // exec
    ::execve(ArgCharPtrs[0], ArgCharPtrs.FirstRead(), environ);
    
    return ::raise(SIGKILL); // no exit (will crash)
  }
  else if (ProcessId > 0)
  {
    // > parent process
    if (WaitTilFinished)
    {
      int WaitStatus = 0;
      ProcessId = ::waitpid(ProcessId, &WaitStatus, 0);
      
      if (WIFEXITED(WaitStatus))
      {
        return WEXITSTATUS(WaitStatus);
      }
      else
      {
        MInvalid("Process was killed or terminated anormaly")
        return EXIT_FAILURE;
      }
    }    
    else
    {
      // auto-reap it as soon as it gets killed to avoid zombies
      sAutoReapChildPids.push_back(ProcessId);

      // return process id when not waiting for the process to finish
      return ProcessId;
    }
  }
  else
  {
    TLog::SLog()->AddLine("System",
      "Failed to fork a child process (error %d).", errno);
      
    return (WaitTilFinished) ? EXIT_FAILURE : kInvalidProcessId;
  }
}

// -------------------------------------------------------------------------------------------------

bool TSystem::ProcessIsRunning(int ProcessId)
{
  char ProcPath[128];
  ::snprintf(ProcPath, sizeof(ProcPath), "/proc/%d", ProcessId);

  struct stat dirInfo;
  if (::stat(ProcPath, &dirInfo) == 0) 
  {
    return true; // proc entry exists
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

bool TSystem::KillProcess(int ProcessId)
{
  return (::kill(ProcessId, SIGKILL) == 0);
}

// -------------------------------------------------------------------------------------------------

bool TSystem::ProcessKilled()
{
   return sProcessKilled;
}

// -------------------------------------------------------------------------------------------------

void TSystem::KillProcess()
{
  TSystemGlobals::TPProcessKilledHandlerList& ProcessKilledHandlers =
    TSystemGlobals::SSystemGlobals()->mProcessKilledHandlers;

  TSystemGlobals::TPProcessKilledHandlerList::const_iterator Iter = 
    ProcessKilledHandlers.begin();

  for (; Iter != ProcessKilledHandlers.end(); ++Iter)
  {
    (*Iter)();
  }

  sProcessKilled = true;
  ::kill(0, SIGKILL);
}

// -------------------------------------------------------------------------------------------------

TList<TSystem::TArchInfo> TSystem::ExecutableArchitectures(const TString& FileName)
{
  MAssert(gFileExists(FileName), "File must exist!");

  try
  {
    TFile File(FileName);
    if (! File.Open(TFile::kRead))
    {
      throw TReadableException("Failed to open the file!");
    }

    TArchInfo Info;  
    Info.mCpuType = TArchInfo::kUnknown;
    Info.mOffset = 0;
    Info.mLength = TFile(FileName).SizeInBytes();

    // need to byte swap below when this is fired:
    MStaticAssert(MSystemByteOrder == MIntelByteOrder);

    // Elf32_Ehdr and ELF64_Ehdr are identical for the necessary fields:
    Elf32_Ehdr ElfHeader32;
    File.Read((char*)&ElfHeader32, sizeof(ElfHeader32));

    if (ElfHeader32.e_ident[0] != ELFMAG0 ||
        ElfHeader32.e_ident[1] != ELFMAG1 ||
        ElfHeader32.e_ident[2] != ELFMAG2 ||
        ElfHeader32.e_ident[3] != ELFMAG3 ||
        ElfHeader32.e_ident[EI_CLASS] < ELFCLASS32 ||
        ElfHeader32.e_ident[EI_CLASS] > ELFCLASS64)
    {
      // not an ELF object file, or not supported
      return MakeList<TArchInfo>(Info);
    }
    else
    {
      switch (ElfHeader32.e_machine)
      {
      case EM_PPC:
        Info.mCpuType = TArchInfo::kPPC;
        break;
      case EM_386:
        Info.mCpuType = TArchInfo::kI386;
        break;
      case EM_X86_64:
        Info.mCpuType = TArchInfo::kX86_64;
        break;

      default:
        // unknown or unsupported machine
        break;
      }

      return MakeList<TArchInfo>(Info);
    }
  }
  catch (const TReadableException& Exception)
  { 
    MInvalid(Exception.Message().CString()); MUnused(Exception);
    return TList<TArchInfo>();
  }
}

// -------------------------------------------------------------------------------------------------

void* TSystem::ApplicationHandle()
{
  return TSystemGlobals::SSystemGlobals()->mAppHandle;
}

// -------------------------------------------------------------------------------------------------

void TSystem::SetApplicationHandle(void* AppHandle)
{
  TSystemGlobals::SSystemGlobals()->mAppHandle = AppHandle;
}

// -------------------------------------------------------------------------------------------------

bool TSystem::ShowHiddenFilesAndFolders()
{
  return true;
}

// -------------------------------------------------------------------------------------------------

TString TSystem::ApplicationPathAndFileName(bool ResolveDllName)
{
  // cached, as this is called very frequently
  static std::string sApplicationPathAndFileName_Dll;
  static std::string sApplicationPathAndFileName_Exe;
  
  if (ResolveDllName)
  {
    if (! sApplicationPathAndFileName_Dll.empty())
    {
      return TString(sApplicationPathAndFileName_Dll.c_str(), 
        TString::kFileSystemEncoding);
    }
  }
  else
  {
    if (! sApplicationPathAndFileName_Exe.empty())
    {
      return TString(sApplicationPathAndFileName_Exe.c_str(), 
        TString::kFileSystemEncoding);
    }
  }

  TString Ret;
  

  // ... Try Dl_info for shared libraries first
  
  if (ResolveDllName && !gRunningAsExecutable)
  {
    Dl_info dl_info; 
    TMemory::Zero(&dl_info, sizeof(Dl_info));
  
    if (::dladdr((void*)ApplicationPathAndFileName, &dl_info) && 
          dl_info.dli_fname != NULL)
    {
      Ret = TString(dl_info.dli_fname, TString::kFileSystemEncoding);

      TLog::SLog()->AddLine("System", "Application path retrieved from dladdr: '%s'",
        Ret.CString());

      MAssert(TFile(Ret).Exists(), "Invalid dl_info path");
    }
  }

  
  // ... Read from /proc/self/exe (symlink)   
  
  if (Ret.IsEmpty())
  {  
    size_t PathBufferSize = PATH_MAX - 1;
    char* pPath = (char*)::malloc(PathBufferSize);
    
    if (pPath == NULL) 
    {
      throw TOutOfMemoryException();
    }
    
    char* pPath2 = (char*)::malloc(PathBufferSize);
    
    if (pPath2 == NULL) 
    {
      ::free(pPath); 
      throw TOutOfMemoryException();
    }

    ::strncpy(pPath2, "/proc/self/exe", PathBufferSize - 1);

    for (MEver) 
    {
      int i;

      ssize_t LinkBuferSize = ::readlink(pPath2, pPath, PathBufferSize - 1);
      
      if (LinkBuferSize == -1) 
      {
        ::free(pPath2);
        pPath2 = NULL;
        break;
      }

      // readlink() success.
      pPath[LinkBuferSize] = '\0';

      // Check whether the symlink's target is also a symlink.
      // We want to get the final target.
      struct stat StatBuffer;
      i = ::stat(pPath, &StatBuffer);
      
      if (i == -1) 
      {
        ::free(pPath2);
        pPath2 = NULL;
        break;
      }

      // stat() success.
      if (!S_ISLNK(StatBuffer.st_mode)) 
      {
        // path is not a symlink. Done.
        ::free(pPath2);
        pPath2 = NULL;
        
        Ret = TString(pPath, TString::kFileSystemEncoding); 

        ::free(pPath);
        pPath = NULL;
    
        // remove memory disk prefixes
        if (Ret.StartsWith("/cow"))
        {
          Ret.RemoveFirst("/cow");
        }
        else if (Ret.StartsWith("/rofs"))
        {
          Ret.RemoveFirst("/rofs");
        }

        // found path
        TLog::SLog()->AddLine("System", "Application path retrieved from /proc/self/exe: '%s'",
          Ret.CString());

        break;
      }

      // path is a symlink. Continue loop and resolve this.
      ::strncpy(pPath, pPath2, PathBufferSize - 1);
    }
      
    ::free(pPath);
    pPath = NULL;
  }
    

  // ... Read from /proc/self/maps as fallback.
  if (Ret.IsEmpty())
  {
    // (readlink() or stat() failed; this can happen when the program is
    // running in Valgrind 2.2.)
   
    size_t PathBufferSize = PATH_MAX + 128;
    char* pLine = (char*)::malloc(PathBufferSize);
      
    if (pLine == NULL) 
    {
      throw TOutOfMemoryException();
    }

    FILE* pMapsFile = ::fopen("/proc/self/maps", "r");
    
    if (pMapsFile == NULL) 
    {
      ::free(pLine);
      
      throw TReadableException(MText("Failed to resolve the application path "
        "(Read of /proc/self/maps failed)!"));
    }

    // The first entry should be the executable name.
    char* pLineResult = ::fgets(pLine, (int)PathBufferSize, pMapsFile);
    
    if (pLineResult == NULL) 
    {
      ::fclose(pMapsFile);
      free(pLine);

      throw TReadableException(MText("Failed to resolve the application path"
        " (Read of /proc/self/maps fgets failed)!"));
    }

    // Get rid of newline character.
    PathBufferSize = ::strlen(pLine);
    
    if (PathBufferSize <= 0) 
    {
      // Huh? An empty string?
      ::fclose(pMapsFile);
      ::free(pLine);

      throw TReadableException(MText("Failed to resolve the application path"
        " (Read of /proc/self/maps is empty)!"));
    }
    
    if (pLine[PathBufferSize - 1] == 10)
    {
      pLine[PathBufferSize - 1] = 0;
    }
    
    // Extract the filename; it is always an absolute path.
    char* pPath = ::strchr(pLine, '/');

    // Sanity check.    
    if (::strstr(pLine, " r-xp ") == NULL || pPath == NULL) 
    {
      ::fclose(pMapsFile);
      ::free(pLine);
      
      throw TReadableException(MText("Failed to resolve the application path"
        " (Read of /proc/self/maps is invalid)!"));
    }

    pPath = ::strdup(pPath);
    ::free(pLine);
    ::fclose(pMapsFile);

    Ret = TString(pPath, TString::kFileSystemEncoding); 
    ::free(pPath);
    
    // remove memory disk prefixes
    if (Ret.StartsWith("/cow"))
    {
      Ret.RemoveFirst("/cow");
    }
    else if (Ret.StartsWith("/rofs"))
    {
      Ret.RemoveFirst("/rofs");
    }

    TLog::SLog()->AddLine("System", "Application path retrieved from /proc/self/maps: '%s'",
      Ret.CString());
  }
    
  // cache the result...
  MAssert(! Ret.IsEmpty(), "Expected a valid path here")

  if (ResolveDllName)  
  {
    sApplicationPathAndFileName_Dll = Ret.CString(TString::kFileSystemEncoding);
  }
  else
  {
    sApplicationPathAndFileName_Exe = Ret.CString(TString::kFileSystemEncoding);
  }
  
  return Ret;
}

// -------------------------------------------------------------------------------------------------

bool TSystem::WindowControlsAreLeft()
{
  return false;
}

// -------------------------------------------------------------------------------------------------

TString TSystem::DefaultFileViewerName()
{
  return TString("File Explorer");
}
  
// -------------------------------------------------------------------------------------------------

bool TSystem::OpenURL(const TString& Url)
{
  // create a command that tries to launch a bunch of likely browsers, using
  // xdg-open first, which should choose the users default (when installed)
  static const char* const spBrowserNames[] = { 
    "xdg-open",
    "/etc/alternatives/x-www-browser", 
    "firefox", 
    "google-chrome",
    "chromium-browser",
    "mozilla", 
    "konqueror", 
    "opera" 
  };

  const int sNumBrowserNames = (int)(sizeof(spBrowserNames) / sizeof(spBrowserNames[0]));
  
  TString CmdString;
  
  for (int i = 0; i < sNumBrowserNames; ++i)
  {
    CmdString.Append(TString(spBrowserNames[i]) + " '" + TString(Url).Strip() + "'");
    
    if (i != sNumBrowserNames - 1)
    {
      CmdString.Append(" || ");
    }
  }
  
  const bool WaitTilFinished = false;
  return LaunchProcess("/bin/sh", MakeList<TString>("-c", CmdString), WaitTilFinished);
}

// -------------------------------------------------------------------------------------------------

bool TSystem::OpenPath(const TDirectory& Directory, const TString& SelectFilename)
{
  // create a command that tries to launch a bunch of likely Explorers, using
  // xdg-open first, which should choose the users default (when installed)
  static const char* const spExplorerNames[] = { 
    "xdg-open",
    "thunar", 
    "nautilus", 
    "konqueror", 
    "rox" 
  };

  static const int sNumExplorerNames = (int)(sizeof(spExplorerNames) / sizeof(spExplorerNames[0]));
  
  TString CmdString;
  
  for (int i = 0; i < sNumExplorerNames; ++i)
  {
    CmdString.Append(TString(spExplorerNames[i]) + " '" + 
      Directory.Path().Replace("'", "\\'") + "'");
    
    if (i != sNumExplorerNames - 1)
    {
      CmdString.Append(" || ");
    }
  }

  const bool WaitTilFinished = false;
  return LaunchProcess("/bin/sh", MakeList<TString>("-c", CmdString), WaitTilFinished);
}

