#include "CoreTypesPrecompiledHeader.h"

#include <execinfo.h>

// =================================================================================================

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
  void* StackArray[100];
  
  const size_t NumFrames = ::backtrace(StackArray, 
    sizeof(StackArray) / sizeof(StackArray[0]));

  const size_t EndAt = (Depth == -1) ? 
    (size_t)(StartAt + NumFrames) : MMin<size_t>(Depth, NumFrames);
  
  for (size_t i = StartAt; i < EndAt; ++i)
  {
    FramePointers.push_back((uintptr_t)StackArray[i]);  
  }
}

// -------------------------------------------------------------------------------------------------

void TStackWalk::DumpStack(
  const std::vector<uintptr_t>& FramePointers, 
  std::vector<std::string>&     Stack)
{
  if (FramePointers.size())
  {
    char** ppStrings = backtrace_symbols((void**)&FramePointers[0], 
      (int)FramePointers.size());  
     
    if (ppStrings)
    {
      for (size_t i= 0; i < FramePointers.size(); ++i)  
      {
        Stack.push_back(ppStrings[i]);
      }
        
      ::free(ppStrings);  
    }
  }
}

