#pragma once 

#if !defined(Sqlite_h)
#define Sqlite_h

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("Sqlite")
#endif

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma pack(push)
  #pragma warning(push)
#endif

#include <sqlite3.h>

#if defined(MCompiler_VisualCPP)
  #pragma warning(pop)
  #pragma pack(pop)
#endif


#endif

