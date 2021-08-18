#include "CoreTypesPrecompiledHeader.h"

#include <windows.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <intrin.h>
#include <process.h>
#include <Shlobj.h>

#include <string>
#include <list>
#include <algorithm>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "iphlpapi.lib")

// =================================================================================================

HINSTANCE gDllInstanceHandle = NULL;
int gInMain = 0;

#pragma comment(linker, "/SECTION:.shr,RWS")
#pragma data_seg(".shr")

// in a shared region for the single instance checks
HWND gMainAppWindowHandle = NULL;
WCHAR gMainAppWindowUserName[UNLEN] = {0};

#pragma data_seg()

// =================================================================================================

static bool sProcessKilled = false;

// -------------------------------------------------------------------------------------------------

static void SGetWindowsVersion(
  DWORD&  MajorVersion, 
  DWORD&  MinorVersion, 
  WCHAR   pCSDVersion[128] = NULL)
{
  // GetVersionEx looks at the apps manifest version which is not what we want
  typedef LONG(WINAPI *TPRtlGetVersion)(RTL_OSVERSIONINFOEXW*);
  
  static TPRtlGetVersion sRtlGetVersion = (TPRtlGetVersion)
    ::GetProcAddress(::GetModuleHandle(L"ntdll.dll"), "RtlGetVersion");

  if (sRtlGetVersion)
  {
    RTL_OSVERSIONINFOEXW Version = { 0 };
    Version.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    sRtlGetVersion(&Version);

    MajorVersion = Version.dwMajorVersion;
    MinorVersion = Version.dwMinorVersion;
    
    if (pCSDVersion != NULL)
    {
      MStaticAssert(sizeof(Version.szCSDVersion) == 128 * sizeof(WCHAR));
      ::memcpy(pCSDVersion, Version.szCSDVersion, sizeof(Version.szCSDVersion));
    }
  }
  else
  {
    OSVERSIONINFO Version = { 0 };
    Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    ::GetVersionEx(&Version);

    MajorVersion = Version.dwMajorVersion;
    MinorVersion = Version.dwMinorVersion;

    if (pCSDVersion != NULL)
    {
      MStaticAssert(sizeof(Version.szCSDVersion) == 128 * sizeof(WCHAR));
      ::memcpy(pCSDVersion, Version.szCSDVersion, sizeof(Version.szCSDVersion));
    }
  }
}

// -------------------------------------------------------------------------------------------------

static void SAppendCommandLineChars(TList<wchar_t>& List, const TString& Arg)
{
  TString EscapedArg(Arg);
  EscapedArg.ReplaceChar(L'\\', L'/');
  EscapedArg.Replace("\"", "'\\\"");

  if (EscapedArg.ContainsChar(L' '))
  {
    List.Append(L'\"');

    for (int c = 0; c < EscapedArg.Size(); ++c)
    {
      List.Append(EscapedArg[c]);
    }

    List.Append(L'\"');
  }
  else
  {
    for (int c = 0; c < EscapedArg.Size(); ++c)
    {
      List.Append(EscapedArg[c]);
    }
  }
}

// -------------------------------------------------------------------------------------------------

static HWND SDefaultDialogOwnerWindow()
{
  HWND WindowHandle = (HWND)TSystem::ApplicationHandle();

  // If the foreground window is a child of the app window, use this one instead.
  // also use the foreground window when the app window is not (yet) visible. Will
  // especially happen in plugins: the host then hopefully is the foreground window.
  // See also HWND SDefaultBrowseDialogOwnerWindow() in WinDirectory.cpp

  if (!::IsWindow(WindowHandle) || !::IsWindowVisible(WindowHandle) || 
      (::GetForegroundWindow() != WindowHandle && 
       WindowHandle == ::GetWindow(::GetForegroundWindow(), GW_OWNER)))
  {
    WindowHandle = ::GetForegroundWindow();
  }

  return WindowHandle;
}  

// =================================================================================================

