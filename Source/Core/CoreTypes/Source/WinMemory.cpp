#include "CoreTypesPrecompiledHeader.h"

#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if defined(MDebug)
  #include <crtdbg.h>
#endif

#include <xmmintrin.h>

// =================================================================================================

#if !defined(MDebug)

  // Disable "HeapEnableTerminationOnCorruption" exceptions in release builds, 
  // which get enabled by VS2010's runtime startup routines by default. No 
  // exception handler except the debugger's can catch such exceptions, so we 
  // would immediately get closed without getting the chance to save crash 
  // backups. See http://dev.log.mumbi.net/534 for the leaked runtime startup 
  // code.

  extern "C" int _NoHeapEnableTerminationOnCorruption = 1;

#endif // !defined(MDebug)

// =================================================================================================

#if defined(MDebug)

namespace {

class TDebugLeakChecker
{
public:
  TDebugLeakChecker()
  {
    int DebugFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

    DebugFlags |= _CRTDBG_ALLOC_MEM_DF;        // Turn ON debug allocation
    DebugFlags |= _CRTDBG_LEAK_CHECK_DF;       // Turn ON leak-checking bit

    DebugFlags &= ~_CRTDBG_CHECK_CRT_DF;       // Turn OFF CRT block checking bit
    DebugFlags &= ~_CRTDBG_DELAY_FREE_MEM_DF;  // Turn OFF Don't actually free memory
    DebugFlags &= ~_CRTDBG_CHECK_ALWAYS_DF;    // Turn OFF Check heap every alloc/dealloc
    DebugFlags &= ~_CRTDBG_CHECK_CRT_DF;       // Turn OFF Leak check/diff CRT blocks

    _CrtSetDbgFlag(DebugFlags);
  }
};

// enable leak checks with the static initializers
TDebugLeakChecker sInvokeDebugLeakChecker;

} // namespace {

#endif // defined(MDebug)

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static void SSetByteMemset(void* pDest, TUInt8 FillByte, size_t ByteCount)
{
  // memset should be optimized already (but has a different signature):
  ::memset(pDest, (int)FillByte, ByteCount);
}

// -------------------------------------------------------------------------------------------------

static void SSetWordSSE(void* pDest, TUInt16 FillWord, size_t WordCount)
{
  TUInt16* pWordBuffer = (TUInt16*)pDest;

  // need more than 8 words (16 avoids overhead) and word aligned buffers for SSE
  if (WordCount < 16 || ((ptrdiff_t)pWordBuffer & 0x01) != 0)
  {
    for (size_t i = 0; i < WordCount; ++i)
    {
      pWordBuffer[i] = FillWord;
    }
  }
  else
  {
    // we're hackily filling float registers with int bytes:
    M__DisableFloatingPointAssertions

    const ptrdiff_t AlignBytes = ((ptrdiff_t)pWordBuffer & 0x0F);

    switch (AlignBytes >> 1)
    {
    case 1: --WordCount; *pWordBuffer++ = FillWord;
    case 2: --WordCount; *pWordBuffer++ = FillWord;
    case 3: --WordCount; *pWordBuffer++ = FillWord;
    case 4: --WordCount; *pWordBuffer++ = FillWord;
    case 5: --WordCount; *pWordBuffer++ = FillWord;
    case 6: --WordCount; *pWordBuffer++ = FillWord;
    case 7: --WordCount; *pWordBuffer++ = FillWord;
    }

    MAssert(((ptrdiff_t)pWordBuffer & 0x0F) == 0, "Buffer must be aligned here");
    __m128 *pVectorBuffer = (__m128 *)pWordBuffer;

    const TUInt32 FillDWord = 
      ((TUInt32)FillWord << 0) | ((TUInt32)FillWord << 16);

    MStaticAssert(sizeof(float) == sizeof(TUInt32));
    const __m128 vFillDWord =  _mm_set_ps1(*((float*)&FillDWord));

    size_t VectorCount = (WordCount >> 3);

    while (VectorCount--)
    {
      *pVectorBuffer = vFillDWord;
      ++pVectorBuffer;
    }

    pWordBuffer = (TUInt16*)pVectorBuffer;

    switch (WordCount&7)
    {
    case 7: *pWordBuffer++ = FillWord;
    case 6: *pWordBuffer++ = FillWord;
    case 5: *pWordBuffer++ = FillWord;
    case 4: *pWordBuffer++ = FillWord;
    case 3: *pWordBuffer++ = FillWord;
    case 2: *pWordBuffer++ = FillWord;
    case 1: *pWordBuffer++ = FillWord;
    }

    M__EnableFloatingPointAssertions
  }
}

