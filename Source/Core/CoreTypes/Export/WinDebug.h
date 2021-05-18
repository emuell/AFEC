#if defined(__WinDebug_h__)
  #error do not include this file directly!
#endif

#define __WinDebug_h__

// =================================================================================================

#if defined (MDebug)

  namespace Win32Debug // avoid including windows headers
  {
    extern "C" int __cdecl _CrtDbgReport(int, const char *, int, 
      const char *, const char *, ...);
    
    extern "C" int __stdcall IsDebuggerPresent();

    extern "C" char* __cdecl strcpy(char*, const char*);
    extern "C" char* __cdecl strcat(char*, const char*);
  }

  #include <cfloat>

  #define M__EnableFloatingPointAssertions  \
    if (gEnableFloatingPointExceptions) \
    { \
      ::_clearfp(); \
      \
      ::_controlfp((unsigned)~(_EM_INVALID | _EM_ZERODIVIDE | \
        _EM_OVERFLOW | _EM_UNDERFLOW |_EM_DENORMAL),  \
        (unsigned)_MCW_EM); \
    }
                                                                 
  #define M__DisableFloatingPointAssertions \
    if (gEnableFloatingPointExceptions) \
    { \
      ::_clearfp(); \
      \
      ::_controlfp((unsigned)(_EM_INVALID | _EM_ZERODIVIDE |  \
        _EM_OVERFLOW | _EM_UNDERFLOW |_EM_DENORMAL | _EM_INEXACT),  \
        (unsigned)_MCW_EM); \
    }

  #define MBaseAssertNoThrow(Expression, pText) \
    if (!(Expression))  \
    { \
      char __Text[2048];  \
      Win32Debug::strcpy(__Text, pText);  \
      Win32Debug::strcat(__Text, "\n"); \
      \
      Win32Debug::_CrtDbgReport(2, __FILE__, __LINE__,  \
        #Expression, __Text); \
      \
      if (gAssertBreaksIntoDebugger)  \
      { \
        __debugbreak(); \
      } \
    }

  #define MBaseAssert(Expression, pText)  \
    if (!(Expression))  \
    { \
      char __Text[2048];  \
      Win32Debug::strcpy(__Text, pText);  \
      Win32Debug::strcat(__Text, "\n"); \
      \
      if (Win32Debug::IsDebuggerPresent()) \
      { \
        Win32Debug::_CrtDbgReport(0, __FILE__, __LINE__,  \
          #Expression, __Text); \
        \
        if (gAssertBreaksIntoDebugger)  \
        { \
          __debugbreak(); \
        } \
        else if (gAssertTracesStack) \
        { \
          gTraceStack(); \
        } \
      } \
      else  \
      { \
        Win32Debug::_CrtDbgReport(2, __FILE__, __LINE__,  \
          #Expression, __Text); \
      } \
    }                            

#else

  #define M__EnableFloatingPointAssertions
  #define M__DisableFloatingPointAssertions

  #define MBaseAssert(Expression, Text)
  
#endif  

