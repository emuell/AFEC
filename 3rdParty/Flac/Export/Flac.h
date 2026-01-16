#pragma once

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("Flac")

#elif defined(MCompiler_Clang)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma pack(push)
  #pragma warning(push)
#endif

#if defined(MCompiler_VisualCPP)
  #define FLAC__NO_DLL
#endif

extern "C" {
  #include <FLAC/all.h>
}

#include <FLAC++/export.h>
#include <FLAC++/encoder.h>
#include <FLAC++/decoder.h>
#include <FLAC++/metadata.h>

#if defined(MCompiler_VisualCPP)
  #pragma warning(pop)
  #pragma pack(pop)

#elif defined(MCompiler_Clang)
  #pragma clang diagnostic pop
#endif


