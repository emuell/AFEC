#pragma once

#ifndef _ButterworthFilters_h_
#define _ButterworthFilters_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

// =================================================================================================

/*!
* A cascaded butter worth filter implementation
* See TRbjBiquadFilter notes about the two processing forms DFII and DFI...
!*/

template <int sNumOrders>
class TButterworthFilter
{
  MStaticAssert(sNumOrders >= 4); // should be at least 4th order
  MStaticAssert((sNumOrders & 1) == 0); // Order must be odd

public:
  enum TType
  {
    kInvalidType = -1,

    kLowPass,
    kHighPass,
    kBandPass,
    kBandStop,
    kHighShelf,
    kLowShelf,

    kNumberOfFilterTypes
  };

  TButterworthFilter();

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

  //! direct form I processing
  double ProcessSampleLeftDFI(double Input);
  double ProcessSampleRightDFI(double Input);

  void ProcessSamplesDFI(
    float*  pLeftBuffer, 
    float*  pRightBuffer, 
    int     NumberOfSamples);

  //! direct form II processing
  double ProcessSampleLeftDFII(double Input);
  double ProcessSampleRightDFII(double Input);

  void ProcessSamplesDFII(
    float*  pLeftBuffer, 
    float*  pRightBuffer, 
    int     NumberOfSamples);

  //! flush all filter buffers
  void FlushBuffers();

private:
  enum { kNumStages = sNumOrders / 2 };

  // filter setup
  TType mType;
  double mFrequency;
  double mSampleRate;
  double mQ;
  double mDbGain;

  // filter coeffs
  double mShelfGain;
  double mB0a0[kNumStages], mB1a0[kNumStages], mB2a0[kNumStages]; 
  double mA1a0[kNumStages], mA2a0[kNumStages];

  // in/out history
  double mZ1Left[kNumStages], mZ2Left[kNumStages], 
    mZ3Left[kNumStages], mZ4Left[kNumStages];

  double mZ1Right[kNumStages], mZ2Right[kNumStages], 
    mZ3Right[kNumStages], mZ4Right[kNumStages];
};

// =================================================================================================

typedef TButterworthFilter<4> TButterworth4Filter;
typedef TButterworthFilter<6> TButterworth6Filter;
typedef TButterworthFilter<8> TButterworth8Filter;
typedef TButterworthFilter<10> TButterworth10Filter;
typedef TButterworthFilter<12> TButterworth12Filter;
typedef TButterworthFilter<14> TButterworth14Filter;
typedef TButterworthFilter<16> TButterworth16Filter;
typedef TButterworthFilter<18> TButterworth18Filter;



#include "AudioTypes/Export/ButterworthFilters.inl"


#endif // _ButterworthFilters_h_

