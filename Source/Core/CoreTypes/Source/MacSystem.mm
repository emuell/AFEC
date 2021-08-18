#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Version.h"

#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>

#include <cstdio>
#include <clocale>

#include <list>
#include <algorithm>

#include <sched.h>

#include <mach/mach_host.h>
#include <mach/machine.h>

#include <mach-o/fat.h>
#include <mach-o/arch.h>
#include <mach-o/loader.h>

#include <sys/sysctl.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <objc/message.h> 

#include <IOKit/IOKitLib.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkInterface.h>
#include <IOKit/network/IOEthernetController.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

#include <Carbon/Carbon.h> // for AESend and TISInputSourceRef

// =================================================================================================

int gInMain = 0;
bool gRunningAsExecutable = false;

// =================================================================================================

static bool sProcessKilled = false;

// -------------------------------------------------------------------------------------------------

//! Returns an iterator containing the primary (built-in) Ethernet interface. 
//! The caller is responsible for releasing the iterator after the caller is done with it.

static kern_return_t SFindEthernetInterfaces(io_iterator_t *matchingServices)
{
  kern_return_t kernResult; 
  CFMutableDictionaryRef matchingDict;
  CFMutableDictionaryRef propertyMatchDict;
    
  // Ethernet interfaces are instances of class kIOEthernetInterfaceClass. 
  // IOServiceMatching is a convenience function to create a dictionary with 
  // the key kIOProviderClassKey and the specified value.
  matchingDict = IOServiceMatching(kIOEthernetInterfaceClass);

  // Note that another option here would be:
  // matchingDict = IOBSDMatching("en0");
        
  if (NULL == matchingDict) 
  {
    MInvalid("IOServiceMatching returned a NULL dictionary.");
  }
  else 
  {
    // Each IONetworkInterface object has a Boolean property with the key 
    // kIOPrimaryInterface. Only the primary (built-in) interface has this 
    // property set to TRUE.

    // IOServiceGetMatchingServices uses the default matching criteria defined
    // by IOService. This considers only the following properties plus any 
    // family-specific matching in this order of precedence 
    // (see IOService::passiveMatch):
    //
    // kIOProviderClassKey (IOServiceMatching)
    // kIONameMatchKey (IOServiceNameMatching)
    // kIOPropertyMatchKey
    // kIOPathMatchKey
    // kIOMatchedServiceCountKey
    // family-specific matching
    // kIOBSDNameKey (IOBSDNameMatching)
    // kIOLocationMatchKey

    // The IONetworkingFamily does not define any family-specific matching. 
    // This means that in order to have IOServiceGetMatchingServices consider 
    // the kIOPrimaryInterface property, we must add that property to a separate 
    // dictionary and then add that to our matching dictionary specifying 
    // kIOPropertyMatchKey.
            
    propertyMatchDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    if (NULL == propertyMatchDict) 
    {
      MInvalid("CFDictionaryCreateMutable returned a NULL dictionary.");
    }
    else 
    {
      // Set the value in the dictionary of the property with the given key, 
      // or add the key to the dictionary if it doesn't exist. This call retains 
      // the value object passed in.
      CFDictionarySetValue(propertyMatchDict, 
        CFSTR(kIOPrimaryInterface), kCFBooleanTrue); 
      
      // Now add the dictionary containing the matching value for kIOPrimaryInterface
      // to our main matching dictionary. This call will retain propertyMatchDict, so 
      // we can release our reference on propertyMatchDict after adding it to matchingDict.
      CFDictionarySetValue(matchingDict, 
        CFSTR(kIOPropertyMatchKey), propertyMatchDict);
      
      CFRelease(propertyMatchDict);
    }
  }
    
  // IOServiceGetMatchingServices retains the returned iterator, so release the 
  // iterator when we're done with it. IOServiceGetMatchingServices also consumes a 
  // reference on the matching dictionary so we don't need to release the dictionary 
  // explicitly.
  kernResult = IOServiceGetMatchingServices(
    kIOMasterPortDefault, matchingDict, matchingServices);    
  
  if (KERN_SUCCESS != kernResult) 
  {
    MInvalid("IOServiceGetMatchingServices failed");
  }
      
  return kernResult;
}
    