class TUrlHelper
{
public:
  bool Open(const TString& Url)
  {
    TString FinalUrl(Url);
    
    // wrap into quotes if needed...
    if (FinalUrl.Find(" ") != -1)
    {
      FinalUrl = "\"" + FinalUrl + "\"";
    }
    
    HINSTANCE Ret = ::ShellExecute(NULL, NULL, GetBrowser(), 
      FinalUrl.Chars(), NULL, SW_SHOWNORMAL);

    if (Ret > (HINSTANCE)(uintptr_t)0x20)
    {
      return true;
    }

    // try with the open command as fallback (will not always use the default browser)
    Ret = ::ShellExecute(NULL, L"open", FinalUrl.Chars(), NULL, NULL, SW_SHOWNORMAL);

    return (Ret > (HINSTANCE)(uintptr_t)0x20);
  }

private:
  LPCTSTR GetBrowser(void)
  {
    // Do we have the default browser yet?
    if (mDefaultBrowser.empty())
    {
      // Get the default browser from HKCR\http\shell\open\command
      HKEY KeyHandle = NULL;

      if (::RegOpenKeyEx(HKEY_CLASSES_ROOT, L"http\\shell\\open\\command", 0,
            KEY_READ, &KeyHandle) == ERROR_SUCCESS)
      {
        DWORD ValueLen = 0;
 
        // Get the default value
        if (::RegQueryValueEx(KeyHandle, NULL, NULL, NULL, 
              NULL, &ValueLen) == ERROR_SUCCESS && ValueLen > 0)
        {
          TArray<TCHAR> ValueArray(ValueLen);
          
          if (::RegQueryValueEx(KeyHandle, NULL, NULL,
                NULL, (LPBYTE)ValueArray.FirstWrite(), &ValueLen) == ERROR_SUCCESS)
          {
            mDefaultBrowser = ValueArray.FirstRead();
          }
        }
          
        ::RegCloseKey(KeyHandle);
      }
      
      // Do we have the browser?
      if (!mDefaultBrowser.empty())
      {
        // extract the executable name and path from the registry value

        // Get the exe path from a quoted string like, e.g.:
        //   "C:\PROGRAM FILES\Mozilla\Firefox.exe" -h -url "%1"
        std::wstring::size_type FirstQuoteCharIndex = mDefaultBrowser.find('"');
        
        if (FirstQuoteCharIndex == 0)
        {
          std::wstring::size_type NextQuoteCharIndex = 
            mDefaultBrowser.find('"', FirstQuoteCharIndex + 1);

          if (NextQuoteCharIndex != std::wstring::npos)
          {            
            // Get the full path
            mDefaultBrowser = mDefaultBrowser.substr(
              FirstQuoteCharIndex + 1, NextQuoteCharIndex - FirstQuoteCharIndex - 1);
          }
        }
        else
        {
          // We may have a pathname with spaces but no quotes (Netscape), e.g.:
          //   C:\PROGRAM FILES\NETSCAPE\COMMUNICATOR\PROGRAM\NETSCAPE.EXE -h "%1"
          std::wstring::size_type LastBackSlashIndex = mDefaultBrowser.rfind('\\');
          
          // Success?
          if (LastBackSlashIndex != std::wstring::npos)
          {
            // Look for the next space after the final backslash
            std::wstring::size_type SpaceCharIndex = 
              mDefaultBrowser.find(' ', LastBackSlashIndex);

            // Do we have a space?
            if (SpaceCharIndex != std::wstring::npos)
            {
              mDefaultBrowser = mDefaultBrowser.substr(0, SpaceCharIndex);                
            }
          }
        }
      }
    }
    
    // Done
    return mDefaultBrowser.c_str();
  }
  
  std::wstring mDefaultBrowser;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

// DLL instance main entry point

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
  gDllInstanceHandle = hInst; 

  // N.B: We do not call CoreTypeInit/Exit here, because we cant do so
  // in the Linux/OSX impls in the static initializers. DLLs can to take
  // care of this in their entry functions manually on all platforms.
    
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    ++gInMain;
  }
  else if (dwReason == DLL_PROCESS_DETACH)
  {
    --gInMain;
  }

  return 1;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TSystem::Sleep(int Ms)
{
  ::Sleep(Ms);
}

// -------------------------------------------------------------------------------------------------

bool TSystem::SwitchToOtherThread()
{
  return !!::SwitchToThread();
}

