#pragma once

#ifndef _Debug_h_
#define _Debug_h_

// =================================================================================================

//! Non const true, false statements. Internally used by the debug macros.
extern bool sFalse, sTrue;

//! MAssert behavior (options exclude each others in following order).
extern bool gAssertBreaksIntoDebugger; // true
extern bool gAssertTracesStack; // false

//! FPU exceptions.
//! Also controls M__Enable/DisableFloatingPointAssertions behavior.
extern bool gEnableFloatingPointExceptions; // true

#include "CoreTypes/Export/CompilerDefines.h"

#if defined(MWindows)
  #include "CoreTypes/Export/WinDebug.h"
#elif defined(MMac)
  #include "CoreTypes/Export/MacDebug.h"
#elif defined(MLinux)
  #include "CoreTypes/Export/LinuxDebug.h"
#else
  #error "Unknown platform"
#endif

// =================================================================================================

#if !(defined(MDebug) || defined(MRelease)) || (defined(MDebug) && defined(MRelease))
  #error "you have to define either MDebug or MRelease !"
#endif

// -------------------------------------------------------------------------------------------------

#if defined(MDebug)
  #define MAssert(Expression, Text) MBaseAssert(Expression, Text)
  #define MInvalid(Text) MAssert(sFalse, Text)
  #define MCodeMissing MInvalid("Code Missing")
  
  #define MTrace(Text) gTraceVar(Text);
  
  #define MTrace1(FormatText, Var1) \
    gTraceVar(FormatText, Var1);
  #define MTrace2(FormatText, Var1, Var2) \
    gTraceVar(FormatText, Var1, Var2);
  #define MTrace3(FormatText, Var1, Var2, Var3) \
    gTraceVar(FormatText, Var1, Var2, Var3);
  #define MTrace4(FormatText, Var1, Var2, Var3, Var4) \
    gTraceVar(FormatText, Var1, Var2, Var3, Var4);
  #define MTrace5(FormatText, Var1, Var2, Var3, Var4, Var5) \
    gTraceVar(FormatText, Var1, Var2, Var3, Var4, Var5);
  #define MTrace6(FormatText, Var1, Var2, Var3, Var4, Var5, Var6) \
    gTraceVar(FormatText, Var1, Var2, Var3, Var4, Var5, Var6);
  #define MTrace7(FormatText, Var1, Var2, Var3, Var4, Var5, Var6, Var7) \
    gTraceVar(FormatText, Var1, Var2, Var3, Var4, Var5, Var6, Var7);
  #define MTrace8(FormatText, Var1, Var2, Var3, Var4, Var5, Var6, Var7, Var8) \
    gTraceVar(FormatText, Var1, Var2, Var3, Var4, Var5, Var6, Var7, Var8);

#elif defined(MRelease)
  #define MAssert(Expression, Text) 
  #define MInvalid(Text)
  #define MCodeMissing 
  
  #define MTrace(Text)
  #define MTrace1(FormatText, Var1)
  #define MTrace2(FormatText, Var1, Var2)
  #define MTrace3(FormatText, Var1, Var2, Var3)
  #define MTrace4(FormatText, Var1, Var2, Var3, Var4)
  #define MTrace5(FormatText, Var1, Var2, Var3, Var4, Var5)
  #define MTrace6(FormatText, Var1, Var2, Var3, Var4, Var5, Var6)
  #define MTrace7(FormatText, Var1, Var2, Var3, Var4, Var5, Var6, Var7)
  #define MTrace8(FormatText, Var1, Var2, Var3, Var4, Var5, Var6, Var7, Var8)
  
#endif

// =================================================================================================

//! Available in debug and release builds. Prints a message to the std out.
void gTrace(const char* pString);
void gTrace(const class TString& String);

#if defined(MCompiler_GCC)
  void gTraceVar(const char* pString, ...) 
    __attribute__((format(printf, 1, 2)));
#else
  void gTraceVar(const char* pString, ...);
#endif
 
//! Available in debug and release builds. Prints the stack to the std out.
void gTraceStack();


#endif // _Debug_h_

