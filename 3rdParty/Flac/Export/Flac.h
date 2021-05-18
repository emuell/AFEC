#pragma once

#if !defined(_Freetype_h_)
#define _Freetype_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("Flac")
#endif

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma pack(push)
  #pragma warning(push)

#elif defined(MCompiler_Clang)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Woverloaded-virtual"
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


#endif