// -------------------------------------------------------------------------------------------------
  
// Given an iterator across a set of Ethernet interfaces, return the MAC address
// of the last one.
// If no interfaces are found the MAC address is set to an empty string.

static kern_return_t SGetMACAddress(
  io_iterator_t intfIterator, 
  UInt8*        MACAddress,
  UInt8         bufferSize)
{
  io_object_t intfService;
  io_object_t controllerService;
  kern_return_t kernResult = KERN_FAILURE;
    
  // Make sure the caller provided enough buffer space. 
  // Protect against buffer overflow problems.
  if (bufferSize < kIOEthernetAddressSize) 
  {
    return kernResult;
  }
  
  // Initialize the returned address
  bzero(MACAddress, bufferSize);
    
  // IOIteratorNext retains the returned object, so release it when 
  // we're done with it.
  while ((intfService = IOIteratorNext(intfIterator)))
  {
    CFTypeRef  MACAddressAsCFData;        

    // IONetworkControllers can't be found directly by the 
    // IOServiceGetMatchingServices call, since they are hardware nubs and 
    // do not participate in driver matching. In other words, registerService() 
    // is never called on them. So we've found the IONetworkInterface and will 
    // get its parent controller by asking for it specifically.
        
    // IORegistryEntryGetParentEntry retains the returned object, so release it 
    // when we're done with it.
    kernResult = IORegistryEntryGetParentEntry(intfService,
      kIOServicePlane, &controllerService);
    
    if (KERN_SUCCESS != kernResult) 
    {
      MInvalid("IORegistryEntryGetParentEntry failed");
    }
    else 
    {
      // Retrieve the MAC address property from the I/O Registry in the 
      // form of a CFData
      MACAddressAsCFData = IORegistryEntryCreateCFProperty(controllerService,
        CFSTR(kIOMACAddress), kCFAllocatorDefault, 0);
                                 
      if (MACAddressAsCFData) 
      {
        // Get the raw bytes of the MAC address from the CFData
        CFDataGetBytes((CFDataRef)MACAddressAsCFData, 
          CFRangeMake(0, kIOEthernetAddressSize), MACAddress);
                
        CFRelease(MACAddressAsCFData);
      }
                
      // Done with the parent Ethernet controller object so we release it.
      (void)IOObjectRelease(controllerService);
    }
        
    // Done with the Ethernet interface object so we release it.
    (void)IOObjectRelease(intfService);
  }
        
  return kernResult;
}

// -------------------------------------------------------------------------------------------------

static int SGetBSDProcessList(kinfo_proc** ppProcList, size_t* ProcCount)
{
  *ppProcList = NULL;
  *ProcCount = 0;

  int err;
  kinfo_proc* result = NULL;
  bool done = false;

  do
  {
    // Call sysctl with a NULL buffer to get proper length
    size_t length = 0;
    static const int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };

    err = ::sysctl((int *)name,(sizeof(name)/sizeof(*name))-1,NULL,&length,NULL,0);
    if (err == -1)
    {
      err = errno;
    }
    
    // Now, proper length is optained
    if (err == 0)
    {
      result = (kinfo_proc*)::malloc(length);

      if (result == NULL)
      {
        err = ENOMEM;   // not allocated
      }
    }

    if (err == 0)
    {
      err = ::sysctl((int *)name, 
        (sizeof(name)/sizeof(*name))-1, result, &length, NULL, 0);

      if (err == -1)
      {
        err = errno;
      }

      if (err == 0)
      {
        *ProcCount = length / sizeof(kinfo_proc);
        *ppProcList = result; // will return the result as procList

        done = true;
      }
        
      else if (err == ENOMEM)
      {
        ::free(result);
        result = NULL;
        err = 0;
      }
    }
  }
  while (err == 0 && ! done);

  // Clean up and establish post condition
  if (err != 0 && result != NULL)
  {
    ::free(result);
    result = NULL;
  }

  return err;
}

