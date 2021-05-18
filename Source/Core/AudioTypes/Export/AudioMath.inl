#pragma once

#ifndef _AudioMath_inl_
#define _AudioMath_inl_

// =================================================================================================

#include "CoreTypes/Export/InlineMath.h"
#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/Memory.h"

#include "AudioTypes/Export/AudioTypes.h"
#include "AudioTypes/Export/Fourier.h"

#include <cmath>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline float TAudioMath::FastLinToDb(float Value)
{
  static const double sLinToDbFactor = 20.0 / ::log(10.0);

  if (Value == 1.0f)
  {
    // avoid rounding errors at exactly 0 dB
    return 0.0f;
  }
  else if (Value > MEpsilon)
  {
    return (float)(TMath::FastLog(Value) * sLinToDbFactor);
  }

  return (float)MMinusInfInDb;
}

MForceInline float TAudioMath::LinToDb(float Value)
{
  static const double sLinToDbFactor = 20.0 / ::log(10.0);

  if (Value == 1.0f)
  {
    // avoid rounding errors at exactly 0 dB
    return 0.0f;
  }
  else if (Value > MEpsilon)
  {
    return (float)(::log(Value) * sLinToDbFactor);
  }

  return (float)MMinusInfInDb;
}

MForceInline double TAudioMath::LinToDb(double Value)
{
  static const double sLinToDbFactor = 20.0 / ::log(10.0);

  if (Value == 1.0)
  {
    // avoid rounding errors at exactly 0 dB
    return 0.0;
  }
  else if (Value > MEpsilon)
  {
    return ::log(Value) * sLinToDbFactor;
  }

  return (double)MMinusInfInDb;
}

// -------------------------------------------------------------------------------------------------

MForceInline float TAudioMath::FastDbToLin(float Value)
{
  static const double sDbToLinFactor = ::log(10.0) / 20.0;

  if (Value == 0.0f)
  {
    // avoid rounding errors at exactly 0 dB
    return 1.0f;
  }
  else if (Value > MMinusInfInDb)
  {
    return TMath::FastExp(Value * (float)sDbToLinFactor);
  }

  return 0.0f;
}

MForceInline float TAudioMath::DbToLin(float Value)
{
  static const double sDbToLinFactor = ::log(10.0) / 20.0;

  if (Value == 0.0f)
  {
    // avoid rounding errors at exactly 0 dB
    return 1.0f;
  }
  else if (Value > MMinusInfInDb)
  {
    return (float)::exp(Value * sDbToLinFactor);
  }

  return 0.0f;
}

MForceInline double TAudioMath::DbToLin(double Value)
{
  static const double sDbToLinFactor = ::log(10.0) / 20.0;

  if (Value == 0.0)
  {
    // avoid rounding errors at exactly 0 dB
    return 1.0;
  }
  else if (Value > MMinusInfInDb)
  {
    return ::exp(Value * sDbToLinFactor);
  }

  return 0.0;
}

// -------------------------------------------------------------------------------------------------

MForceInline int TAudioMath::MsToSamples(int SamplesPerSec, float Ms)
{
  return TMath::f2iRound((float)SamplesPerSec / 1000.0f * Ms);
}

// -------------------------------------------------------------------------------------------------

MForceInline float TAudioMath::SamplesToMs(int SamplesPerSec, int Samples)
{
  return (float)Samples / ((float)SamplesPerSec / 1000.0f);
}

// -------------------------------------------------------------------------------------------------

MForceInline double TAudioMath::NoteFrequency(double Note)
{
  MAssert(Note > 0 && Note < 2*120, "Invalid note value");
  return 440.0 * ::pow(2.0, (Note - 69) / 12.0);
}

// -------------------------------------------------------------------------------------------------

MForceInline float TAudioMath::NoteVolumeFromVelocity(int Velocity)
{
  return (float)DbToLin(40.0 * ::log(MMin(0x7f, Velocity) / (double)0x7f));
}

// -------------------------------------------------------------------------------------------------

MForceInline bool TAudioMath::IsHalfNote(int NoteValue)
{
  return !IsFullNote(NoteValue);
}

// -------------------------------------------------------------------------------------------------

MForceInline bool TAudioMath::IsFullNote(int NoteValue)
{
  MAssert(NoteValue >= 0 && NoteValue < 120, "Invalid note");

  switch (NoteValue % 12)
  {
  case 0: case 2: case 4: case 5: case 7: case 9: case 11:
    return true;

  default:
    return false;
  }
}

#endif // _AudioMath_inl_

