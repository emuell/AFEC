#if defined(__LinuxDebug_h__)
  #error do not include this file directly!
#endif

#define __LinuxDebug_h__

// =================================================================================================

#if defined(MDebug)

  #include <fenv.h> // feenableexcept, feclearexcept, ...

  namespace TLinuxDebug // avoid including system headers
  {
    extern "C" int printf(const char * __restrict, ...);
  }

  #define M__EnableFloatingPointAssertions  \
    if (gEnableFloatingPointExceptions) \
    { \
      ::feenableexcept(FE_DIVBYZERO | FE_INVALID | \
        FE_OVERFLOW | FE_UNDERFLOW); \
      ::feclearexcept(FE_ALL_EXCEPT); \
    }
                                                                 
  #define M__DisableFloatingPointAssertions \
    if (gEnableFloatingPointExceptions) \
    { \
      ::fedisableexcept(FE_ALL_EXCEPT); \
      ::feclearexcept(FE_ALL_EXCEPT); \
    }

  #define MBaseAssert(Expression, pText)  \
    if (!(Expression))  \
    { \
      TLinuxDebug::printf("## FAILED assertion: %s:%d %s %s\n",  \
        __FILE__, __LINE__, #Expression, pText);  \
      \
      if (gAssertBreaksIntoDebugger) \
      { \
        __asm__ __volatile__("int3"); \
      } \
      else if (gAssertTracesStack) \
      { \
        gTraceStack(); \
      } \
    }

#else

  #define M__EnableFloatingPointAssertions
  #define M__DisableFloatingPointAssertions

  #define MBaseAssert(Expression, Text)

#endif // #!MDebug

