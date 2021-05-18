#include "FeatureExtraction/Export/SampleDescriptors.h"

// =================================================================================================

#if defined(MEnableDebugSampleDescriptors)
  #pragma message("WARNING: *** "\
    "Compiling SampleDescriptors with 'debug_R/VR/VVR' descriptors enabled. "\
    "This option only should be enabled in local development builds^:")
#endif

// =================================================================================================

typedef TSampleDescriptors::TDescriptor TDescriptor;

// used by low and high-level descritpors: NOT binary, accessed in SQlite queries
const TDescriptor::TExportFlags SharedDescriptorFlags = 0;

// low-level descrotors, used for model generation: use doubles but allow compression
const TDescriptor::TExportFlags LowLevelDescriptorFlags =
  TDescriptor::kAllowBinaryStorage;

// high-level features: can use floating points to save space and want 
// readable text (non binary) data
const TDescriptor::TExportFlags HighLevelDescriptorFlags =
  TDescriptor::kAllowFloatingPointPrecisionStorage;

// -------------------------------------------------------------------------------------------------

TSampleDescriptors::TSampleDescriptors() 
  : // shared (low and high level)
    mFileType("file_type", SharedDescriptorFlags),
    mFileSize("file_size", SharedDescriptorFlags),
    mFileLength("file_length", SharedDescriptorFlags),
    mFileSampleRate("file_sample_rate", SharedDescriptorFlags),
    mFileChannelCount("file_channel_count", SharedDescriptorFlags),
    mFileBitDepth("file_bit_depth", SharedDescriptorFlags),
    // low level
    mEffectiveLength48dB("effectve_length_48dB", LowLevelDescriptorFlags),
    mEffectiveLength24dB("effectve_length_24dB", LowLevelDescriptorFlags),
    mEffectiveLength12dB("effectve_length_12dB", LowLevelDescriptorFlags),
    mAnalyzationOffset("analyzation_offset", LowLevelDescriptorFlags),
    mAmplitudeSilence("amplitude_silence", LowLevelDescriptorFlags),
    mAmplitudePeak("amplitude_peak", LowLevelDescriptorFlags),
    mAmplitudeRms("amplitude_rms", LowLevelDescriptorFlags),
    mAmplitudeEnvelope("amplitude_envelope", LowLevelDescriptorFlags),
    mSpectralRms("spectral_rms", LowLevelDescriptorFlags),
    mSpectralCentroid("spectral_centroid", LowLevelDescriptorFlags),
    mSpectralSpread("spectral_spread", LowLevelDescriptorFlags),
    mSpectralSkewness("spectral_skewness", LowLevelDescriptorFlags),
    mSpectralKurtosis("spectral_kurtosis", LowLevelDescriptorFlags),
    mSpectralFlatness("spectral_flatness", LowLevelDescriptorFlags),
    mSpectralRolloff("spectral_rolloff", LowLevelDescriptorFlags),
    mSpectralInharmonicity("spectral_inharmonicity", LowLevelDescriptorFlags),
    mSpectralComplexity("spectral_complexity", LowLevelDescriptorFlags),
    mSpectralContrast("spectral_contrast", LowLevelDescriptorFlags),
    mSpectralFlux("spectral_flux", LowLevelDescriptorFlags),
    mF0("f0", LowLevelDescriptorFlags),
    mF0Confidence("f0_confidence", LowLevelDescriptorFlags),
    mFailSafeF0("failsafe_f0", LowLevelDescriptorFlags),
    mTristimulus1("tristimulus1", LowLevelDescriptorFlags),
    mTristimulus2("tristimulus2", LowLevelDescriptorFlags),
    mTristimulus3("tristimulus3", LowLevelDescriptorFlags),
    mAutoCorrelation("auto_correlation", LowLevelDescriptorFlags),
    mRhythmComplexOnsets("rhythm_complex_onsets", LowLevelDescriptorFlags),
    mRhythmComplexOnsetCount("rhythm_complex_onset_count", LowLevelDescriptorFlags),
    mRhythmComplexOnsetContrast("rhythm_complex_onset_contrast", LowLevelDescriptorFlags),
    mRhythmComplexOnsetFrequencyMean("rhythm_complex_onset_frequency_mean", LowLevelDescriptorFlags),
    mRhythmComplexOnsetStrength("rhythm_complex_onset_strength", LowLevelDescriptorFlags),
    mRhythmComplexTempo("rhythm_complex_tempo", LowLevelDescriptorFlags),
    mRhythmComplexTempoConfidence("rhythm_complex_tempo_confidence", LowLevelDescriptorFlags),
    mRhythmPercussiveOnsets("rhythm_percussive_onsets", LowLevelDescriptorFlags),
    mRhythmPercussiveOnsetCount("rhythm_percussive_onset_count", LowLevelDescriptorFlags),
    mRhythmPercussiveOnsetContrast("rhythm_percussive_onset_contrast", LowLevelDescriptorFlags),
    mRhythmPercussiveOnsetFrequencyMean("rhythm_percussive_onset_frequency_mean", LowLevelDescriptorFlags),
    mRhythmPercussiveOnsetStrength("rhythm_percussive_onset_strength", LowLevelDescriptorFlags),
    mRhythmPercussiveTempo("rhythm_percussive_tempo", LowLevelDescriptorFlags),
    mRhythmPercussiveTempoConfidence("rhythm_percussive_tempo_confidence", LowLevelDescriptorFlags),
    mRhythmFinalTempo("rhythm_final_tempo", LowLevelDescriptorFlags),
    mRhythmFinalTempoConfidence("rhythm_final_tempo_confidence", LowLevelDescriptorFlags),
    mSpectralRmsBands("spectral_rms_bands", LowLevelDescriptorFlags),
    mSpectralFlatnessBands("spectral_flatness_bands", LowLevelDescriptorFlags),
    mSpectralFluxBands("spectral_flux_bands", LowLevelDescriptorFlags),
    mSpectralComplexityBands("spectral_complexity_bands", LowLevelDescriptorFlags),
    mSpectralContrastBands("spectral_contrast_bands", LowLevelDescriptorFlags),
    mSpectrumBands("frequency_bands", LowLevelDescriptorFlags),
    mCepstrumBands("cepstrum_bands", LowLevelDescriptorFlags),
    // high level assets
    mClassSignature("class_signature", HighLevelDescriptorFlags),
    mClasses("classes", HighLevelDescriptorFlags),
    mClassStrengths("class_strengths", HighLevelDescriptorFlags),
    mCategorySignature("category_signature", HighLevelDescriptorFlags),
    mCategories("categories", HighLevelDescriptorFlags),
    mCategoryStrengths("category_strengths", HighLevelDescriptorFlags),
    mHighLevelBaseNote("base_note", HighLevelDescriptorFlags),
    mHighLevelBaseNoteConfidence("base_note_confidence", HighLevelDescriptorFlags),
    mHighLevelPeakDb("peak_db", HighLevelDescriptorFlags),
    mHighLevelRmsDb("rms_db", HighLevelDescriptorFlags),
    mHighLevelBpm("bpm", HighLevelDescriptorFlags),
    mHighLevelBpmConfidence("bpm_confidence", HighLevelDescriptorFlags),
    mHighLevelBrightness("brightness", HighLevelDescriptorFlags),
    mHighLevelNoisiness("noisiness", HighLevelDescriptorFlags),
    mHighLevelHarmonicity("harmonicity", HighLevelDescriptorFlags),
    // high level extended assets
    mHighLevelSpectrumSignature("spectrum_signature", HighLevelDescriptorFlags),
    mHighLevelSpectrumFeatures("spectrum_features", HighLevelDescriptorFlags),
    mHighLevelTristimulus("tristimulus", HighLevelDescriptorFlags),
    mHighLevelPitch("pitch", HighLevelDescriptorFlags),
    mHighLevelPitchConfidence("pitch_confidence", HighLevelDescriptorFlags),
    mHighLevelPeak("peak", HighLevelDescriptorFlags)
    // high level debug columns
    #if defined(MEnableDebugSampleDescriptors)
      , mHighLevelDebugScalarValue("debug", HighLevelDescriptorFlags)
      , mHighLevelDebugVectorValue("debug", HighLevelDescriptorFlags)
      , mHighLevelDebugVectorVectorValue("debug", HighLevelDescriptorFlags)
    #endif
{ }

