#pragma once

#ifndef _RhythmTracker_h_
#define _RhythmTracker_h_

#include "CoreTypes/Export/Pair.h"
#include "CoreTypes/Export/Array.h"

#include "FeatureExtraction/Source/CannyWindow.h"

#include "../../3rdParty/Aubio/Export/Aubio.h"

class TOnsetFftProcessor;
class TOnsetDetector;

// =================================================================================================

/*!
 * Calculates various rhytm features from onsets, using aubio's beat tracker
 * and onset detectors.
 *
 * NB: HopSize should be <= 256 frames to be precise enough for bpm tracking.
!*/

class TRhythmTracker
{
public:
  TRhythmTracker(int SampleRate, int FftSize, int HopSize);
  virtual ~TRhythmTracker();

  // Process a single FFT frame
  void ProcessFrame(const fvec_t* SampleInput);

  // Onset detection methods
  enum TOnsetType 
  {
    // tweaked to find onsets in mixed sound contents (percussion and tonal sounds)
    kComplex,
    // tweaked to find onsets in percussive sounds
    kPercussive,

    kNumberOfOnsetTypes
  };
  
  // Raw access to raw and processed onsets
  int OnsetCount(TOnsetType OnsetType)const;
  TList<double> Onsets(TOnsetType OnsetType)const;
  TList<double> SharpenedOnsets(TOnsetType OnsetType)const;

  // calculate tempo from processed onsets
  // \param SampleDurationInSeconds: total duration of the sample
  // \param OnsetOffsetInSeconds: amount of silence which probably has been cut 
  // away from the sample during preprocessing
  double CalculateTempo(double& Confidence, TOnsetType OnsetType) const;
  // same as CalculateTempo, but try to get the bpm from the length too, and override
  // the passed beat tracker tempo with it when it's matching better.
  double CalculateTempoWithHeuristics(
    double&     Confidence,
    double      DetectedTempo,
    double      DetectedTempoConfidence,
    double      SampleDurationInSeconds,
    double      OnsetOffsetInSeconds,
    TOnsetType  OnsetType) const;
  // calculate average frequency from onset peaks
  double CalculateRhythmFrequencyMean(TOnsetType OnsetType) const;
  // calculate rhythm strenght detection from processed onsets
  double CalculateRhythmStrength(TOnsetType OnsetType) const;
  // calculate peak / valley ration detection from processed onsets
  double CalculateRhythmContrast(TOnsetType OnsetType) const;

private:
  double OnsetThreshold(TOnsetType OnsetType)const;

  double GuessNumberOfBeatsFromDuration(
    double  MinBpmExpected,
    double  MaxBpmExpected,
    double  DurationInSeconds) const;

  double OnsetMatchConfidence(
    const TList<double>&  Onsets,
    double                OnsetOffsetInSeconds,
    double                NumberOfBeats,
    double                Tempo,
    TOnsetType            OnsetType) const;

  void SharpenedOnsetValues(
    TList<double>& Onsets, 
    TOnsetType OnsetType) const;

  void CalculatePeaks(
    const TList<double>&  Onsets,
    TList<double>&        OnsetPeaks,
    TOnsetType            OnsetType) const;

  TOwnerPtr<TOnsetFftProcessor> mpOnsetFftProcessor;
  TStaticArray<TOwnerPtr<TOnsetDetector>, kNumberOfOnsetTypes> mpOnsetDetectors;
  
  TCannyWindow mWindow;

  TStaticArray<TList<double>, kNumberOfOnsetTypes> mOnsetValues;
  mutable TStaticArray<TList<double>, kNumberOfOnsetTypes> mSharpenedOnsetValues;

  const uint_t mSamplerate;
  const uint_t mHopSize;
  const uint_t mFftSize;
};

#endif // _RhythmTracker_h_

