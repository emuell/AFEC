#include "AudioTypesPrecompiledHeader.h"

#include <limits>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TEnvelopeDetector::TEnvelopeDetector(TType Type, double Ms, double Samplerate)
  : mType(Type),
    mMs(Ms),
    mSamplerate(Samplerate)
{
  CalcCoef();
}

// -------------------------------------------------------------------------------------------------

double TEnvelopeDetector::SampleRate()const
{
  return mSamplerate;
}

// -------------------------------------------------------------------------------------------------

void TEnvelopeDetector::SetSampleRate(double Samplerate)
{
  if (mSamplerate != Samplerate)
  {
    mSamplerate = Samplerate;
    CalcCoef();
  }
}

// -------------------------------------------------------------------------------------------------

double TEnvelopeDetector::TimeConstant()const
{
  return mMs;
}

// -------------------------------------------------------------------------------------------------

void TEnvelopeDetector::SetTimeConstant(double Ms)
{
  if (mMs != Ms)
  {
    mMs = Ms;
    CalcCoef();
  }
}

// -------------------------------------------------------------------------------------------------

void TEnvelopeDetector::CalcCoef()
{
  switch (mType)
  {
  default:
    MInvalid("");

  case kSlow:
    // rises to 70% of in value over duration of time constant
    mCoef = ::exp(-1000.0 / (mMs * mSamplerate));
    break;

  case kFast:
    // rises to 99% of in value over duration of time constant
    mCoef = ::pow(0.01, (1000.0 / (mMs * mSamplerate)));
    break;
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TEnvelopeFollower::TEnvelopeFollower()
{
  Reset(0.0);  
    
  SetAttack(1.0, 44100.0);
  SetRelease(10.0, 44100.0);
}

// -------------------------------------------------------------------------------------------------

void TEnvelopeFollower::Reset(double InitialValue)
{
  mLastValue = InitialValue;
  mLastValue2 = InitialValue;
}

// -------------------------------------------------------------------------------------------------

void TEnvelopeFollower::SetAttack(double AttackInMs, double SamplesPerSec)
{
  const double SampleRatePerMs = SamplesPerSec / 1000.0;

  mAttackCoeff = ::exp(-1.0 / (SampleRatePerMs * AttackInMs));
  mPostAttackCoeff = ::exp(-1.0 / (SampleRatePerMs * 10.0 * AttackInMs));
}

// -------------------------------------------------------------------------------------------------

void TEnvelopeFollower::SetRelease(double ReleaseInMs, double SamplesPerSec)
{
  const double SampleRatePerMs = SamplesPerSec / 1000.0f;

  mReleaseCoeff = ::exp(-1.0 / (SampleRatePerMs * ReleaseInMs));
}

