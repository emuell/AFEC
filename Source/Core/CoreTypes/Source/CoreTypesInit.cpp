#include "CoreTypesPrecompiledHeader.h"

// for srand
#include <cstdlib>

#if defined(MHaveIntelIPP)
  #include "../../3rdParty/IPP/Export/IPPs.h"
#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void CoreTypesInit() 
{
  // seed rand for TRandom
  ::srand(TSystem::TimeInMsSinceStartup());

  // init IPP
  #if defined(MHaveIntelIPP)
    const IppStatus Status = ::ippInit(); MUnused(Status);

    MAssert(Status == ippStsNoErr || Status == ippStsNonIntelCpu,
      ::ippGetStatusString(Status));
  #endif

  TCpu::Init();
  TMath::Init();
  TMemory::Init();

  TSystemGlobals::SCreate();

  TWeakReferenceable::SInit();
  
  TString::SInit();
  
  TOutOfMemoryException::SInit();

  #if defined(MCompiler_GCC)
    TStructuredExceptionHandler::SInit();
  #endif

  TStackWalk::Init();

  TLog::SCreate();
  
  gCleanUpTempDir(); // clean up old ones

  #if defined(MArch_X86)
    if (!(TCpu::Caps()&TCpu::kSse))
    {
      TLog::SLog()->AddLine("CPU", "SSE instruction set not supported. Bailing out..");

      throw TReadableException(MText("$MProductString requires a CPU with the SSE instruction "
        "set enabled and hence cannot run on this computer."));
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

void CoreTypesExit() 
{
  if (gTempDir().Exists() && !gTempDir().Unlink())
  {
    MInvalid("Failed to remove the temp dir...");
  }

  TLog::SDestroy();

  TStackWalk::Exit();

  TOutOfMemoryException::SExit();

  #if defined(MCompiler_GCC)
    TStructuredExceptionHandler::SExit();
  #endif  

  TString::SExit();
  
  TWeakReferenceable::SExit();

  TSystemGlobals::SDestroy();
}