// -------------------------------------------------------------------------------------------------

unsigned int TSystem::TimeInMsSinceStartup()
{
  return ::timeGetTime();
}

// -------------------------------------------------------------------------------------------------

bool TSystem::SetProcessPriority(TProcessPriority NewPriority)
{
  switch (NewPriority)
  {
  case kPriorityHigh:
    return (::SetPriorityClass(::GetCurrentProcess(), 
      HIGH_PRIORITY_CLASS) == TRUE);
    break;

  default: MInvalid("Unknown priority");
  case kPriorityNormal:
    return (::SetPriorityClass(::GetCurrentProcess(), 
      NORMAL_PRIORITY_CLASS) == TRUE);
    break;

  case kPriorityLow:
    return (::SetPriorityClass(::GetCurrentProcess(), 
      BELOW_NORMAL_PRIORITY_CLASS) == TRUE);
    break;
  }
}

// -------------------------------------------------------------------------------------------------

int TSystem::CurrentProcessId()
{
  return ::getpid();
}

// -------------------------------------------------------------------------------------------------

int TSystem::LaunchProcess(
  const TString&          FileName, 
  const TList<TString>&   Args,
  bool                    WaitTilFinished )
{
  // validate filename
  if (! TFile(FileName).Exists())
  {
    MInvalid("Should validate the executable's filename before launching");
    return (WaitTilFinished) ? EXIT_FAILURE : kInvalidProcessId;
  }

  // build command line
  TList<wchar_t> CommandLine;
  SAppendCommandLineChars(CommandLine, FileName);

  for (int i = 0; i < Args.Size(); ++i)
  {
    CommandLine.Append(L' ');
    SAppendCommandLineChars(CommandLine, Args[i]);
  }
  CommandLine.Append(L'\0');

  TLog::SLog()->AddLine( "System", "Launching Process: %s", 
    TString(CommandLine.FirstRead()).StdCString().c_str() );

  // launch
  const BOOL InheritHandles = FALSE; // set to TRUE to redirect std out and err to parent
  const DWORD CreationFlags = CREATE_NO_WINDOW;
  
  PROCESS_INFORMATION ProcessInfo;
  TMemory::Zero(&ProcessInfo, sizeof(ProcessInfo));

  STARTUPINFO Startupinfo;
  TMemory::Zero(&Startupinfo, sizeof(Startupinfo));
  Startupinfo.cb = sizeof(Startupinfo);
  if ( InheritHandles )
  {
    Startupinfo.dwFlags |= STARTF_USESTDHANDLES;
    Startupinfo.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
    Startupinfo.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
  }

  const BOOL Succeeded = ::CreateProcessW(
    NULL, // ApplicationName
    (wchar_t*)CommandLine.FirstRead(),
    NULL, // ProcessAttributes,
    NULL, // ThreadAttributes,
    InheritHandles,
    CreationFlags,
    NULL, // Environment,
    NULL, // CurrentDirectory,
    &Startupinfo, 
    &ProcessInfo
  );

  if (Succeeded)
  {
    if ( WaitTilFinished )
    {
      // wait till the process finished
      ::WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

      DWORD ExitCode = EXIT_FAILURE;
      if ( ::GetExitCodeProcess( ProcessInfo.hProcess, &ExitCode ) == FALSE )
      {
        MInvalid("GetExitCodeProcess failed");
        ExitCode = EXIT_FAILURE;
      }
      else if ( ExitCode == STILL_ACTIVE )
      {
        MInvalid( "Process still running - should not happen" );
        ExitCode = EXIT_FAILURE;
      }

      // no longer need the handles 
      ::CloseHandle( ProcessInfo.hProcess );
      ::CloseHandle( ProcessInfo.hThread );

      return (int)ExitCode;
    }
    else
    {
      // allow "SetForegroundWindow" calls for processes that we've created
      ::LockSetForegroundWindow(LSFW_UNLOCK);
      ::AllowSetForegroundWindow(ProcessInfo.dwProcessId);

      // no longer need the handles (will use the process id to resolve the handles)
      ::CloseHandle( ProcessInfo.hProcess );
      ::CloseHandle( ProcessInfo.hThread );

      // return process id when not waiting for the process to finish
      return ProcessInfo.dwProcessId;
    }
  }
  else
  {
    TLog::SLog()->AddLine("System", "CreateProcess failed (Error: %d)",
      ::GetLastError());

    return (WaitTilFinished) ? EXIT_FAILURE : kInvalidProcessId;
  }
}

