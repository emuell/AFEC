#pragma once

//! Do not compile this file with the project, but only when included from
//! MainEntry.h. This way we can strip all main defs for DLLs
#ifdef MCompilingMainEntry

#include "CoreTypes/Export/CoreTypesInit.h"
#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/StackWalk.h"

#include <windows.h>
#include <Lmcons.h> // UNLEN

// unreachable code in the ASM catch(...) handlers
#pragma warning(disable: 4702) 

// =================================================================================================

// Defined in WinSystem.cpp
extern HWND gMainAppWindowHandle;
extern WCHAR gMainAppWindowUserName[UNLEN];

extern int gInMain;

// =================================================================================================

// -------------------------------------------------------------------------------------------------

#ifndef MNoSingleInstanceChecks

static bool SForwardArgsToRunningApplication(const TList<TString>& Arguments)
{
  // check if another exe is running
  if (gMainAppWindowHandle != NULL && 
      ::IsWindow(gMainAppWindowHandle) && 
      ::IsWindowVisible(gMainAppWindowHandle))
  {
    WCHAR MainAppWindowUserName[UNLEN]; 
    DWORD MainAppWindowUserNameSize = UNLEN;

    // and make sure the window is from the current user
    if (::GetUserName(MainAppWindowUserName, &MainAppWindowUserNameSize) && 
        ::wcscmp(MainAppWindowUserName, gMainAppWindowUserName) == 0)
    {
      // ...  check if any on the args are files
      
      bool HasFileArguments = false;

      for (int i = 1; i < Arguments.Size(); ++i)
      {
        if (gFileExists(Arguments[i]))
        {
          HasFileArguments = true;
          break;
        }
      }

      if (HasFileArguments)
      {
        for (int i = 1; i < Arguments.Size(); ++i)
        {
          if (gFileExists(Arguments[i]))
          {
            const std::string ArgumentCString = Arguments[i].StdCString(TString::kUtf8);

            COPYDATASTRUCT CopyDataStruct;
            CopyDataStruct.cbData = (DWORD)ArgumentCString.size();
            CopyDataStruct.lpData = (PVOID)ArgumentCString.c_str();
            
            DWORD_PTR MessageResult = 0;
            const LRESULT SendResult = ::SendMessageTimeout(gMainAppWindowHandle, 
              WM_COPYDATA, 0, (LPARAM)&CopyDataStruct, 
              SMTO_NORMAL | SMTO_ABORTIFHUNG, 5000, &MessageResult);

            if (SendResult == 0 || MessageResult == 0)
            {
              TLog::SLog()->AddLineNoVarArgs("Application", 
                "Other instance neglected the file argument window message. "
                "Continue with regular startup...");

              return false;
            }
          }
        }

        TLog::SLog()->AddLineNoVarArgs("Application", 
          "Another instance of the application is running. "
          "Passed arguments and will now exit...");

        if (::IsIconic(gMainAppWindowHandle))
        {
          ::ShowWindow(gMainAppWindowHandle, SW_RESTORE);
        }

        ::SetForegroundWindow(gMainAppWindowHandle);
        
        return true;
      }
    }
  }

  return false;
}

#endif // MNoSingleInstanceChecks

// =================================================================================================

namespace TWinCrtHandler
{
  void InvalidParameterHandler(
    const wchar_t*  pExpression,
    const wchar_t*  pFunction, 
    const wchar_t*  pFile, 
    unsigned int     Line, 
    uintptr_t       pReserved)
  {
    MInvalid("Invalid Parameter in CRT runtime!");
    
    ::wprintf(L"Invalid parameter detected in function %s."
      L" File: %s Line: %u\n", pFunction, pFile, Line);
    
    ::wprintf(L"Expression: %s\n", pExpression);
  }
  
  void Install()
  {
    // if no handler is installed, the program will exit, which is definitely not 
    // what we want (Plugs could cause these errors for example)
    ::_set_invalid_parameter_handler(&TWinCrtHandler::InvalidParameterHandler);
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

// separate function for SEH, which extracts args and calls gMain

static int SMain()
{
  // get args from unicode "GetCommandLineW()"
  TList<TString> Arguments;

  TString CommandLine(::GetCommandLineW());
  CommandLine.Strip(); // ignore initial leading/trailing spaces

  // split args at spaces, respecting quotes
  if (! CommandLine.IsEmpty())
  {
    int Start = 0, End = 1;
    bool InQuote = (CommandLine[Start] == L'\"');

    while (End < CommandLine.Size())
    {
      if (CommandLine[End] == L' ' && ! InQuote)
      {
        Arguments.Append(CommandLine.SubString(Start, End));
        
        // skip all following separating spaces 
        while (++End < CommandLine.Size() && CommandLine[End] == L' ') { ; }

        Start = End;
      }

      if (CommandLine[End] == L'\"' && CommandLine[End - 1] != L'\\')
      {
        InQuote = (! InQuote);
      }

      ++End;
    }

    // add remainder
    if (End >= Start)
    {
      Arguments.Append(CommandLine.SubString(Start, End));
    }
  }

  // fix backspaced quotes and remove extra outer quotes for each arg
  for (int i = 0; i < Arguments.Size(); ++i)
  {
    TString Arg = Arguments[i];
    
    while (Arg.StartsWithChar(L'\"') && Arg.EndsWithChar(L'\"'))
    {
      Arg = Arg.SubString(1, Arg.Size() - 1);
    }

    Arg.Replace(L"\\\"", L"\""); 
    
    Arguments[i] = Arg;
  } 

  // forward to an already running app or call this app's gMain
  #ifndef MNoSingleInstanceChecks
    if (! SForwardArgsToRunningApplication(Arguments))
    {
      extern int gMain(const TList<TString>& Arguments);
      return gMain(Arguments);
    }
    else
    {
      return 0;
    }
  #else
    extern int gMain(const TList<TString>& Arguments);
    return gMain(Arguments);
  #endif
}

// -------------------------------------------------------------------------------------------------

// SEH guarded console app main entry point (see MainEntry.h)

int main_platform_impl()
{
  ++gInMain;

  // make sure FPU exceptions are enabled
  M__EnableFloatingPointAssertions

  TWinCrtHandler::Install();
  
  // try increasing the max open files limit (the default is 512)
  if (::_setmaxstdio(2048) == -1)
  {
    if (::_setmaxstdio(1024) == -1)
    {
      MInvalid("_setmaxstdio(2048 or 1024) failed!!");
      ::perror("_setmaxstdio(2048 or 1024) failed");
    }
  }

  int Ret = 0;

  MTry_SystemException
  {
    CoreTypesInit();

    Ret = SMain();

    CoreTypesExit();
  }
  MCatch_SystemExceptionAndDumpStack
  {
    CoreTypesExit();
    Ret = 666; // magic return value for a crash, used by other tools
  }
  MFinalize_SystemException

  --gInMain;
  return Ret;
}


#endif // MCompilingMainEntry

