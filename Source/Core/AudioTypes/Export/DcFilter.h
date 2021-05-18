#pragma once

#ifndef _DcFilter_h_
#define _DcFilter_h_

// =================================================================================================

/*!
 * DC removal high-pass filter
!*/

class TDcFilter
{
public:
  enum TMode
  {
    // gentle, only touching DC offsets, keeping low frequencies intact
    kSlow,
    // good compromize between adjustment speed and sound quality
    kDefault,
    // faster and more aggressive mode, may dull a few low frequencies < 50 Hz
    kFast
  };

  TDcFilter(
    double  InitialSampleRate = 44100.0, 
    TMode   Mode = kDefault);

  //! Setup/change coefs (sample rate)
  void Setup(double SampleRate, TMode Mode = kDefault);

  //! Process a single sample
  double ProcessSampleLeft(double Input);
  double ProcessSampleRight(double Input);
  
  //! Process a mono or stereo buffer
  void ProcessSamples(
    float*  pLeftSamples, 
    float*  pRightSamples, 
    int     NumOfSamples);
  void ProcessSamples(
    float*  pSamples, 
    int     NumOfSamples);

  //! Flush internal buffers
  void FlushBuffers();

private:
  double mFilterCoef;

  double mIn1Left, mIn2Left;
  double mIn1Right, mIn2Right;
};

// =================================================================================================

#include "AudioTypes/Export/DcFilter.inl"


#endif // _DcFilter_h_

