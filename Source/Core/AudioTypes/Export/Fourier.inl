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
  return (double*)mpProcessedReal;
}

// -------------------------------------------------------------------------------------------------

MForceInline double* TFftTransformComplex::Im()
{
  return (double*)mpProcessedImag;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename SampleType>
SampleType TFftWindow::SFillBuffer(
  const TWindowType& Type, SampleType* pBuffer, int Size)
{
  const double DeltaSize = 1.0 / (double)(Size - 1);

  SampleType WindowFactor = (SampleType)(0.0);

  switch(Type)
  {
  case kNone:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = (SampleType)(1.0);

      WindowFactor += pBuffer[i];
    }
    break;

  case kRectangular:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = (SampleType)(0.707);

      WindowFactor += pBuffer[i];
    }
    break;

  case kRaisedCosine:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = (SampleType)(-0.5 * ::cos(M2Pi * (double)i * DeltaSize) + 0.5);
      WindowFactor += pBuffer[i];
    }
    break;

  case kHamming:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = (SampleType)(0.54 - (0.46 * ::cos(M2Pi * (double)i * DeltaSize)));
      WindowFactor += pBuffer[i];
    }
    break;

  case kHanning:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = (SampleType)(0.5 * (1.0 - ::cos(M2Pi * (double)i * DeltaSize)));
      WindowFactor += pBuffer[i];
    }
    break;

  case kBlackman:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = (SampleType)(0.42 - (0.5 * ::cos(M2Pi * (double)i * DeltaSize)) +
          (0.08 * ::cos(4.0 * MPi * (double)i * DeltaSize)));

      WindowFactor += pBuffer[i];
    }
    break;

  case kBlackmanHarris:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = (SampleType)(0.42323 - (0.49755 * ::cos(M2Pi * (double)i * DeltaSize)) +
          (0.07922 * ::cos(4.0 * MPi * (double)i * DeltaSize)));

      WindowFactor += pBuffer[i];
    }
    break;

  case kGaussian:
    for (int i = 0; i < Size; ++i)
    {
      static const double SConstE = 2.7182818284590452353602875;

      pBuffer[i] = (SampleType)(::pow(SConstE, -0.5 *
        ::pow(((double)i - (double)(Size - 1) / 2.0) /
          (0.1 * (double)(Size - 1) / 2.0), 2.0)));

      WindowFactor += pBuffer[i];
    }
    break;

  case kFlatTop:
    for (int i = 0; i < Size; ++i)
    {
      static const double SA0 = 1.0, SA1 = 1.93, SA2 = 1.29, SA3 = 0.388, SA4 = 0.032;

      pBuffer[i] = (SampleType)(SA0 -
        SA1 * ::cos(2.0 * MPi * i * DeltaSize) +
        SA2 * ::cos(4.0 * MPi * i * DeltaSize) -
        SA3 * ::cos(6.0 * MPi * i * DeltaSize) +
        SA4 * ::cos(8.0 * MPi * i * DeltaSize));

      WindowFactor += pBuffer[i];
    }
    break;

  default:
    MInvalid("Wrong fft window type");
    break;
  }

  return WindowFactor;
}


#endif // _Fourier_inl_

