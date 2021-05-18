#pragma once

#ifndef _SampleDescriptors_h_
#define _SampleDescriptors_h_

// =================================================================================================

#include "CoreTypes/Export/Pair.h"
#include "CoreTypes/Export/StlIterator.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Array.h"
#include "CoreTypes/Export/List.h"

#include "FeatureExtraction/Export/Statistics.h"

#include <boost/variant.hpp>

// =================================================================================================

// enable, to write additional debug_R/VR/VVR descriptors in high-level databases
// !! changes the default column set, so only enable in development builds !! 
// #define MEnableDebugSampleDescriptors

// =================================================================================================

/*!
 * Holds a set of descriptors which gets evaluated from a single sample.
!*/

class TSampleDescriptors
{
public:
  TSampleDescriptors();


  //@{ ... Sample Descriptor baseclass

  class TDescriptor
  {
  public:
    // provides variant access to all available value types of descriptor members
    typedef boost::variant<
      int*,
      double*,
      TString*,
      TList<double>*,
      TList<TString>*,
      TStaticArray<double, 14>*,
      TStaticArray<double, 28>*,
      TList< TList<double> >*,
      TList< TStaticArray<double, 14> >*,
      TList< TStaticArray<double, 28> >*
    > TValue;

    // get all available descriptor names and values (as variant type)
    TList< TPair<TString, TValue> > Values() const { 
      return const_cast<TDescriptor*>(this)->OnValues(); 
    };
    TList< TPair<TString, TValue> > Values() { 
      return OnValues(); 
    };

    // Calc initial set of statistics from the "main" vector value, if present
    // Only implemented for VR and VVR data.
    void CalcStatistics()
    {
      OnCalcStatistics();
    };

    // Descriptor export "recommendations"
    enum {
      // allow binary instead of text storage, if suitable, for example for vector data 
      kAllowBinaryStorage                 = (1 << 0),
      // allow storing numbers in floating point precision instead of double precision 
      kAllowFloatingPointPrecisionStorage = (1 << 1)
    }; 
    typedef int TExportFlags;
    const TExportFlags mExportFlags;

    // the descriptor's base name, such as "pitch"
    const char* const mpName;

  protected:
    virtual TList< TPair<TString, TValue> > OnValues() = 0;
    virtual void OnCalcStatistics() = 0;

    TDescriptor(const char* pName, TExportFlags ExportFlags)
      : mpName(pName),
        mExportFlags(ExportFlags)
    { }
  };


  //@{ ... R or S descriptor which holds a single value

  template <typename T>
  class TScalarData : public TDescriptor
  {
  public:
    TScalarData(const char* pName, TExportFlags ExportFlags)
      : TDescriptor(pName, ExportFlags),
        mValue()
    { }

    T mValue;

  private:
    virtual TList< TPair<TString, TDescriptor::TValue> > OnValues()
    {
      return MakeList(
        MakePair(TString(mpName), TDescriptor::TValue(&mValue))
      );
    }

    virtual void OnCalcStatistics()
    {
      // nothing to do
    }
  };
  //@}


  //@{ ... VR or VS descriptor which holds a list of vector data

  template <typename T>
  struct TVectorData : public TDescriptor
  {
    TVectorData(const char* pName, TExportFlags ExportFlags)
      : TDescriptor(pName, ExportFlags)
    { }

    TList<T> mValues;

  private:
    virtual TList< TPair<TString, TDescriptor::TValue> > OnValues()
    {
      return MakeList(
        MakePair(TString(mpName), TDescriptor::TValue(&mValues))
      );
    }

    virtual void OnCalcStatistics()
    {
      // nothing to do
    }
  };
  //@}


  //@{ ... VR descriptor which holds a aingle real value per frame + statistics

  struct TFramedScalarData : public TDescriptor
  {
    TFramedScalarData(const char* pName, TExportFlags ExportFlags)
      : TDescriptor(pName, ExportFlags),
        mMin(0.0),
        mMax(0.0),
        mMedian(0.0),
        mMean(0.0),
        mGeometricMean(0.0),
        mVariance(0.0),
        mCentroid(0.0),
        mSpread(0.0),
        mSkewness(0.0),
        mKurtosis(0.0),
        mFlatness(0.0),
        mDMean(0.0),
        mDVariance(0.0)
    { }

    TList<double> mValues; // [frame]
    double mMin;
    double mMax;
    double mMedian;
    double mMean;
    double mGeometricMean;
    double mVariance;
    double mCentroid;
    double mSpread;
    double mSkewness;
    double mKurtosis;
    double mFlatness;
    double mDMean;
    double mDVariance;

