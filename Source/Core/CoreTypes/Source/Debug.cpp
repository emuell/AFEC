#include "CoreTypesPrecompiledHeader.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#include <vector>
#include <string>

// =================================================================================================

MStaticAssert(sizeof(TInt8) == 1);
MStaticAssert(sizeof(TUInt8) == 1);
MStaticAssert(sizeof(TInt16) == 2);
MStaticAssert(sizeof(TUInt16) == 2);
MStaticAssert(sizeof(T24) == 3);
MStaticAssert(sizeof(TInt32) == 4);
MStaticAssert(sizeof(TUInt32) == 4);
MStaticAssert(sizeof(TInt64) == 8);
MStaticAssert(sizeof(TUInt64) == 8);

// =================================================================================================

bool sFalse = false;
bool sTrue = true;

#if defined(MArch_X86)
  // only enabled in x86 builds where denormal numbers are a problem
  bool gEnableFloatingPointExceptions = true;
#else
  bool gEnableFloatingPointExceptions = false;
#endif

bool gAssertBreaksIntoDebugger = true;
bool gAssertTracesStack = false;

// -------------------------------------------------------------------------------------------------

void gTrace(const char* pString)
{
  ::puts(pString);
  ::fflush(stdout);
}

void gTrace(const TString& String)
{
  TArray<char> CStringArray;
  String.CreateCStringArray(CStringArray, TString::kPlatformEncoding);
  CStringArray.Grow(CStringArray.Size() + 1);
  CStringArray[CStringArray.Size() - 1] = '\0';

  ::puts(CStringArray.FirstRead());
  ::fflush(stdout);
}

// -------------------------------------------------------------------------------------------------

void gTraceVar(const char* pString, ...)
{
  char TempChars[4096];

  va_list ArgList;
  va_start(ArgList, pString);
  vsnprintf(TempChars, sizeof(TempChars), 
    pString, ArgList);
  va_end(ArgList);  

  gTrace(TempChars);
}

// -------------------------------------------------------------------------------------------------

void gTraceStack()
{
  const int Depth = -1;
  const int StartAt = 2;
  std::vector<uintptr_t> FramePointers;
  std::vector<std::string> Stack;

  TStackWalk::GetStack(FramePointers, Depth, StartAt);
  TStackWalk::DumpStack(FramePointers, Stack);

  for (std::vector<std::string>::const_iterator Iter = Stack.begin();
        Iter != Stack.end(); ++Iter)
  {
    gTrace(("\t" + *Iter).c_str());
  }
}

