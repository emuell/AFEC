#include "FeatureExtraction/Export/SampleClassificationDescriptors.h"
#include "FeatureExtraction/Export/SampleDescriptors.h"
#include "FeatureExtraction/Export/SampleAnalyser.h"

#include "CoreTypes/Export/Directory.h"
#include "CoreTypes/Export/File.h"
#include "CoreTypes/Export/Range.h"

#include "CoreFileFormats/Export/WaveFile.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

// ideal for short percussive sounds
// static const int sTimeSeries[] = {
//   1,2,3,4,5,6,7,8,9,10,11,12, // ~1/4 sec
//   16,32,128,256,512
// }

// OR
// good compromise between long and short sounds - works with "smaller" nets too
// static const int sTimeSeries[] = {
//   0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24, // ~1/2 sec
//   32,64,128,256,512
// };

// OR
// ideal for long tonal sounds and speech - needs bigger networks
// static const int sTimeSeries[] = {
//   0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
//   25,26,27,28,29,30,31,32,33, // ~2/3 sec
//   64,128,256,512
// };

// OR
// more extreme - ideal for long tonal sounds or loops - needs even bigger nets
static const int sTimeSeries[] = {
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
  25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43, // ~1 sec
  64,128,256,512
};

static const int sTimeSeriesLength = (int)MCountOf(sTimeSeries);

// -------------------------------------------------------------------------------------------------

// static const double sSpectrumFrequencies[]  = {
//   50.0, 100.0, 150.0, 200.0, 300.0, 400.0, 510.0, 630.0, 770.0, 920.0,
//   1080.0, 1270.0, 1480.0, 1720.0, 2000.0, 2320.0, 2700.0, 3150.0, 3700.0, 4400.0,
//   5300.0, 6400.0, 7700.0, 9500.0, 12000.0, 15500.0, 19000.0, 22050.0
// }

// big set of merged frequency bands (all bands up to 9.500 kHz)
// static const int sSpectrumBands[] = {
//   0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
//   20,21,22,23
// };

// small set of merged frequency bands (up to 15.5 kHz)
// similar to TSampleAnalyser::CalcSpectralBandFeatures
static const int sSpectrumBands[] = {
  // 50, 100, 200, 400, 630, 920, 1270, 1720, 2320, 3150, 4400, 6400, 9500, 15500
  0, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25
};

static const int sNumberSpectrumBands = (int)MCountOf(sSpectrumBands);

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static void SAddDescriptorSpectrumTimeFrames(
  std::vector<std::string>*                     pDestNameVector,
  std::vector<double>*                          pDestVector,
  const TSampleDescriptors::TFramedScalarData&  FrameData,
  double                                        NoValueValue)
{
  for (size_t i = 0; i < sTimeSeriesLength; ++i)
  {
    if (pDestVector != NULL)
    {
      const int Frame = sTimeSeries[i];

      const double Value = (Frame < FrameData.mValues.Size()) ?
        FrameData.mValues[Frame] : NoValueValue;

      if (TMath::IsNaN(Value) || TMath::IsInf(Value))
      {
        throw TReadableException("Invalid Descriptor array value (NAN of INF)");
      }

      pDestVector->push_back(Value);
    }

    if (pDestNameVector != NULL) 
    {
      pDestNameVector->push_back(
        std::string(FrameData.mpName) + "_t" + std::to_string(i));
    }
  }
}

// -------------------------------------------------------------------------------------------------

