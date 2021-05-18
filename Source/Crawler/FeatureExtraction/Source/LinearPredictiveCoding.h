#pragma once

#ifndef _LinearPredictiveCoding_h_
#define _LinearPredictiveCoding_h_

// =================================================================================================

namespace TLinearPredictiveCoding 
{
  //! Calculate LPC coefficients from an audio signal
  void Calc(
    const double* pSampleData,
    int           NumSamples,
    double*       pLpcCoeffs,
    int           Order);

  //! Convert LPC coefficients to LPC based cepstral coefficients
  void CalcCepstrum(
    const double* pLPCArray,
    int           LpcOrder,
    double*       Cepstrum,
    int           CepstrumOrder);
}


#endif // _LinearPredictiveCoding_h_

