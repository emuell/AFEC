#pragma once

#ifndef _LibXtract_h_
#define _LibXtract_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("LibXtract")
#endif

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma pack(push)
  #pragma warning(push)
#endif

#include "xtract/libxtract.h"
#include "xtract/xtract_stateful.h"
#include "xtract/xtract_scalar.h"
#include "xtract/xtract_helper.h"

#if defined(MCompiler_VisualCPP)
  #pragma warning(pop)
  #pragma pack(pop)
#endif


#endif // _LibXtract_h_

