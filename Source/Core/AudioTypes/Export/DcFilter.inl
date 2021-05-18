#pragma once

#ifndef _DcFilter_inl_
#define _DcFilter_inl_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"
#include "AudioTypes/Export/AudioMath.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline double TDcFilter::ProcessSampleLeft(double Input)
{
  mIn1Left = Input - mIn2Left + mFilterCoef * mIn1Left;
  MUnDenormalize(mIn1Left);

  mIn2Left = Input;

  return mIn1Left; 
}

MForceInline double TDcFilter::ProcessSampleRight(double Input)
{
  mIn1Right = Input - mIn2Right + mFilterCoef * mIn1Right;
  MUnDenormalize(mIn1Right);

  mIn2Right = Input;

  return mIn1Right; 
}


#endif // _DcFilter_inl_

