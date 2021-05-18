#if defined(__MacDebug_h__)
  #error do not include this file directly!
#endif

#define __MacDebug_h__

// =================================================================================================

#if defined(MDebug)

  namespace MacDebug // avoid including Mac Headers
  {
    extern "C" int printf(const char * __restrict, ...);
    extern "C" void Debugger();
  }

  // Code Missing
  #define M__EnableFloatingPointAssertions
  #define M__DisableFloatingPointAssertions

  #define MBaseAssert(Expression, pText)  \
    if (!(Expression))  \
    { \
      MacDebug::printf("failed assertion: %s:%d %s %s\n", \
        __FILE__, __LINE__, #Expression, pText);  \
      \
      if (gAssertBreaksIntoDebugger) \
      { \
        MacDebug::Debugger(); \
      } \
      else if (gAssertTracesStack) \
      { \
        gTraceStack(); \
      } \
    }

  #define MBaseAssertNoThrow(Expression, pText) \
    if (!(Expression))  \
    { \
      MacDebug::printf("failed assertion: %s:%d %s %s\n", \
        __FILE__, __LINE__, #Expression, pText);  \
      \
      if (gAssertBreaksIntoDebugger)  \
      { \
        MacDebug::Debugger(); \
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

