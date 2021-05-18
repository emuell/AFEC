#include "AudioTypes/Export/AudioMath.h"

#include "AudioTypes/Export/OnsetDetector.h"

#include "FeatureExtraction/Export/Statistics.h"
#include "FeatureExtraction/Source/RhythmTracker.h"

#include <algorithm> // for TList::Sort

// =================================================================================================

// when enabled, trace debug info 
// #define MTraceRhythmTracking

// =================================================================================================

// Fft process & whitening settings (must work for all onset algorithms)
#define MOnsetWhiteningType TOnsetFftProcessor::kWhiteningAdaptMax
#define MOnsetWhiteningRelaxTime 25.0
#define MOnsetLogMagnitudes false

// Tweaked to detect mixed (tonal and percussive) content
#define MComplexOnsetDetectionAlgorithm TOnsetDetector::kFunctionRComplex
#define MComplexOnsetThreshold 0.2 // quite sensitive - for the heuristic onset match
#define MComplexOnsetThresholdMedianSpan 0.2
#define MComplexOnsetMinimumGap 0.06

// Tweaked to detect percussive content
#define MPercussiveOnsetDetectionAlgorithm TOnsetDetector::kFunctionPower
#define MPercussiveOnsetThreshold 0.8 
#define MPercussiveOnsetThresholdMedianSpan 0.2
#define MPercussiveOnsetMinimumGap 0.12

// canny window coeffs to sharpen onsets
#define MCannyWindowLength 12
#define MCannyWindowShape 16.0

// peak windows for (peak) contrast calculation
#define MPeakWindowLength 24
#define MPeakThreshold 0.1

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TRhythmTracker::TRhythmTracker(
  int SampleRate,
  int FftFrameSize,
  int HopSize)
  : mSamplerate(SampleRate),
    mFftSize(FftFrameSize),
    mHopSize(HopSize),
    mWindow(MCannyWindowLength, MCannyWindowShape)
{
  MAssert(HopSize <= 512, "Should use small windows for rhythm tracking");

  mpOnsetFftProcessor = TOwnerPtr<TOnsetFftProcessor>(
    new TOnsetFftProcessor(
      (float)SampleRate, 
      (unsigned int)FftFrameSize, 
      (unsigned int)HopSize,
      MOnsetLogMagnitudes,
      MOnsetWhiteningType, 
      (float)MOnsetWhiteningRelaxTime)
    );

  mpOnsetDetectors[kComplex] = TOwnerPtr<TOnsetDetector>(
    new TOnsetDetector(
      MComplexOnsetDetectionAlgorithm, 
      (float)SampleRate, 
      (unsigned int)FftFrameSize, 
      (unsigned int)HopSize,
      (float)MComplexOnsetThreshold, 
      (float)MComplexOnsetThresholdMedianSpan, 
      (float)MComplexOnsetMinimumGap)
    );

  mpOnsetDetectors[kPercussive] = TOwnerPtr<TOnsetDetector>(
    new TOnsetDetector(
      MPercussiveOnsetDetectionAlgorithm, 
      (float)SampleRate, 
      (unsigned int)FftFrameSize, 
      (unsigned int)HopSize,
      (float)MPercussiveOnsetThreshold, 
      (float)MPercussiveOnsetThresholdMedianSpan, 
      (float)MPercussiveOnsetMinimumGap)
    );
}

// -------------------------------------------------------------------------------------------------

TRhythmTracker::~TRhythmTracker() 
{
  // just be virtual and avoid getting inlined
}

// -------------------------------------------------------------------------------------------------

void TRhythmTracker::ProcessFrame(const fvec_t* SampleInput)
{
  MAssert(SampleInput->length, "Expecting valid data here");
 
  const TOnsetFftProcessor::TPolarBuffer* pWhitenedPolarBuffer = 
    mpOnsetFftProcessor->Process(SampleInput->data);

  for (int t = 0; t < kNumberOfOnsetTypes; ++t) 
  {
    if (mpOnsetDetectors[t]->Process(pWhitenedPolarBuffer)) 
    {
      mOnsetValues[t].Append(mpOnsetDetectors[t]->CurrentOdfValue());
    }
    else 
    {
      mOnsetValues[t].Append(0.0);
    }
  }
}

// -------------------------------------------------------------------------------------------------

