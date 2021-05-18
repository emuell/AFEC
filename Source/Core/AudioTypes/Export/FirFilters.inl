#pragma once

#ifndef _FirFilters_inl_
#define _FirFilters_inl_

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/Memory.h"
#include "CoreTypes/Export/InlineMath.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <int sFirSize, int sRatio>
TFirUpsampler<sFirSize, sRatio>::TFirUpsampler()
  : mpCoeffs(NULL)
{
  FlushBuffers();
}

// -------------------------------------------------------------------------------------------------

template <int sFirSize, int sRatio>
const double* TFirUpsampler<sFirSize, sRatio>::Coeffs()const
{
  return mpCoeffs;
}

// -------------------------------------------------------------------------------------------------

template <int sFirSize, int sRatio>
void TFirUpsampler<sFirSize, sRatio>::SetCoeffs(const double* pCoefs)
{
  mpCoeffs = pCoefs;
}

// -------------------------------------------------------------------------------------------------

template <int sFirSize, int sRatio>
inline double TFirUpsampler<sFirSize, sRatio>::Upsample(double Sample)
{
  MAssert(mpCoeffs, "No FIR coeffs set");

  mBuffer[mBufferIndex] = Sample;

  Sample = 0.0;

  int Z = 0, z = mBufferIndex;

  // help compiler to optimize unrolling
  while (Z < sFirSize - 4*sRatio + 1)
  {
    Sample += mpCoeffs[Z + 0*sRatio] * mBuffer[(z + kBufferSize - 0) & kBufferWrap];
    Sample += mpCoeffs[Z + 1*sRatio] * mBuffer[(z + kBufferSize - 1) & kBufferWrap];
    Sample += mpCoeffs[Z + 2*sRatio] * mBuffer[(z + kBufferSize - 2) & kBufferWrap];
    Sample += mpCoeffs[Z + 3*sRatio] * mBuffer[(z + kBufferSize - 3) & kBufferWrap];

    z -= 4; Z += 4*sRatio;
  }

  // process remaining unrolled coefs
  while (Z < sFirSize)
  {
    Sample += mpCoeffs[Z] * mBuffer[(z + kBufferSize) & kBufferWrap];
    --z; Z += sRatio;
  }

  MUnDenormalize(Sample);

  ++mBufferIndex &= kBufferWrap;

  return Sample;
}

// -------------------------------------------------------------------------------------------------

template <int sFirSize, int sRatio>
inline double TFirUpsampler<sFirSize, sRatio>::Pad(int Z)
{
  MAssert(mpCoeffs, "No FIR coeffs set");

  double Sample = 0.0;
  
  int z = mBufferIndex - 1;

  // help compiler to optimize unrolling
  while (Z < sFirSize - 4*sRatio + 1)
  {
    Sample += mpCoeffs[Z + 0*sRatio] * mBuffer[(z + kBufferSize - 0) & kBufferWrap];
    Sample += mpCoeffs[Z + 1*sRatio] * mBuffer[(z + kBufferSize - 1) & kBufferWrap];
    Sample += mpCoeffs[Z + 2*sRatio] * mBuffer[(z + kBufferSize - 2) & kBufferWrap];
    Sample += mpCoeffs[Z + 3*sRatio] * mBuffer[(z + kBufferSize - 3) & kBufferWrap];
    
    z -= 4; Z += 4*sRatio;
  }

  // process remaining unrolled coefs
  while (Z < sFirSize)
  {
    Sample += mpCoeffs[Z] * mBuffer[(z + kBufferSize) & kBufferWrap];
    --z; Z += sRatio;
  }

  MUnDenormalize(Sample);

  return Sample;
}

// -------------------------------------------------------------------------------------------------

template <int sFirSize, int sRatio>
void TFirUpsampler<sFirSize, sRatio>::FlushBuffers()
{
  mBufferIndex = 0;
  TMemory::Zero(mBuffer, sizeof(mBuffer));
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <int sFirSize>
TFirDownsampler<sFirSize>::TFirDownsampler()
  : mpCoeffs(NULL)
{
  FlushBuffers();
}

// -------------------------------------------------------------------------------------------------

template <int sFirSize>
const double* TFirDownsampler<sFirSize>::Coeffs()const
{
  return mpCoeffs;
}

// -------------------------------------------------------------------------------------------------

template <int sFirSize>
void TFirDownsampler<sFirSize>::SetCoeffs(const double* pCoefs)
{
  mpCoeffs = pCoefs;
}

// -------------------------------------------------------------------------------------------------

template <int sFirSize>
inline double TFirDownsampler<sFirSize>::Downsample(double Sample)
{
  MAssert(mpCoeffs, "No FIR coeffs set");

  mBuffer[mBufferIndex] = Sample;

  Sample *= mpCoeffs[0];
  MUnDenormalize(Sample);

  int Z = 1, z = mBufferIndex - 1;

  // help compiler to optimize unrolling
  while (Z < sFirSize - 3)
  {
    Sample += mpCoeffs[Z + 0] * mBuffer[(z + kBufferSize - 0) & kBufferWrap];
    Sample += mpCoeffs[Z + 1] * mBuffer[(z + kBufferSize - 1) & kBufferWrap];
    Sample += mpCoeffs[Z + 2] * mBuffer[(z + kBufferSize - 2) & kBufferWrap];
    Sample += mpCoeffs[Z + 3] * mBuffer[(z + kBufferSize - 3) & kBufferWrap];

    Z += 4; z -= 4;
  }

  // process remaining unrolled coefs
  while (Z < sFirSize)
  {
    Sample += mpCoeffs[Z] * mBuffer[(z + kBufferSize) & kBufferWrap];
    ++Z; --z;
  }

  MUnDenormalize(Sample);

  ++mBufferIndex &= kBufferWrap;

  return Sample;
}

// -------------------------------------------------------------------------------------------------

template <int sFirSize>
inline void TFirDownsampler<sFirSize>::Flush(double Sample)
{
  mBuffer[mBufferIndex] = Sample;
  ++mBufferIndex &= kBufferWrap;
}

// -------------------------------------------------------------------------------------------------

template <int sFirSize>
void TFirDownsampler<sFirSize>::FlushBuffers()
{
  mBufferIndex = 0;
  TMemory::Zero(mBuffer, sizeof(mBuffer));
}

// -------------------------------------------------------------------------------------------------

template <int sFirSize>
void TFirDownsampler<sFirSize>::CopyBuffers(const TFirDownsampler<sFirSize>& Other)
{
  mBufferIndex = Other.mBufferIndex;
  TMemory::Copy(mBuffer, Other.mBuffer, sizeof(mBuffer));
}


#endif // _FirFilters_inl_