static void SAddDescriptorSpectrumStatistics(
  std::vector<std::string>*                     pDestNameVector,
  std::vector<double>*                          pDestVector,
  const TSampleDescriptors::TFramedScalarData&  ScalarData)
{
  static const char* sStatisticsNames[] = {
    "min",
    "max",
    // "median",
    "mean",
    // "gmean",
    "variance",
    // "centroid",
    // "spread",
    // "skewness",
    // "kurtosis",
    "flatness",
    "dmean",
    "dvariance"
  };

  const double Statistics[] = {
    ScalarData.mMin,
    ScalarData.mMax,
    // ScalarData.mMedian,
    ScalarData.mMean,
    // ScalarData.mGeometricMean,
    ScalarData.mVariance,
    // ScalarData.mCentroid,
    // ScalarData.mSpread,
    // ScalarData.mSkewness,
    // ScalarData.mKurtosis,
    ScalarData.mFlatness,
    ScalarData.mDMean,
    ScalarData.mDVariance
  };

  MStaticAssert(MCountOf(sStatisticsNames) == MCountOf(Statistics));
  for (size_t i = 0; i < MCountOf(Statistics); ++i)
  {
    if (pDestVector != NULL)
    {
      const double Value = Statistics[i];

      if (TMath::IsNaN(Value) || TMath::IsInf(Value))
      {
        throw TReadableException("Invalid Descriptor array value (NAN of INF)");
      }

      pDestVector->push_back(Value);
    }

    if (pDestNameVector != NULL) 
    {
      pDestNameVector->push_back(
        std::string(ScalarData.mpName) + "_" + sStatisticsNames[i]);
    }
  }
}

// -------------------------------------------------------------------------------------------------

template <size_t sNumberOfBands>
static void SAddDescriptorBandTimeFrames(
  std::vector<std::string>*                                     pDestNameVector,
  std::vector<double>*                                          pDestVector,
  const TSampleDescriptors::TFramedVectorData<sNumberOfBands>&  BandDescriptor,
  TStaticArray<double, sNumberOfBands>                          NoValueValues)
{
  for (int b = 0; b < (int)sNumberOfBands; ++b)
  {
    for (size_t i = 0; i < sTimeSeriesLength; ++i)
    {
      if (pDestVector != NULL)
      {
        const int Frame = sTimeSeries[i];

        const double Value = (Frame < BandDescriptor.mValues.Size()) ?
          BandDescriptor.mValues[Frame][b] : NoValueValues[b];

        if (TMath::IsNaN(Value) || TMath::IsInf(Value))
        {
          throw TReadableException("Invalid Descriptor array value (NAN of INF)");
        }

        pDestVector->push_back(Value);
      }

      if (pDestNameVector != NULL)
      {
        pDestNameVector->push_back(
          std::string(BandDescriptor.mpName) + "_b" +
          std::to_string(b) + "_t" + std::to_string(i));
      }
    }
  }
}

// -------------------------------------------------------------------------------------------------

template <size_t sNumberOfBands>
static void SAddDescriptorBandStatistics(
  std::vector<std::string>*                                     pDestNameVector,
  std::vector<double>*                                          pDestVector,
  const TSampleDescriptors::TFramedVectorData<sNumberOfBands>&  BandDescriptor)
{
  for (int BandIndex = 0; BandIndex < (int)sNumberOfBands; ++BandIndex)
  {
    static const char* sStatisticsNames[] = {
      "min",
      "max",
      // "median",
      "mean",
      // "gmean",
      "variance",
      // "centroid",
      // "spread",
      // "skewness",
      // "kurtosis",
      "flatness",
      "dmean",
      "dvariance"
    };

    const double Statistics[] = {
      BandDescriptor.mMin[BandIndex],
      BandDescriptor.mMax[BandIndex],
      // BandDescriptor.mMedian[BandIndex],
      BandDescriptor.mMean[BandIndex],
      // BandDescriptor.mGeometricMean[BandIndex],
      BandDescriptor.mVariance[BandIndex],
      // BandDescriptor.mCentroid[BandIndex],
      // BandDescriptor.mSpread[BandIndex],
      // BandDescriptor.mSkewness[BandIndex],
      // BandDescriptor.mKurtosis[BandIndex],
      BandDescriptor.mFlatness[BandIndex],
      BandDescriptor.mDMean[BandIndex],
      BandDescriptor.mDVariance[BandIndex]
    };

    MStaticAssert(MCountOf(sStatisticsNames) == MCountOf(Statistics));
    for (size_t i = 0; i < MCountOf(Statistics); ++i)
    {
      if (pDestVector != NULL)
      {
        const double Value = Statistics[i];

        if (TMath::IsNaN(Value) || TMath::IsInf(Value))
        {
          throw TReadableException("Invalid Descriptor array value (NAN of INF)");
        }

        pDestVector->push_back(Value);
      }

      if (pDestNameVector != NULL) 
      {
        pDestNameVector->push_back(std::string(BandDescriptor.mpName) + "_" +
          sStatisticsNames[i] + "_b" + std::to_string(BandIndex));
      }
    }
  }
}