int TRhythmTracker::OnsetCount(TOnsetType OnsetType)const 
{
  const double Threshold = OnsetThreshold(OnsetType);
    
  int Count = 0;
  for (int i = 0; i < mOnsetValues[OnsetType].Size(); ++i) 
  {
    if (mOnsetValues[OnsetType][i] > Threshold) 
    {
      ++Count;
    }
  }
  return Count;
}
  
// -------------------------------------------------------------------------------------------------

TList<double> TRhythmTracker::Onsets(TOnsetType OnsetType)const
{
  return mOnsetValues[OnsetType];
}

// -------------------------------------------------------------------------------------------------

TList<double> TRhythmTracker::SharpenedOnsets(TOnsetType OnsetType)const
{
  // calc or fetch sharpened onset values
  TList<double> Onsets;
  SharpenedOnsetValues(Onsets, OnsetType);
  return Onsets;
}

// -------------------------------------------------------------------------------------------------

double TRhythmTracker::CalculateTempo(double& Confidence, TOnsetType OnsetType) const
{
  MAssert(!mOnsetValues[OnsetType].IsEmpty(), "Call 'ProcessFrame' first to collect onsets");

  // need at least 4 valid onset values to compute a reasonable value
  if (OnsetCount(OnsetType) < 4)
  {
    #if defined(MTraceRhythmTracking)
      MTrace("RhythmTracking: Too few onsets to calc tempo");
    #endif

    Confidence = 0.0;
    return 0.0;
  }

  // calc or fetch sharpened onset values
  TList<double> Onsets;
  SharpenedOnsetValues(Onsets, OnsetType);
  
  // run aubio beat tracker on onsets
  aubio_beattracking_t* pBeatTracker = ::new_aubio_beattracking(
    Onsets.Size(), mHopSize, mSamplerate);

  fvec_t BeatTrackInputVector;
  BeatTrackInputVector.length = Onsets.Size();
  BeatTrackInputVector.data = &Onsets[0];

  fvec_t* pBeatTrackOutputVector = new_fvec(Onsets.Size());

  ::aubio_beattracking_do(
    pBeatTracker, &BeatTrackInputVector, pBeatTrackOutputVector);

  // get and validate tempo
  double Tempo = 0.0;

  Tempo = ::aubio_beattracking_get_bpm(pBeatTracker);
  MAssert(!TMath::IsNaN(Tempo), "Invalid tempo");

  Confidence = ::aubio_beattracking_get_confidence(pBeatTracker);
  Confidence = MClip(Confidence * 16.0, 0.0, 1.0); // normalize

  #if defined(MTraceRhythmTracking)
    MTrace3("RhythmTracking: BeatTracking results: "
      "NumberOfBeats: %d Tempo: %lf Confidence: %lf",
      (int)pBeatTrackOutputVector->data[0], Tempo, Confidence);
  #endif

  // reject tempi which are not in the usual loop range
  if (Tempo < 20.0 || Tempo > 300.0)
  {
    Confidence = 0.0;
    Tempo = 0.0;
  }
  else
  {
    // wrap tempo into "common" BPMs
    while (Tempo < 80.0)
    {
      Tempo *= 2.0;
    }
    while (Tempo >= 200.0)
    {
      Tempo /= 2.0;
    }

    #if defined(MTraceRhythmTracking)
      MTrace1("RhythmTracking: Final Tempo %lf", Tempo);
    #endif
  }

  // cleanup
  ::del_fvec(pBeatTrackOutputVector);
  ::del_aubio_beattracking(pBeatTracker);

  return Tempo;
}

// -------------------------------------------------------------------------------------------------

