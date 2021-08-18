#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/Exception.h"
#include "CoreTypes/Export/InlineMath.h"

#include "AudioTypes/Export/AudioMath.h"
#include "AudioTypes/Export/Envelopes.h"

#include "CoreFileFormats/Export/AudioFile.h"

#include "FeatureExtraction/Export/SampleAnalyser.h"
#include "FeatureExtraction/Export/SampleDescriptorPool.h"
#include "FeatureExtraction/Export/SampleClassificationDescriptors.h"
#include "FeatureExtraction/Export/Statistics.h"

#include "FeatureExtraction/Source/Autocorrelation.h"
#include "FeatureExtraction/Source/RhythmTracker.h"
#include "FeatureExtraction/Source/ClassificationTools.h"
#include "FeatureExtraction/Source/ClassificationHeuristics.h"

#include "Classification/Export/DefaultClassificationModel.h"

#include "../../3rdParty/Aubio/Export/Aubio.h"
#include "../../3rdParty/LibXtract/Export/LibXtract.h"
#include "../../3rdParty/Resample/Export/Resample.h"

#include <vector>
#include <algorithm>

// =================================================================================================

#define MLogPrefix "SampleAnalyzer"

// maximum analyzation time in ms - avoids spending too much CPU time 
// when analyzing very long samples. 
// NOTE: when changing this, make sure the TRhythmTracker algorithm still
// gets enough samples to do its sample duration -> loop length checks.
#define MAnalyzationDurationMaxInMs 1000*20

// analyzation range of most spectral features
#define MAnalyzationFreqMin 20.0
#define MAnalyzationFreqMax 15500.0

// relax time in seconds for the spectral whitening
#define MSpectralWhiteningDecay 22

// threshold factor for spectral peak detection
#define MPeakThreshold 0.25
// distance for harmonic peak detection
#define MHarmonicThreshold 0.5
// threshold for silence detection
#define MSilenceThresholdDb -48.0

// yinfft | yin | yinfast | mcomb | fcomb | schmitt | specacf
#define MPitchDetectionAlgorithm "yinfast"
// only yinfft, yin, yinfast and specacf have pitch confidence
#define MHavePitchConfidence
// algorithm dependent: confidence of a plain sine wave
#define MMaxPitchConfidenceValue 0.25
// only yinfft, yin and yinfast have a pitch tolerance setting
// NB: will behave differently for every MPitchDetectionAlgorithm, so test changes 
// via the 'MDebugTracePitch' define on the 'Descriptor-Pitch-Test' collection
#define MPitchTolerance 0.75
// pitch confidence values
#define MLowPitchConfidenceValue 0.2
#define MMediumPitchConfidenceValue 0.5
#define MHighPitchConfidenceValue 0.8

// envelope attack/release time
#define MEnvelopeTimeInMs 8.0

// when defined, "fix" class models with heuristics
#define MUseClassificationHeuristics

// -------------------------------------------------------------------------------------------------

// when defined, write src spectrum as pgm file next to audio file
// #define MWritePgmSpectrum

// when defined, log rhythmtracker and filename BPM differences 
// #define MDebugTraceBpms

// when defined, log pitch detection and filename pitch differences 
// #define MDebugTracePitch 1

#if defined(MWritePgmSpectrum)
  #include <shark/data/pgm.h>
#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

// Copy and isolate peaks in a spectrum.

static void SCreatePeakSpectrum(
  const TArray<double>& Spectrum,
  TArray<double>&       PeakSpectrum,
  int                   MagnitudeSize,
  double                RelativeThreshold)
{
  MAssert(RelativeThreshold >= 0.0 && RelativeThreshold <= 1.0, "");

  // calculate non absolute threshold
  const double Max = TStatistics::Max(Spectrum.FirstRead(), MagnitudeSize);
  const double Threshold = RelativeThreshold * Max;

  // find peaks
  const TList< TPair<int, double> > Peaks =
    TStatistics::Peaks(Spectrum.FirstRead(), MagnitudeSize, Threshold);

  // copy (second half frequencies) and clear magnitudes
  PeakSpectrum = Spectrum;
  for (int i = 0; i < MagnitudeSize; ++i)
  {
    PeakSpectrum[i] = 0.0;
  }

  // copy peaks
  for (int i = 0; i < Peaks.Size(); ++i)
  {
    PeakSpectrum[Peaks[i].First()] = Peaks[i].Second();
  }
}

// -------------------------------------------------------------------------------------------------

// Calculate flatness, dB scaled

static double SFlatnessDb(const double* pX, int Length)
{
  const double Flatness = TStatistics::Flatness(pX, Length);
  return MMin(TAudioMath::LinToDb(Flatness) / -60.0, 1.0); // limit to -60 dB
}

// -------------------------------------------------------------------------------------------------

// Simple cubic interpolation of a value

