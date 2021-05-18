#pragma once

#ifndef _Envelopes_h_
#define _Envelopes_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

// =================================================================================================

/*!
 * Simple but very efficient envelope follower as for example used in compressors
 * with two "speed" modes. See also class TEnvelopeFollower for a follower with
 * custom attack and release rates in milliseconds.
!*/

class TEnvelopeDetector
{
public:
  enum TType
  {
    // 0.0 -> 1.0 at 70% of the time
    kSlow,
    // 0.0 -> 1.0 at 99% of the time
    kFast
  };

  //! Create a envelope detector with the given default setup
  TEnvelopeDetector(TType Type, double Ms, double Samplerate);

  //! Get/update the current sample rate
  double SampleRate()const;
  void SetSampleRate(double Samplerate);

  //! Set attack/release time in ms. Will automatically adopt on 
  //! sample rate changes.
  double TimeConstant()const;
  void SetTimeConstant(double Ms);

  //! Run the envelope on a single sample on the passed state
  void Run(double Input, double& State);

private:
  void CalcCoef();

  double mSamplerate;

  double mCoef;
  double mMs;

  TType mType;
};

// =================================================================================================

/*!
 * Simple envelope follower with attack and release rates in milliseconds. 
 * See also class TEnvelopeDetector for a less fancy but more efficient detector.
!*/

class TEnvelopeFollower
{
public:
  TEnvelopeFollower();

  //! Reset internal envelope state
  void Reset(double EnvelopeValue = 0.0);

  //! Calc output.
  double Calc(double Input);

  //! Set attack and release rates.
  void SetAttack(double AttackInMs, double SamplesPerSec);
  void SetRelease(double ReleaseInMs, double SamplesPerSec);

private:
  double mLastValue;
  double mLastValue2;

  double mAttackCoeff;
  double mPostAttackCoeff;
  double mReleaseCoeff;
};

// =================================================================================================

#include "AudioTypes/Export/Envelopes.inl"


#endif // _Envelopes_h_

