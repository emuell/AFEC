#pragma once

#ifndef _Aubio_h_
#define _Aubio_h_

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("Aubio")
#endif

// =================================================================================================

// we want to use "double" as smp_t type in Aubio
#define HAVE_AUBIO_DOUBLE 1

// include all public aubio internfaces
#include <aubio.h>

// include a few internals that we're using
#include <tempo/beattracking.h>


#endif // _Aubio_h_

