#pragma once

//! Do not compile this file with the project, but only when included from
//! MainEntry.h. This way we can strip all main defs for DLLs.
#ifdef MCompilingMainEntry

#include "CoreTypes/Export/CoreTypesInit.h"
#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/System.h"

#include <cstring> // for memset

#include <signal.h>

// defined in LinuxSystem.cpp
extern int gInMain;
extern bool gRunningAsExecutable;

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static void SSigTrapHandler(int sig)
{
  // do nothing. simply ignore and continue...
}

// -------------------------------------------------------------------------------------------------

#ifndef MDebug

static void SFatalErrorHandler(int sig)
{
  if (TStructuredExceptionHandler::SRunningInExceptionHandler())
  {
    // have a guarded Try_Catch long jump? Then jump to the catch hander...
    TStructuredExceptionHandler::SInvokeExceptionHander();
  }

  // else dump out and quit
  ::fprintf(stdout, "Caught signal %d. Terminating...\n", sig);

  ::exit( 666 ); // magic return value for a crash, used by other tools
}

#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

int main_platform_impl(int argc, char *argv[])
{ 
  // . install signal handlers (until TApplication overrides them)

  // never handle any fatal signals in debug builds
  #ifndef MDebug
    struct sigaction FatalSigAction;
    ::memset(&FatalSigAction, 0, sizeof(struct sigaction));
    FatalSigAction.sa_handler = (void(*)(int))SFatalErrorHandler;
    FatalSigAction.sa_flags = SA_NODEFER | SA_ONSTACK;

    ::sigaction(SIGSEGV, &FatalSigAction, NULL);
    ::sigaction(SIGBUS, &FatalSigAction, NULL);
    ::sigaction(SIGILL, &FatalSigAction, NULL);
  #endif

  // ignore SIGTRAPS (ext libs might fire them) when not running in gdb
  if (! TSystem::RunningInDebugger())
  {
    struct sigaction TrapSigAction;
    ::memset(&TrapSigAction, 0, sizeof(struct sigaction));
    TrapSigAction.sa_handler = (void(*)(int))SSigTrapHandler;
    TrapSigAction.sa_flags = SA_NODEFER;
    ::sigaction(SIGTRAP, &TrapSigAction, NULL);
  }
  else
  {
    ::fprintf(stdout, "%s: Running in GDB. "
      "Will pass SIGTRAPS to the debugger...\n", argv[0]);
  }
  

  // . run the API's main

  int Ret = 0;

  // when main is called, we're using the API as executable
  gRunningAsExecutable = true;
  
  ++gInMain;

  try
  {
    CoreTypesInit();
    {
      TList<TString> Arguments;
      for (int i = 0; i < argc; ++i)
      {
        TString Arg(argv[i], TString::kFileSystemEncoding);
        Arg.RemoveFirst("file://"); // remove URI file prefixes when present

        Arguments.Append(Arg);
      }

      extern int gMain(const TList<TString>& Arguments);
      Ret = gMain(Arguments);
    }
    CoreTypesExit();
  }
  catch (...)
  {
    CoreTypesExit();
    Ret = 1; 
  }

  --gInMain;

  return Ret;
}


#else // !MCompilingMainEntry

static int sSuppressNoSymbolsWarning = sSuppressNoSymbolsWarning;

#endif // MCompilingMainEntry

