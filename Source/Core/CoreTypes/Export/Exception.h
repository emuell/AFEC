#pragma once

#ifndef _Exception_h_
#define _Exception_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"

#if defined(MCompiler_GCC)
  #include "CoreTypes/Export/CompilerDefines.h"
  #include "CoreTypes/Export/ThreadLocalValue.h"
  #include "CoreTypes/Export/List.h"
#endif

#include <exception>

#if defined(MCompiler_VisualCPP)
  #include <excpt.h>

#elif defined(MCompiler_GCC)
  #include <csetjmp>
#endif

// =================================================================================================

//! Define to completely disable structured exception handling in release builds:
//! This will allow to attach a debugger as soon as the crash happens...
// #define MNoSEH

// =================================================================================================

class TReadableException : public std::exception
{
public:
  explicit TReadableException(const TString& Message)
    : mMessage(Message)
  {
    WriteIntoLog();
  }
  
  virtual ~TReadableException() throw () { }

  // return the exception message
  const TString& Message()const throw () { return mMessage; }

private:
  // implement std::exception. Private, because clients should use Message()
  virtual const char* what() const throw () { return mMessage.CString(); }
  
  void WriteIntoLog() throw ();

  TString mMessage;
};

// =================================================================================================

class TOutOfMemoryException : public TReadableException
{
public:
  //! Preallocates the error string.
  //! To be called from the projects init/exit functions...
  static void SInit();
  static void SExit();

  TOutOfMemoryException()
    : TReadableException(sErrorMessage) { }

private:
  static TString sErrorMessage;
};

// =================================================================================================

/*!
 * Structured exception handling:
 *
 * Hardware exceptions are not thrown as C++ Exceptions. The structured exception 
 * handling macros below will nevertheless make it possible to catch and recover 
 * from such exceptions.
 * 
 * Visual CPP has built in support for this, for GCC we do "emulate" this behavior
 * via longjumps. Please note that with GCC you need to manually call 
 * TStructuredExceptionHandler when catching signals (see defines below). 
 *
 * Currently TApplication handles all this for apps automatically.
!*/

#if defined(MCompiler_GCC)

  class TStructuredExceptionHandler 
  {
  public:
    // sigjmp_buf wrapper needed for TList (sigjmp_buf is an array)
    struct TSigJmpBuffer {
      sigjmp_buf mBuffer;
    };

    static void SInit();
    static void SExit();
    
    //! By default enabled. When disabled, exceptions are still logged 
    //! but handlers are no longer executed.
    static bool SIsEnabled();
    static void SSetEnabled(bool Enabled);

    static TSigJmpBuffer& SPushJumpBuffer();
    static void SPopJumpBuffer();

    //! dumps stack and always returns true to execute the handler
    static bool SHandleException();
    
    static bool SRunningInExceptionHandler();
    static void SInvokeExceptionHander();

  private:
    friend class TThreadLocalValue<TStructuredExceptionHandler>;
    
    TStructuredExceptionHandler();
    
    TList<TSigJmpBuffer> mJumpBuffers;

    static TThreadLocalValue<TStructuredExceptionHandler>* spJumpHandler;
    static bool sEnabled;
  };

#elif defined(MCompiler_VisualCPP)

  class TStructuredExceptionHandler
  {
  public:
    //! By default enabled. When disabled exceptions are logged but handlers
    //! are no longer executed.
    static bool SIsEnabled();
    static void SSetEnabled(bool Enabled);

    //! dumps stack to the log and returns handler code for "__except"
    static int SHandleException(struct _EXCEPTION_POINTERS* ep);

  private:
    static bool sEnabled;
  };

#else
  #error "Unknown compiler"

#endif


//! let the debugger catch all exceptions in debug or no SEH builds
#if defined(MDebug) || defined(MNoSEH)

  #define MTry_SystemException
  #define MCatch_SystemException if (sFalse)
  #define MCatch_SystemExceptionAndDumpStack if (sFalse)
  #define MFinalize_SystemException

#else

  #if defined(MCompiler_VisualCPP)
  
    #define MTry_SystemException \
      bool __MFinalize_SystemException__missing__; \
      __try
      
    #define MCatch_SystemException \
      __except(1) // 1 means "execute catch block" here

    #define MCatch_SystemExceptionAndDumpStack \
      __except(TStructuredExceptionHandler::SHandleException(GetExceptionInformation()))

    #define MFinalize_SystemException \
      __MFinalize_SystemException__missing__ = false; \
      (void)(__MFinalize_SystemException__missing__);


  #elif defined(MCompiler_GCC)

    #define MTry_SystemException \
      bool __MFinalize_SystemException__missing__; \
      if (::sigsetjmp(TStructuredExceptionHandler::SPushJumpBuffer().mBuffer, 1) == 0)
    
    #define MCatch_SystemException \
      else 
    
    #define MCatch_SystemExceptionAndDumpStack \
      else if (TStructuredExceptionHandler::SHandleException())

    #define MFinalize_SystemException \
      __MFinalize_SystemException__missing__ = false; \
      (void)(__MFinalize_SystemException__missing__); \
      TStructuredExceptionHandler::SPopJumpBuffer();

    // signal handers can invoke the current catch block via:
    // if (TStructuredExceptionInfo::SRunningInExceptionHandler())
    // {
    //    TStructuredExceptionInfo::SInvokeExceptionHander();
    // }

  #else
    #error "Unknown compiler" 
  #endif

#endif


#endif // _Exception_h_

