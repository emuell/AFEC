#pragma once

#if !defined(_Iconv_h_)
#define _Iconv_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("Iconv")
#endif

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma pack(push)
  #pragma warning(push)
#endif

extern "C"
{
  #if defined(MWindows)
    #define _SECURECRT_ERRCODE_VALUES_DEFINED
    #include <iconv.h.msvc-static>
  #else
    #include <iconv.h.in>
  #endif
}

#if defined(MCompiler_VisualCPP)
  #pragma warning(pop)
  #pragma pack(pop)
#endif


#endif