// -------------------------------------------------------------------------------------------------

bool TSystem::ProcessIsRunning(int ProcessId)
{
  const BOOL InheritHandle = FALSE;
  HANDLE ProcessHandle = ::OpenProcess(
    PROCESS_QUERY_INFORMATION, InheritHandle, ProcessId);

  // when we can resolve it's handle, assume it's still alive
  if (ProcessHandle)
  {
    ::CloseHandle(ProcessHandle);
    return true;
  }
  else
  {
    return false;
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
  const BOOL InheritHandle = FALSE;
  HANDLE ProcessHandle = ::OpenProcess(
    PROCESS_TERMINATE, InheritHandle, ProcessId);

  if (ProcessHandle)
  {
    sProcessKilled = (::TerminateProcess(ProcessHandle, 0) == TRUE);
    return sProcessKilled;
  }
  else
  {
    return false;
  }
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

  // suppress the runtime error dialog...
  ::_set_abort_behavior( 0, _WRITE_ABORT_MSG);
  
  DWORD dwExitCode;
  ::GetExitCodeProcess(::GetCurrentProcess(), &dwExitCode);
  
  ::TerminateProcess(::GetCurrentProcess(), dwExitCode); 
}

// -------------------------------------------------------------------------------------------------

bool TSystem::RunningInDebugger()
{
  return (::IsDebuggerPresent() == TRUE);
}

// -------------------------------------------------------------------------------------------------

bool TSystem::RunningAsPlugin()
{
  return (gDllInstanceHandle != NULL);
}

// -------------------------------------------------------------------------------------------------

void TSystem::DispatchSystemEvents(int TimeoutInMs)
{
  MAssert(TimeoutInMs >= 0, "");

  TStamp PeekStart;

  MSG msg;
  while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    if (msg.message == WM_QUIT)
    {
      // we can not handle WM_QUIT here, so repost and bail out...
      ::PostQuitMessage((int)msg.wParam);
      break;
    }

    // avoid buggy Plugin floating point error messages...
    M__DisableFloatingPointAssertions

    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);

    M__EnableFloatingPointAssertions

    if (PeekStart.DiffInMs() > TimeoutInMs)
    {
      // don't block forever and avoid reentered never ending messages...
      // MTrace("DispatchSystemEvents: Timeout while dispatching system events");
      break;
    }
  }
}

// -------------------------------------------------------------------------------------------------

TString TSystem::LastSystemErrorMessage()
{
  LPVOID pMsgBuf = NULL;
  
  const DWORD NumChars = ::FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,
    ::GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR)&pMsgBuf,
    0, NULL);
  
  if (NumChars)
  {
    TString Ret = (wchar_t*)pMsgBuf;
    ::LocalFree(pMsgBuf);

    return Ret.Strip().RemoveNewlines();
  }
  else
  {
    return "Unknown error";
  }
}

// -------------------------------------------------------------------------------------------------

bool TSystem::ShowHiddenFilesAndFolders()
{
  SHELLFLAGSTATE Flags;
  SHGetSettings(&Flags, SSF_SHOWALLOBJECTS);

  return Flags.fShowAllObjects != 0;
}

// -------------------------------------------------------------------------------------------------

TString TSystem::ApplicationPathAndFileName(bool ResolveDllName)
{
  // gDllInstanceHandle will be NULL when running as application

  WCHAR FileName[1024] = {0};
  ::GetModuleFileName(
    ResolveDllName ? gDllInstanceHandle : NULL,
    FileName,
    sizeof(FileName) / sizeof(WCHAR));
  
  return FileName;
}

// -------------------------------------------------------------------------------------------------

bool TSystem::InMain()
{
  return (gInMain > 0);
}

// -------------------------------------------------------------------------------------------------

