#pragma once

#ifndef _BiquadFilters_inl_
#define _BiquadFilters_inl_

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/InlineMath.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline TRbjBiquadFilter::TType TRbjBiquadFilter::Type()const
{
  return mType;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TRbjBiquadFilter::Frequency()const
{
  return mFrequency;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TRbjBiquadFilter::SampleRate()const
{
  return mSampleRate;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TRbjBiquadFilter::Q()const
{
  return mQ;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TRbjBiquadFilter::DbGain()const
{
  return mDbGain;  
}

// -------------------------------------------------------------------------------------------------

MForceInline double TRbjBiquadFilter::ProcessSampleLeftDFI(double Input)
{
  double Output = mB0a0 * Input + mB1a0 * mZ1Left + mB2a0 * mZ2Left - 
    mA1a0 * mZ3Left - mA2a0 * mZ4Left;
  MUnDenormalize(Output);
  
  mZ2Left = mZ1Left;
  mZ1Left = Input;
  mZ4Left = mZ3Left;
  mZ3Left = Output;
  
  return Output;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TRbjBiquadFilter::ProcessSampleRightDFI(double Input)
{
  double Output = mB0a0 * Input + mB1a0 * mZ1Right + mB2a0 * mZ2Right - 
    mA1a0 * mZ3Right - mA2a0 * mZ4Right;
  MUnDenormalize(Output);
  
  mZ2Right = mZ1Right;
  mZ1Right = Input;
  mZ4Right = mZ3Right;
  mZ3Right = Output;
  
  return Output;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TRbjBiquadFilter::ProcessSampleLeftDFII(double Input)
{
  double Output = mB0a0*Input + mZ1Left;
  MUnDenormalize(Output);

  mZ1Left = mB1a0*Input - mA1a0*Output + mZ2Left;
  MUnDenormalize(mZ1Left);

  mZ2Left = mB2a0*Input - mA2a0*Output;
  MUnDenormalize(mZ2Left);
  
  return Output;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TRbjBiquadFilter::ProcessSampleRightDFII(double Input)
{
  double Output = mB0a0*Input + mZ1Right;
  MUnDenormalize(Output);

  mZ1Right = mB1a0*Input - mA1a0*Output + mZ2Right;
  MUnDenormalize(mZ1Right);

  mZ2Right = mB2a0*Input - mA2a0*Output;
  MUnDenormalize(mZ2Right);
  
  return Output;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline TTptBiquadFilter::TType TTptBiquadFilter::Type()const
{
  return mType;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TTptBiquadFilter::Frequency()const
{
  return mFrequency;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TTptBiquadFilter::SampleRate()const
{
  return mSampleRate;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TTptBiquadFilter::Q()const
{
  return mQ;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TTptBiquadFilter::ProcessSampleLeft(double Input)
{
  // compute highpass output via Eq. 5.1:
  double yhl = (Input - mR2 * mS1Left - mG * mS1Left - mS2Left) * mH;
  MUnDenormalize(yhl);

  // compute bandpass output by applying 1st integrator to highpass output
  double ybl = mG * yhl + mS1Left;
  MUnDenormalize(ybl);

  // state update in 1st integrator
  mS1Left = mG * yhl + ybl;
  MUnDenormalize(mS1Left);

  // compute lowpass output by applying 2nd integrator to bandpass output
  double yll = mG * ybl + mS2Left;
  MUnDenormalize(yll);

  // state update in 2nd integrator
  mS2Left = mG * ybl + yll;

  double y = mLowOut*yll + mBandOut*ybl + mHighOut*yhl;
  MUnDenormalize(y);

  return y;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TTptBiquadFilter::ProcessSampleRight(double Input)
{
  // compute highpass output via Eq. 5.1:
  double yhr = (Input - mR2 * mS1Right - mG * mS1Right - mS2Right) * mH;
  MUnDenormalize(yhr);

  // compute bandpass output by applying 1st integrator to highpass output
  double ybr = mG * yhr + mS1Right;
  MUnDenormalize(ybr);

  // state update in 1st integrator
  mS1Right = mG * yhr + ybr;
  MUnDenormalize(mS1Right);

  // compute lowpass output by applying 2nd integrator to bandpass output
  double ylr = mG * ybr + mS2Right;
  MUnDenormalize(ybr);

  // state update in 2nd integrator
  mS2Right = mG * ybr + ylr;

  double y = mLowOut*ylr + mBandOut*ybr + mHighOut*yhr;
  MUnDenormalize(y);

  return y;
}


#endif // _BiquadFilters_inl_

