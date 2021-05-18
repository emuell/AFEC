#include "FeatureExtraction/Source/LinearPredictiveCoding.h"
#include "FeatureExtraction/Source/Autocorrelation.h"

#include "CoreTypes/Export/Array.h"

// =================================================================================================

// when defined, use warped autocorrelation (slower)
// #define MUseWarpedAutoCorrelation

// =================================================================================================

// -------------------------------------------------------------------------------------------------

//! Using Durbin's recursive method calculating LPC coefficients
//! from AutoCoerrlation coefficients.

static void SCalcLpcCoeffs(
  const double* pCorrelationCoeffs,
  double*       pLpcCoeffs,
  double*       pReflectionCoeffs,
  int           Order)
{
  int Cnt, j;
  double Error, alpha;

  // Tempory buffer for prediction error
  TArray<double> E(Order + 2);
  TArray<double> LPCArray(Order + 2);

  // Recursive calculated LPC coefficients
  LPCArray[1] = 1.0;
  E[0] = pCorrelationCoeffs[0];
  if (E[0] != 0)
  {
    pReflectionCoeffs[1] = -pCorrelationCoeffs[1] / E[0];
  }
  else
  {
    pReflectionCoeffs[1] = 0.0;
  }

  alpha = E[0] * (1.0 - pReflectionCoeffs[1] * pReflectionCoeffs[1]);
  LPCArray[2] = pReflectionCoeffs[1];

  for (Cnt = 2; Cnt <= Order; Cnt++)
  {
    Error = 0.0;
    for (j = 1; j <= Cnt; j++)
    {
      Error += LPCArray[j] * pCorrelationCoeffs[Cnt + 1 - j];
    }

    if (TMathT<double>::Abs(alpha) > MEpsilon)
    {
      pReflectionCoeffs[Cnt] = -Error / alpha;
    }
    else
    {
      pReflectionCoeffs[Cnt] = 0;
    }

    alpha *= (1.0 - pReflectionCoeffs[Cnt] * pReflectionCoeffs[Cnt]);
    LPCArray[Cnt + 1] = pReflectionCoeffs[Cnt];

    for (j = 2; j <= Cnt; j++)
    {
      E[j] = LPCArray[j] + pReflectionCoeffs[Cnt] * LPCArray[Cnt + 2 - j];
    }

    for (j = 2; j <= Cnt; j++)
    {
      LPCArray[j] = E[j];
    }
  }

  // Re-organize LPC coefficients, index from 0
  for (Cnt = 0; Cnt < Order; Cnt++)
  {
    pLpcCoeffs[Cnt] = LPCArray[Cnt + 2];
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TLinearPredictiveCoding::Calc(
  const double* pSampleData,
  int           NumSamples,
  double*       pLpcCoeffs,
  int           Order)
{
  // Allocate Temp memory
  TArray<double> Reflection(Order + 1);
  TArray<double> AutoCorrelation(Order + 1);

  // Order + 1 AutoCoeff -> Order LPC
#if defined(MUseWarpedAutoCorrelation)
  const double Lambda = 0.75;
  TAutocorrelation::CalcWarped(pSampleData, NumSamples,
    AutoCorrelation.FirstWrite(), Order + 1, Lambda);
#else
  TAutocorrelation::Calc(pSampleData, NumSamples,
    AutoCorrelation.FirstWrite(), Order + 1);
#endif

// NB: LPC_Coef should be (1, a1, a2, a3, ...)
// but the first "1" is not calculated here:
  SCalcLpcCoeffs(AutoCorrelation.FirstRead(),
    pLpcCoeffs, Reflection.FirstWrite(), Order);
}

// -------------------------------------------------------------------------------------------------

void SCalcLpcCepstrum(
  const double* LPCArray,
  int           LpcOrder,
  double*       Cep,
  int           CepOrder)
{
  int CepCnt, Cnt;
  double Sum;

  // Calculate Complex cepstrum  with recursive relation. See. Rabina's book
  for (CepCnt = 0; CepCnt < CepOrder; CepCnt++)
  {
    // First term in the formula    
    if (CepCnt < LpcOrder)
    {
      Sum = LPCArray[CepCnt];
    }
    else
    {
      Sum = 0.0;  /* CepOrder can larger LpcOrder */
    }

    // Second term in the formula    
    for (Cnt = 0; Cnt < CepCnt; Cnt++)
    {
      if (CepCnt - Cnt < LpcOrder)
      {
        Sum += Cep[Cnt] * LPCArray[CepCnt - Cnt] * (Cnt + 1) / (CepCnt + 1);
      }
    }

    Cep[CepCnt] = Sum;
  }
}