// -------------------------------------------------------------------------------------------------

template <typename T>
static void SAddDescriptorScalarValue(
  std::vector<std::string>*                 pDestNameVector,
  std::vector<double>*                      pDestVector,
  const TSampleDescriptors::TScalarData<T>& ScalarData)
{
  if (pDestVector != NULL)
  {
    const double Value = ScalarData.mValue;

    if (TMath::IsNaN(Value) || TMath::IsInf(Value))
    {
      throw TReadableException("Invalid Descriptor array value (NAN of INF)");
    }

    pDestVector->push_back(Value);
  }

  if (pDestNameVector != NULL)
  {
    pDestNameVector->push_back(ScalarData.mpName);
  }
}

// -------------------------------------------------------------------------------------------------

static void SAddDescriptorValue(
  std::vector<std::string>* pDestNameVector,
  std::vector<double>*      pDestVector,
  const std::string&        ValueName,
  double                    Value)
{
  if (pDestVector != NULL) 
  {
    if (TMath::IsNaN(Value) || TMath::IsInf(Value))
    {
      throw TReadableException("Invalid Descriptor array value (NAN of INF)");
    }

    pDestVector->push_back(Value);
  }

  if (pDestNameVector != NULL) 
  {
    pDestNameVector->push_back(ValueName);
  }
}

// -------------------------------------------------------------------------------------------------

//! Analyzes a dummy, empty sample file to get descriptor values for silent 
//! frames. We'll use those frames to pad descriptors which are shorted than 
//! the analyzation time frame.

static TSampleDescriptors SCreateSilenceSampleDescriptors()
{
  const int SamplesPerSec = 44100;
  const int NumSamples = SamplesPerSec / 2;

  TArray<float> SilentSamples(NumSamples);
  SilentSamples.Init(0.0f);

  TArray<const float*> MonoChannelPtrs(1);
  MonoChannelPtrs[0] = SilentSamples.FirstRead();

  const TString TempSampleName = gGenerateTempFileName(gTempDir(), ".wav");

  TOwnerPtr<TWaveFile> pSilentWave(new TWaveFile());

  pSilentWave->OpenForWrite(
    TempSampleName, SamplesPerSec,
    TAudioFile::k32BitFloat, MonoChannelPtrs.Size());

  const bool Dither = false;
  pSilentWave->Stream()->WriteSamples(MonoChannelPtrs, NumSamples, Dither);

  pSilentWave->Close();

  try
  {
    TSampleAnalyser Analyzer(SamplesPerSec, 2048, 2048 / 2);
    return Analyzer.Analyze(TempSampleName);
  }
  catch (const std::exception& Exception)
  {
    throw TReadableException(
      MText("Failed to analyze silent sample data file: %s", Exception.what()));
  }
}

// =================================================================================================

int TSampleClassificationDescriptors::sNumberOfTimeFrames = sTimeSeriesLength;
int TSampleClassificationDescriptors::sNumberOfSpectrumBands = sNumberSpectrumBands;
TOwnerPtr<TSampleDescriptors> TSampleClassificationDescriptors::spSilenceDescriptors;

// -------------------------------------------------------------------------------------------------

void TSampleClassificationDescriptors::SInit()
{
  // cache descriptor values for silence
  spSilenceDescriptors = TOwnerPtr<TSampleDescriptors>(
    new TSampleDescriptors(SCreateSilenceSampleDescriptors()));
}

// -------------------------------------------------------------------------------------------------

void TSampleClassificationDescriptors::SExit()
{
  spSilenceDescriptors.Delete();
}

// -------------------------------------------------------------------------------------------------

TSampleClassificationDescriptors::TSampleClassificationDescriptors()
  : mFeatures(),
    mFileName()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