    virtual TList< TPair<TString, TDescriptor::TValue> > OnValues()
    {
      return 
        MakeList(
          MakePair(TString(mpName), TDescriptor::TValue(&mValues)),
          MakePair(TString(mpName) + "_min", TDescriptor::TValue(&mMin)),
          MakePair(TString(mpName) + "_max", TDescriptor::TValue(&mMax)),
          MakePair(TString(mpName) + "_median", TDescriptor::TValue(&mMedian)),
          MakePair(TString(mpName) + "_mean", TDescriptor::TValue(&mMean)),
          MakePair(TString(mpName) + "_gmean", TDescriptor::TValue(&mGeometricMean)),
          MakePair(TString(mpName) + "_variance", TDescriptor::TValue(&mVariance))
        ) +
        MakeList(
          MakePair(TString(mpName) + "_centroid", TDescriptor::TValue(&mCentroid)),
          MakePair(TString(mpName) + "_spread", TDescriptor::TValue(&mSpread)),
          MakePair(TString(mpName) + "_skewness", TDescriptor::TValue(&mSkewness)),
          MakePair(TString(mpName) + "_kurtosis", TDescriptor::TValue(&mKurtosis)),
          MakePair(TString(mpName) + "_flatness", TDescriptor::TValue(&mFlatness))
        ) + 
        MakeList(
          MakePair(TString(mpName) + "_dmean", TDescriptor::TValue(&mDMean)),
          MakePair(TString(mpName) + "_dvariance", TDescriptor::TValue(&mDVariance)
        )
      );
    }

    virtual void OnCalcStatistics()
    {
      TStatistics::Calc(
        mMin,
        mMax,
        mMedian,
        mMean,
        mGeometricMean,
        mVariance,
        mCentroid,
        mSpread,
        mSkewness,
        mKurtosis,
        mFlatness,
        mDMean,
        mDVariance,
        mValues.FirstRead(),
        mValues.Size());
    }
  };
  //@}


  //@{ ... VVR or VVS descriptor which holds a list of vector of vector data

  template <typename T>
  struct TVectorVectorData : public TDescriptor
  {
    TVectorVectorData(const char* pName, TExportFlags ExportFlags)
      : TDescriptor(pName, ExportFlags)
    { }

    TList< TList<T> > mValues;

  private:
    virtual TList< TPair<TString, TDescriptor::TValue> > OnValues()
    {
      return MakeList(
        MakePair(TString(mpName), TDescriptor::TValue(&mValues))
      );
    }

    virtual void OnCalcStatistics()
    {
      // nothing to do
    }
  };
  //@}


  //@{ ... VVR descriptor which holds an array of real values per frame + statistics

  template <size_t sSize>
  struct TFramedVectorData : public TDescriptor
  {
    TFramedVectorData(const char* pName, TExportFlags ExportFlags)
      : TDescriptor(pName, ExportFlags)
    {
      mMin.Init(0.0);
      mMax.Init(0.0);
      mMedian.Init(0.0);
      mMean.Init(0.0);
      mGeometricMean.Init(0.0);
      mVariance.Init(0.0);
      mCentroid.Init(0.0);
      mSpread.Init(0.0);
      mSkewness.Init(0.0);
      mKurtosis.Init(0.0);
      mFlatness.Init(0.0);
      mDMean.Init(0.0);
      mDVariance.Init(0.0);
    }

    TList< TStaticArray<double, sSize> > mValues; // [frame][band]
    TStaticArray<double, sSize> mMin; // [band]
    TStaticArray<double, sSize> mMax;
    TStaticArray<double, sSize> mMedian;
    TStaticArray<double, sSize> mMean;
    TStaticArray<double, sSize> mGeometricMean;
    TStaticArray<double, sSize> mVariance;
    TStaticArray<double, sSize> mCentroid;
    TStaticArray<double, sSize> mSpread;
    TStaticArray<double, sSize> mSkewness;
    TStaticArray<double, sSize> mKurtosis;
    TStaticArray<double, sSize> mFlatness;
    TStaticArray<double, sSize> mDMean;
    TStaticArray<double, sSize> mDVariance;

