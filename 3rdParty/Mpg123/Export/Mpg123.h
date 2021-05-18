#pragma once 

#if !defined(Mpg123_h)
#define Mpg123_h

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("Mpg123")
#endif

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma pack(push)
  #pragma warning(push)
#endif

#define MPG123_NO_CONFIGURE
#include <mpg123.h>
// #include <libmpg123/mpg123.h>
#undef MPG123_NO_CONFIGURE


#ifdef __cplusplus
extern "C" {
#endif

  #define off_t long long
  
  int mpg123_open_64(mpg123_handle*, const char*);
  off_t mpg123_seek_64(mpg123_handle*, off_t, int whence);
  off_t mpg123_tell_64(mpg123_handle*);
  int mpg123_position_64(mpg123_handle*, off_t, off_t, off_t*, off_t*, double*, double*);
  
  #undef off_t

#ifdef __cplusplus
} 
#endif


#if defined(MCompiler_VisualCPP)
  #pragma warning(pop)
  #pragma pack(pop)
#endif


#endif

