#pragma once

#ifndef _Autocorrelation_h_
#define _Autocorrelation_h_

// =================================================================================================

namespace TAutocorrelation 
{
  // find the order-P autocorrelation array, R, for the sequence \param pSamples of 
  // length \param NumSamples and warping of lambda
  void CalcWarped(
    const double* pSampleData,
    int           NumSamples,
    double*       pCorrelationCoeffs,
    int           Order,
    double        Lambda = 0.0);

  //! Calculate Auto-Correlation coefficients - non warped
  void Calc(
    const double* pSampleData,
    int           NumSamples,
    double*       pCorrelationCoeffs,
    int           Order);
}

#endif // _Autocorrelation_h_

