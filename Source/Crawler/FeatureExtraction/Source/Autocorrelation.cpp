#include "FeatureExtraction/Source/Autocorrelation.h"

#include "CoreTypes/Export/Array.h"

#if defined(MHaveIntelIPP)
  #include "../../3rdParty/IPP/Export/IPPs.h"
#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TAutocorrelation::CalcWarped(
  const double* pSampleData,
  int           NumSamples,
  double*       pCorrelationCoeffs,
  int           Order,
  double        Lambda)
{
  MAssert(Lambda > 0.0 && Lambda < 1.0, "Invalid lambda");

  // copy the input
  TArray<double> TempSampleData(NumSamples);
  for (int i = 0; i < NumSamples; ++i)
  {
    TempSampleData[i] = pSampleData[i];
  }

  // calc coeffs
  for (int i = 0; i < Order; ++i)
  {
    double PreviousInput = 0.0;
    pCorrelationCoeffs[i] = 0.0;

    for (int j = 0; j < NumSamples; ++j)
    {
      // the auto correlation
      pCorrelationCoeffs[i] += TempSampleData[j] * pSampleData[j];

      // warp the correlation vector by applying the allpass filter
      const double tmp = (j == 0) ?
        -Lambda * TempSampleData[j] :
        (TempSampleData[j - 1] - TempSampleData[j])*Lambda + PreviousInput;

      PreviousInput = TempSampleData[j];
      TempSampleData[j] = tmp;
    }
  }

  // normalize coefs by dividing by first element
  if (pCorrelationCoeffs[0] != 0)
  {
    for (int i = Order - 1; i >= 0; i--)
    {
      pCorrelationCoeffs[i] /= pCorrelationCoeffs[0];
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TAutocorrelation::Calc(
  const double* pSampleData,
  int           NumSamples,
  double*       pCorrelationCoeffs,
  int           Order)
{
  // calc coeffs
#if defined(MHaveIntelIPP)
  IppEnum Algorithm = (IppEnum)(ippAlgAuto | ippsNormNone);

  int BufferSize = 0;
  MCheckIPPStatus(
    ippsAutoCorrNormGetBufferSize(NumSamples, Order,
      ipp64f, Algorithm, &BufferSize));

  Ipp8u* pTempBuffer = ::ippsMalloc_8u(BufferSize);

  MCheckIPPStatus(
    ippsAutoCorrNorm_64f(pSampleData, NumSamples,
      pCorrelationCoeffs, Order, Algorithm, pTempBuffer));

  ::ippsFree(pTempBuffer);

#else
  for (int i = 0; i < Order; ++i)
  {
    pCorrelationCoeffs[i] = 0.0;
    for (int j = 0, jEnd = (NumSamples - i); j < jEnd; ++j)
    {
      pCorrelationCoeffs[i] += pSampleData[j] * pSampleData[j + i];
    }
  }
#endif

// normalize coefs by dividing by first element
  if (pCorrelationCoeffs[0] != 0)
  {
    for (int i = Order - 1; i >= 0; i--)
    {
      pCorrelationCoeffs[i] /= pCorrelationCoeffs[0];
    }
  }
}

