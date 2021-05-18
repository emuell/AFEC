#pragma once

#if !defined(_OggVorbis_h_)
#define _OggVorbis_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("Ogg")
  #pragma MAddLibrary("Vorbis")
  #pragma MAddLibrary("VorbisFile")
#endif

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma pack(push)
  #pragma warning(push)
#endif
 
#include <vorbis/vorbisfile.h>

#if defined(MCompiler_VisualCPP)
  #pragma warning(pop)
  #pragma pack(pop)
#endif


#endif