  private:
    virtual TList< TPair<TString, TDescriptor::TValue> > OnValues()
    {
      return 
        MakeList(
          MakePair(TString(mpName), TDescriptor::TValue(&mValues)),
          MakePair(TString(mpName) + "_min", TDescriptor::TValue(&mMin)),
          MakePair(TString(mpName) + "_max", TDescriptor::TValue(&mMax)),
          MakePair(TString(mpName) + "_median", TDescriptor::TValue(&mMedian)),
          MakePair(TString(mpName) + "_mean", TDescriptor::TValue(&mMean)),
          MakePair(TString(mpName) + "_gmean", TDescriptor::TValue(&mGeometricMean)),
          MakePair(TString(mpName) + "_variance", TDescriptor::TValue(&mVariance))
        ) +
        MakeList(
          MakePair(TString(mpName) + "_centroid", TDescriptor::TValue(&mCentroid)),
          MakePair(TString(mpName) + "_spread", TDescriptor::TValue(&mSpread)),
          MakePair(TString(mpName) + "_skewness", TDescriptor::TValue(&mSkewness)),
          MakePair(TString(mpName) + "_kurtosis", TDescriptor::TValue(&mKurtosis)),
          MakePair(TString(mpName) + "_flatness", TDescriptor::TValue(&mFlatness))
        ) +
        MakeList(
          MakePair(TString(mpName) + "_dmean", TDescriptor::TValue(&mDMean)),
          MakePair(TString(mpName) + "_dvariance", TDescriptor::TValue(&mDVariance)
        )
      );
    }

    virtual void OnCalcStatistics()
    {
      for (int BandIndex = 0; BandIndex < (int)sSize; ++BandIndex)
      {
        TArray<double> BandValues(mValues.Size());
        for (int FrameIndex = 0; FrameIndex < mValues.Size(); ++FrameIndex)
        {
          BandValues[FrameIndex] = mValues[FrameIndex][BandIndex];
        }

        TStatistics::Calc(
          mMin[BandIndex],
          mMax[BandIndex],
          mMedian[BandIndex],
          mMean[BandIndex],
          mGeometricMean[BandIndex],
          mVariance[BandIndex],
          mCentroid[BandIndex],
          mSpread[BandIndex],
          mSkewness[BandIndex],
          mKurtosis[BandIndex],
          mFlatness[BandIndex],
          mDMean[BandIndex],
          mDVariance[BandIndex],
          BandValues.FirstRead(),
          BandValues.Size()
        );
      }
    }
  };
  //@}


  //@{ ... Descriptor (sub)set

  enum TDescriptorSet
  {
    //! general low level set of descriptors, needed to compute high level descriptors 
    //! and to train the classification models.
    kLowLevelDescriptors,

    //! high level descriptors, which are mostly calculated from the low level descriptors.
    //! this is the descriptor set which is most useful for humans.
    kHighLevelDescriptors,

    kNumberOfDescriptorSet
  };

  //! get an list of descriptors (a sub set) for the given descriptor set type.
  //! NOTE: don't keep the pointers dangling around. They will die with their parent object!
  TList<const TDescriptor*> Descriptors(TDescriptorSet DescriptorSet) const;
  TList<TDescriptor*> Descriptors(TDescriptorSet DescriptorSet);
  //@}


  //@{ ... Basic Data

  TString mFileName;

  // audio file properties
  TScalarData<TString> mFileType;
  TScalarData<int> mFileSize; // bytes
  TScalarData<double> mFileLength; // seconds
  TScalarData<int> mFileSampleRate;
  TScalarData<int> mFileChannelCount;
  TScalarData<int> mFileBitDepth;
  //@}


  //@{ ... Low-Level Data

  // Effective length (skipping leading and trailing silence)
  TScalarData<double> mEffectiveLength48dB; // seconds
  TScalarData<double> mEffectiveLength24dB;
  TScalarData<double> mEffectiveLength12dB;
  
  // offset of all following descriptors
  TScalarData<double> mAnalyzationOffset; // seconds

  // Framed Data + Statistics
  TFramedScalarData mAmplitudeSilence;
  TFramedScalarData mAmplitudePeak;
  TFramedScalarData mAmplitudeRms;
  TFramedScalarData mAmplitudeEnvelope;

  TFramedScalarData mSpectralRms;
  TFramedScalarData mSpectralCentroid;
  TFramedScalarData mSpectralRolloff;
  TFramedScalarData mSpectralSpread;
  TFramedScalarData mSpectralSkewness;
  TFramedScalarData mSpectralKurtosis;
  TFramedScalarData mSpectralFlatness;
  TFramedScalarData mSpectralInharmonicity;
  TFramedScalarData mSpectralComplexity;
  TFramedScalarData mSpectralContrast;
  TFramedScalarData mSpectralFlux;

