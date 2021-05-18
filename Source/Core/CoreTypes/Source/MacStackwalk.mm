#include "CoreTypesPrecompiledHeader.h"

#include <cxxabi.h>

#define DLOPEN_NO_WARN // suppress uncritical warning for OSX 10.3.9
#include <dlfcn.h>

// =================================================================================================

// note could use <execinfo> as soon as we are targeting OSX >= 10.5

typedef int (*backtrace_proc)(void**,int);
typedef char** (*backtrace_symbols_proc)(void* const*,int);

backtrace_proc backtrace = NULL;
backtrace_symbols_proc backtrace_symbols = NULL;

// -------------------------------------------------------------------------------------------------

static void* SBuiltinReturnAddress (const int level)
{
  #define GET_STACK_ADDRESS(level) \
    return (__builtin_frame_address (level) ? \
      __builtin_return_address (level) : NULL);

  switch (level)
  {
    case  0: GET_STACK_ADDRESS (1)
    case  1: GET_STACK_ADDRESS (2)
    case  2: GET_STACK_ADDRESS (3)
    case  3: GET_STACK_ADDRESS (4)
    case  4: GET_STACK_ADDRESS (5)
    case  5: GET_STACK_ADDRESS (6)
    case  6: GET_STACK_ADDRESS (7)
    case  7: GET_STACK_ADDRESS (8)
    case  8: GET_STACK_ADDRESS (9)
    case  9: GET_STACK_ADDRESS (10)
    case 10: GET_STACK_ADDRESS (11)
    case 11: GET_STACK_ADDRESS (12)
    case 12: GET_STACK_ADDRESS (13)
    case 13: GET_STACK_ADDRESS (14)
    case 14: GET_STACK_ADDRESS (15)
    case 15: GET_STACK_ADDRESS (16)
    case 16: GET_STACK_ADDRESS (17)
    case 17: GET_STACK_ADDRESS (18)
    case 18: GET_STACK_ADDRESS (19)
    case 19: GET_STACK_ADDRESS (20)
    case 20: GET_STACK_ADDRESS (21)
    case 21: GET_STACK_ADDRESS (22)
    case 22: GET_STACK_ADDRESS (23)
    case 23: GET_STACK_ADDRESS (24)
    case 24: GET_STACK_ADDRESS (25)
    case 25: GET_STACK_ADDRESS (26)
    case 26: GET_STACK_ADDRESS (27)
    case 27: GET_STACK_ADDRESS (28)
    case 28: GET_STACK_ADDRESS (29)
    case 29: GET_STACK_ADDRESS (30)
    case 30: GET_STACK_ADDRESS (31)

    default: return (void*)0;
  }
}

// -------------------------------------------------------------------------------------------------

static int SBacktrace(void **ppFrameArray, int maxFrames)
{
  int i = 0;

  for (; i < maxFrames; ++i)
  {
    if ((ppFrameArray[i] = SBuiltinReturnAddress(i + 1)) == NULL)
    {  
      // We have reached stack top
      break;
    }
  }

  return i;
}

// -------------------------------------------------------------------------------------------------

static void SLoadBacktraceSyms()
{
  void* pSystemlib = ::dlopen("/usr/lib/libSystem.dylib", RTLD_NOW);
  
  if (pSystemlib) 
  {
    backtrace = (backtrace_proc)::dlsym(pSystemlib, "backtrace");
    backtrace_symbols = (backtrace_symbols_proc)::dlsym(pSystemlib, "backtrace_symbols");
  }
}

// =================================================================================================

static bool sStackWalkInitialized = false;

// -------------------------------------------------------------------------------------------------

void TStackWalk::Init()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

void TStackWalk::Exit()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

void TStackWalk::GetStack(
  std::vector<uintptr_t>& FramePointers,
  int                     Depth,  
  int                     StartAt)
{
  if (!sStackWalkInitialized)
  {
    sStackWalkInitialized = true;
    SLoadBacktraceSyms();
  }

  void* StackArray[100] = {0};
  
  size_t NumFrames;
  
  if (backtrace != NULL)
  {
    NumFrames = ::backtrace(StackArray, 
      sizeof(StackArray) / sizeof(StackArray[0]));
  }
  else
  {
    NumFrames = ::SBacktrace(StackArray, 
      sizeof(StackArray) / sizeof(StackArray[0]));
  }
  
  for (int i = 0; i < (int)NumFrames; ++i)
  {    
    if (i >= StartAt)
    {
      FramePointers.push_back((uintptr_t)StackArray[i]); 
       
      if (Depth > 0 && i - StartAt > Depth)
      {
        break;
      }
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TStackWalk::DumpStack(
  const std::vector<uintptr_t>& FramePointers, 
  std::vector<std::string>&     Stack)
{
  if (!sStackWalkInitialized)
  {
    sStackWalkInitialized = true;
    SLoadBacktraceSyms();
  }
  
  if (backtrace_symbols != NULL)
  {
    if (FramePointers.size())
    {
      char** ppStrings = ::backtrace_symbols((void**)&FramePointers[0], 
        (int)FramePointers.size());  
     
      if (ppStrings)
      {
        for (size_t i= 0; i < FramePointers.size(); ++i)  
        {
          if (ppStrings[i])
          {
            Stack.push_back(ppStrings[i]);
          }
        }
        
        ::free(ppStrings);  
      }
    }
  }
  else
  {
    for (int i = 0; i < (int)FramePointers.size(); ++i)  
    {
      char AddressString[256];
      ::snprintf(AddressString, sizeof(AddressString), "%3d 0x%p", i, (void*)FramePointers[i]);
      
      char SymbolInfoString[2048];
      
      Dl_info info;
      int result = ::dladdr((void*)FramePointers[i], &info);

      if (0 == result)
      {
        ::snprintf(SymbolInfoString, sizeof(SymbolInfoString), " - ???");
      }
      else
      {
        int demangleStatus;
        char *pName = __cxxabiv1::__cxa_demangle (info.dli_sname, NULL, NULL, &demangleStatus);

        ::snprintf(SymbolInfoString, sizeof(SymbolInfoString), " - %s + %p %s", 
          (0 == demangleStatus) ? pName : info.dli_sname, 
          (void*)((ptrdiff_t)FramePointers[i] - (ptrdiff_t)info.dli_saddr), 
          info.dli_fname);

        if (NULL != pName)
        {
          delete pName;
        } 
      }
        
      Stack.push_back(std::string(AddressString) + std::string(SymbolInfoString));
    }
  }
}