TString TSystem::GetOsAsString()
{
  TString OsString;

  DWORD MajorVersion, MinorVersion; WCHAR CSDVersion[128];
  SGetWindowsVersion(MajorVersion, MinorVersion, CSDVersion);

  switch (MajorVersion)
  {
  case 4:
    switch (MinorVersion)
    {
    case 0:
      OsString = TString("Win95 ") + TString(CSDVersion);
      break;

    case 10:
      OsString = TString("Win98 ") + TString(CSDVersion);
      break;
      
    case 90:
      OsString = TString("WinMe ") + TString(CSDVersion);
      break;
    }
    break;

  case 5:
    switch (MinorVersion)
    {
    case 0:
      OsString = TString("Win2000 ") + TString(CSDVersion);
      break;
    
    case 1:
      OsString = TString("WinXP ") + TString(CSDVersion);
      break;
      
    case 2:
      OsString = TString("Windows Server 2003 ") + TString(CSDVersion);
      break;
    }
    break;

  case 6:
    switch (MinorVersion)
    {
    case 0:
      OsString = TString("Windows Vista ") + TString(CSDVersion);
      break;
    
    case 1:
      OsString = TString("Windows 7 ") + TString(CSDVersion);
      break;

    case 2:
      OsString = TString("Windows 8 ") + TString(CSDVersion);
      break;

    case 3:
      OsString = TString("Windows 8.1 ") + TString(CSDVersion);
      break;
    }
    break;

  case 10:
    switch (MinorVersion)
    {
    case 0:
      OsString = TString("Windows 10 ") + TString(CSDVersion);
      break;
    }
    break;
  }

  if (OsString.IsEmpty())
  {
    OsString = TString("Unknown Windows Version (") + 
      ToString((int)MajorVersion) + "." + 
      ToString((int)MinorVersion) + ")";
  }

  #if defined(MArch_X86)
    const TString ArchString = "(i386)";
  #elif defined(MArch_X64)
    const TString ArchString = "(x86_64)";
  #else
    #error "Unknown Architecture"
  #endif

  return TString(OsString).Strip() + " " + ArchString;
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
    return false; // always is 64 bit

  #elif defined(MArch_X86)
    static BOOL sIsWow64Process = -1;
  
    if (sIsWow64Process == -1)
    {
      // IsWow64Process is not available on all supported versions of Windows:
      typedef BOOL (WINAPI *TPIsWow64ProcessFunc) (HANDLE, PBOOL);

      TPIsWow64ProcessFunc pIsWow64ProcessFunc = (TPIsWow64ProcessFunc)
        ::GetProcAddress(::GetModuleHandle(L"kernel32"), "IsWow64Process");

      if (pIsWow64ProcessFunc != NULL)
      {
        if (! pIsWow64ProcessFunc(::GetCurrentProcess(), &sIsWow64Process))
        {
          TLog::SLog()->AddLine("System", 
            "WARNING: failed to find out if we're running in a 64bit "
            "Windows ('IsWow64Process' failed with error: %d)", GetLastError());

          sIsWow64Process = 0; // assume we're not
        }
        else
        {
          TLog::SLog()->AddLine("System", 
            "Running as 32bit process in a %dbit operating system...", 
            sIsWow64Process ? 64 : 32);
        }
      }
      else
      {
        TLog::SLog()->AddLine("System", 
          "Running as 32bit process in a 32bit operating system "
          "('IsWow64Process' not present in kernel)...");

        // 'IsWow64Process' not present. Must be a 32bit Windows
        sIsWow64Process = 0;
      }
    }

    return !!sIsWow64Process;

  #else
    #error "unknown architecture"
  #endif
}

// -------------------------------------------------------------------------------------------------