  TFramedScalarData mF0;
  TFramedScalarData mF0Confidence;
  TFramedScalarData mFailSafeF0;

  TFramedScalarData mTristimulus1;
  TFramedScalarData mTristimulus2;
  TFramedScalarData mTristimulus3;

  TFramedScalarData mAutoCorrelation;

  TFramedScalarData mRhythmComplexOnsets;
  TScalarData<double> mRhythmComplexOnsetCount;
  TScalarData<double> mRhythmComplexOnsetFrequencyMean;
  TScalarData<double> mRhythmComplexOnsetStrength;
  TScalarData<double> mRhythmComplexOnsetContrast;
  
  TFramedScalarData mRhythmPercussiveOnsets;
  TScalarData<double> mRhythmPercussiveOnsetCount;
  TScalarData<double> mRhythmPercussiveOnsetFrequencyMean;
  TScalarData<double> mRhythmPercussiveOnsetStrength;
  TScalarData<double> mRhythmPercussiveOnsetContrast;

  TScalarData<double> mRhythmComplexTempo;
  TScalarData<double> mRhythmComplexTempoConfidence;
  TScalarData<double> mRhythmPercussiveTempo;
  TScalarData<double> mRhythmPercussiveTempoConfidence;
  
  TScalarData<double> mRhythmFinalTempo;
  TScalarData<double> mRhythmFinalTempoConfidence;

  // Framed Vector Data + Statistics
  enum { kNumberOfSpectrumSubBands = 14 };
  TFramedVectorData<kNumberOfSpectrumSubBands> mSpectralRmsBands;
  TFramedVectorData<kNumberOfSpectrumSubBands> mSpectralFlatnessBands;
  TFramedVectorData<kNumberOfSpectrumSubBands> mSpectralFluxBands;
  TFramedVectorData<kNumberOfSpectrumSubBands> mSpectralComplexityBands;
  TFramedVectorData<kNumberOfSpectrumSubBands> mSpectralContrastBands;

  enum { kNumberOfSpectrumBands = 28 };
  TFramedVectorData<kNumberOfSpectrumBands> mSpectrumBands;
  enum { kNumberOfCepstrumCoefficients = 14 };
  TFramedVectorData<kNumberOfCepstrumCoefficients> mCepstrumBands;
  //@}


  //@{ ... High-Level Data ("Sound characteristics")

  // classification
  TVectorData<double> mClassSignature;
  TVectorData<TString> mClasses;
  TVectorData<double> mClassStrengths;

  // categorization
  TVectorData<double> mCategorySignature;
  TVectorData<TString> mCategories;
  TVectorData<double> mCategoryStrengths;

  // loudness
  TScalarData<double> mHighLevelPeakDb;
  TScalarData<double> mHighLevelRmsDb;

  // pitch
  TScalarData<double> mHighLevelBaseNote;
  TScalarData<double> mHighLevelBaseNoteConfidence;

  // BPM
  TScalarData<double> mHighLevelBpm;
  TScalarData<double> mHighLevelBpmConfidence;

  // characteristcs
  TScalarData<double> mHighLevelBrightness;
  TScalarData<double> mHighLevelNoisiness;
  TScalarData<double> mHighLevelHarmonicity;
  //@}


  //@{ ... High-Level Similarity Aspect Data ("Sound characteristics" for similarity searches)

  // spectrum
  enum { kNumberOfHighLevelSpectrumBandFrames = 64 };
  enum { kNumberOfHighLevelSpectrumBands = 14 };
  TVectorVectorData<double> mHighLevelSpectrumSignature; // [64][14]
  // Flatness, Flux, Complexity, Contrast, Inharmonicity
  TVectorData<double> mHighLevelSpectrumFeatures; // [5]

  // timbre
  TVectorVectorData<double> mHighLevelTristimulus; // [frame][3]
  // TODO: TScalarData<double> mHighLevelTristimulusConfidence;

  // pitch
  TVectorData<double> mHighLevelPitch;
  TScalarData<double> mHighLevelPitchConfidence;

  // amplitude
  TVectorData<double> mHighLevelPeak;

  // debug
  #if defined(MEnableDebugSampleDescriptors)
    TScalarData<double> mHighLevelDebugScalarValue;
    TVectorData<double> mHighLevelDebugVectorValue;
    TVectorVectorData<double> mHighLevelDebugVectorVectorValue;
  #endif
  //@}
};


#endif //_SampleDescriptors_h_

