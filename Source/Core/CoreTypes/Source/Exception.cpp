#include "CoreTypesPrecompiledHeader.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TReadableException::WriteIntoLog() throw()
{
  try
  {
    if (TLog::SLog())
    {
      TLog::SLog()->AddLineNoVarArgs("Exception", Message());
    }
  }
  catch (...)
  {
    // silently ignore    
  }
}

// =================================================================================================

TString TOutOfMemoryException::sErrorMessage;

// -------------------------------------------------------------------------------------------------

void TOutOfMemoryException::SInit()
{
  #if defined(MLinux)
    sErrorMessage = MText(
      "Out of Memory! Please try to free up some memory and try again.\n"
      "If you think that you've got enough memory installed, make sure that "
      "your current users' or user groups' memory quota settings are correct. "
      "Please also check the amount of allowed locked memory.");
  #else
    sErrorMessage = MText("Out of Memory! Please try to "
      "free up some memory and try again.");
  #endif
}

// -------------------------------------------------------------------------------------------------

void TOutOfMemoryException::SExit()
{
  sErrorMessage = TString();
}

// =================================================================================================

#if defined(MCompiler_GCC)

bool TStructuredExceptionHandler::sEnabled = true;

TThreadLocalValue<TStructuredExceptionHandler>* 
  TStructuredExceptionHandler::spJumpHandler = NULL;

// -------------------------------------------------------------------------------------------------

void TStructuredExceptionHandler::SInit()
{
  spJumpHandler = new TThreadLocalValue<TStructuredExceptionHandler>();
}

// -------------------------------------------------------------------------------------------------

void TStructuredExceptionHandler::SExit()
{
  if (spJumpHandler)
  {
    delete spJumpHandler;
    spJumpHandler = NULL;
  }
}

// -------------------------------------------------------------------------------------------------

bool TStructuredExceptionHandler::SIsEnabled()
{
  return sEnabled;
}

// -------------------------------------------------------------------------------------------------

void TStructuredExceptionHandler::SSetEnabled(bool Enabled)
{
  sEnabled = Enabled;
}

// -------------------------------------------------------------------------------------------------

TStructuredExceptionHandler::TSigJmpBuffer& 
  TStructuredExceptionHandler::SPushJumpBuffer()
{
  return spJumpHandler->Object()->mJumpBuffers.Append(TSigJmpBuffer()).Last();
}

// -------------------------------------------------------------------------------------------------

void TStructuredExceptionHandler::SPopJumpBuffer()
{
  spJumpHandler->Object()->mJumpBuffers.DeleteLast();
}

// -------------------------------------------------------------------------------------------------

bool TStructuredExceptionHandler::SHandleException()
{
  const int Depth = -1; // unlimited
  const int Offset = 2;
    
  std::vector<uintptr_t> FramePointers;
  TStackWalk::GetStack(FramePointers, Depth, Offset);
  
  std::vector<std::string> Stack;
  TStackWalk::DumpStack(FramePointers, Stack);
  
  for (size_t i = 0; i < Stack.size(); ++i)
  {
    TLog::SLog()->AddLineNoVarArgs("CrashLog", Stack[i].c_str());
  }

  return true;
}
            
// -------------------------------------------------------------------------------------------------

bool TStructuredExceptionHandler::SRunningInExceptionHandler()
{
  return !spJumpHandler->Object()->mJumpBuffers.IsEmpty();
}

// -------------------------------------------------------------------------------------------------

void TStructuredExceptionHandler::SInvokeExceptionHander()
{
  MAssert(SRunningInExceptionHandler(), "Nothing to invoke!");

  TSigJmpBuffer& LastJumpBuffer = 
    spJumpHandler->Object()->mJumpBuffers.Last();
      
  ::siglongjmp(LastJumpBuffer.mBuffer, 1);
}
   
// -------------------------------------------------------------------------------------------------
   
TStructuredExceptionHandler::TStructuredExceptionHandler()
{
  // add some space for nested exception handlers...
  mJumpBuffers.PreallocateSpace(8); 
}



#elif defined(MCompiler_VisualCPP)


bool TStructuredExceptionHandler::sEnabled = true;

// -------------------------------------------------------------------------------------------------

bool TStructuredExceptionHandler::SIsEnabled()
{
  return sEnabled;
}

// -------------------------------------------------------------------------------------------------

void TStructuredExceptionHandler::SSetEnabled(bool Enabled)
{
  sEnabled = Enabled;
}

// -------------------------------------------------------------------------------------------------

int TStructuredExceptionHandler::SHandleException(_EXCEPTION_POINTERS* ep)
{
  TStackWalk::DumpExceptionStack(ep);

  return (sEnabled) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}


#endif 