double TRhythmTracker::CalculateTempoWithHeuristics(
  double&     Confidence,
  double      DetectedTempo,
  double      DetectedTempoConfidence,
  double      SampleDurationInSeconds,
  double      OnsetOffsetInSeconds,
  TOnsetType  OnsetType) const
{
  MAssert(!mOnsetValues[OnsetType].IsEmpty(), "Call 'ProcessFrame' first to collect onsets");

  // skip heuristics, when we got no base tempo
  if (DetectedTempo == 0)
  {
    Confidence = 0.0;
    return 0.0;
  }
 
  double Tempo = DetectedTempo;
  Confidence = DetectedTempoConfidence;

  // calc or fetch sharpened onset values
  TList<double> Onsets;
  SharpenedOnsetValues(Onsets, OnsetType);

  // assume it's not a loop when it's duration is less than 3 beats
  const double SamplesPerBeat = 60.0 / Tempo * mSamplerate;

  int LastNonSilentPeak = Onsets.Size() - 1;
  while (LastNonSilentPeak > 0 && Onsets[LastNonSilentPeak] < MPeakThreshold)
  {
    --LastNonSilentPeak;
  }

  const double NumberOfSamples = LastNonSilentPeak * mHopSize;
  if (NumberOfSamples < SamplesPerBeat * 3)
  {
    #if defined(MTraceRhythmTracking)
      MTrace("RhythmTracking: Sample not long enough - ignoring BPM");
    #endif

    Confidence = 0.0;
    return 0.0;
  }

  // get bpm from sample duration
  else
  {
    const double MinBpm = 80;
    const double MaxBpm = 180;
    const double GuessedNumberOfBeats = GuessNumberOfBeatsFromDuration(
      MinBpm, MaxBpm, SampleDurationInSeconds);

    #if defined(MTraceRhythmTracking)
      MTrace1("RhythmTracking: Guessed number of beats from duration: %lf",
        GuessedNumberOfBeats);
    #endif

    // skip for longer samples - rhythm tracker usually does a good job then
    if (GuessedNumberOfBeats >= 4 && GuessedNumberOfBeats <= 16)
    {
      const double GuessedBpm = GuessedNumberOfBeats / (SampleDurationInSeconds / 60);

      // calc how well the raw onsets are matching the guessed BPM
      // DON'T use the sharpened peaks, the canny window interpolates & smears the peaks. 
      const TList<double>& RawOnsets = mOnsetValues[OnsetType];
      const double OnsetTrackerDelay = TAudioMath::SamplesToMs(mSamplerate, mHopSize / 2) / 1000.0;
      
      const double GuessedOnsetConfidence = OnsetMatchConfidence(
        RawOnsets, 
        OnsetOffsetInSeconds + OnsetTrackerDelay, 
        GuessedNumberOfBeats, 
        GuessedBpm,
        OnsetType);

      // when the guessed BPM is better than the one from the rhythm detector,
      // or it's nearly matching, use the BPM from the duration
      if ((GuessedOnsetConfidence > 0.5) ||
          (Confidence < 0.1 && GuessedOnsetConfidence > 0.1) ||
          (Confidence < 0.5 && TMathT<double>::Abs(GuessedBpm - Tempo) < 10))
      {
        #if defined(MTraceRhythmTracking)
          MTrace2("RhythmTracking: Using BPM from duration: %lf - Confidence: %lf",
            GuessedBpm, GuessedOnsetConfidence);
        #endif

        Tempo = GuessedBpm;
        Confidence = MMax(0.5, GuessedOnsetConfidence);
      }
    }
    return Tempo;
  }
}

// -------------------------------------------------------------------------------------------------

double TRhythmTracker::CalculateRhythmFrequencyMean(TOnsetType OnsetType) const
{
  MAssert(!mOnsetValues[OnsetType].IsEmpty(), "Call 'ProcessFrame' first to collect onsets");

  // calc or fetch sharpened onset values
  TList<double> Onsets;
  SharpenedOnsetValues(Onsets, OnsetType);

  if (Onsets.Size())
  {
    // rhythm strength is the mean of all peaks
    TList<double> Peaks;
    CalculatePeaks(Onsets, Peaks, OnsetType);

    const double AverageOnsetFreq = (double)Peaks.Size() / (double)Onsets.Size() *
      (double)mHopSize / (double)mFftSize;

    #if defined(MTraceRhythmTracking)
      MTrace1("RhythmTracking: Peak frequency: %lf", AverageOnsetFreq);
    #endif

    return AverageOnsetFreq;
  }

  return 0.0;
}

// -------------------------------------------------------------------------------------------------

double TRhythmTracker::CalculateRhythmStrength(TOnsetType OnsetType) const
{
  MAssert(!mOnsetValues[OnsetType].IsEmpty(), "Call 'ProcessFrame' first to collect onsets");

  // calc or fetch sharpened onset values
  TList<double> Onsets;
  SharpenedOnsetValues(Onsets, OnsetType);

  // rhythm strength is the mean of all peaks
  TList<double> Peaks;
  CalculatePeaks(Onsets, Peaks, OnsetType);

  if (Peaks.Size())
  {
    const double PeakMean = TStatistics::Mean(Peaks.FirstRead(), Peaks.Size());

    #if defined(MTraceRhythmTracking)
      MTrace1("RhythmTracking: Peak mean: %lf", PeakMean);
    #endif

    return MClip(PeakMean / 4.0, 0.0, 1.0);
  }
  else
  {
    #if defined(MTraceRhythmTracking)
      MTrace("RhythmTracking: Peak mean: empty");
    #endif

    return 0.0;
  }
}

