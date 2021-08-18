#pragma once

#ifndef _SampleAnalyser_h_
#define _SampleAnalyser_h_

#pragma once

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "AudioTypes/Export/Envelopes.h"
#include "FeatureExtraction/Export/SampleDescriptors.h"

#include <mutex>

struct xtract_mel_filter_;
class TClassificationModel;
class TSampleDescriptorPool;

// =================================================================================================

/*!
 * Analyzes audio features of a single audio file and puts low level or high 
 * level results into a TSampleDescriptorPool
!*/

class TSampleAnalyser
{
public:
  TSampleAnalyser(
    int SampleRate,
    int FftFrameSize,
    int HopFrameSize);

  virtual ~TSampleAnalyser();

  //! classification model names, when a classification model was set
  TList<TString> ClassificationClasses() const;
  //! Set name of the classification model, which is needed to extract high 
  //! level class features.
  void SetClassificationModel(const TString& ModelPath);

  //! categorization model names, when a categorization model was set
  TList<TString> OneShotCategorizationClasses() const;
  //! Set name of the categorization model, which is needed to extract high 
  //! level category features.
  void SetOneShotCategorizationModel(const TString& ModelPath);

  //! Analyze (low level features only) a single audio file and return results
  //! @throws TReadableException on errors
  TSampleDescriptors Analyze(const TString& FileName);
  //! Analyze and extract a single audio file. Results or errors are passed to pool.
  //! The given mutex serializes all pool writes.
  void Extract(const TString& FileName, TSampleDescriptorPool* pPool, std::mutex& PoolLock);

private:
  // get a few consts from TSampleDescriptors
  enum {
    kNumberOfSpectrumBands =
    TSampleDescriptors::kNumberOfSpectrumBands,
    kNumberOfCepstrumCoefficients =
    TSampleDescriptors::kNumberOfCepstrumCoefficients,

    kNumberOfSpectrumSubBands =
    TSampleDescriptors::kNumberOfSpectrumSubBands,

    kNumberOfHighLevelSpectrumBands =
    TSampleDescriptors::kNumberOfHighLevelSpectrumBands,
    kNumberOfHighLevelSpectrumBandFrames =
    TSampleDescriptors::kNumberOfHighLevelSpectrumBandFrames
  };

  // normalized sample data, as needed by the AnalyzeXXXDescriptors functions
  class TSampleData
  {
  public:
    // converted, resampled mono sample data
    TArray<double> mData;
    // how many samples got added or removed, comparing mData with the original
    int mDataOffset;

    // original file info
    TString mOriginalFileName;
    int mOriginalFileSize;
    int mOriginalNumberOfSamples;
    int mOriginalNumberOfChannels;
    int mOriginalBitDepth;
    int mOriginalSampleRate;

    // wavefor peakValues: peaks[originalChannel][frame/kWaveformPeakBlockSize]
    TArray<TArray<float>> mWaveformPeaksMin;
    TArray<TArray<float>> mWaveformPeaksMax;

    // peak and rms of the original sample buffer, calculated before normalizing
    float mPeakValue;
    float mRmsValue;
  };

  // Calculated in AnalyzeLowLevelDescriptors
  struct TSilenceStatus
  {
    TList<bool> mSpectrumFrameIsAudible;
    TList<bool> mRhythmFrameIsAudible;
  };

  // filter out non audible frames from the given descriptor values
  TList<double> AudibleSpectrumFrames(
    TSilenceStatus&      SilenceStatus, 
    const TList<double>& SpectrumDescriptor)const;

  //! Load and normalize sample data.
  void LoadSample(const TString& FileName, TSampleData& Data);
  //! Analyze given file and put low level descriptor results into mpResults
  //! @throw TReadableException on Errors
  void AnalyzeLowLevelDescriptors(
    const TSampleData&   RawSampleData, 
    TSilenceStatus&      SilenceStatus,
    TSampleDescriptors&  Results);
  //! Analyze high level descriptors from low level descriptor results
  void AnalyzeHighLevelDescriptors(
    const TSampleData&   RawSampleData,
    TSilenceStatus&      SilenceStatus,
    TSampleDescriptors&  Results);

  void CalcEffectiveLength(
    TSampleDescriptors& Results, 
    const double*       pSampleData, 
    int                 NumberOfSamples);

  void CalcAmplitudePeak(
    TSampleDescriptors& Results, 
    const double*       pSampleData, 
    int                 NumberOfSamples);
  void CalcAmplitudeRms(
    TSampleDescriptors& Results, 
    const double*       pSampleData, 
    int                 NumberOfSamples);
  void CalcAmplitudeEnvelope(
    TSampleDescriptors& Results, 
    const double*       pSampleData, 
    int                 NumberOfSamples);

  void CalcSpectralRms(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum);
  void CalcSpectralCentroidAndSpread(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum);
  void CalcSpectralSkewnessAndKurtosis(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum);
  void CalcSpectralRolloff(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum);
  void CalcSpectralFlatness(
    TSampleDescriptors&   Results,
    const TArray<double>& MagnitudeSpectrum);
  void CalcSpectralFlux(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum,
    const TArray<double>& LastMagnitudeSpectrum);

  void CalcAutoCorrelation(
    TSampleDescriptors& Results, 
    const double*       pSampleData, 
    int                 RemainingSamples);

  void CalcSpectralComplexity(
    TSampleDescriptors&   Results, 
    const TArray<double>& PeakSpectrum);
  void CalcSpectralInharmonicity(
    TSampleDescriptors&   Results, 
    const TArray<double>& PeakSpectrum,
    double                F0,
    double                F0Confidence);
  void CalcTristimulus(
    TSampleDescriptors&   Results, 
    const TArray<double>& HarmonicSpectrum,
    double                F0,
    double                F0Confidence);

  void CalcSpectralBandFeatures(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum,
    const TArray<double>& LastMagnitudeSpectrum);

  void CalcSpectrumBands(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum);
  void CalcCepstrumBands(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum);

  void CalcStatistics(TSampleDescriptors& Results);

  const int mSampleRate;
  const int mFftFrameSize;
  const int mHopFrameSize;
  
  int mFirstAnalyzationBin;
  int mLastAnalyzationBin;
  int mAnalyzationBinCount;

  double* mpWindow;

  TOwnerPtr<xtract_mel_filter_> mpXtractMelFilters;

  TOwnerPtr<TClassificationModel> mpClassificationModel;
  TOwnerPtr<TClassificationModel> mpOneShotCategorizationModel;
};

#endif //_SampleAnalyser_h_

