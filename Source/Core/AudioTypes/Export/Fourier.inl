#pragma once

#ifndef _Fourier_inl_
#define _Fourier_inl_

// =================================================================================================

#include "CoreTypes/Export/InlineMath.h"
#include "CoreTypes/Export/Debug.h"

#include <cmath>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline double* TFftTransformComplex::Re()
{
  return mpProcessedReal;
}

// -------------------------------------------------------------------------------------------------

MForceInline double* TFftTransformComplex::Im()
{
  return mpProcessedImag;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline double* TFftTransformComplexOouora::Re()
{
  return mpProcessedReal;
}

// -------------------------------------------------------------------------------------------------

MForceInline double* TFftTransformComplexOouora::Im()
{
  return mpProcessedImag;
}

#endif // _Fourier_inl_

