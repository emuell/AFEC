#pragma once

#ifndef _BiquadFilters_h_
#define _BiquadFilters_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

// =================================================================================================

/*!
 * A standard biquad filter implementation.
 *
 * A note about the two processing forms DFII and DFI:
 * DFII is usually more precise and faster, but also produces
 * more artifacts on parameter changes. So use DFI if you process/setup
 * the filters unramped, else DFII...
!*/

class TRbjBiquadFilter
{
public:
  enum TType
  {
    kInvalidType = -1,

    kLowPass,
    kHighPass,
    kBandPassCsg,
    kBandPassCzpg,
    kNotch,
    kAllPass,
    kPeaking,
    kLowShelf,
    kHighShelf,

    kNumberOfFilterTypes
  };

  TRbjBiquadFilter();
  
  //! return the last set parameters
  TType Type()const;
  double Frequency()const;
  double SampleRate()const;
  double Q()const;
  double DbGain()const;

  //! return the filters current magnitude response (linear - no dB!) 
  //! for the given frequency
  double FrequencyResponse(double Hz)const;

  //! \param Frequency: [0 - 22050 Hz]
  //! \param SampleRate: [11025 - 192000 Hz]
  //! \param Q: [0 - ~20.0f]
  //! \param DbGain: [-InfDb - ~40 dB]
  void Setup(
    TType   Type,
    double  Frequency,
    double  SampleRate,
    double  Q,
    double  DbGain);

  // direct form I processing
  double ProcessSampleLeftDFI(double Input);
  double ProcessSampleRightDFI(double Input);
  
  void ProcessSamplesDFI(
    float*  pLeftBuffer, 
    float*  pRightBuffer, 
    int     NumberOfSamples);

  // direct form II processing
  double ProcessSampleLeftDFII(double Input);
  double ProcessSampleRightDFII(double Input);

  void ProcessSamplesDFII(
    float*  pLeftBuffer, 
    float*  pRightBuffer, 
    int     NumberOfSamples);

  void FlushBuffers();

protected:
  // filter setup
  TType mType;
  double mFrequency;
  double mSampleRate;
  double mQ;
  double mDbGain;

  // filter coeffs
  double mB0a0, mB1a0, mB2a0, mA0, mA1a0, mA2a0;
  
  // in/out history
  double mZ1Left, mZ2Left, mZ3Left, mZ4Left;
  double mZ1Right, mZ2Right, mZ3Right, mZ4Right;
};

// =================================================================================================

/*!
 * Topology Preserving Transforms (aka Zero Delay Feedback) SVF 2nd Order (12db/oct)
 *
 * Based on Vadim's excellent work:
 * http://www.kvraudio.com/forum/viewtopic.php?t=350246
 *
 * - Low-pass
 * - High-pass
 * - Band-pass (constant skirt gain)
 * - Band-pass (constant peak gain)
 * - Band-stop (notch)
 * - All-pass
 * - Bell (peaking EQ)
 * - Low-shelf
 * - High-shelf
!*/

class TTptBiquadFilter
{
public:
 enum TType
 {
   kInvalidType = -1,

   kLowPass,
   kHighPass,
   kBandPassCsg, // constant skirt gain
   kBandPassCpg, // constant peak gain
   kNotch,
   kAllPass,
   kPeaking,
   kLowShelf,
   kHighShelf,

   kNumberOfFilterTypes
  };

  TTptBiquadFilter();

  TType Type()const;
  double Frequency()const;
  double SampleRate()const;
  double Q()const;

  //! \param Frequency: [0 - 22050]
  //! \param SampleRate: [11025 - 192000]
  //! \param Q or BW: [0.0 - 4.0 gain with LP/HP,LS/HS else octaves ]
  //! \param DbGain: [-INF - 40.0 dB] (used for LS/HS only)
  void Setup(
    TType   Type, 
    double  Frequency, 
    double  SampleRate, 
    double  QOrBw,
    double  DbGain);

  //! process filter on a single sample
  double ProcessSampleLeft(double Input);
  double ProcessSampleRight(double Input);

  //! process filter on a mono or stereo float buffer
  void ProcessSamples(
    float*  pBuffer, 
    int     NumberOfSamples);
  void ProcessSamples(
    float*  pLeftBuffer, 
    float*  pRightBuffer, 
    int     NumberOfSamples);
 
  //! flush internal buffers
  void FlushBuffers();
 
private:
 // filter setup
 TType mType;
 double mFrequency;
 double mSampleRate;
 double mQ;
 double mDbGain;

 // coefficients
 double mG, mH, mR2;

 // output selection
 double mLowOut, mBandOut, mHighOut;

 // 1st/2nd integrator
 double mS1Left, mS1Right;
 double mS2Left, mS2Right;
};

// =================================================================================================

#include "AudioTypes/Export/BiquadFilters.inl"


#endif // _BiquadFilters_h_

