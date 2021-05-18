#include "CoreTypesPrecompiledHeader.h"

#include <list>

// =================================================================================================

TSystemGlobals* TSystemGlobals::spSystemGlobals = nullptr;

// -------------------------------------------------------------------------------------------------

TSystemGlobals* TSystemGlobals::SSystemGlobals()
{
  return spSystemGlobals;
}

// -------------------------------------------------------------------------------------------------

void TSystemGlobals::SCreate()
{
  spSystemGlobals = new TSystemGlobals();
}

// -------------------------------------------------------------------------------------------------

void TSystemGlobals::SDestroy()
{
  if (spSystemGlobals) 
  {
    delete spSystemGlobals;
    spSystemGlobals = nullptr;
  }
}

// -------------------------------------------------------------------------------------------------

TSystemGlobals::TSystemGlobals()
  : mAppHandle(NULL),
    mShowMessageFunc(TSystem::StandardMessageRedirector),
    mSupressInfoMessages(0)
{
}

