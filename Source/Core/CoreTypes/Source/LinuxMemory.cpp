#include "CoreTypesPrecompiledHeader.h"

#include <cstdlib>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TMemory::Init()
{
  #if defined(MDebug)
    ::atexit(TMemory::DumpLeaks);
  #endif
}