// =================================================================================================

// Global (and shared library) entry/exit points

// -------------------------------------------------------------------------------------------------

void __attribute__ ((constructor)) gInit()
{
  // To suppress startup allocation warnings. Never gets released.
  static NSAutoreleasePool* spAutoReleasePool = NULL;
  spAutoReleasePool = [[NSAutoreleasePool alloc] init];

  // setup max files limit (older OSX versions start with a limit of 25)
  rlimit maxfiles;
  maxfiles.rlim_cur = 4096;
  maxfiles.rlim_max = 4096;

  if (::setrlimit(RLIMIT_NOFILE, &maxfiles) < 0)
  {
    MInvalid("setrlimit(RLIMIT_NOFILE) failed!!");
    ::perror("setrlimit(RLIMIT_NOFILE) failed");
  }
 
  // se locale to C, to emulate the default Windows behaviour
  if (::setlocale(LC_ALL, "C") == NULL)
  {
    MInvalid("Failed to set the default locale to 'C'!");
    ::perror("Failed to set the default locale to 'C'");
  }

  ++gInMain;
  
  // do not try to run API initilalizers here. This is called 
  // before other base project's statics got initialized.
}

// -------------------------------------------------------------------------------------------------

void __attribute__ ((destructor)) gExit()
{
  // do not try to cleanly release the autorelease pool or API here. This is 
  // called after the base projects statics are released.
  --gInMain;
}

// =================================================================================================

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
  // Initialize the flags so that, if sysctl fails for some bizarre
  // reason, we get a predictable result.
  
  struct kinfo_proc info;
  info.kp_proc.p_flag = 0;
  size_t size = sizeof(info);
  
  // Initialize mib, which tells sysctl the info we want, in this case
  // we're looking for information about a specific process ID.
  
  int mib[4];
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = ::getpid();

  // Call sysctl.
  
  const int junk = ::sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
  MAssert(junk == 0, ""); MUnused(junk);

  // We're being debugged if the P_TRACED flag is set.
  return (info.kp_proc.p_flag & P_TRACED) != 0;
}

// -------------------------------------------------------------------------------------------------

bool TSystem::RunningAsPlugin()
{
  return !gRunningAsExecutable;
}

// -------------------------------------------------------------------------------------------------

bool TSystem::ShowHiddenFilesAndFolders()
{
  return false;
}

// -------------------------------------------------------------------------------------------------

TString TSystem::ApplicationPathAndFileName(bool ResolveDllName)
{
  if (ResolveDllName)
  {
    // use dladdr exe to resolve shared libraries paths
    Dl_info dl_info; 
    ::memset(&dl_info, 0, sizeof(Dl_info));
  
    if (::dladdr((void*)ApplicationPathAndFileName, &dl_info) && 
        dl_info.dli_fname != NULL)
    {
      return TString(dl_info.dli_fname, TString::kFileSystemEncoding);
    }
  }

  // then the main bundle as fallback
  NSBundle* pBundle = [NSBundle mainBundle];
  
  if (pBundle == NULL)
  {
    MInvalid("Couldn't find Application Bundle!");

    return TString("/Applications/" + MProductString + 
      ".app/Contents/MacOS/" + MProductString);
  } 
  else 
  {
    return gCreateStringFromCFString(
      (CFStringRef)[pBundle executablePath]);
  }
}

// -------------------------------------------------------------------------------------------------

