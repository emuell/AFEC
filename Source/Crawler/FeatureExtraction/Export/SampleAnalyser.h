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
  using TDescriptorSet = TSampleDescriptors::TDescriptorSet;

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

  //! Analyze a single audio file and return results
  //! @throws TReadableException on errors
  TSampleDescriptors Analyze(
    const TString&  FileName,
    TDescriptorSet  DescriptorSet = TSampleDescriptors::kLowLevelDescriptors) const;
  //! Analyze and extract a single audio file ant path \param FileName. Results or errors 
  //! are passed to pool. To be extracted DescriptorSet is queried from the \param pPool.
  //! The given \param PoolLock serializes all pool writes.
  void Extract(
    const TString&          FileName, 
    TSampleDescriptorPool*  pPool, 
    std::mutex&             PoolLock) const;

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

    // waveform peakValues: peaks[originalChannel][frame/kWaveformPeakBlockSize]
    TArray<TArray<float>> mWaveformPeaksMin;
    TArray<TArray<float>> mWaveformPeaksMax;

    // peak and rms of the original sample buffer, calculated before normalizing
    float mPeakValue;
    float mRmsValue;
  };

  // Silence status, calculated in AnalyzeLowLevelDescriptors
  struct TSilenceStatus
  {
    TList<bool> mSpectrumFrameIsAudible;
    TList<bool> mRhythmFrameIsAudible;
  };

  // filter out non audible frames from the given descriptor values
  TList<double> AudibleSpectrumFrames(
    TSilenceStatus&      SilenceStatus, 
    const TList<double>& SpectrumDescriptor) const;

  //! Load and normalize sample data.
  void LoadSample(const TString& FileName, TSampleData& Data) const;
  //! Analyze given file and put low level descriptor results into mpResults
  //! @throw TReadableException on Errors
  void AnalyzeLowLevelDescriptors(
    const TSampleData&   RawSampleData, 
    TSilenceStatus&      SilenceStatus,
    TSampleDescriptors&  Results)  const;
  //! Analyze high level descriptors from low level descriptor results
  void AnalyzeHighLevelDescriptors(
    const TSampleData&   RawSampleData,
    TSilenceStatus&      SilenceStatus,
    TSampleDescriptors&  Results) const;

  void CalcEffectiveLength(
    TSampleDescriptors& Results, 
    const double*       pSampleData, 
    int                 NumberOfSamples) const;

  void CalcAmplitudePeak(
    TSampleDescriptors& Results, 
    const double*       pSampleData, 
    int                 NumberOfSamples) const;
  void CalcAmplitudeRms(
    TSampleDescriptors& Results, 
    const double*       pSampleData, 
    int                 NumberOfSamples) const;
  void CalcAmplitudeEnvelope(
    TSampleDescriptors& Results, 
    const double*       pSampleData, 
    int                 NumberOfSamples) const;

  void CalcSpectralRms(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum) const;
  void CalcSpectralCentroidAndSpread(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum) const;
  void CalcSpectralSkewnessAndKurtosis(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum) const;
  void CalcSpectralRolloff(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum) const;
  void CalcSpectralFlatness(
    TSampleDescriptors&   Results,
    const TArray<double>& MagnitudeSpectrum) const;
  void CalcSpectralFlux(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum,
    const TArray<double>& LastMagnitudeSpectrum) const;

  void CalcAutoCorrelation(
    TSampleDescriptors& Results, 
    const double*       pSampleData, 
    int                 RemainingSamples) const;

  void CalcSpectralComplexity(
    TSampleDescriptors&   Results, 
    const TArray<double>& PeakSpectrum) const;
  void CalcSpectralInharmonicity(
    TSampleDescriptors&   Results, 
    const TArray<double>& PeakSpectrum,
    double                F0,
    double                F0Confidence) const;
  void CalcTristimulus(
    TSampleDescriptors&   Results, 
    const TArray<double>& HarmonicSpectrum,
    double                F0,
    double                F0Confidence) const;

  void CalcSpectralBandFeatures(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum,
    const TArray<double>& LastMagnitudeSpectrum) const;

  void CalcSpectrumBands(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum) const;
  void CalcCepstrumBands(
    TSampleDescriptors&   Results, 
    const TArray<double>& MagnitudeSpectrum) const;

  void CalcStatistics(TSampleDescriptors& Results) const;

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