TString TSystem::MacAdress()
{
  TString MacAdress = "00:00:00:00:00:00";

  PIP_ADAPTER_INFO pAdapterInfos = (PIP_ADAPTER_INFO)::malloc(sizeof(IP_ADAPTER_INFO));
  ULONG OutBufLen = sizeof(IP_ADAPTER_INFO);

  if (::GetAdaptersInfo(pAdapterInfos, &OutBufLen) == ERROR_BUFFER_OVERFLOW) 
  {
    ::free(pAdapterInfos);
    pAdapterInfos = (IP_ADAPTER_INFO *)::malloc(OutBufLen); 
  }

  DWORD dwStatus = ::GetAdaptersInfo(pAdapterInfos, &OutBufLen);
  MAssert(dwStatus == ERROR_SUCCESS, "");

  if (pAdapterInfos && dwStatus == ERROR_SUCCESS)
  {
    PIP_ADAPTER_INFO pCurrentAdapterInfo = pAdapterInfos;
    
    do 
    {
      char CharBuffer[1024];
      
      ::sprintf(CharBuffer, "%02x:%02x:%02x:%02x:%02x:%02x", 
        pCurrentAdapterInfo->Address[0], pCurrentAdapterInfo->Address[1], 
        pCurrentAdapterInfo->Address[2], pCurrentAdapterInfo->Address[3], 
        pCurrentAdapterInfo->Address[4], pCurrentAdapterInfo->Address[5]);

      MacAdress = TString(CharBuffer);

      // use the last one
      pCurrentAdapterInfo = pCurrentAdapterInfo->Next;  
    }
    while (pCurrentAdapterInfo);
  }

  ::free(pAdapterInfos);
  
  return MacAdress;
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
    Info.mLength = File.SizeInBytes();
  
    // read IMAGE_FILE_HEADER to find out the architecture:
    IMAGE_DOS_HEADER ImageDosHeader = {0};
    IMAGE_NT_HEADERS ImageFileHeader = {0};
      
    size_t ImageDosHeaderSize = sizeof(ImageDosHeader);
    File.ReadBytes((char*)&ImageDosHeader, &ImageDosHeaderSize);

    if (ImageDosHeaderSize != sizeof(ImageDosHeader) ||
        ImageDosHeader.e_magic != IMAGE_DOS_SIGNATURE)
    {
      return MakeList<TArchInfo>(Info);
    }

    File.SetPosition(ImageDosHeader.e_lfanew);

    size_t ImageFileHeaderSize = sizeof(ImageFileHeader);
    File.ReadBytes((char*)&ImageFileHeader, &ImageFileHeaderSize);
    
    if (ImageFileHeaderSize != sizeof(ImageFileHeader) ||
        ImageFileHeader.Signature != IMAGE_NT_SIGNATURE)
    {
      return MakeList<TArchInfo>(Info);
    }

    switch (ImageFileHeader.FileHeader.Machine)
    {
    case IMAGE_FILE_MACHINE_I386:
      Info.mCpuType = TArchInfo::kI386;
      break;

    case IMAGE_FILE_MACHINE_AMD64:
      Info.mCpuType = TArchInfo::kX86_64;
      break;

    default:
      // unknown or unsupported machine
      break;
    }

    return MakeList<TArchInfo>(Info);
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
  gMainAppWindowHandle = (HWND)AppHandle;
  
  DWORD MainAppWindowUserNameSize = MCountOf(gMainAppWindowUserName);
  ::GetUserName(gMainAppWindowUserName, &MainAppWindowUserNameSize);
}

// -------------------------------------------------------------------------------------------------

bool TSystem::OpenURL(const TString& Url)
{
  return TUrlHelper().Open(Url);
}

// -------------------------------------------------------------------------------------------------

bool TSystem::WindowControlsAreLeft()
{
  return false;
}

// -------------------------------------------------------------------------------------------------

TString TSystem::DefaultFileViewerName()
{
  return TString("Explorer");
}

// -------------------------------------------------------------------------------------------------

bool TSystem::OpenPath(const TDirectory& Directory, const TString& SelectFilename)
{
  TString Command;

  // explorer does weired things when the file does not exist, so check here
  if (SelectFilename.IsEmpty() || ! TFile(Directory.Path() + SelectFilename).Exists())
  {
    Command = "/e,\"" + Directory.Path() + "\"";
  }
  else
  {
    Command = "/e,/select,\"" + Directory.Path() + SelectFilename + "\"";
  }

  return ::ShellExecute(NULL, L"open", L"explorer.exe", 
    Command.Chars(), NULL, SW_SHOWNORMAL) != 0;  
}

