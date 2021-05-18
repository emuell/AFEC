#pragma once

#ifndef _Envelopes_inl_
#define _Envelopes_inl_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline void TEnvelopeDetector::Run(double Input, double& State)
{
  State = Input + mCoef * (State - Input);
  MUnDenormalize(State);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline double TEnvelopeFollower::Calc(double Input)
{
  const double K = TMathT<double>::Abs(Input);

  // ... apply attack/release

  if (mLastValue <= K)
  {
    mLastValue = (mLastValue - K) * mAttackCoeff + K;
  }
  else
  {
    if (mLastValue > 0.00001f)
    {
      mLastValue = (mLastValue - K) * mReleaseCoeff + K;
    }
  }

  // ... apply post low pass filter

  mLastValue2 = (mLastValue2 - mLastValue) * mPostAttackCoeff + mLastValue;

  return mLastValue2;
}


#endif // _Envelopes_h_