TSampleClassificationDescriptors::TSampleClassificationDescriptors(
  const TSampleDescriptors&   Descriptors,
  TFeatureExtractionFlags     Flags)
{
  MStaticAssert(sTimeSeriesLength >=
    TSampleDescriptors::kNumberOfSpectrumSubBands);
  MStaticAssert(sTimeSeriesLength >=
    TSampleDescriptors::kNumberOfCepstrumCoefficients);
  MStaticAssert(sNumberSpectrumBands <=
    TSampleDescriptors::kNumberOfSpectrumBands);

  // cached descriptor values for silence
  MAssert(spSilenceDescriptors, "Project's Init() function was not called?");

  // memorize filename
  mFileName = Descriptors.mFileName;

  // reserve a bit of free space for the dest vector
  const int ExpectedMaxFeatureLength = sTimeSeriesLength * 2 * sTimeSeriesLength;
  
  std::vector<double>* pFeatures = NULL;
  if (Flags & kExtractFeatureValues)
  {
    pFeatures = &mFeatures;
    pFeatures->reserve(ExpectedMaxFeatureLength);
  }

  std::vector<std::string>* pFeatureNames = NULL;
  if (Flags & kExtractFeatureNames)
  {
    pFeatureNames = &mFeatureNames;
    pFeatureNames->reserve(ExpectedMaxFeatureLength);
  }


  // !!! ... add sharpened, condensed spectrum bands

  for (int b = 0; b < sNumberSpectrumBands; ++b)
  {
    for (int i = 0; i < sTimeSeriesLength; ++i)
    {
      const int TimeFrame = sTimeSeries[i];

      const std::string FeatureName = (pFeatureNames != NULL) ?
        "spectrum_signature_b" + std::to_string(b) + "_t" + std::to_string(TimeFrame) : 
        "";

      if (TimeFrame < Descriptors.mSpectrumBands.mValues.Size())
      {
        // start from 0 or continue behind last band's band
        const int SrcStartBand = (b - 1) >= 0 ? sSpectrumBands[b - 1] + 1 : 0;
        const int SrcEndBand = sSpectrumBands[b];
        const int SrcBandRange = SrcEndBand - SrcStartBand + 1;

        double MergedBandValue = 0.0;
        for (int sb = SrcStartBand; sb <= SrcEndBand; ++sb)
        {
          MergedBandValue += Descriptors.mSpectrumBands.mValues[TimeFrame][sb];
        }
        MergedBandValue /= (double)SrcBandRange;

        MAssert(MergedBandValue >= 0.0 && MergedBandValue <= 1.0,
          "Unexpected spectrum value range");

        // normalize (with a bit overscaling), then compress
        SAddDescriptorValue(pFeatureNames, pFeatures, FeatureName,
          ::pow(MergedBandValue * 1.25, 1.0 / 6.0));
      }
      else
      {
        SAddDescriptorValue(pFeatureNames, pFeatures, FeatureName,
          spSilenceDescriptors->mSpectrumBands.mValues.Last()[b]);
      }
    }
  }


  // ... add spectrum time series vectors

  // !! average density of spectrum bands -> loudness 
  SAddDescriptorSpectrumTimeFrames(pFeatureNames, pFeatures, Descriptors.mSpectralRms,
    spSilenceDescriptors->mSpectralRms.mValues.Last());
  // !! flatness of spectrum bands -> noisiness
  SAddDescriptorSpectrumTimeFrames(pFeatureNames, pFeatures, Descriptors.mSpectralFlatness,
    spSilenceDescriptors->mSpectralFlatness.mValues.Last());
  // !!! changes in spectrum bands compared to previous frame -> temporal flux
  SAddDescriptorSpectrumTimeFrames(pFeatureNames, pFeatures, Descriptors.mSpectralFlux,
    spSilenceDescriptors->mSpectralFlux.mValues.Last());
  // !!! difference between peaks and valleys in the spectrum
  SAddDescriptorSpectrumTimeFrames(pFeatureNames, pFeatures, Descriptors.mSpectralContrast,
    spSilenceDescriptors->mSpectralContrast.mValues.Last());
  // !! complexity of the spectrum based on the number of peaks -> noisiness
  SAddDescriptorSpectrumTimeFrames(pFeatureNames, pFeatures, Descriptors.mSpectralComplexity,
    spSilenceDescriptors->mSpectralComplexity.mValues.Last());
  // !! autocorrelation matches -> harmonicity measure
  SAddDescriptorSpectrumTimeFrames(pFeatureNames, pFeatures, Descriptors.mF0Confidence,
    spSilenceDescriptors->mF0Confidence.mValues.Last());
  

  // ... add spectrum time series statistics 

  SAddDescriptorSpectrumStatistics(pFeatureNames, pFeatures, Descriptors.mSpectralRms);
  SAddDescriptorSpectrumStatistics(pFeatureNames, pFeatures, Descriptors.mSpectralFlatness);
  SAddDescriptorSpectrumStatistics(pFeatureNames, pFeatures, Descriptors.mSpectralFlux);
  SAddDescriptorSpectrumStatistics(pFeatureNames, pFeatures, Descriptors.mSpectralContrast);
  SAddDescriptorSpectrumStatistics(pFeatureNames, pFeatures, Descriptors.mSpectralComplexity);
  SAddDescriptorSpectrumStatistics(pFeatureNames, pFeatures, Descriptors.mF0Confidence);
  

  // ... add band statistics

  SAddDescriptorBandStatistics(pFeatureNames, pFeatures, Descriptors.mSpectralRmsBands);
  SAddDescriptorBandStatistics(pFeatureNames, pFeatures, Descriptors.mSpectralFlatnessBands);
  SAddDescriptorBandStatistics(pFeatureNames, pFeatures, Descriptors.mSpectralFluxBands);
  SAddDescriptorBandStatistics(pFeatureNames, pFeatures, Descriptors.mSpectralComplexityBands);
  SAddDescriptorBandStatistics(pFeatureNames, pFeatures, Descriptors.mSpectralContrastBands);

  // !! add extra cepstrum bands bands as condensed other view of the spectrum
  SAddDescriptorBandStatistics(pFeatureNames, pFeatures, Descriptors.mCepstrumBands);


  // ... add rhythm statistics and length (mainly, but not only for oneshot-vs-loop classifiers)

  // !!! amplitude - peak measure (also, but not only as rough onset measure)
  SAddDescriptorSpectrumTimeFrames(pFeatureNames, pFeatures, Descriptors.mAmplitudeRms,
    spSilenceDescriptors->mAmplitudeRms.mValues.Last());
  SAddDescriptorSpectrumStatistics(pFeatureNames, pFeatures, Descriptors.mAmplitudeRms);

  // !! amplitude - silence detection (very rough, but reliable onset measure)
  SAddDescriptorSpectrumStatistics(pFeatureNames, pFeatures, Descriptors.mAmplitudeSilence);

  // !!! tempo confidence (steady peak/rhythm measure)
  SAddDescriptorScalarValue(pFeatureNames, pFeatures, Descriptors.mRhythmComplexTempoConfidence);
  SAddDescriptorScalarValue(pFeatureNames, pFeatures, Descriptors.mRhythmPercussiveTempoConfidence);

  // !!(! for Loop vs.OneShot) onset statistics
  SAddDescriptorScalarValue(pFeatureNames, pFeatures, Descriptors.mRhythmComplexOnsetContrast);
  SAddDescriptorScalarValue(pFeatureNames, pFeatures, Descriptors.mRhythmPercussiveOnsetContrast);

  SAddDescriptorScalarValue(pFeatureNames, pFeatures, Descriptors.mRhythmComplexOnsetStrength);
  SAddDescriptorScalarValue(pFeatureNames, pFeatures, Descriptors.mRhythmPercussiveOnsetStrength);

  // !! effective length
  SAddDescriptorScalarValue(pFeatureNames, pFeatures, Descriptors.mEffectiveLength12dB);
    
  
  // ... pad to sTimeSeriesLength width 

  int PaddingIndex = 0;
  while ((mFeatures.size() % sTimeSeriesLength) != 0)
  {
    // add "some" valid value
    const double PaddingValue = Descriptors.mSpectralRms.mMean;
    const std::string FeatureName = (pFeatureNames != NULL) ?
      "padding_" + std::to_string(PaddingIndex) :
      "";
    SAddDescriptorValue(pFeatureNames, pFeatures, FeatureName, PaddingValue);
    ++PaddingIndex;
  }
 

  // ... validate

  // ensure that all names and value sizes match
  MAssert(MImplies(pFeatureNames && pFeatures, pFeatureNames->size() == pFeatures->size()), 
    "Expecting a name for each feature to be set");
}

