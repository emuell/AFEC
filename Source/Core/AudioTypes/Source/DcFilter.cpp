#include "AudioTypesPrecompiledHeader.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TDcFilter::TDcFilter(double InitialSampleRate, TMode Mode)
{
  FlushBuffers();
  Setup(InitialSampleRate, Mode);
}

// -------------------------------------------------------------------------------------------------

void TDcFilter::Setup(double SampleRate, TMode Mode)
{
  MAssert(SampleRate > 0, "Invalid rate");

  double AmountCoef;
  switch (Mode)
  {
  default:
    MInvalid("unknown speed mode");
  case kSlow:
    AmountCoef = 1.0;
    break;
  case kDefault:
    AmountCoef = 5.0;
    break;
  case kFast:
    AmountCoef = 20.0;
    break;
  }

  mFilterCoef = 1.0 - M2Pi * (AmountCoef / SampleRate);
}

// -------------------------------------------------------------------------------------------------

void TDcFilter::ProcessSamples(float* pSamples, int NumOfSamples)
{
  for (int i = 0; i < NumOfSamples; ++i)
  {
    pSamples[i] = (float)ProcessSampleLeft(pSamples[i]);
    MUnDenormalize(pSamples[i]);
  }
}

void TDcFilter::ProcessSamples(
  float*  pLeftSamples, 
  float*  pRightSamples, 
  int     NumOfSamples)
{
  for (int i = 0; i < NumOfSamples; ++i)
  {
    pLeftSamples[i] = (float)ProcessSampleLeft(pLeftSamples[i]);
    MUnDenormalize(pLeftSamples[i]);

    pRightSamples[i] = (float)ProcessSampleRight(pRightSamples[i]);
    MUnDenormalize(pRightSamples[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TDcFilter::FlushBuffers()
{
  mIn1Left = mIn2Left = 0.0;
  mIn1Right = mIn2Right = 0.0;
}