// -------------------------------------------------------------------------------------------------

TList<const TSampleDescriptors::TDescriptor*>
  TSampleDescriptors::Descriptors(TDescriptorSet DescriptorSet) const
{
  // avoid duplicating code and cast ptrs to const ptrs
  const TList<TSampleDescriptors::TDescriptor*> MutableDescriptors = 
    const_cast<TSampleDescriptors*>(this)->Descriptors(DescriptorSet);

  TList<const TSampleDescriptors::TDescriptor*> Ret;
  for (int i = 0; i < MutableDescriptors.Size(); ++i)
  {
    Ret.Append(const_cast<const TSampleDescriptors::TDescriptor*>(
      MutableDescriptors[i]));
  }
  return Ret;
}

TList<TSampleDescriptors::TDescriptor*> 
  TSampleDescriptors::Descriptors(TDescriptorSet DescriptorSet) 
{
  TList<TSampleDescriptors::TDescriptor*> Ret;

  // common
  Ret.Append(&mFileType);
  Ret.Append(&mFileSize);
  Ret.Append(&mFileLength);
  Ret.Append(&mFileSampleRate);
  Ret.Append(&mFileChannelCount);
  Ret.Append(&mFileBitDepth);

  // low level only
  if (DescriptorSet == kLowLevelDescriptors)
  {
    Ret.Append(&mEffectiveLength48dB);
    Ret.Append(&mEffectiveLength24dB);
    Ret.Append(&mEffectiveLength12dB);
    Ret.Append(&mAnalyzationOffset);
    Ret.Append(&mAmplitudeSilence);
    Ret.Append(&mAmplitudePeak);
    Ret.Append(&mAmplitudeRms);
    Ret.Append(&mAmplitudeEnvelope);
    Ret.Append(&mSpectralRms);
    Ret.Append(&mSpectralCentroid);
    Ret.Append(&mSpectralRolloff);
    Ret.Append(&mSpectralSpread);
    Ret.Append(&mSpectralSkewness);
    Ret.Append(&mSpectralKurtosis);
    Ret.Append(&mSpectralFlatness);
    Ret.Append(&mSpectralInharmonicity);
    Ret.Append(&mSpectralComplexity);
    Ret.Append(&mSpectralContrast);
    Ret.Append(&mSpectralFlux);
    Ret.Append(&mF0);
    Ret.Append(&mF0Confidence);
    Ret.Append(&mFailSafeF0);
    Ret.Append(&mTristimulus1);
    Ret.Append(&mTristimulus2);
    Ret.Append(&mTristimulus3);
    Ret.Append(&mAutoCorrelation);
    Ret.Append(&mRhythmComplexOnsets);
    Ret.Append(&mRhythmComplexOnsetCount);
    Ret.Append(&mRhythmComplexOnsetContrast);
    Ret.Append(&mRhythmComplexOnsetFrequencyMean);
    Ret.Append(&mRhythmComplexOnsetStrength);
    Ret.Append(&mRhythmComplexTempo);
    Ret.Append(&mRhythmComplexTempoConfidence);
    Ret.Append(&mRhythmPercussiveOnsets);
    Ret.Append(&mRhythmPercussiveOnsetCount);
    Ret.Append(&mRhythmPercussiveOnsetContrast);
    Ret.Append(&mRhythmPercussiveOnsetFrequencyMean);
    Ret.Append(&mRhythmPercussiveOnsetStrength);
    Ret.Append(&mRhythmPercussiveTempo);
    Ret.Append(&mRhythmPercussiveTempoConfidence);
    Ret.Append(&mRhythmFinalTempo);
    Ret.Append(&mRhythmFinalTempoConfidence);
    Ret.Append(&mSpectralRmsBands);
    Ret.Append(&mSpectralFlatnessBands);
    Ret.Append(&mSpectralFluxBands);
    Ret.Append(&mSpectralComplexityBands);
    Ret.Append(&mSpectralContrastBands);
    Ret.Append(&mSpectrumBands);
    Ret.Append(&mCepstrumBands);
  }

  // high level only
  else if (DescriptorSet == kHighLevelDescriptors)
  {
    Ret.Append(&mClassSignature);
    Ret.Append(&mClasses);
    Ret.Append(&mClassStrengths);
    Ret.Append(&mCategorySignature);
    Ret.Append(&mCategories);
    Ret.Append(&mCategoryStrengths);
    Ret.Append(&mHighLevelBaseNote);
    Ret.Append(&mHighLevelBaseNoteConfidence);
    Ret.Append(&mHighLevelPeakDb);
    Ret.Append(&mHighLevelRmsDb);
    Ret.Append(&mHighLevelBpm);
    Ret.Append(&mHighLevelBpmConfidence);
    Ret.Append(&mHighLevelBrightness);
    Ret.Append(&mHighLevelNoisiness);
    Ret.Append(&mHighLevelHarmonicity);
    Ret.Append(&mHighLevelSpectrumSignature);
    Ret.Append(&mHighLevelSpectrumFeatures);
    Ret.Append(&mHighLevelTristimulus);
    Ret.Append(&mHighLevelPitch);
    Ret.Append(&mHighLevelPitchConfidence);
    Ret.Append(&mHighLevelPeak);
    #if defined(MEnableDebugSampleDescriptors)
      Ret.Append(&mHighLevelDebugScalarValue);
      Ret.Append(&mHighLevelDebugVectorValue);
      Ret.Append(&mHighLevelDebugVectorVectorValue);
    #endif
  }
  else
  {
    MInvalid("Unknown descriptor set");
  }

  return Ret;
}

