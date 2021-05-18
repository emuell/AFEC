#pragma once

#ifndef _OnsetDetector_h_
#define _OnsetDetector_h_

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Array.h"

#include "AudioTypes/Export/Fourier.h"

template <typename TEnumType> struct SEnumStrings;

// =================================================================================================

class TOnsetFftProcessor
{
public:
  enum {
    kMinFftSize = 128,
    kMaxFftSize = 2048
  };

  struct TPolarBuffer
  {
    float mDC, mNyquist;
    struct TBin 
    {
      float mMagn;
      float mPhase;
    };
    TArray<TBin> mBin;
  };

  enum TWhiteningType
  {
    kWhiteningNone,     // No whitening
    kWhiteningAdaptMax, // Tracks recent-peak-magnitude in each bin, normalises that to 1

    kNumberOfWhiteningTypes
  };

  //! @param SampleRate The sampling rate of the input audio
  //! @param FftSize Size of FFT: 512 or 1024 is recommended.
  //! @param Hopsize The FFT frame hopsize (typically this will be half the FFT frame size)
  //! @param LogMagnitudes When true, scale magnitude values logarithmically
  //! @param WhiteningType see @enum TWhiteningType
  //! @param WhiteningRelaxTime The number of seconds for the whitening memory to decay by 60 dB
  TOnsetFftProcessor(
    float               SampleRate,
    unsigned int        FftSize = 512,
    unsigned int        HopSize = 256,
    bool                LogMagnitudes = false,
    TWhiteningType      WhiteningType = kWhiteningAdaptMax,
    float               WhiteningRelaxTime = 1.0f);

  //! Process a single FFT data frame for the given (normalized) mono audio signal.
  const TPolarBuffer* Process(const float* pBuffer);
  const TPolarBuffer* Process(const double* pBuffer);
  
private:
  //! Load the current frame of FFT data into the class.
  // void LoadFrame(const float* pBuffer);
  void LoadFrame(const double* pBuffer);
  
  void ApplyLogMags();
  void Whiten();
  
  //! Set the "memory coefficient" indirectly via the time for the memory to decay by 60 dB.
  //! @param time The time in seconds
  void SetRelaxTime(float Time);

  const float mSampleRate;
  const unsigned int mFftsize;
  const unsigned int mHopsize;
  const unsigned int mNumbins;

  TWhiteningType mWhtype;
  bool mLogmags;
  
  float mWhiteningRelaxTime;
  float mWhiteningRelaxCoef;
  float mWhiteningFloor;
  
  TArray<double> mPsp;
  TPolarBuffer mPolarBuffer;

  TArray<double> mWindowBuffer;

  TFftTransformComplex mFFT;
};

// =================================================================================================

/*!
 * Based on Paul Brossier's onset detector with a few modifications to integrate it into our API.
 */

class TOnsetDetector
{
public:

  enum TOnsetFunctionType
  {
    kFunctionPower,     // Power
    kFunctionMagSum,    // Sum of magnitudes
    kFunctionComplex,   // Complex-domain deviation
    kFunctionRComplex,  // Complex-domain deviation, rectified (only increases counted)
    kFunctionPhase,     // Phase deviation
    kFunctionWPhase,    // Weighted phase deviation
    kFunctionMKL,       // Modified Kullback-Liebler deviation

    kNumberOfOnsetFunctionTypes
  };

  //! Initialise the TOnsetDetector class and its associated memory, ready to detect
  //! onsets using the specified settings.
  //! @param OdfType Which TOnsetFunctionType detection function you'll be using
  //! @param SampleRate The sampling rate of the input audio
  //! @param FftSize Size of FFT: 512 or 1024 is recommended.
  //! @param Hopsize The FFT frame hopsize (typically this will be half the FFT frame size)
  //! @param ThresholdMedianSpan The number of seconds in the past the median calculation
  //!        for the threshold will look during peak picking
  //! @param Threshold 
  //! @param MinimumGap The smallest allowed gap between onsets in seconds
  TOnsetDetector(
    TOnsetFunctionType  OdfType,
    float               SampleRate,
    unsigned int        FftSize = 512,
    unsigned int        HopSize = 256,
    float               Threshold = 0.5f,
    float               ThresholdMedianSpan = 0.12f,
    float               MinimumGap = 0.06f);

  //! Process a single FFT data frame in the audio signal. Note that processing
  //! assumes that each call to Process() is on a subsequent frame in the same audio stream
  //! to handle multiple streams you must use separate instances!
  bool Process(const TOnsetFftProcessor::TPolarBuffer* pBuffer);
  
  //! Last processed Odf value (onset intensity).
  float CurrentOdfValue()const;

private:
  //! Calculate the Onset Detection Function
  void CalculateOnsetFunction(const TOnsetFftProcessor::TPolarBuffer* pBuffer);
  
  //! Detects salient peaks in Onset Detection Function by removing the median,
  //! then thresholding. Afterwards, the member mDetected will indicate whether
  //! or not an onset was detected.
  bool DetectOnset();

  const TOnsetFunctionType mOdftype;
  const float mSampleRate;
  const unsigned int mFftsize;
  const unsigned int mHopsize;
  const unsigned int mNumbins;
  const float mThresh;
  
  unsigned int mMedSpan;
  bool mMedOdd;
  
  unsigned int mMinGap;
  unsigned int mGapLeft;
  
  float mOdfparam;
  float mNormfactor;
  
  float mOdfvalpost;
  float mOdfvalpostprev;

  TArray<float> mpOdfvals;
  TArray<float> mpOther;
};

// =================================================================================================

template <> struct SEnumStrings<TOnsetDetector::TOnsetFunctionType>
{
  operator TList<TString>()const
  {
    MStaticAssert(TOnsetDetector::kNumberOfOnsetFunctionTypes == 7);
    
    TList<TString> Ret;

    Ret.Append("Power");
    Ret.Append("Sum of magnitudes");
    Ret.Append("Complex-domain deviation");
    Ret.Append("Rectified complex-domain deviation");
    Ret.Append("Phase deviation");
    Ret.Append("Weighted phase deviation");
    Ret.Append("Modified Kullback-Liebler deviation");

    return Ret;
  }
};


#endif // _OnsetDetector_h_