TString TSystem::GetOsAsString()
{
  #if defined(MArch_PPC)
    const TString ArchString = "(ppc)";

  #elif defined(MArch_X86)
    const TString ArchString = "(i386)";

  #elif defined(MArch_X64)
    const TString ArchString = "(x86_64)";

  #else
    #error "Unknown Architecture"
  #endif
  
  NSDictionary* pDict = [NSDictionary 
    dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
  
  if (pDict)
  {
    NSString* pProductString = [pDict objectForKey:@"ProductName"];
    NSString* pVersionString = [pDict objectForKey:@"ProductVersion"];
    
    if (pProductString && pVersionString)
    {
      return gCreateStringFromCFString((CFStringRef)pProductString) + " " + 
        gCreateStringFromCFString((CFStringRef)pVersionString) + " " + ArchString;
    }
  }
  
  return TString("Unknown Mac OS X Version ") + ArchString;
}

// -------------------------------------------------------------------------------------------------

bool TSystem::Is64BitOs()
{
  #if defined(MArch_PPC)
    return false; // definitely is not

  #elif defined(MArch_X64)
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
  #if defined(MArch_PPC)
    return false; // never 64bit

  #elif defined(MArch_X64)
    return false; // always 64bit

  #elif defined(MArch_X86)
    // OSX 10.6 (Snow Leopard) or later is 64bit
    return true;

  #else
    #error "unknown architecture"
  #endif
}

// -------------------------------------------------------------------------------------------------

TString TSystem::MacAdress()
{
  kern_return_t kernResult = KERN_SUCCESS;
  io_iterator_t intfIterator;
  UInt8 MACAddress[kIOEthernetAddressSize];

  kernResult = ::SFindEthernetInterfaces(&intfIterator);
  
  TString MacAdressString = TString("00:00:00:00:00:00");
  
  if (kernResult == KERN_SUCCESS) 
  {
    kernResult = ::SGetMACAddress(intfIterator, MACAddress, sizeof(MACAddress));
    
    if (KERN_SUCCESS == kernResult) 
    {
      char CharBuffer[1024];
      ::sprintf(CharBuffer, "%02x:%02x:%02x:%02x:%02x:%02x", 
        MACAddress[0], MACAddress[1], MACAddress[2], 
        MACAddress[3], MACAddress[4], MACAddress[5]);
    
      MacAdressString = TString(CharBuffer);
    }
  }
  
  ::IOObjectRelease(intfIterator);
  
  return MacAdressString;
}

// -------------------------------------------------------------------------------------------------

TList<TSystem::TArchInfo> TSystem::ExecutableArchitectures(const TString& FileName)
{
  try
  {
    TFile File(FileName);
    File.SetByteOrder(TByteOrder::kMotorola);
    
    if (! File.Open(TFile::kRead))
    {
      throw TReadableException("Failed to open the file!");
    }
    
    fat_header Header;
    File.Read(Header.magic);
    File.Read(Header.nfat_arch);
    

    // ... FAT binary
       
    if (Header.magic == FAT_MAGIC)
    {
      TList<TArchInfo> Ret;
    
      for (size_t i = 0; i < Header.nfat_arch; ++i)
      {
        fat_arch Arch;
        File.Read(Arch.cputype);
        File.Read(Arch.cpusubtype);
        File.Read(Arch.offset);
        File.Read(Arch.size);
        File.Read(Arch.align);
            
        TArchInfo Info;  
        Info.mCpuType = TArchInfo::kUnknown;
        Info.mOffset = Arch.offset;
        Info.mLength = Arch.size;
        
        switch (Arch.cputype)
        {
        case CPU_TYPE_POWERPC:
          Info.mCpuType = TArchInfo::kPPC;
          break;
        case CPU_TYPE_I386:
          Info.mCpuType = TArchInfo::kI386;
          break;
        case CPU_TYPE_X86_64:
          Info.mCpuType = TArchInfo::kX86_64;
          break;

        default:
          // unknown or unsupported machine
          break;
        }
        
        Ret.Append(Info);
      }
      
      return Ret;
    }

    // ... No FAT binary

    else  
    {
      File.SetPosition(0);
      File.SetByteOrder(TByteOrder::kIntel);

      TArchInfo Info;  
      Info.mCpuType = TArchInfo::kUnknown;
      Info.mOffset = 0;
      Info.mLength = File.SizeInBytes();
      
      uint32_t HeaderMagic;
      cpu_type_t CpuType;
      
      File.Read(HeaderMagic);
      File.Read(CpuType);
    
      if (HeaderMagic == MH_MAGIC || 
          HeaderMagic == MH_MAGIC_64)
      {
        switch (CpuType)
        {
        case CPU_TYPE_POWERPC:
          Info.mCpuType = TArchInfo::kPPC;
          break;
        case CPU_TYPE_I386:
          Info.mCpuType = TArchInfo::kI386;
          break;
        case CPU_TYPE_X86_64:
          Info.mCpuType = TArchInfo::kX86_64;
          break;

        default:
          // unknown or unsupported machine
          break;
        }
      }

      return MakeList<TArchInfo>(Info);
    }
  }
  catch (const TReadableException& Exception)
  {
    MInvalid(Exception.what()); MUnused(Exception);
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

void TSystem::Sleep(int Ms)
{
  // windows sleep(0) behavior
  if (Ms == 0)
  {
    ::sched_yield();
  }
  else
  {
    ::usleep(Ms * 1000);
  }
}

// -------------------------------------------------------------------------------------------------

bool TSystem::SwitchToOtherThread()
{
  return (::sched_yield() == 0);
}

// -------------------------------------------------------------------------------------------------

unsigned int TSystem::TimeInMsSinceStartup()
{
  MStaticAssert(sizeof(UnsignedWide) == sizeof(long long unsigned int));
  long long unsigned int Now;
  ::Microseconds((UnsignedWide*)&Now);

  unsigned int IntTime = (unsigned int)(Now / 1000.0);
    
  return IntTime;
}

// -------------------------------------------------------------------------------------------------

bool TSystem::SetProcessPriority(TProcessPriority NewPriority)
{
  // pri values are -20 to 20. 0 is the default
  int PriorityNiceValue;
  
  switch (NewPriority)
  {
  default: 
    MInvalid("Unknown priority");

  case kPriorityHigh:
    PriorityNiceValue = 10;
    break;

  case kPriorityNormal:
    PriorityNiceValue = 0;
    break;
    
  case kPriorityLow:
    PriorityNiceValue = -10;
    break;
  }
  
  if ((::setpriority(PRIO_PROCESS, ::getpid(), PriorityNiceValue) != 0))
  {
    MInvalid("setpriority failed (permissions?)");
    return false;
  } 
  else
  {
    return true;
  }
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
  
  if (WaitTilFinished)
  {
    // make sure we can wait for children 
    ::signal(SIGCHLD, SIG_DFL);
  }

  // log
  TString DebugString = "Executing '" + FileName;
  for (int i = 0; i < Args.Size(); ++i)
  {
    DebugString += " " + Args[i];
  }
  DebugString += "'";
  TLog::SLog()->AddLineNoVarArgs("System", DebugString.StdCString().c_str());
  
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
    char File[PATH_MAX];
    ::snprintf(File, PATH_MAX, "%s", 
      FileName.StdCString(TString::kFileSystemEncoding).c_str());
    
    TList< TArray<char> > ArgChars;
    ArgChars.PreallocateSpace(Args.Size());
    for (int i = 0; i < Args.Size(); ++i)
    {
      const std::string Arg = Args[i].StdCString(TString::kFileSystemEncoding);

      TArray<char> Chars;
      Chars.SetSize((int)Arg.size() + 1);
      ::strcpy(Chars.FirstWrite(), Arg.c_str());
      
      ArgChars.Append(Chars);
    }
    
    TList<char*> ArgCharPtrs;
    ArgCharPtrs.PreallocateSpace(Args.Size() + 2);
    ArgCharPtrs.Append(File);
    for (int i = 0; i < Args.Size(); ++i)
    {      
      ArgCharPtrs.Append(ArgChars[i].FirstWrite());
    }
    ArgCharPtrs.Append(NULL);
    
    // exec
    ::execv(File, ArgCharPtrs.FirstRead());
    
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
  
  if (!TFile(FileName).Exists())
  {
    MInvalid("Should validate the executable's filename before launching");
    return (WaitTilFinished) ? EXIT_FAILURE : kInvalidProcessId;
  }
}

// -------------------------------------------------------------------------------------------------

bool TSystem::ProcessIsRunning(int ProcessId)
{
  // check for child processes
  const int WaitResult = ::waitpid(ProcessId, NULL, WNOHANG);
  if (WaitResult > 0)
  {
    return false; // we've reaped a zombi, so it's no longer running
  }
  else if (WaitResult == 0)
  {
    return true; // child process valid, but has no wait status set (still running)
  }
  else
  {
    // check for other non child processes
    ProcessSerialNumber SerialNumber;
    if (::GetProcessForPID(ProcessId, &SerialNumber) == noErr)
    {
      return true;
    }
    else
    {
    // get GetProcessForPID seems to be unreliable, use GetBSDProcessList to double check
      size_t ProcListCount = 0;
      kinfo_proc* pProcList = (kinfo_proc*)::malloc(sizeof(kinfo_proc));
      ::SGetBSDProcessList(&pProcList, &ProcListCount);
      for (size_t i = 0; i < ProcListCount; ++i) 
      {
        if (pProcList[i].kp_proc.p_pid == ProcessId)
        {
          ::free(pProcList);
          return true;
        }
      }
      
      ::free(pProcList);
      return false;
    }
  }
}

// -------------------------------------------------------------------------------------------------

bool TSystem::ProcessKilled()
{
   return sProcessKilled;
}

// -------------------------------------------------------------------------------------------------

bool TSystem::KillProcess(int ProcessId)
{
  return (::kill(ProcessId, SIGKILL) == 0);
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
  ::raise(SIGKILL);
}

// -------------------------------------------------------------------------------------------------

bool TSystem::WindowControlsAreLeft()
{
  return true;
}

// -------------------------------------------------------------------------------------------------

TString TSystem::DefaultFileViewerName()
{
  return TString("Finder");
}

// -------------------------------------------------------------------------------------------------

bool TSystem::OpenURL(const TString& Url)
{
  NSWorkspace* pWorkspace = [NSWorkspace sharedWorkspace];
    
  const bool IsUrl = 
    (Url.StartsWithIgnoreCase("http://") || 
     Url.StartsWithIgnoreCase("https://") ||
     Url.StartsWithIgnoreCase("www."));
  
  if (IsUrl)
  {
    NSString* pEscapedUrl = [gCreateNSString(Url) 
      stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];

    const bool Result = [pWorkspace openURL: [NSURL URLWithString:pEscapedUrl]];

    return Result;
  }
  else
  {
    NSString* pDefaultBrowserName = @"Safari";
    
    // Hack: need a valid html file, an URL wont work...
    NSURL* pTestUrl = [NSURL URLWithString: 
      @"/Library/Documentation/Commands/grep/grep.html"];
    
    NSString* pAppName = NULL; 
    NSString* pType = NULL;
    
    if ([pWorkspace getInfoForFile:[pTestUrl path] 
          application:&pAppName type:&pType])
    {
      pDefaultBrowserName = pAppName;
    }
     
    return [pWorkspace openFile:gCreateNSString(Url) 
      withApplication:pDefaultBrowserName];
  }
}

// -------------------------------------------------------------------------------------------------

bool TSystem::OpenPath(const TDirectory& Directory, const TString& SelectFilename)
{
  NSWorkspace* pWorkspace = [NSWorkspace sharedWorkspace];
  
  if (!SelectFilename.IsEmpty())
  {
    return [pWorkspace selectFile:gCreateNSString(Directory.Path() + SelectFilename) 
      inFileViewerRootedAtPath:gCreateNSString(Directory.Path())];
  }
  else
  {
    return [pWorkspace openFile:gCreateNSString(Directory.Path())];
  }
}