static double SInterpolateCubic(
  double ym1,
  double y0,
  double y1,
  double y2,
  double Pos)
{
  const double x = Pos - floor(Pos);
  const double xx = x * x;
  const double xxx = xx * x;

  const double a = -0.5 * xxx + xx - 0.5 * x;
  const double b = 1.5  * xxx - 2.5 * xx + 1.0;
  const double c = -1.5 * xxx + 2.0 * xx + 0.5 * x;
  const double d = 0.5  * xxx - 0.5 * xx;

  return (float)(a * ym1 + b * y0 + c * y1 + d * y2);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TSampleAnalyser::TSampleAnalyser(
  int SampleRate,
  int FftFrameSize,
  int HopFrameSize)
  : mSampleRate(SampleRate),
    mFftFrameSize(FftFrameSize),
    mHopFrameSize(HopFrameSize),
    mEnvelopeDetector(TEnvelopeDetector::kFast, MEnvelopeTimeInMs, SampleRate),
    mEnvelope(0.0)
{
  // analyzation bin area
  const double FrequenciesPerBin = mSampleRate / mFftFrameSize;
  
  mFirstAnalyzationBin = TMath::d2iRound(MAnalyzationFreqMin / FrequenciesPerBin);
  mLastAnalyzationBin = TMath::d2iRound(MAnalyzationFreqMax / FrequenciesPerBin);
  mAnalyzationBinCount = mLastAnalyzationBin - mFirstAnalyzationBin + 1;

  // create the window function
  mpWindow = ::xtract_init_window(mFftFrameSize, XTRACT_HANN);
  // as we have half of the energy in negative frequencies, we need to multiply 
  // the window by two. Otherwise a sinusoid at 0db will result in 0.5 in the spectrum.
  TAudioMath::ScaleBuffer(mpWindow, mFftFrameSize, 2.0);

  // Allocate Mel filters
  mpXtractMelFilters = TOwnerPtr<xtract_mel_filter>(new xtract_mel_filter_());
  mpXtractMelFilters->n_filters = kNumberOfCepstrumCoefficients;
  mpXtractMelFilters->filters = (double **)TMemory::Alloc(
    "#libXtract", kNumberOfCepstrumCoefficients * sizeof(double*));

  for (uint8_t k = 0; k < kNumberOfCepstrumCoefficients; ++k)
  {
    mpXtractMelFilters->filters[k] = (double*)TMemory::Alloc(
      "#libXtract", (mFftFrameSize / 2) * sizeof(double));
  }

  ::xtract_init_mfcc(mFftFrameSize / 2, mSampleRate / 2, XTRACT_EQUAL_GAIN,
    MAnalyzationFreqMin, MAnalyzationFreqMax,
    mpXtractMelFilters->n_filters, mpXtractMelFilters->filters);
}

// -------------------------------------------------------------------------------------------------

TSampleAnalyser::~TSampleAnalyser()
{
  for (int n = 0; n < kNumberOfCepstrumCoefficients; ++n)
  {
    TMemory::Free(mpXtractMelFilters->filters[n]);
  }

  TMemory::Free(mpXtractMelFilters->filters);

  mpXtractMelFilters.Delete();

  ::xtract_free_window(mpWindow);
}

// -------------------------------------------------------------------------------------------------

TList<TString> TSampleAnalyser::ClassificationClasses() const
{
  TList<TString> Ret;

  if (mpClassificationModel)
  {
    for (TString Category : mpClassificationModel->OutputClasses())
    {
      Ret.Append(Category);
    }
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::SetClassificationModel(const TString& ModelPath)
{
  try
  {
    if (!TFile(ModelPath).Exists())
    {
      throw TReadableException(MText("No such file: '%s'", ModelPath));
    }

    mpClassificationModel =
      TOwnerPtr<TClassificationModel>(new TDefaultBaggingClassificationModel());

    mpClassificationModel->Load(ModelPath);
  }
  catch (const std::exception& Exception)
  {
    throw TReadableException(
      MText("Error loading classification model: %s", Exception.what()));
  }
}

// -------------------------------------------------------------------------------------------------

TList<TString> TSampleAnalyser::OneShotCategorizationClasses() const
{
  TList<TString> Ret;

  if (mpOneShotCategorizationModel)
  {
    for (TString Category : mpOneShotCategorizationModel->OutputClasses())
    {
      Ret.Append(Category);
    }
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::SetOneShotCategorizationModel(const TString& ModelPath)
{
  try
  {
    if (!TFile(ModelPath).Exists())
    {
      throw TReadableException(MText("No such file: '%s'", ModelPath));
    }

    mpOneShotCategorizationModel =
      TOwnerPtr<TClassificationModel>(new TDefaultBaggingClassificationModel());

    mpOneShotCategorizationModel->Load(ModelPath);
  }
  catch (const std::exception& Exception)
  {
    throw TReadableException(
      MText("Error loading one-shot categorization model: %s", Exception.what()));
  }
}

// -------------------------------------------------------------------------------------------------

TSampleDescriptors TSampleAnalyser::Analyze(const TString& FileName)
{
  // recreate results
  mpResults = TOwnerPtr<TSampleDescriptors>(new TSampleDescriptors());

  // load sample data 
  TSampleData SampleData;
  LoadSample(FileName, SampleData);

  // analyze low level descriptors
  AnalyzeLowLevelDescriptors(SampleData);

  return *mpResults;
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::Extract(const TString& FileName, TSampleDescriptorPool* pPool)
{
  // recreate results
  mpResults = TOwnerPtr<TSampleDescriptors>(new TSampleDescriptors());

  // mark sample as failed without giving a reason, before starting to load and 
  // analyze it: when something crashes below, we won't try to access the file 
  // again when updating the database and thus avoid successive crashes...
  pPool->InsertFailedSample(FileName, "");

  TSampleData SampleData;


  // ... load sample data 

  try
  {
    LoadSample(FileName, SampleData);
  }
  catch (const std::exception& exception)
  {
    TLog::SLog()->AddLine(MLogPrefix, "Failed to load sample '%s' - '%s'",
      FileName.StdCString().c_str(), exception.what());

    pPool->InsertFailedSample(FileName,
      TString() + "Sample failed to load: " + exception.what());

    return;
  }


  // ... analyze and write descriptors

  try
  {
    AnalyzeLowLevelDescriptors(SampleData);

    if (pPool->DescriptorSet() == TSampleDescriptors::kHighLevelDescriptors)
    {
      AnalyzeHighLevelDescriptors(SampleData);
    }
  }
  catch (const std::exception& exception)
  {
    TLog::SLog()->AddLine(MLogPrefix, "Failed to analyse sample '%s' - '%s'",
      FileName.StdCString().c_str(), exception.what());

    pPool->InsertFailedSample(FileName,
      TString() + "Sample failed to analyse: " + exception.what());

    return;
  }


  // ... save results

  pPool->InsertSample(FileName, *mpResults);
}

// -------------------------------------------------------------------------------------------------

TList<double> TSampleAnalyser::AudibleSpectrumFrames(
  const TList<double>& SpectrumDescriptor)const
{
  MAssert(SpectrumDescriptor.Size() == mSpectrumFrameIsAudible.Size(),
    "Unexpected descriptor (size)");

  TList<double> Ret;
  Ret.PreallocateSpace(SpectrumDescriptor.Size());

  for (int i = 0; i < mSpectrumFrameIsAudible.Size(); ++i)
  {
    if (mSpectrumFrameIsAudible[i])
    {
      Ret.Append(SpectrumDescriptor[i]);
    }
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::LoadSample(
  const TString&  FileName,
  TSampleData&    SampleData)
{
  // ... Open Audio file
  
  TPtr<TAudioFile> pAudioFile; TString FileLoadError;
  if (! TAudioFile::SCreateFromFile(pAudioFile, FileName, FileLoadError))
  {
    // NB: the error will usually start with "Failed to load" or something like this, 
    // so we don't need to prefix something here...
    throw TReadableException(FileLoadError.IsEmpty() ?
      MText("Audio file failed to load: Unknown error") :
      FileLoadError);
  }


  // ... Assign "original" file properties

  SampleData.mOriginalFileName = FileName;
  SampleData.mOriginalFileSize = (int)TFile(FileName).SizeInBytes();
  SampleData.mOriginalNumberOfSamples = (int)pAudioFile->NumSamples();
  SampleData.mOriginalNumberOfChannels = pAudioFile->NumChannels();
  SampleData.mOriginalBitDepth = pAudioFile->BitsPerSample();
  SampleData.mOriginalSampleRate = pAudioFile->SamplingRate();


  // ... Load sample data

  if (pAudioFile->Stream()->NumChannels() < 1 ||
      pAudioFile->Stream()->NumChannels() > 8)
  {
    throw TReadableException("Unsupported audio file channel layout: "
      "Supporting mono, stereo, 3.0, 5.0, 5.1 and 7.1 audio files only.");
  }

  if (pAudioFile->Stream()->NumSamples() == 0)
  {
    throw TReadableException("Sample file is empty, probably failed to read.");
  }
    
  int NumberOfSampleFrames = (int)pAudioFile->Stream()->NumSamples();
  int NumberOfSampleChannels = pAudioFile->Stream()->NumChannels();

  TList< TArray<float> > TempSampleBuffers;
  TempSampleBuffers.PreallocateSpace(NumberOfSampleChannels);
  for (int c = 0; c < NumberOfSampleChannels; ++c)
  {
    TempSampleBuffers.Append(TArray<float>(NumberOfSampleFrames));
  }

  // read samples into TempSampleBuffers in blocks of the stream's prefered blocksize
  {
    const int BlockSize = pAudioFile->Stream()->PreferedBlockSize();

    int TotalSamplesRead = 0;
    while (TotalSamplesRead < NumberOfSampleFrames)
    {
      const int SamplesToReadInThisBlock = 
        MMin(BlockSize, NumberOfSampleFrames - TotalSamplesRead);

      TArray<float*> SampleBufferPtrs(NumberOfSampleChannels);
      for (int c = 0; c < NumberOfSampleChannels; ++c)
      {
        SampleBufferPtrs[c] = TempSampleBuffers[c].FirstWrite() + TotalSamplesRead;
      }

      try
      {
        pAudioFile->Stream()->ReadSamples(SampleBufferPtrs, SamplesToReadInThisBlock);
      }
      catch (const TReadableException& Exception)
      {
        // don't abort loading, but zero out blocks which failed to load
        TLog::SLog()->AddLine(MLogPrefix, "Decoder error at sample frame %d: %s",
          TotalSamplesRead, Exception.Message().StdCString().c_str());

        for (int c = 0; c < NumberOfSampleChannels; ++c)
        {
          TAudioMath::ClearBuffer(SampleBufferPtrs[c], SamplesToReadInThisBlock);
        }
      }

      TotalSamplesRead += SamplesToReadInThisBlock;
    }
  }


  // ... Mix down to mono (to ease and speed up following processing)

  static const float sScaleFactor = M16BitSampleRange / 2.0f;

  if (NumberOfSampleChannels > 1)
  {
    // use first channel as dest channel
    float* pDestMonoBuffer = TempSampleBuffers[0].FirstWrite();

    const float MixDownScaling = 1.0f / (float)NumberOfSampleChannels;
    for (int n = 0; n < NumberOfSampleFrames; ++n)
    {
      for (int c = 1; c < NumberOfSampleChannels; ++c)
      {
        pDestMonoBuffer[n] += TempSampleBuffers[c][n];
      }
      pDestMonoBuffer[n] *= MixDownScaling;
    }

    // all mono now
    NumberOfSampleChannels = 1;
    while (TempSampleBuffers.Size() > 1)
    {
      TempSampleBuffers.DeleteLast();
    }
  }

  TArray<float>& AnalyzationSampleBuffer = TempSampleBuffers[0];


  // ... Resample (when necessary)

  const double Speed = (double)pAudioFile->SamplingRate() / (double)mSampleRate;

  if (Speed != 1.0)
  {
    TLog::SLog()->AddLine(MLogPrefix, "Resampling from %d Hz to %d Hz...",
      (int)pAudioFile->SamplingRate(), (int)mSampleRate);

    const unsigned int OldSizeInSamples = NumberOfSampleFrames;

    const unsigned int NewSizeInSamples = (unsigned int)
      MMax(1, TMath::d2iRound(NumberOfSampleFrames / Speed));

    const float* pSrcSamples = AnalyzationSampleBuffer.FirstWrite();

    // create resampled dest sample buffer
    TArray<float> ResampledSampleBuffer;
    ResampledSampleBuffer.SetSize(NewSizeInSamples);

    // resample
    const int HighQuality = 1;
    void* pResampler = ::resample_open(HighQuality, 1.0 / Speed, 1.0 / Speed);

    const int LastFlag = 1;
    int SrcSamplesUsed = 0;

    const int DestSamplesWritten = ::resample_process(
      pResampler, 1.0 / Speed,
      (float*)pSrcSamples, OldSizeInSamples, LastFlag, &SrcSamplesUsed,
      ResampledSampleBuffer.FirstWrite(), NewSizeInSamples);

    MUnused(DestSamplesWritten);

    MAssert(SrcSamplesUsed == (int)OldSizeInSamples,
      "Expected all input samples to be used");
    MAssert(DestSamplesWritten == (int)NewSizeInSamples,
      "Unexpected dest size");

    ::resample_close(pResampler);

    // assign resampled dest
    AnalyzationSampleBuffer = std::move(ResampledSampleBuffer);

    // update sample count
    NumberOfSampleFrames = NewSizeInSamples;
  }


  // ... Calc RMS 

  double RmsValue = 0.0;
  for (int n = 0; n < NumberOfSampleFrames; ++n)
  {
    RmsValue += TMathT<float>::Square(AnalyzationSampleBuffer[n] / sScaleFactor);
  }

  SampleData.mRmsValue = (float)MMin(1.0,
    ::sqrt(RmsValue / (NumberOfSampleChannels * NumberOfSampleFrames)));


  // ... Calc peak and normalization factor

  float Min = AnalyzationSampleBuffer[0];
  float Max = AnalyzationSampleBuffer[0];

  TMathT<float>::GetMinMax(Min, Max,
    0, AnalyzationSampleBuffer.Size(), AnalyzationSampleBuffer.FirstRead());

  const double MaxAmplitude = (double)MMax(
    TMathT<float>::Abs(Min), TMathT<float>::Abs(Max));

  // assign normalized peak value
  SampleData.mPeakValue = (float)MMin(1.0, MaxAmplitude / sScaleFactor);

  const double Amplification = (MaxAmplitude > MEpsilon) ?
    sScaleFactor / MaxAmplitude : 1.0;

  if (Amplification > 1.1)
  {
    TLog::SLog()->AddLine(MLogPrefix,
      "Normalizing sample with factor: %g ...", Amplification);
  }


  // ... Check how many leading samples can be skipped

  static const double sSilenceFloor = sScaleFactor * 
    TAudioMath::DbToLin(MSilenceThresholdDb);

  int SilentLeadingSamples = 0;
  for (int f = 0; f < NumberOfSampleFrames; ++f, ++SilentLeadingSamples)
  {
    if (TMathT<double>::Abs(Amplification *
          AnalyzationSampleBuffer[f]) > sSilenceFloor)
    {
      break;
    }
  }

  int SilentTrailingSamples = 0;
  for (int f = NumberOfSampleFrames - 1; f > SilentLeadingSamples; --f, ++SilentTrailingSamples)
  {
    if (TMathT<double>::Abs(Amplification * 
          AnalyzationSampleBuffer[f]) > sSilenceFloor)
    {
      break;
    }
  }

  if (SilentLeadingSamples || SilentTrailingSamples)
  {
    TLog::SLog()->AddLine(MLogPrefix,
      "Skipping %d silent leading and %d trailing samples...", 
      SilentLeadingSamples, SilentTrailingSamples);
  }


  // ... Finalize: convert to double, pad and scale

  const int AudibleNumberOfSampleFrames = NumberOfSampleFrames - 
    SilentLeadingSamples - SilentTrailingSamples;

  // make sure we analyze at least half of the last frame
  int EndFrameOffset = 0;
  if ((AudibleNumberOfSampleFrames % mFftFrameSize) < mFftFrameSize / 2)
  {
    EndFrameOffset += mFftFrameSize / 2;
  }

  // and ensure we analyze at least one full frame
  int StartFrameOffset = 0;
  if (AudibleNumberOfSampleFrames + EndFrameOffset < mFftFrameSize)
  {
    StartFrameOffset = mFftFrameSize - AudibleNumberOfSampleFrames - EndFrameOffset;
  }
  
  SampleData.mData.SetSize(AudibleNumberOfSampleFrames + 
    StartFrameOffset + EndFrameOffset);
  
  SampleData.mDataOffset = -SilentLeadingSamples + StartFrameOffset;

  // clear padded start and end
  double* pSampleBuffer = SampleData.mData.FirstWrite();

  TMemory::Zero(pSampleBuffer,
    sizeof(double) * StartFrameOffset);
  TMemory::Zero(pSampleBuffer + SampleData.mData.Size() - EndFrameOffset,
    sizeof(double) * EndFrameOffset);

  // fill in the rest with the dest sample data and apply "Amplification"
  const double FinalScaling = (double)Amplification / sScaleFactor;

  for (int n = 0; n < NumberOfSampleFrames - SilentLeadingSamples - SilentTrailingSamples; ++n)
  {
    pSampleBuffer[n + StartFrameOffset] =
      AnalyzationSampleBuffer[n + SilentLeadingSamples] * FinalScaling;
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::AnalyzeLowLevelDescriptors(const TSampleData& SampleData)
{
  MAssert(SampleData.mData.Size() >= mHopFrameSize,
    "Data should be valid and padded here");


  // ... Basic file properties

  mpResults->mFileName = SampleData.mOriginalFileName;

  mpResults->mFileType.mValue = gExtractFileExtension(
    SampleData.mOriginalFileName).ToLower();

  mpResults->mFileSize.mValue = SampleData.mOriginalFileSize;

  mpResults->mFileLength.mValue = TAudioMath::SamplesToMs(
    SampleData.mOriginalSampleRate, SampleData.mOriginalNumberOfSamples) / 1000.0;

  mpResults->mFileSampleRate.mValue = SampleData.mOriginalSampleRate;
  mpResults->mFileBitDepth.mValue = SampleData.mOriginalBitDepth;
  mpResults->mFileChannelCount.mValue = SampleData.mOriginalNumberOfChannels;

  mpResults->mAnalyzationOffset.mValue = TAudioMath::SamplesToMs(
    mSampleRate, SampleData.mDataOffset) / 1000.0;


  // ... Effective length

  CalcEffectiveLength(SampleData.mData.FirstRead(), SampleData.mData.Size());


  // ... Spectral features

  // clamp to analyze sample length
  const int MaxAnalyzationTimeInSamples = TAudioMath::MsToSamples(mSampleRate,
    MAnalyzationDurationMaxInMs);

  const int SampleDataAnalyzationLength = MMin(MaxAnalyzationTimeInSamples,
    SampleData.mData.Size());

  #if defined(MWritePgmSpectrum)
    std::vector<double> PmgSpectrum;
  #endif

  TArray<double> WindowedSampleFrame(mFftFrameSize);
  WindowedSampleFrame.Init(0.0);

  TArray<double> MagnitudeSpectrum(mFftFrameSize);
  MagnitudeSpectrum.Init(0.0);
  TArray<double> LastMagnitudeSpectrum(mFftFrameSize);
  LastMagnitudeSpectrum.Init(0.0);

  TArray<double> WhitenedSpectrum(mFftFrameSize);
  WhitenedSpectrum.Init(0.0);

  TArray<double> PeakSpectrum(mFftFrameSize);
  PeakSpectrum.Init(0.0);

  TArray<double> HarmonicSpectrum(mFftFrameSize);
  HarmonicSpectrum.Init(0.0);

  // reset envelope
  mEnvelope = 0.0;

  // reset silence state
  mSpectrumFrameIsAudible.ClearEntries();
  mSpectrumFrameIsAudible.PreallocateSpace(
    SampleDataAnalyzationLength / mHopFrameSize);

  // init fft transform
  TFftTransformComplex FftTransform;
  const bool HighQuality = true;
  FftTransform.Initialize(mFftFrameSize, HighQuality, 
    TFftTransformComplex::kDivFwdByN);

  // init Aubio pitch tracker
  aubio_pitch_t* pAubioPitchTracker = ::new_aubio_pitch(MPitchDetectionAlgorithm,
    mFftFrameSize, mHopFrameSize, mSampleRate);
  ::aubio_pitch_set_tolerance(pAubioPitchTracker, MPitchTolerance);
  ::aubio_pitch_set_silence(pAubioPitchTracker, MSilenceThresholdDb);
  ::aubio_pitch_set_unit(pAubioPitchTracker, "freq");

  // init Aubio spectral whitening
  aubio_spectral_whitening_t* pAubioSpectralWhitening =
    ::new_aubio_spectral_whitening(mFftFrameSize, mHopFrameSize, mSampleRate);
  ::aubio_spectral_whitening_set_relax_time(
    pAubioSpectralWhitening, MSpectralWhiteningDecay);

  // ignore FPU exceptions from aubio and libXtract
  M__DisableFloatingPointAssertions

  for (int n = 0; (n + mFftFrameSize - 1) < SampleDataAnalyzationLength; n += mHopFrameSize)
  {
    // fvec_t input for aubio 
    fvec_t SampleInputHopSize;
    SampleInputHopSize.length = mHopFrameSize;
    SampleInputHopSize.data = SampleData.mData.FirstWrite() + n;

    fvec_t SampleInputFrameSize;
    SampleInputFrameSize.length = mFftFrameSize;
    SampleInputFrameSize.data = SampleData.mData.FirstWrite() + n;

    // Apply window to sample frame
    ::xtract_windowed(SampleData.mData.FirstRead() + n,
      mFftFrameSize, mpWindow, WindowedSampleFrame.FirstWrite());

    // Apply FFT on windowed input
    {
      TAudioMath::CopyBuffer(WindowedSampleFrame.FirstRead(),
        FftTransform.Re(), mFftFrameSize);
      TAudioMath::ClearBuffer(FftTransform.Im(), mFftFrameSize);

      FftTransform.Forward();

      TAudioMath::Magnitude(FftTransform.Re(), FftTransform.Im(), 
        MagnitudeSpectrum.FirstWrite(), mFftFrameSize / 2);

      #if 0 // phase is currently not used: avoid wasting processing time
        TAudioMath::Phase(FftTransform.Re(), FftTransform.Im(),
          MagnitudeSpectrum.FirstWrite() + mFftFrameSize / 2, mFftFrameSize / 2);
      #else
        TAudioMath::ClearBuffer(
          MagnitudeSpectrum.FirstWrite() + mFftFrameSize / 2, mFftFrameSize / 2);
      #endif
    }

    // Whitened spectrum 
    {
      cvec_t WhitenedFftgrain;
      WhitenedFftgrain.length = mFftFrameSize / 2;
      WhitenedFftgrain.norm = WhitenedSpectrum.FirstWrite();
      WhitenedFftgrain.phas = NULL; // unused

      WhitenedSpectrum = MagnitudeSpectrum;
      ::aubio_spectral_whitening_do(pAubioSpectralWhitening, &WhitenedFftgrain);
    }

    // Peak spectrum (from whitened spectrum)
    SCreatePeakSpectrum(WhitenedSpectrum,
      PeakSpectrum, mFftFrameSize / 2, MPeakThreshold);

    // Silence detection
    const bool IsSilentFrame =
      (::aubio_silence_detection(&SampleInputHopSize, MSilenceThresholdDb) == 1);
    mSpectrumFrameIsAudible.Append(!IsSilentFrame);
    mpResults->mAmplitudeSilence.mValues.Append(IsSilentFrame ? 1.0 : 0.0);

    // Amplitude Peak and RMS
    CalcAmplitudePeak(SampleInputHopSize.data, SampleInputHopSize.length);
    CalcAmplitudeRms(SampleInputHopSize.data, SampleInputHopSize.length);
    CalcAmplitudeEnvelope(SampleInputHopSize.data, SampleInputHopSize.length);

    // F0 (fundamental frequency)
    double F0 = 0.0;
    double F0Confidence = 0.0;
    double F0FailSafe = 0.0;
    {
      fvec_t PitchOut;
      PitchOut.length = 1;
      PitchOut.data = &F0;

      ::aubio_pitch_do(pAubioPitchTracker, &SampleInputFrameSize, &PitchOut);

      #if defined(MHavePitchConfidence)
        F0Confidence = MClip(
          ::aubio_pitch_get_confidence(pAubioPitchTracker) / MMaxPitchConfidenceValue, 
          0.0, 1.0);
      #else
        F0Confidence = 1.0;
      #endif

      mpResults->mF0.mValues.Append(F0);
      mpResults->mF0Confidence.mValues.Append(F0Confidence);

      if (F0 > 0.0 && F0Confidence > MLowPitchConfidenceValue)
      {
        // low confident F0 by default
        F0FailSafe = F0;
      }
      else
      {
        // spectral centroid as fallback - for audible frames
        if (!IsSilentFrame)
        {
          const double CentroidBin = TStatistics::Centroid(
            MagnitudeSpectrum.FirstRead(), mFftFrameSize / 2);

          const double CentroidFrequency =
            (double)mSampleRate / (double)mFftFrameSize * MMax(CentroidBin, 0.0);

          F0FailSafe = CentroidFrequency;
        }
      }
      mpResults->mFailSafeF0.mValues.Append(F0FailSafe);
    }

    // Harmonic spectrum  (from whitened peak spectrum & F0)
    {
      double Arguments[4] = { 0 };
      Arguments[0] = F0FailSafe;
      Arguments[1] = MHarmonicThreshold; // harmonic threshold
      const int XtractResult = ::xtract_harmonic_spectrum(PeakSpectrum.FirstRead(),
        mFftFrameSize, Arguments, HarmonicSpectrum.FirstWrite());
      MAssert(XtractResult == XTRACT_SUCCESS, ""); MUnused(XtractResult);
    }

    #if defined(MWritePgmSpectrum)
      for (int b = 0; b < mFftFrameSize / 2; ++b)
      {
        PmgSpectrum.push_back(HarmonicSpectrum[b]);
      }
    #endif

    // initialize memorized spectrum states on first frame
    if (n == 0)
    {
      LastMagnitudeSpectrum = MagnitudeSpectrum;
    }

    // Autocorrelation
    const int RemainingSamples = SampleData.mData.Size() - n;
    CalcAutoCorrelation(SampleData.mData.FirstRead() + n, RemainingSamples);

    // Spectral RMS
    CalcSpectralRms(MagnitudeSpectrum);
    // Spectral Centroid & Spread
    CalcSpectralCentroidAndSpread(MagnitudeSpectrum);
    // Spectral Skewness and Kurtosis
    CalcSpectralSkewnessAndKurtosis(MagnitudeSpectrum);
    // Spectral Rolloff
    CalcSpectralRolloff(MagnitudeSpectrum);
    // Spectral Flatness
    CalcSpectralFlatness(MagnitudeSpectrum);
    // Spectral Flux
    CalcSpectralFlux(MagnitudeSpectrum, LastMagnitudeSpectrum);

    // Spectral Complexity
    CalcSpectralComplexity(PeakSpectrum); // Yup, peaks
    // Spectral Inharmonicity
    CalcSpectralInharmonicity(PeakSpectrum, F0FailSafe, F0Confidence); // Yup, peaks
    // Tristimulus
    CalcTristimulus(HarmonicSpectrum, F0FailSafe, F0Confidence); // Yup, harmonics

    // Spectral RMS, flux and contrast band features
    CalcSpectralBandFeatures(MagnitudeSpectrum, LastMagnitudeSpectrum);

    // Spectrum Bands
    CalcSpectrumBands(MagnitudeSpectrum);
    // Cepstrum Bands
    CalcCepstrumBands(MagnitudeSpectrum);

    // memorize last spectrum
    LastMagnitudeSpectrum = MagnitudeSpectrum;
  }

  // cleanup
  ::del_aubio_pitch(pAubioPitchTracker);
  ::del_aubio_spectral_whitening(pAubioSpectralWhitening);


  // ... Rhythm features (with smaller FFT and hop sizes)

  const int TempoFftSize = 512;
  const int TempoHopSize = 128;

  // init rhythm tracker
  TRhythmTracker RhythmTracker(mSampleRate, TempoFftSize, TempoHopSize);

  for (int n = 0; (n + TempoFftSize - 1) < SampleDataAnalyzationLength; n += TempoHopSize)
  {
    fvec_t SampleInput;
    SampleInput.length = TempoFftSize;
    SampleInput.data = SampleData.mData.FirstWrite() + n;

    RhythmTracker.ProcessFrame(&SampleInput);
  }

  // add ryhthm stats
  const double SampleDurationInSeconds = TAudioMath::SamplesToMs(
    SampleData.mOriginalSampleRate, SampleData.mOriginalNumberOfSamples) / 1000;
  const double OnsetOffsetInSeconds = TAudioMath::SamplesToMs(
    SampleData.mOriginalSampleRate, SampleData.mDataOffset) / 1000;

  mpResults->mRhythmComplexOnsets.mValues = RhythmTracker.Onsets(TRhythmTracker::kComplex);
  mpResults->mRhythmComplexOnsetCount.mValue = RhythmTracker.OnsetCount(TRhythmTracker::kComplex);
  mpResults->mRhythmComplexTempo.mValue = RhythmTracker.CalculateTempo(
    mpResults->mRhythmComplexTempoConfidence.mValue, TRhythmTracker::kComplex);
  mpResults->mRhythmComplexOnsetFrequencyMean.mValue = 
    RhythmTracker.CalculateRhythmFrequencyMean(TRhythmTracker::kComplex);
  mpResults->mRhythmComplexOnsetStrength.mValue = 
    RhythmTracker.CalculateRhythmStrength(TRhythmTracker::kComplex);
  mpResults->mRhythmComplexOnsetContrast.mValue = 
    RhythmTracker.CalculateRhythmContrast(TRhythmTracker::kComplex);

  mpResults->mRhythmPercussiveOnsets.mValues = RhythmTracker.Onsets(TRhythmTracker::kPercussive);
  mpResults->mRhythmPercussiveOnsetCount.mValue = RhythmTracker.OnsetCount(TRhythmTracker::kPercussive);
  mpResults->mRhythmPercussiveTempo.mValue = RhythmTracker.CalculateTempo(
    mpResults->mRhythmPercussiveTempoConfidence.mValue, TRhythmTracker::kPercussive);
  mpResults->mRhythmPercussiveOnsetFrequencyMean.mValue = 
    RhythmTracker.CalculateRhythmFrequencyMean(TRhythmTracker::kPercussive);
  mpResults->mRhythmPercussiveOnsetStrength.mValue = 
    RhythmTracker.CalculateRhythmStrength(TRhythmTracker::kPercussive);
  mpResults->mRhythmPercussiveOnsetContrast.mValue = 
    RhythmTracker.CalculateRhythmContrast(TRhythmTracker::kPercussive);

  // calculate "final" tempo from the one which seems more confident
  if (mpResults->mRhythmPercussiveTempoConfidence.mValue > mpResults->mRhythmComplexTempoConfidence.mValue) 
  {
    mpResults->mRhythmFinalTempo.mValue = RhythmTracker.CalculateTempoWithHeuristics(
      mpResults->mRhythmFinalTempoConfidence.mValue, // out
      mpResults->mRhythmPercussiveTempo.mValue, // in
      mpResults->mRhythmPercussiveTempoConfidence.mValue, // in
      SampleDurationInSeconds, 
      OnsetOffsetInSeconds,
      TRhythmTracker::kPercussive);
  }
  else 
  {
    mpResults->mRhythmFinalTempo.mValue = RhythmTracker.CalculateTempoWithHeuristics(
      mpResults->mRhythmFinalTempoConfidence.mValue, // out
      mpResults->mRhythmComplexTempo.mValue, // in
      mpResults->mRhythmComplexTempoConfidence.mValue, // in
      SampleDurationInSeconds, 
      OnsetOffsetInSeconds,
      TRhythmTracker::kComplex);
  }


  // ... Spectrum debug dumps
  
  // save debug pmg file
  #if defined(MWritePgmSpectrum)
    const TString PmgFile(gCutExtension(SampleData.mOriginalFileName) + "_Spectrum.pgm");

    shark::exportPGM(PmgFile.StdCString(TString::kFileSystemEncoding), PmgSpectrum,
      mFftFrameSize / 2, PmgSpectrum.size() / (mFftFrameSize / 2),
      true);
  #endif


  // ... calc statistics

  CalcStatistics();
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::AnalyzeHighLevelDescriptors(const TSampleData& SampleData)
{
  // extract classification features
  const TSampleClassificationDescriptors ModelDescriptors(*mpResults);


  // ... Classes

  if (mpClassificationModel)
  {
    // create single test item 
    const TClassificationTestDataItem ClassificationTestItem(
      ModelDescriptors,
      *mpClassificationModel->Normalizer(),
      mpClassificationModel->OutlierLimits());

    // . Evaluate model
    
    TList<float> ClassificationWeights =
      mpClassificationModel->Evaluate(ClassificationTestItem);

    
    // . ClassSignature

    mpResults->mClassSignature.mValues = ClassificationWeights;


    // . Class & ClassStrengths

    const double MinWeight = 0.0; // include all
    mpResults->mClassStrengths.mValues = TClassificationTools::CategoryStrengths(
      mpResults->mClassSignature.mValues, MinWeight);


    // . Apply Heuristics

    MAssert(mpClassificationModel->OutputClasses() == std::vector<TString>({"Loop", "OneShot"}), 
      "Expecting a OneShotVsLoop model as 'mpClassificationModel' here");

    #if defined(MUseClassificationHeuristics)
      if (mpClassificationModel->OutputClasses() == std::vector<TString>({"Loop", "OneShot"})) 
      {
        static const int kLoopClassIndex = 0;
        static const int kOneShotClassIndex = 1;

        double IsOneShotConfidence = -1.0;
        double IsLoopConfidence = -1.0;

        if (TClassificationHeuristics::IsOneShot(*mpResults, IsOneShotConfidence)) 
        {
          if (mpResults->mClassStrengths.mValues[kLoopClassIndex] > 
                mpResults->mClassStrengths.mValues[kOneShotClassIndex]) 
          {
            TLog::SLog()->AddLine(MLogPrefix, 
              "NB: Overriding model's 'IsOneShot' result with heuristics (confidence: %g)",
              IsOneShotConfidence);

            mpResults->mClassStrengths.mValues[kLoopClassIndex] = MMin(IsOneShotConfidence / 2,
              mpResults->mClassStrengths.mValues[kLoopClassIndex]);
            mpResults->mClassStrengths.mValues[kOneShotClassIndex] = IsOneShotConfidence;
          }
        }
        else if (TClassificationHeuristics::IsLoop(*mpResults, IsLoopConfidence)) 
        {
          if (mpResults->mClassStrengths.mValues[kLoopClassIndex] < 
                mpResults->mClassStrengths.mValues[kOneShotClassIndex]) 
          {
            TLog::SLog()->AddLine(MLogPrefix, 
              "NB: Overriding model's 'IsLoop' result with heuristics (confidence: %g)",
              IsLoopConfidence);

            mpResults->mClassStrengths.mValues[kLoopClassIndex] = IsLoopConfidence;
            mpResults->mClassStrengths.mValues[kOneShotClassIndex] = MMin(IsLoopConfidence / 2, 
              mpResults->mClassStrengths.mValues[kOneShotClassIndex]);
          }
        }
      }
    #endif


    // . Class List

    // collect class list from class strength
    const double MinDefaultWeight = 0.2;
    const double MinFallbackWeight = 0.01;
    mpResults->mClasses.mValues = TClassificationTools::PickAllStrongCategories(
      mpClassificationModel->OutputClasses(),
      mpResults->mClassStrengths.mValues,
      MinDefaultWeight,
      MinFallbackWeight);
  }
  else // ! mpClassificationModel
  {
    mpResults->mClassSignature.mValues.Empty();
    mpResults->mClasses.mValues = TList<TString>();
    mpResults->mClassStrengths.mValues.Empty();
  }


  // ... One-Shot Categories

  // NB: also for loops - we're using the categories for general similarity checks

  if (mpOneShotCategorizationModel)
  {
    // . CategoryWeights

    // create single test item 
    const TClassificationTestDataItem CategorizationTestItem(
      ModelDescriptors,
      *mpOneShotCategorizationModel->Normalizer(),
      mpOneShotCategorizationModel->OutlierLimits());

    // evaluate item with the model
    const TList<float> CategoryWeights =
      mpOneShotCategorizationModel->Evaluate(CategorizationTestItem);

    mpResults->mCategorySignature.mValues = CategoryWeights;

    // . Categories & CategoryStrengths (for OneShots only)

    if (!mpResults->mClasses.mValues.IsEmpty() &&
        !mpResults->mClasses.mValues.Contains("OneShot") &&
        !mpResults->mClasses.mValues.Contains("Loop"))
    {
      throw TReadableException("Unexpected classification model class: "
        "expecting 'OneShot' or 'Loop' for now...");
    }

    if (mpResults->mClasses.mValues.IsEmpty() ||
        mpResults->mClasses.mValues.Contains("OneShot"))
    {
      // calculate relative strength from absolute weights
      const double MinWeight = 0.0; // include all
      mpResults->mCategoryStrengths.mValues = TClassificationTools::CategoryStrengths(
        mpResults->mCategorySignature.mValues, MinWeight);
      
      // collect category list from relative strength
      const double MinDefaultWeight = 0.2;
      const double MinFallbackWeight = 0.01;
      mpResults->mCategories.mValues = TClassificationTools::PickAllStrongCategories(
        mpOneShotCategorizationModel->OutputClasses(),
        mpResults->mCategoryStrengths.mValues,
        MinDefaultWeight,
        MinFallbackWeight);
    }
    else // mpResults->mClass.mValue == "Loop"
    {
      // NB: keep mCategorySignature - it's used by the general similarity aspect
      mpResults->mCategoryStrengths.mValues.Init(0.0);
      mpResults->mCategories.mValues = TList<TString>();
    }
  }
  else // ! mpOneShotCategorizationModel
  {
    mpResults->mCategorySignature.mValues.Empty();
    mpResults->mCategoryStrengths.mValues.Empty();
    mpResults->mCategories.mValues = TList<TString>();
  }


  // ... BaseNote 

  auto IsHighConfidentPitch = [=] (double PitchHzValue, double PitchConfidence) {
    // sort out 0 and non confident and bogus ones
    return PitchConfidence > MHighPitchConfidenceValue &&
      PitchHzValue > 20 &&
      PitchHzValue < mSampleRate / 4;
  };

  auto IsMediumConfidentPitch = [=] (double PitchHzValue, double PitchConfidence) {
    return PitchConfidence > MMediumPitchConfidenceValue &&
      PitchHzValue > 20 &&
      PitchHzValue < mSampleRate / 4;
  };

  auto IsLowConfidentPitch = [=] (double PitchHzValue, double PitchConfidence) {
    return PitchConfidence > MLowPitchConfidenceValue &&
      PitchHzValue > 20 &&
      PitchHzValue < mSampleRate / 4;
  };

  // decide weather to use high, mid or low confident pitch as base
  const TList<double> AudiblePitchConfidence =
    AudibleSpectrumFrames(mpResults->mF0Confidence.mValues);

  const double AudiblePitchConfidenceMean = AudiblePitchConfidence.IsEmpty() ? 0.0 :
    TStatistics::Mean(AudiblePitchConfidence.FirstRead(), AudiblePitchConfidence.Size());

  std::function<bool(double, double)> IsConfidentPitch;
  if (AudiblePitchConfidenceMean >= MHighPitchConfidenceValue)
  {
    // there mostly is highly confident pitch present: only use this one to reduce in statistics
    IsConfidentPitch = IsHighConfidentPitch;
  }
  else if (AudiblePitchConfidenceMean >= MMediumPitchConfidenceValue)
  {
    // mixed content: use average confident pitch
    IsConfidentPitch = IsMediumConfidentPitch;
  }
  else 
  {
    // use low confident pitch as fallback
    IsConfidentPitch = IsLowConfidentPitch;
  }

  mpResults->mHighLevelBaseNote.mValue = -1.0;
  
  // calc "confident" pitch from f0
  TList<double> ConfidentPitch;
  ConfidentPitch.PreallocateSpace(mpResults->mF0.mValues.Size());

  for (int i = 0; i < mpResults->mF0.mValues.Size(); ++i)
  {
    if (IsConfidentPitch(mpResults->mF0.mValues[i],
          mpResults->mF0Confidence.mValues[i]))
    {
      ConfidentPitch.Append(mpResults->mF0.mValues[i]);
    }
  }

  if (!ConfidentPitch.IsEmpty())
  {
    const double Hz = TStatistics::Median(
      ConfidentPitch.FirstRead(), ConfidentPitch.Size());

    if (Hz > 20 && Hz < mSampleRate / 4)
    {
      mpResults->mHighLevelBaseNote.mValue = ::aubio_freqtomidi(Hz);
    }
  }

  // base note confidence
  if (mpResults->mHighLevelBaseNote.mValue > 0.0)
  {
    // add base note variance as penalty factor: so the more steady, the more 
    // confident is the detected base note the only fundamental pitch in the sound
    TList<double> BaseNoteOffsets;
    BaseNoteOffsets.PreallocateSpace(ConfidentPitch.Size());
    for (int i = 0; i < ConfidentPitch.Size(); ++i)
    {
      const double BaseNoteOffset = TMathT<double>::Abs(
        mpResults->mHighLevelBaseNote.mValue - ::aubio_freqtomidi(ConfidentPitch[i]));
      BaseNoteOffsets.Append(BaseNoteOffset);
    }

    const double BaseNoteStdDev = TStatistics::StandardDeviation(
      BaseNoteOffsets.FirstRead(), BaseNoteOffsets.Size());

    const double BaseNoteVariancePenalty = 1.0 - MMin(1.0, BaseNoteStdDev / 6.0);

    // use the audible pitch confident mean as base
    mpResults->mHighLevelBaseNoteConfidence.mValue =
      AudiblePitchConfidenceMean * BaseNoteVariancePenalty;
  }
  else
  {
    mpResults->mHighLevelBaseNoteConfidence.mValue = 0.0;
  }


  // ... Loudness

  mpResults->mHighLevelPeakDb.mValue =
    TAudioMath::LinToDb(SampleData.mPeakValue);
  mpResults->mHighLevelRmsDb.mValue =
    TAudioMath::LinToDb(SampleData.mRmsValue);


  // ... BPM 

  // use combined beat tracker & heuristics results
  mpResults->mHighLevelBpm.mValue = mpResults->mRhythmFinalTempo.mValue;
  mpResults->mHighLevelBpmConfidence.mValue = mpResults->mRhythmFinalTempoConfidence.mValue;

  // make pretty
  TMath::Quantize(mpResults->mHighLevelBpm.mValue, 0.5, TMath::kRoundToNearest);


  // ... Characteristics

  // . Brightness

  const TList<double> SpectralRolloff =
    AudibleSpectrumFrames(mpResults->mSpectralRolloff.mValues);
  const TList<double> SpectralCentroid =
    AudibleSpectrumFrames(mpResults->mSpectralCentroid.mValues);

  if (SpectralRolloff.Size() && SpectralCentroid.Size())
  {
    const double SpectralRolloffMean =
      TStatistics::Mean(SpectralRolloff.FirstRead(), SpectralRolloff.Size());
    const double SpectralCentroidMax =
      TStatistics::Max(SpectralCentroid.FirstRead(), SpectralCentroid.Size());

    double WeightedCentroid =
      (::aubio_freqtomidi(SpectralRolloffMean) / 128.0 * 0.7 +
        ::aubio_freqtomidi(SpectralCentroidMax) / 128.0 * 0.3);

    WeightedCentroid = MMax(0.0, MMin(1.0, WeightedCentroid));
    WeightedCentroid = ::pow(WeightedCentroid, 4.0);

    mpResults->mHighLevelBrightness.mValue = WeightedCentroid;
    MAssert(mpResults->mHighLevelBrightness.mValue >= 0 &&
      mpResults->mHighLevelBrightness.mValue <= 1.0, "");
  }
  else // silence
  {
    mpResults->mHighLevelBrightness.mValue = 0.0;
  }

  // . Noisiness

  const TList<double> SpectralFlatness =
    AudibleSpectrumFrames(mpResults->mSpectralFlatness.mValues);

  if (SpectralFlatness.Size())
  {
    const double SpectralFlatnessMin =
      TStatistics::Min(SpectralFlatness.FirstRead(), SpectralFlatness.Size());
    const double SpectralFlatnessMax =
      TStatistics::Max(SpectralFlatness.FirstRead(), SpectralFlatness.Size());
    const double SpectralFlatnessMean =
      TStatistics::Mean(SpectralFlatness.FirstRead(), SpectralFlatness.Size());

    double WeightedFlatness =
      ((1.0 - SpectralFlatnessMin) * 0.2 +
        (1.0 - SpectralFlatnessMean) * 0.6 +
        (1.0 - SpectralFlatnessMax) * 0.2);

    WeightedFlatness = MMax(0.0, MMin(1.0, WeightedFlatness));
    WeightedFlatness = ::pow(WeightedFlatness, 2.0);

    mpResults->mHighLevelNoisiness.mValue = WeightedFlatness;
    MAssert(mpResults->mHighLevelNoisiness.mValue >= 0 &&
      mpResults->mHighLevelNoisiness.mValue <= 1.0, "");
  }
  else // silence
  {
    mpResults->mHighLevelNoisiness.mValue = 0.0;
  }


  // . Harmonicity

  const TList<double> AutoCorrelation =
    AudibleSpectrumFrames(mpResults->mAutoCorrelation.mValues);

  if (AutoCorrelation.Size())
  {
    const double AutoCorrelationMean =
      TStatistics::Mean(AutoCorrelation.FirstRead(), AutoCorrelation.Size());

    const double SpectralFlatnessMean =
      TStatistics::Mean(SpectralFlatness.FirstRead(), SpectralFlatness.Size());

    double WeightedHarmonicity =
      (MMin(1.0, 1.5 * AutoCorrelationMean) * 0.4 +
        MMin(1.0, 2.0 * AudiblePitchConfidenceMean) * 0.3 +
        SpectralFlatnessMean * 0.3);

    WeightedHarmonicity = MMax(0.0, MMin(1.0, WeightedHarmonicity));
    WeightedHarmonicity = ::pow(WeightedHarmonicity, 2.0);

    mpResults->mHighLevelHarmonicity.mValue = WeightedHarmonicity;
    MAssert(mpResults->mHighLevelHarmonicity.mValue >= 0 &&
      mpResults->mHighLevelHarmonicity.mValue <= 1.0, "");
  }
  else // silcence
  {
    mpResults->mHighLevelHarmonicity.mValue = 0.0;
  }

  
  // ... Spectrum bands (Spectral Signature)

  // small set of merged frequency bands (up to 15.5 kHz)
  static const int sSpectrumBands[kNumberOfHighLevelSpectrumBands] = {
    // 50, 100, 200, 400, 630, 920, 1270, 1720, 2320, 3150, 4400, 6400.0, 9500, 15500
    0, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25
  };

  const TSampleDescriptors::TFramedVectorData<kNumberOfSpectrumBands>&
    SpectrumBands = mpResults->mSpectrumBands;

  TList< TStaticArray<double, kNumberOfHighLevelSpectrumBands> > ScaledSpectrumBands;

  const int NumberOfFrames = SpectrumBands.mValues.Size();
  for (int f = 0; f < NumberOfFrames; ++f)
  {
    TStaticArray<double, kNumberOfHighLevelSpectrumBands> BandValues;

    for (int b = 0; b < kNumberOfHighLevelSpectrumBands; ++b)
    {
      // start from 0 or continue behind last band's band
      const int SrcStartBand = (b - 1) >= 0 ? sSpectrumBands[b - 1] + 1 : 0;
      const int SrcEndBand = sSpectrumBands[b];
      const int SrcBandRange = SrcEndBand - SrcStartBand + 1;

      double MergedBandValue = 0.0;
      for (int sb = SrcStartBand; sb <= SrcEndBand; ++sb)
      {
        MergedBandValue += SpectrumBands.mValues[f][sb];
      }
      MergedBandValue /= (double)SrcBandRange;

      MAssert(MergedBandValue >= 0.0 && MergedBandValue <= 1.0,
        "Unexpected spectrum value range");

      // normalize (with a bit overscaling), then compress
      const double FinalBandValue = ::pow(MergedBandValue * 1.25, 1.0 / 6.0);
      BandValues[b] = FinalBandValue;
    }

    ScaledSpectrumBands.Append(BandValues);
  }

  // resample spectrum
  const int OldLength = ScaledSpectrumBands.Size();
  const double Step = (double)OldLength / kNumberOfHighLevelSpectrumBandFrames;

  mpResults->mHighLevelSpectrumSignature.mValues.ClearEntries();

  double CurrentPos = 0;
  for (int i = 0; i < kNumberOfHighLevelSpectrumBandFrames; ++i)
  {
    TList<double> ResampledBandValues;
    ResampledBandValues.PreallocateSpace(kNumberOfHighLevelSpectrumBands);

    const int ipos = TMath::d2i(CurrentPos);
    const int im1 = MMax(0, ipos - 1);
    const int i0 = ipos;
    const int i1 = MMin(OldLength - 1, ipos + 1);
    const int i2 = MMin(OldLength - 1, ipos + 2);

    for (int j = 0; j < kNumberOfHighLevelSpectrumBands; ++j)
    {
      ResampledBandValues.Append(SInterpolateCubic(
        ScaledSpectrumBands[im1][j],
        ScaledSpectrumBands[i0][j],
        ScaledSpectrumBands[i1][j],
        ScaledSpectrumBands[i2][j],
        CurrentPos
        ));
    }
    mpResults->mHighLevelSpectrumSignature.mValues.Append(ResampledBandValues);
    CurrentPos += Step;
  }

  MAssert(mpResults->mHighLevelSpectrumSignature.mValues.Size() ==
    kNumberOfHighLevelSpectrumBandFrames, "");


  // ... Spectral Features (Audible ones only)

  // SpectralFlatness defined above
  const TList<double> SpectralFlux =
    AudibleSpectrumFrames(mpResults->mSpectralFlux.mValues);
  const TList<double> SpectralContrast =
    AudibleSpectrumFrames(mpResults->mSpectralContrast.mValues);
  const TList<double> SpectralComplexity =
    AudibleSpectrumFrames(mpResults->mSpectralComplexity.mValues);
  const TList<double> SpectralInharmonicity =
    AudibleSpectrumFrames(mpResults->mSpectralInharmonicity.mValues);

  const double SpectrumFlatness = SpectralFlatness.IsEmpty() ? 0.0 :
    TStatistics::Mean(SpectralFlatness.FirstRead(), SpectralFlatness.Size());
  const double SpectrumFlux = SpectralFlux.IsEmpty() ? 0.0 :
    TStatistics::Mean(SpectralFlux.FirstRead(), SpectralFlux.Size());
  const double SpectrumContrast = SpectralContrast.IsEmpty() ? 0.0 :
    TStatistics::Mean(SpectralContrast.FirstRead(), SpectralContrast.Size());
  const double SpectrumComplexity = SpectralComplexity.IsEmpty() ? 0.0 :
    TStatistics::Mean(SpectralComplexity.FirstRead(), SpectralComplexity.Size());
  const double SpectrumInharmonicity = SpectralInharmonicity.IsEmpty() ? 0.0 :
    TStatistics::Mean(SpectralInharmonicity.FirstRead(), SpectralInharmonicity.Size());

  mpResults->mHighLevelSpectralFlatness.mValue = SpectrumFlatness;
  mpResults->mHighLevelSpectralFlux.mValue = SpectrumFlux;
  mpResults->mHighLevelSpectralContrast.mValue = SpectrumContrast;
  mpResults->mHighLevelSpectralComplexity.mValue = SpectrumComplexity;
  mpResults->mHighLevelSpectralInharmonicity.mValue = SpectrumInharmonicity;

  // ... Pitch

  // calc "confident" pitch from f0
  TList<double> ConfidentPitchWithFallback;
  ConfidentPitchWithFallback.PreallocateSpace(mpResults->mF0.mValues.Size());

  // look-ahead to find first confident pitch
  double LastConfidentPitch = 0.0;
  if (mpResults->mF0.mValues.Size() > 1)
  {
    for (int i = 0; i <= MMax(1, mpResults->mF0.mValues.Size() / 4); ++i)
    {
      if (mSpectrumFrameIsAudible[i] && IsConfidentPitch(
            mpResults->mF0.mValues[i], mpResults->mF0Confidence.mValues[i]))
      {
        LastConfidentPitch = mpResults->mF0.mValues[i];
        break;
      }
    }
  }

  for (int i = 0; i < mpResults->mF0.mValues.Size(); ++i)
  {
    if (mSpectrumFrameIsAudible[i] && IsConfidentPitch(
          mpResults->mF0.mValues[i], mpResults->mF0Confidence.mValues[i]))
    {
      ConfidentPitchWithFallback.Append(mpResults->mF0.mValues[i]);
      LastConfidentPitch = mpResults->mF0.mValues[i];
    }
    else
    {
      ConfidentPitchWithFallback.Append(LastConfidentPitch);
    }
  }

  mpResults->mHighLevelPitch.mValues = ConfidentPitchWithFallback;

  // convert Hz to notes
  std::transform(mpResults->mHighLevelPitch.mValues.Begin(), 
    mpResults->mHighLevelPitch.mValues.End(), 
    mpResults->mHighLevelPitch.mValues.Begin(), 
    ::aubio_freqtomidi);


  // ... Pitch Confidence

  mpResults->mHighLevelPitchConfidence.mValue = AudiblePitchConfidenceMean;


  // ... Peak

  mpResults->mHighLevelPeak.mValues = mpResults->mAmplitudePeak.mValues;


  // ... Debug

  #if defined(MEnableDebugSampleDescriptors)
  
    // pitch debugging
    mpResults->mHighLevelDebugScalarValue.mValue = AudiblePitchConfidenceMean;
    mpResults->mHighLevelDebugVectorValue.mValues = mpResults->mF0.mValues;

    mpResults->mHighLevelDebugVectorVectorValue.mValues.Empty();
    for (int i = 0; i < mpResults->mFailSafeF0.mValues.Size(); ++i)
    {
      mpResults->mHighLevelDebugVectorVectorValue.mValues.Append(
        MakeList<double>(
          mpResults->mFailSafeF0.mValues[i],
          mpResults->mF0Confidence.mValues[i]
        )
      );
    }
  #endif


  // ... Debug tracing

  // trace wrong BPMs - assumes that filenames are prefixed with XXXBPM
  #if defined(MDebugTraceBpms)
    static int sNumberOfWrongBpms = 0;

    const TString Filename = gCutPath(SampleData.mOriginalFileName);
    if (Filename.MatchesWildcardIgnoreCase("???BPM*") ||
        Filename.MatchesWildcardIgnoreCase("??BPM*"))
    {
      TString BpmName = Filename.SubString(0, 3);
      float FileBpm;
      if (StringToValue(FileBpm, BpmName))
      {
        if (TMathT<float>::Abs((float)mpResults->mHighLevelBpm.mValue - FileBpm) > 5)
        {
          gTraceVar(">>>>> Wrong BPM %d: %.2f -> %.2f ", sNumberOfWrongBpms + 1,
            FileBpm, mpResults->mHighLevelBpm.mValue);

          ++sNumberOfWrongBpms;
        }
      }
    }
  #endif

  
  // trace wrong root keys - assumes that files are in a sub folder which denotes the root key
  #if defined(MDebugTracePitch)
    static int sNumberOfWrongPitches = 0;
    static std::vector<double> sPitchOffsets;

    static std::vector<double> sPitchStdDevs;

    static std::map<std::string, int> sRootKeysMap = {
      { "c", 0 }, { "c#", 1 }, { "d", 2}, { "d#", 3}, { "e", 4 }, { "f", 5 }, 
      { "f#", 6 }, { "g", 7 }, { "g#", 8 }, { "a", 9 }, { "a#", 10 }, { "b", 11 }
    };

    const TDirectory FullPath = gExtractPath(SampleData.mOriginalFileName);
    const TString Filename = gCutPath(SampleData.mOriginalFileName);

    const TString RootKeyFolderName = FullPath.ParentDirCount() > 0 ?
      FullPath.SplitPathComponents().Last() : "";

    sPitchStdDevs.push_back(TStatistics::StandardDeviation(
      mpResults->mHighLevelPitch.mValues.FirstRead(),
      mpResults->mHighLevelPitch.mValues.Size()));

    const double TotalPitchStdDevMean = TStatistics::Mean(
      sPitchStdDevs.data(), (int)sPitchStdDevs.size());

    gTraceVar("> Total pitch stddev mean: %.2f", TotalPitchStdDevMean);

    auto Iter = sRootKeysMap.find(TString(RootKeyFolderName).ToLower().StdCString());
    if (Iter != sRootKeysMap.end())
    {
      const float ExpectedRootKey = (float)Iter->second;
      const float DetectedRootKey = (float)::fmod(mpResults->mHighLevelBaseNote.mValue, 12.0);

      const float Min = MMin(DetectedRootKey, ExpectedRootKey);
      const float Max = MMax(DetectedRootKey, ExpectedRootKey);

      const float Distance = MMin(Max - Min, (Min + 12.0f) - Max); // wrap around edges (> 12)
      sPitchOffsets.push_back(Distance);

      if (Distance > 0.5f) // > 0.5f results into a wrong displayed root key, ignoring cents
      {
        const float PitchDistanceMean = (float)TStatistics::Mean(
          sPitchOffsets.data(), (int)sPitchOffsets.size());

        gTraceVar(">>>>> Wrong Detected Root-Key #%d: %.2f -> %.2f - total distance mean: %.2f",
          sNumberOfWrongPitches + 1, DetectedRootKey, ExpectedRootKey, PitchDistanceMean);

        ++sNumberOfWrongPitches;
      }
    }
    else
    {
      MInvalid("");
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcEffectiveLength(
  const double* pSampleData, int NumberOfSamples) 
{
  const TPair<double, double*> SilenceTestRuns[] = {
    MakePair(TAudioMath::DbToLin(-48.0), &mpResults->mEffectiveLength48dB.mValue),
    MakePair(TAudioMath::DbToLin(-24.0), &mpResults->mEffectiveLength24dB.mValue),
    MakePair(TAudioMath::DbToLin(-12.0), &mpResults->mEffectiveLength12dB.mValue),
  };

  for (size_t s = 0; s < MCountOf(SilenceTestRuns); ++s) 
  {
    const double SilenceFloor = SilenceTestRuns[s].First();
    double* pLengthDescriptor = SilenceTestRuns[s].Second();
    
    int SilentLeadingSamples = 0;
    for (int f = 0; f < NumberOfSamples; ++f, ++SilentLeadingSamples)
    {
      if (TMathT<double>::Abs(pSampleData[f]) > SilenceFloor)
      {
        break;
      }
    }
    int SilentTrailingSamples = 0;
    for (int f = NumberOfSamples - 1; f > SilentLeadingSamples; --f, ++SilentTrailingSamples)
    {
      if (TMathT<double>::Abs(pSampleData[f]) > SilenceFloor)
      {
        break;
      }
    }

    const int EffectiveLengthInSamples = NumberOfSamples - 
      SilentLeadingSamples - SilentTrailingSamples;

    const double EffectiveLengthInSeconds = TAudioMath::SamplesToMs(
        mSampleRate, EffectiveLengthInSamples) / 1000.0;

    *pLengthDescriptor = EffectiveLengthInSeconds;
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcAmplitudePeak(
  const double* pSampleData, int NumberOfSamples)
{
  double Min = *pSampleData, Max = *pSampleData;
  TMathT<double>::GetMinMax(Min, Max, 0, NumberOfSamples, pSampleData);

  mpResults->mAmplitudePeak.mValues.Append(
    MMax(TMathT<double>::Abs(Min), TMathT<double>::Abs(Max)));
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcAmplitudeRms(
  const double* pSampleData, int NumberOfSamples)
{
  double Rms = 0.0;
  ::xtract_rms_amplitude(pSampleData, NumberOfSamples, NULL, &Rms);

  mpResults->mAmplitudeRms.mValues.Append(TMath::IsNaN(Rms) ? 0.0 : Rms);
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcAmplitudeEnvelope(
  const double* pSampleData, int NumberOfSamples)
{
  double FrameMax = mEnvelope;
  for (int i = 0; i < NumberOfSamples; ++i)
  {
    mEnvelopeDetector.Run(TMathT<double>::Abs(pSampleData[i]), mEnvelope);
    FrameMax = MMax(FrameMax, mEnvelope);
  }

  mpResults->mAmplitudeEnvelope.mValues.Append(FrameMax);
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcSpectralRms(
  const TArray<double>& MagnitudeSpectrum)
{
  double Rms = 0.0;
  ::xtract_rms_amplitude(MagnitudeSpectrum.FirstRead() + mFirstAnalyzationBin, 
    mAnalyzationBinCount, NULL, &Rms);

  mpResults->mSpectralRms.mValues.Append(
    TMath::IsNaN(Rms) ? 0.0 : Rms);
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcSpectralCentroidAndSpread(
  const TArray<double>& MagnitudeSpectrum)
{
  const double Centroid = TStatistics::Centroid(
    MagnitudeSpectrum.FirstRead() + mFirstAnalyzationBin, 
    mAnalyzationBinCount);

  mpResults->mSpectralCentroid.mValues.Append(Centroid);

  const double Spread = TStatistics::Spread(
    MagnitudeSpectrum.FirstRead() + mFirstAnalyzationBin, 
    mAnalyzationBinCount, 
    Centroid);

  mpResults->mSpectralSpread.mValues.Append(Spread);

  #if 0 // libXtract's impl - mean and variance
    double Mean = 0.0;
    ::xtract_spectral_mean(MagnitudeSpectrum.FirstRead(),
      mFftFrameSize, NULL, &Mean);

    mpResults->mSpectralCentroid.mValues.Append(
      TMath::IsNaN(Mean) ? 0.0 : Mean);

    double Variance = 0.0;
    ::xtract_spectral_variance(MagnitudeSpectrum.FirstRead(),
      mFftFrameSize, &Mean, &Variance);

    mpResults->mSpectralSpread.mValues.Append(
      TMath::IsNaN(Variance) ? 0.0 : Variance);
  #endif
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcSpectralSkewnessAndKurtosis(
  const TArray<double>& MagnitudeSpectrum)
{
  const double Centroid = TStatistics::Centroid(
    MagnitudeSpectrum.FirstRead() + mFirstAnalyzationBin, 
    mAnalyzationBinCount);

  const double Spread = TStatistics::Spread(
    MagnitudeSpectrum.FirstRead() + mFirstAnalyzationBin, 
    mAnalyzationBinCount, 
    Centroid);

  double Skewness = TStatistics::Skewness(
    MagnitudeSpectrum.FirstRead() + mFirstAnalyzationBin, 
    mAnalyzationBinCount, Centroid, Spread);

  double Kurtosis = TStatistics::Kurtosis(
    MagnitudeSpectrum.FirstRead() + mFirstAnalyzationBin, 
    mAnalyzationBinCount, 
    Centroid, 
    Spread);

  mpResults->mSpectralSkewness.mValues.Append(Skewness);
  mpResults->mSpectralKurtosis.mValues.Append(Kurtosis);
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcSpectralRolloff(
  const TArray<double>& MagnitudeSpectrum)
{
  double Arguments[4] = { 0 };
  Arguments[0] = mSampleRate / (mFftFrameSize / 2);
  Arguments[1] = 85.0f;

  double Rolloff = 0.0;
  ::xtract_rolloff(MagnitudeSpectrum.FirstRead() + mFirstAnalyzationBin, 
    mAnalyzationBinCount, Arguments, &Rolloff);

  mpResults->mSpectralRolloff.mValues.Append(
    TMath::IsNaN(Rolloff) ? 0.0 : Rolloff);
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcSpectralFlatness(
  const TArray<double>& MagnitudeSpectrum)
{
  const double Flatness = SFlatnessDb(
    MagnitudeSpectrum.FirstRead() + mFirstAnalyzationBin, 
    mAnalyzationBinCount);

  mpResults->mSpectralFlatness.mValues.Append(
    TMath::IsNaN(Flatness) ? 0.0 : Flatness);
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcSpectralFlux(
  const TArray<double>& MagnitudeSpectrum,
  const TArray<double>& LastMagnitudeSpectrum)
{
  MAssert(LastMagnitudeSpectrum.Size() == mFftFrameSize &&
    MagnitudeSpectrum.Size() == mFftFrameSize, "");

  const double Flux = TStatistics::Flux(
    MagnitudeSpectrum.FirstRead() + mFirstAnalyzationBin,
    LastMagnitudeSpectrum.FirstRead() + mFirstAnalyzationBin, 
    mAnalyzationBinCount);

  mpResults->mSpectralFlux.mValues.Append(Flux);
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcSpectralComplexity(
  const TArray<double>& PeakSpectrum)
{
  double PeakCount = 0.0;
  ::xtract_nonzero_count(PeakSpectrum.FirstRead() + mFirstAnalyzationBin,
    mAnalyzationBinCount, NULL, &PeakCount);

  mpResults->mSpectralComplexity.mValues.Append(
    TMath::IsNaN(PeakCount) ? 0.0 : PeakCount);
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcSpectralInharmonicity(
  const TArray<double>& PeakSpectrum,
  double                F0,
  double                F0Confidence)
{
  if (F0 > 0.0 && F0Confidence > 0.0)
  {
    double Inharmonicity = 0.0;
    ::xtract_spectral_inharmonicity(PeakSpectrum.FirstRead(),
      mFftFrameSize, &F0, &Inharmonicity);

    mpResults->mSpectralInharmonicity.mValues.Append(
      TMath::IsNaN(Inharmonicity) ? 0.0 : Inharmonicity);
  }
  else // no F0 - assume silence or noise -> not inharmonic
  {
    mpResults->mSpectralInharmonicity.mValues.Append(0.0);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcTristimulus(
  const TArray<double>& HarmonicSpectrum,
  double                F0,
  double                F0Confidence)
{
  if (F0 > 0.0 && F0Confidence > 0.0) // got a fundamental frequency?
  {
    double Tristimulus1 = 0.0;
    ::xtract_tristimulus_1(HarmonicSpectrum.FirstRead(),
      HarmonicSpectrum.Size(), &F0, &Tristimulus1);
    mpResults->mTristimulus1.mValues.Append(Tristimulus1);

    double Tristimulus2 = 0.0;
    ::xtract_tristimulus_2(HarmonicSpectrum.FirstRead(),
      HarmonicSpectrum.Size(), &F0, &Tristimulus2);
    mpResults->mTristimulus2.mValues.Append(Tristimulus2);

    double Tristimulus3 = 0.0;
    ::xtract_tristimulus_3(HarmonicSpectrum.FirstRead(),
      HarmonicSpectrum.Size(), &F0, &Tristimulus3);
    mpResults->mTristimulus3.mValues.Append(Tristimulus3);
  }
  else
  {
    mpResults->mTristimulus1.mValues.Append(0.0);
    mpResults->mTristimulus2.mValues.Append(0.0);
    mpResults->mTristimulus3.mValues.Append(0.0);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcSpectrumBands(
  const TArray<double>& MagnitudeSpectrum)
{
  // . Spectrum bands

  // nearly equal to critical Bark bands, but added two more bands at the lower
  // and upper frequency range (50.0, 150.0, 19000.0 and 22050.0 Hz)
  static const double sBandFrequencies[] = {
    50.0, 100.0, 150.0, 200.0, 300.0, 400.0, 510.0, 630.0, 770.0, 920.0, 
    1080.0, 1270.0, 1480.0, 1720.0, 2000.0, 2320.0, 2700.0, 3150.0, 3700.0, 4400.0, 
    5300.0, 6400.0, 7700.0, 9500.0, 12000.0, 15500.0, 19000.0, 22050.0
  };

  MStaticAssert(kNumberOfSpectrumBands == MCountOf(sBandFrequencies));

  TStaticArray<double, kNumberOfSpectrumBands> FrequencyBands;
  FrequencyBands.Init(0.0);

  const double FrequenciesPerBin = mSampleRate / mFftFrameSize;
  const int FirstBin = TMath::d2iRound(MAnalyzationFreqMin / FrequenciesPerBin);

  for (int b = 0; b < FrequencyBands.Size(); ++b)
  {
    int StartBin = TMath::d2iRound((b == 0) ? 
      FirstBin : sBandFrequencies[b - 1] / FrequenciesPerBin);
    if (StartBin >= mFftFrameSize / 2)
    {
      break;
    }

    int EndBin = TMath::d2iRound(sBandFrequencies[b] / FrequenciesPerBin);
    EndBin = MMin(mFftFrameSize / 2, EndBin);

    for (int s = StartBin; s < EndBin; s++)
    {
      FrequencyBands[b] += TMathT<double>::Square(MagnitudeSpectrum[s]);
    }
  }

  mpResults->mSpectrumBands.mValues.Append(FrequencyBands);
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcCepstrumBands(
  const TArray<double>& MagnitudeSpectrum)
{
  TStaticArray<double, kNumberOfCepstrumCoefficients> MFCCs;
  MFCCs.Init(0.0);

  ::xtract_mfcc(MagnitudeSpectrum.FirstRead(),
    mFftFrameSize / 2, mpXtractMelFilters, MFCCs.FirstWrite());

  mpResults->mCepstrumBands.mValues.Append(MFCCs);
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcSpectralBandFeatures(
  const TArray<double>& MagnitudeSpectrum,
  const TArray<double>& LastMagnitudeSpectrum)
{
  // Setup analysation consts/ranges
  const double NeighbourRatio = 0.3; // Neighbours per band to take mean of
  // const double PartToScale = 0.85; // Static distribution

  // Use a set of 12 condensed critical bark bands, up to 9500 Hz
  static const double sBandFrequencies[] = {
    50.0, 100.0, 200.0, 400.0, 630.0, 920.0, 1270.0, 1720.0, 2320.0, 3150.0, 
    4400.0, 6400.0, 9500.0, 15500.0
  };

  MStaticAssert(kNumberOfSpectrumSubBands == MCountOf(sBandFrequencies));

  // Determine how many bins are in each band
  TStaticArray<int, kNumberOfSpectrumSubBands> NumberOfBinsInBands;

  const double FrequenciesPerBin = mSampleRate / mFftFrameSize;
  const int FirstBin = TMath::d2iRound(MAnalyzationFreqMin / FrequenciesPerBin);

  for (int b = 0; b < (int)MCountOf(sBandFrequencies); ++b)
  {
    const int StartBin = (b == 0) ? FirstBin : 
      TMath::d2iRound(sBandFrequencies[b - 1] / FrequenciesPerBin);
    MAssert(StartBin < mFftFrameSize / 2, "");
    
    const int EndBin = TMath::d2iRound(sBandFrequencies[b] / FrequenciesPerBin);
    MAssert(EndBin < mFftFrameSize / 2, "");

    NumberOfBinsInBands[b] = (EndBin - StartBin + 1);
  }

  // make a copy of the spectrums, because we'll transform (sort) them
  std::vector<double> Magnitudes(mFftFrameSize / 2);
  for (int i = 0; i < mFftFrameSize / 2; ++i)
  {
    Magnitudes[i] = MagnitudeSpectrum[i];
  }

  std::vector<double> LastMagnitudes(mFftFrameSize / 2);
  for (int i = 0; i < mFftFrameSize / 2; ++i)
  {
    LastMagnitudes[i] = LastMagnitudeSpectrum[i];
  }

  const double Epsilon = 1e-30;

  // get the outputs
  TStaticArray<double, kNumberOfSpectrumSubBands> RmsBands;
  RmsBands.Init(0.0);

  TStaticArray<double, kNumberOfSpectrumSubBands> FlatnessBands;
  FlatnessBands.Init(0.0);

  TStaticArray<double, kNumberOfSpectrumSubBands> FluxBands;
  FluxBands.Init(0.0);

  TStaticArray<double, kNumberOfSpectrumSubBands> ComplexityBands;
  ComplexityBands.Init(0.0);

  TStaticArray<double, kNumberOfSpectrumSubBands> ContrastBands;
  ContrastBands.Init(0.0);

  // skip bands below MAnalyzationFreqMin
  for (int BandIndex = 0, CurrentBin = FirstBin;
       BandIndex < NumberOfBinsInBands.Size();
       ++BandIndex)
  {
    const int NumBinsInBand = MMin(NumberOfBinsInBands[BandIndex],
      (int)Magnitudes.size() - CurrentBin);

    MAssert(NumBinsInBand  > 0, "");

    // get the mean and max of the band
    const double BandMean = TStatistics::Mean(
      Magnitudes.data() + CurrentBin, NumBinsInBand);
    // const double BandMax = TStatistics::Max(
    //   Magnitudes.data() + CurrentBin, NumBinsInBand);

    // gTraceVar("Computing Band Features for band %d: From %g Hz to %g Hz", 
    //   BandIndex, CurrentBin * FrequenciesPerBin,
    //   (CurrentBin + NumBinsInBand) * FrequenciesPerBin);

    // calc rms 
    double Rms = 0.0;
    for (int i = 0; i < NumBinsInBand; ++i)
    {
      Rms += TMathT<double>::Square(Magnitudes[CurrentBin + i]);
    }
    Rms = ::sqrt(Rms / (double)NumBinsInBand);
    
    // calc flatness
    const double Flatness = SFlatnessDb(
      Magnitudes.data() + CurrentBin, NumBinsInBand);

    // calc flux
    MAssert(Magnitudes.size() == LastMagnitudes.size(), "");
    const double Flux = TStatistics::Flux(Magnitudes.data() + CurrentBin,
      LastMagnitudes.data() + CurrentBin, NumBinsInBand);

    // calc complexity
    double ComplexityThreshold = 0.0;
    for (int i = 0; i < NumBinsInBand; ++i)
    {
      ComplexityThreshold = MMax(ComplexityThreshold,
        Magnitudes[CurrentBin + i]);
    }
    ComplexityThreshold *= MPeakThreshold;

    double Complexity = 0;
    if (ComplexityThreshold > 0.0)
    {
      for (int i = 0; i < NumBinsInBand; ++i)
      {
        const int b = CurrentBin + i;

        // NB: use unsorted MagnitudeSpectrum here, so we can reach into next bands
        if (MagnitudeSpectrum[b] > ComplexityThreshold)
        {
          if (b > 0 && b < (int)Magnitudes.size() - 1 &&
              MagnitudeSpectrum[b] > MagnitudeSpectrum[b - 1] &&
              MagnitudeSpectrum[b] > MagnitudeSpectrum[b + 1])
          {
            ++Complexity;
          }
        }
      }
    }

    // sort the subbands (ascending order)
    std::sort(Magnitudes.begin() + CurrentBin,
      Magnitudes.begin() + CurrentBin + NumBinsInBand);

    // number of bins to take the mean of
    const int NeighbourBins = MMax(1, (int)(NeighbourRatio * NumBinsInBand));

    // calc valley
    double Sum = 0;
    for (int i = 0; i < NeighbourBins && i < NumBinsInBand; ++i)
    {
      MAssert(CurrentBin + i >= 0 &&
        CurrentBin + i < (int)Magnitudes.size(), "");

      Sum += Magnitudes[CurrentBin + i];
    }

    const double Valley = Sum / NeighbourBins + Epsilon;

     // calc peak
    Sum = 0;
    for (int i = NumBinsInBand; i > NumBinsInBand - NeighbourBins; --i)
    {
      MAssert(CurrentBin + i - 1 >= 0 &&
        CurrentBin + i - 1 < (int)Magnitudes.size(), "");

      Sum += Magnitudes[CurrentBin + i - 1];
    }

    const double Peak = Sum / NeighbourBins + Epsilon;

    // calc contrast
    const double Contrast = -1.0 * ::pow(
      Peak / Valley, 1.0 / ::log(BandMean + Epsilon));

    // memoize values
    RmsBands[BandIndex] = Rms;
    FlatnessBands[BandIndex] = Flatness;
    FluxBands[BandIndex] = Flux;
    ComplexityBands[BandIndex] = Complexity;
    ContrastBands[BandIndex] = Contrast;

    CurrentBin += NumBinsInBand;
  }

  mpResults->mSpectralRmsBands.mValues.Append(RmsBands);
  mpResults->mSpectralFlatnessBands.mValues.Append(FlatnessBands);
  mpResults->mSpectralFluxBands.mValues.Append(FluxBands);
  mpResults->mSpectralComplexityBands.mValues.Append(ComplexityBands);
  mpResults->mSpectralContrastBands.mValues.Append(ContrastBands);

  #if 1  // calc average contrast from bands

    double ContrastSum = 0.0;

    for (int b = 0; b < kNumberOfSpectrumSubBands; ++b)
    {
      ContrastSum += ContrastBands[b];
    }

    mpResults->mSpectralContrast.mValues.Append(
      ContrastSum / kNumberOfSpectrumSubBands);

  #else  // calc contrast for the entire frequency range

    // get the mean
    double BandMean = 0.0;
    for (int i = 0; i < Spectrum.size(); ++i)
    {
      BandMean += Spectrum[i];
    }
    BandMean /= Spectrum.size();

    // sort the subbands (ascending order)
    std::sort(Spectrum.begin(), Spectrum.end());

    // number of bins to take the mean of
    const int NeighbourBins = (int)(NeighbourRatio * Spectrum.size());

    // calc valley
    double Sum = 0;
    for (int i = 0;
         i < NeighbourBins && i < Spectrum.size();
         ++i)
    {
      MAssert(i >= 0 && i < Spectrum.size(), "");

      Sum += Spectrum[i];
    }
    const double Valley = Sum / NeighbourBins + Epsilon;

    // calc peak
    Sum = 0;
    for (int i = (int)Spectrum.size();
         i > (int)Spectrum.size() - NeighbourBins;
         --i)
    {
      MAssert(i - 1 >= 0 && i - 1 < Spectrum.size(), "");

      Sum += Spectrum[i - 1];
    }
    const double Peak = Sum / NeighbourBins + Epsilon;

    // calc contrast
    const double Contrast = -1.0 * ::pow(
      Peak / Valley, 1.0 / ::log(BandMean + Epsilon));

    mpResults->mSpectralContrast.mValues.Append(Contrast);
  #endif
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcAutoCorrelation(
  const double* pSampleData,
  int           RemainingSamples)
{
  // consts
  const float MinPeriodLengthInMs = 0.8f; // ~1250 Hz 
  const int MinPeriodLengthInSamples = TAudioMath::MsToSamples(
    mSampleRate, MinPeriodLengthInMs);

  const float SeekWidthInMs = 12.0f; // ~83.3 Hz
  const int SeekWidthInSamples = TAudioMath::MsToSamples(
    mSampleRate, SeekWidthInMs);

  const int MaxSeekRangeInSamples = mFftFrameSize / 2;

  // search for the first zero-crossing
  const double* pSampleDataStart = pSampleData;

  for (int i = 0;
       i < MMin(RemainingSamples, MaxSeekRangeInSamples) - 1;
       ++i)
  {
    if (pSampleData[i + 1] > pSampleData[i])
    {
      pSampleDataStart = pSampleData + i;
      RemainingSamples = RemainingSamples - i;
      break;
    }
  }

  // search for the next zero-crossing
  const int SeekOffset = MMin(RemainingSamples, MinPeriodLengthInSamples);
  const double* pSampleDataEnd = pSampleDataStart + SeekOffset;

  for (int i = 0;
       i < MMin(RemainingSamples - SeekOffset, MaxSeekRangeInSamples) - 1;
       ++i)
  {
    if (pSampleDataStart[SeekOffset + i + 1] > pSampleDataStart[SeekOffset + i])
    {
      pSampleDataEnd = pSampleDataStart + SeekOffset + i;
      break;
    }
  }

  // calc distance between the two zero crossings
  const int PeriodLength = (int)(intptr_t)(pSampleDataEnd - pSampleDataStart);

  if (!RemainingSamples || PeriodLength >= RemainingSamples)
  {
    mpResults->mAutoCorrelation.mValues.Append(0.0);
    return;
  }

  const int CorrelationSeekWidth =
    MMin(RemainingSamples, SeekWidthInSamples);

  TArray<double> AutoCorrelation(CorrelationSeekWidth);
  AutoCorrelation.Init(0.0);

  #if 0 // use warped correlation (makes things worse)

    // See A.H?rm?, M.Karjalainen, L.Savioja, V.V?lim?ki, U.K.Laine, and
    // J. Huopaniemi, Frequency-Warped Signal Processing for Audio Applications
    const double lambda = 1.0674 *
      sqrt(2.0*atan(0.00006583*mSampleRate) / MPi) - 0.1916;
    MAssert(lambda < 1.0, "");

    SWarpedAutoCorrelation(pSampleDataStart, CorrelationSeekWidth,
      AutoCorrelation.FirstWrite(), AutoCorrelation.Size(), lambda);

  #else // simple correlation

    TAutocorrelation::Calc(pSampleDataStart, CorrelationSeekWidth,
      AutoCorrelation.FirstWrite(), AutoCorrelation.Size());
  #endif

  // skip self correlation by skipping half of the PeriodLength coeffs
  double BestResult = 0.0;
  for (int i = PeriodLength / 2; i < AutoCorrelation.Size(); ++i)
  {
    BestResult = MMax(BestResult, AutoCorrelation[i]);
  }

  mpResults->mAutoCorrelation.mValues.Append(BestResult);
}

// -------------------------------------------------------------------------------------------------

void TSampleAnalyser::CalcStatistics()
{
  // calculate all low level statistics from the VR or VVR values 
  const TList<TSampleDescriptors::TDescriptor*> LowLevelDescriptors =
    mpResults->Descriptors(TSampleDescriptors::kLowLevelDescriptors);

  for (int i = 0; i < LowLevelDescriptors.Size(); ++i)
  { 
    LowLevelDescriptors[i]->CalcStatistics();
  }
}