// -------------------------------------------------------------------------------------------------

double TRhythmTracker::CalculateRhythmContrast(TOnsetType OnsetType) const
{
  MAssert(!mOnsetValues[OnsetType].IsEmpty(), "Call 'ProcessFrame' first to collect onsets");

  // calc or fetch sharpened onset values
  TList<double> Onsets;
  SharpenedOnsetValues(Onsets, OnsetType);

  // sort onsets
  TList<double > OnsetsSorted = Onsets;
  OnsetsSorted.Sort();

  const double ThresholdPercentile = 85.0;
  const double Threshold = OnsetsSorted[(int)
    (ThresholdPercentile / 100.0 * (OnsetsSorted.Size() - 1))];

  // fetch peaks and valleys
  TList<double> Peaks, Valleys;

  int ValleyPos = 0;
  double ValleyValue = Threshold;

  for (int i = 0; i < Onsets.Size(); i++)
  {
    // check for valley
    if (Onsets[i] < ValleyValue)
    {
      ValleyPos = i;
      ValleyValue = Onsets[i];
    }

    // if below the threshold, move onto next element
    if (Onsets[i] < Threshold)
    {
      continue;
    }

    // check for other peaks in the area
    bool Success = true;
    for (int j = -MPeakWindowLength; j <= MPeakWindowLength; j++)
    {
      if (i + j >= 0 && i + j < Onsets.Size())
      {
        if (Onsets[i + j] > Onsets[i])
        {
          Success = false;
          break;
        }
      }
    }

    // save peak and valley
    if (Success)
    {
      Peaks.Append(Onsets[i]);
      Valleys.Append(Onsets[ValleyPos]);
      ValleyValue = Onsets[i];
    }
  }

  const double TotalMean = TStatistics::Mean(
    Onsets.FirstRead(), Onsets.Size());

  const double PeakMean = TStatistics::Mean(
    Peaks.FirstRead(), Peaks.Size());
  const double ValleyMean = TStatistics::Mean(
    Valleys.FirstRead(), Valleys.Size()) + 0.0001;

  if (PeakMean != 0.0)
  {
    // calc contrast
    double PeakContrast = -1.0 * ::pow(
      PeakMean / ValleyMean, 1.0 / ::log(TotalMean + 0.0001));

    #if defined(MTraceRhythmTracking)
      MTrace1("RhythmTracking: Peak Contrast: %lf", PeakContrast);
    #endif

    return PeakContrast;
  }
  else
  {
    #if defined(MTraceRhythmTracking)
      MTrace("RhythmTracking: Peak Contrast: 0");
    #endif

    return 0.0;
  }
}

// -------------------------------------------------------------------------------------------------

double TRhythmTracker::OnsetThreshold(TOnsetType OnsetType)const 
{
  MStaticAssert(kNumberOfOnsetTypes == 2);
  switch (OnsetType) 
  {
  default:
    MInvalid("Invalid onset type");
    return 0.0;
  case kComplex:
    return MComplexOnsetThreshold;
    break;
  case kPercussive:
    return MPercussiveOnsetThreshold;
  }
}

// -------------------------------------------------------------------------------------------------

