#pragma once

#ifndef _System_h_
#define _System_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/List.h"

#include <functional>

class TDirectory;
template <typename TEnumType> struct SEnumStrings;
template<typename T> class TAbstractSetMemberFuncPtr;

// =================================================================================================

namespace TSystem
{
  //@{ ... OS version related stuff

  //! return a human readable string which identifies the OS and its version
  TString GetOsAsString();
  
  //! returns true when this either is a 64bit application, or a 32bit 
  //! application running in a 64 bit operating system.
  bool Is64BitOs();
  //! returns true when this is a 32bit application running in a 64 bit 
  //! operating system.
  bool Is32BitProcessIn64BitOs();
  
  #if defined(MWindows)
    //! peek for window messages
    void DispatchSystemEvents(int TimeoutInMs = 250);

    //! return an error message in human readable form of the last ::GetLastError()
    TString LastSystemErrorMessage();
  #endif
  //@}


  //@{ ... General system and locale info

  //! returns true when running in the main function, meaning we have
  //! passed the CRT init and not yet entered the CRT exit.
  bool InMain();
  
  //! returns true if an debugger is attached to the running process.
  bool RunningInDebugger();
  
  //! returns true if the framework is running in a DLL, plugin (not as a executable)
  bool RunningAsPlugin();

  //! return the mac address of the (built-in, when possible) network card
  TString MacAdress();
  
  //! return the time in MS since the OS started. Will wrap when overflowing.
  unsigned int TimeInMsSinceStartup();

  //! Suspend the current Thread for the given amount of ms.
  //! A value of zero causes the thread to relinquish the remainder of its 
  //! time slice to any other thread of equal priority that is ready to run. 
  //! If no other threads of equal priority are ready to run, the function 
  //! returns immediately, and the thread continues execution
  void Sleep(int Ms);
 
  //! Try to switch from the calling thread to another thread that is 
  //! ready to go, giving the current time slice away. May not succeed if all 
  //! other threads are locked. returns true if successfully switched.
  bool SwitchToOtherThread();
  //@}


  //@{ ... Process Creation, Killing and Priority
  
  enum { kInvalidProcessId = 0 };

  //! return the name and paths of all running processes (including self).
  TList<TString> RunningProcesses();

  //! Get the process id of this process (::getpid())
  int CurrentProcessId();

  //! Launch the given executable with the given arguments in a new process. 
  //! When \param WaitTillFinished is false, @returns process ID of the 
  //! launched processed or kInvalidProcessId on errors.
  //! When \param WaitTillFinished is true, @returns process' exit code.
  int LaunchProcess(
    const TString&        FileNameAndPath, 
    const TList<TString>& Args = TList<TString>(), 
    bool                  WaitTilFinished = false);
    
  //! returns true if a process with the given process ID is (or to be correct, 
  //! WAS, at the time the function got invoked) running. Process IDs can be 
  //! reused by the system, so this is not really reliable and more a good hint.
  bool ProcessIsRunning(int ProcessId);

  //! Try killing the process with the given ID. returns success.
  bool KillProcess(int ProcessId);

  enum TProcessPriority
  {
    kPriorityHigh,
    kPriorityNormal,
    kPriorityLow,

    kNumberOfProcessPriorities
  };

  //! Try setting a new priority for the current process. returns success.
  bool SetProcessPriority(TProcessPriority NewPriority);
  //@}

  
  //@{ ... Process Termination

  typedef void (*TPProcessKilledHandlerFunc)();

  //! Add a function that should be called when the program got terminated
  //! with an abort signal (it either crashed or was forced to die). Only add
  //! really important things here, that need to be cleaned up, i.e. drivers!
  void AddProcessKilledHandler(TPProcessKilledHandlerFunc pFunc);

  //! Remove a previously added process kill handler.
  void RemoveProcessKilledHandler(TPProcessKilledHandlerFunc pFunc);

  //! Returns true when the process got killed via TSystem::KillProcess().
  bool ProcessKilled();
  //! Run all registered ProcessKilled handlers, then silently kill the current 
  //! process, suppressing any error dialogs and avoid running the static finalizers
  //! if possible. 
  void KillProcess();
  //@}


  //@{ ... System SuspendMode Notification

  enum TSuspendMode
  {
    kSuspending,
    kResumeSuspending
  };

  typedef void (*TPSuspendModeHandlerFunc)(TSuspendMode Mode);

  //! Add a function that should be called when the system suspends
  //! or wakes up from suspend mode.
  void AddSuspendModeHandler(TPSuspendModeHandlerFunc pFunc);

  //! Remove a previously added suspend mode handler function.
  void RemoveSuspendModeHandler(TPSuspendModeHandlerFunc pFunc);

  //! Calls all registered suspend mode handlers with the given mode.
  void CallSuspendHandlers(TSuspendMode Mode);
  //@}