// -------------------------------------------------------------------------------------------------

static void SSetDWordSSE(void* pDest, TUInt32 FillDWord, size_t DWordCount)
{
  TUInt32* pDWordBuffer = (TUInt32*)pDest;

  // need more than 4 words (8 avoids overhead) and dword aligned buffers for SSE
  if (DWordCount < 8 || ((ptrdiff_t)pDWordBuffer & 0x03) != 0)
  {
    for (TUInt32 i = 0; i < DWordCount; ++i)
    {
      pDWordBuffer[i] = FillDWord;
    }
  }
  else
  {
    // we're hackily filling float registers with int bytes:
    M__DisableFloatingPointAssertions

    const ptrdiff_t AlignBytes = ((ptrdiff_t)pDWordBuffer & 0x0F);

    switch (AlignBytes >> 2)
    {
    case 1: --DWordCount; *pDWordBuffer++ = FillDWord;
    case 2: --DWordCount; *pDWordBuffer++ = FillDWord;
    case 3: --DWordCount; *pDWordBuffer++ = FillDWord;
    }

    MAssert(((ptrdiff_t)pDWordBuffer & 0x0F) == 0, "Buffer must be aligned here");
    __m128 *pVectorBuffer = (__m128 *)pDWordBuffer;
    
    MStaticAssert(sizeof(float) == sizeof(TUInt32));
    const __m128 vFillDWord =  _mm_set_ps1(*((float*)&FillDWord));

    size_t VectorCount = (DWordCount >> 2);

    while (VectorCount--)
    {
      *pVectorBuffer = vFillDWord;
      ++pVectorBuffer;
    }

    pDWordBuffer = (TUInt32*)pVectorBuffer;

    switch (DWordCount&3)
    {
    case 3: *pDWordBuffer++ = FillDWord;
    case 2: *pDWordBuffer++ = FillDWord;
    case 1: *pDWordBuffer++ = FillDWord;
    }

    M__EnableFloatingPointAssertions
  }
}

// =================================================================================================

namespace TMemory 
{
  typedef void (*TPSetByteFunc)(void*, TUInt8, size_t);
  typedef void (*TPSetWordFunc)(void*, TUInt16, size_t);
  typedef void (*TPSetDWordFunc)(void*, TUInt32, size_t);

  TPSetByteFunc spSetByteFunc = NULL;
  TPSetWordFunc spSetWordFunc = NULL;
  TPSetDWordFunc spSetDWordFunc = NULL;
}

// -------------------------------------------------------------------------------------------------

void TMemory::Init()
{
  // ... setup memcopy functions

  spSetByteFunc = ::SSetByteMemset;
  spSetWordFunc = ::SSetWordSSE;
  spSetDWordFunc = ::SSetDWordSSE;

  
  // ... enable Low Fragmentation Heaps 
  
  HINSTANCE pKernelLib = ::LoadLibrary(L"Kernel32.dll");

  if (pKernelLib)
  {
    typedef BOOL (__stdcall *TPHeapSetInformation)(
      HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);

    TPHeapSetInformation pHeapSetInformation = (TPHeapSetInformation)
      ::GetProcAddress(pKernelLib, "HeapSetInformation");

    if (pHeapSetInformation != NULL)
    {              
      // enable Low Fragmentation Heaps
      HANDLE Heaps[1025];
      DWORD NumberOfHeaps = ::GetProcessHeaps(1024, Heaps);

      for (DWORD i = 0; i < NumberOfHeaps; ++i)
      {
        ULONG HeapFragValue = 2; // LFH

        BOOL Result = pHeapSetInformation(Heaps[i], 
          HeapCompatibilityInformation, &HeapFragValue, sizeof(HeapFragValue));

        // will fail for some heaps and also in the debugger, when the 
        // debug heap is enabled...

        MUnused(Result);
      }
    }
  } 

  // run our own leak checker at exit
  #if defined(MDebug)
    ::atexit(TMemory::DumpLeaks);
  #endif
}