double TRhythmTracker::GuessNumberOfBeatsFromDuration(
  double  MinBpmExpected,
  double  MaxBpmExpected,
  double  DurationInSeconds) const
{
  const int SignatureNumerator = 4;
  const int MaxSampleBarLength = 8;

  const double BeatsPerBar = 4.0;

  const double MaxDurationOfOneBeat = (60.0 / MinBpmExpected);
  const double MaxDurationOfOneBar = BeatsPerBar * MaxDurationOfOneBeat;

  if (DurationInSeconds < MaxDurationOfOneBeat)
  {
    return 0.0f; // too short
  }

  // sample <= 1 Bar : look for an even divider of type 2^x; x > 1
  else if (DurationInSeconds < MaxDurationOfOneBar)
  {
    // look for a divider, highest divider is clicks per bar/2
    for (int Divider = SignatureNumerator / 2; Divider >= 1; Divider /= 2)
    {
      double MaxDurationOfDividedBar =
        (((double)SignatureNumerator / (double)Divider) * MaxDurationOfOneBeat);

      if (SignatureNumerator % Divider == 0 &&
        DurationInSeconds < MaxDurationOfDividedBar)
      {
        return (float)((BeatsPerBar / (double)Divider));
      }
    }

    return (float)(BeatsPerBar);
  }

  // sample > 1 Bar
  else
  {
    for (int BarCount = 1; BarCount <= MaxSampleBarLength; BarCount *= 2)
    {
      double NumberOfBeats = BeatsPerBar * BarCount;
      double DurationOfOneBeat = DurationInSeconds / NumberOfBeats;

      if (DurationOfOneBeat < MaxDurationOfOneBeat)
      {
        return (float)(NumberOfBeats);
      }
    }
  }

  return 0.0f; // too long
}

// -------------------------------------------------------------------------------------------------

double TRhythmTracker::OnsetMatchConfidence(
  const TList<double>&  Onsets,
  double                OnsetOffsetInSeconds,
  double                NumberOfBeats,
  double                Tempo,
  TOnsetType            OnsetType) const
{
  const int OnsetOffsetInSamples = TAudioMath::MsToSamples(
    (int)mSamplerate, (float)(OnsetOffsetInSeconds * 1000));

  const double SamplesPerBeat = 60.0 / Tempo * mSamplerate;

  // +- 128th in hop size frames
  const int OnsetSearchRange = (int)(SamplesPerBeat / 32) / mHopSize;
  const double Threshold = OnsetThreshold(OnsetType);

  // check matching 8th note onsets
  double OnsetStrength = 0;

  for (int i = 0; i < NumberOfBeats * 2; ++i)
  {
    const int OnsetSampleTime = (int)(i * SamplesPerBeat / 2.0) + OnsetOffsetInSamples;
    const int OnsetIndex = ((OnsetSampleTime + (int)mHopSize / 2) / (int)mHopSize);

    double Peak = 0.0;
    for (int j = OnsetIndex - OnsetSearchRange;
         j < OnsetIndex + OnsetSearchRange;
         ++j)
    {
      if (j >= 0 && j < Onsets.Size())
      {
        Peak = MMax(Peak, Onsets[j]);
      }
    }

    if (Peak >= Threshold)
    {
      OnsetStrength += 1.0;
    }
  }

  // when 1/2 of the 8th note onsets match, this is a perfect confidence
  const double PossibleOnsets = NumberOfBeats * 2;
  return MMin(1.0, (double)OnsetStrength / (double)PossibleOnsets * 2.0);
}

// -------------------------------------------------------------------------------------------------

void TRhythmTracker::SharpenedOnsetValues(
  TList<double>&  Onsets, 
  TOnsetType      OnsetType) const
{
  // calc or return cached sharpened onsets
  if (mSharpenedOnsetValues[OnsetType].IsEmpty())
  {
    mSharpenedOnsetValues[OnsetType] = mOnsetValues[OnsetType];
    mWindow.Apply(&mSharpenedOnsetValues[OnsetType].First(), 
      mSharpenedOnsetValues[OnsetType].Size());
  }
  Onsets = mSharpenedOnsetValues[OnsetType];
}

// -------------------------------------------------------------------------------------------------

void TRhythmTracker::CalculatePeaks(
  const TList<double>&  Onsets,
  TList<double>&        OnsetPeaks,
  TOnsetType            OnsetType) const
{
  OnsetPeaks.ClearEntries();
  OnsetPeaks.PreallocateSpace(Onsets.Size());

  for (int frame = 0; frame < Onsets.Size(); frame++)
  {
    // ignore onsets below the threshold
    if (Onsets[frame] <= MPeakThreshold)
    {
      continue;
    }

    // if any frames within windowSize have a bigger value, this is not the peak
    bool Success = true;
    for (int i = -MPeakWindowLength; i <= MPeakWindowLength; ++i)
    {
      if (frame + i >= 0 && frame + i < Onsets.Size())
      {
        if (Onsets[frame + i] > Onsets[frame])
        {
          Success = false;
          break;
        }
      }
    }

    // push result
    if (Success)
    {
      OnsetPeaks.Append(Onsets[frame]);
    }
  }
}