  //@{ ... User Messages 

  //! Categorizes a system message.
  enum TMessageType
  {
    kInfo,
    kWarning,
    kError
  };

  //! Show a Message to the user. By default this will print to the stdout,
  //! applications will redirect this function to show the message in their 
  // main window (for example in a status view or message boxes for errors).
  void ShowMessage(const TString &Message, TMessageType Type);

  //! Temporarily suppress all kInfo messages.
  class TSuppressInfoMessages
  {
  public:
    TSuppressInfoMessages();
    ~TSuppressInfoMessages();
  };
  
  //! True when currently suppressing messages or dialogs.
  bool InfoMessagesAreSuppressed();

  //! Delegate to show a message to the user.
  typedef std::function<void(const TString&, TMessageType Type)> TPShowMessageFunc;
  //! Default message redirector. Will dump kSystem messages to the std out show 
  //! system dialogs for warnings and errors.
  void StandardMessageRedirector(const TString& Message, TMessageType Type);

  //! Function which will be called when the above declared "ShowMessage" 
  //! function gets called.
  void SetShowMessageFunc(TPShowMessageFunc Func);
  //! Disable a previously defined redirector. Will reenable the default 
  //! redirector again.
  void DisableShowMessageFunc();
  //@}


  //@{ ... Executable Architectures
   
  //! Structure, describing architectures in a binary
  class TArchInfo
  {
  public:
    enum TCpuType
    {
      kUnknown = -1,

      kPPC,
      kI386,
      kX86_64,
      
      kNumberOfCpuTypes,

      #if defined(MArch_PPC)
        kNative = kPPC
      #elif defined(MArch_X86)
        kNative = kI386
      #elif defined(MArch_X64)
        kNative = kX86_64
      #else
        #error "unknown architecture"
      #endif
    };
  
    TArchInfo() 
      : mCpuType(kUnknown), 
        mOffset(0), 
        mLength(0) { }
    
    //! see TExecutableFormat
    TCpuType mCpuType;
    
    //! Offset in the binary image. 0-FileSize when not a FAT binary
    size_t mOffset;
    size_t mLength;
  };
  
  //! Returns architecture information about the given binary file.
  //! Multiple architectures are currently only used on the Mac with
  //! FAT binaries. All other platforms will return exactly one
  //! arch, when accessing the file succeeded, else none.
  TList<TArchInfo> ExecutableArchitectures(const TString& FileName);
  //@}

  
  //@{ ... Application handles/info

  //! returns the platform handle (HWND, Window or NSWindow) of the 
  //! main application window when available.
  void* ApplicationHandle();
  void SetApplicationHandle(void* AppHandle);

  //! Return the filename and path of this application or plugin DLL.
  //! If called from a DLL and \param ResolveDllName is true, returns the 
  //! path of the DLL, otherwise the path of the main executable the DLL
  //! got loaded from.
  TString ApplicationPathAndFileName(bool ResolveDllName = true);
  //@}
  

  //@{ ... System behavior/conventions

  //! return true when the current running system shows windows controls 
  //! on the left
  bool WindowControlsAreLeft();
  
  //! "Finder" on Mac, "Explorer" on Windows, and so on.
  TString DefaultFileViewerName();

  enum TTextWritingDirection
  {
    kLeftToRightWritingDirection,
    kRightToLeftWritingDirection,
    
    kNumberOfTextWritingDirections
  };
  
  //! On Windows, returns the corresponding Explorer settings.
  //! On OSX, always returns false.
  //! On Linux, always returns true.
  bool ShowHiddenFilesAndFolders();
  //@}


  //@{ ... Locale Helpers

  //! Temporarily set a new locale for the given locale category.
  class TSetAndRestoreLocale 
  {
  public:
    TSetAndRestoreLocale(
      int         LocaleCategory, 
      const char* pNewLocaleSetting);

    ~TSetAndRestoreLocale();
  
  private:
    int mLocaleCategory;

    const char* mpOldLocaleSetting;
    const char* mpNewLocaleSetting;
  };
  //@}
  

  //@{ ... Open File or URLs

  //! Open the given internet address in the system's default browser.
  //! returns true if the URL/browser was successfully opened, else false.
  bool OpenURL(const TString& Url);

  //! Opens the given directory in the systems path viewer (Finder, Explorer).
  bool OpenPath(
    const TDirectory& Directory, 
    const TString&    SelectFilename = TString());
  //@}
}

// =================================================================================================

template <> struct SEnumStrings<TSystem::TArchInfo::TCpuType>
{
  operator TList<TString>()const
  {
    MStaticAssert(TSystem::TArchInfo::kNumberOfCpuTypes == 3);

    TList<TString> Ret;
    Ret.Append("PPC");
    Ret.Append("I386");
    Ret.Append("X86_64");

    return Ret;
  }
};


#endif // _System_h_

