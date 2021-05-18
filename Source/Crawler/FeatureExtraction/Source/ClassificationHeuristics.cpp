#include "AudioTypes/Export/AudioMath.h"

#include "FeatureExtraction/Export/SampleDescriptors.h"
#include "FeatureExtraction/Export/Statistics.h"

#include "FeatureExtraction/Source/ClassificationHeuristics.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool TClassificationHeuristics::IsOneShot(
  const TSampleDescriptors& LowLevelDescriptors,
  double&                   Confidence) 
{
  // . Sort out very short sounds first

  const double EffectiveLength = 
    LowLevelDescriptors.mEffectiveLength24dB.mValue;

  // anything below half a second size, definitely isn't a loop
  if (EffectiveLength < 0.5) 
  {
    Confidence = 0.85;
    return true;
  }
  // same is true for sounds < second, when there is just one or two percussive onsets
  else if (EffectiveLength < 1 && LowLevelDescriptors.mRhythmPercussiveOnsetCount.mValue <= 2)
  {
    Confidence = 0.75;
    return true;
  }
  

  // . LengthConfidence (shorter < 5 seconds = more likely a OneShot)

  // calc base confidence from effective length, cube root compressed
  const double LengthConfidence = 
    std::pow(1.0 - (std::min(4.0, std::max(0.0, EffectiveLength - 1.0)) / 4.0), 0.5); 

  
  // . EnvelopeConfidence (Peak envelope correlates to FadeOut/FadeIn 
  //     envelope = more likely a OneShot)

  const TList<double>& PeakValues = LowLevelDescriptors.mAmplitudePeak.mValues;
  const int NumberOfPeakFrames = PeakValues.Size();

  // ignore leading + tail silence < 24dB in peak frames
  static const double sSilenceFloor = TAudioMath::DbToLin(-24.0);
  
  int SilentLeadingFrames = 0;
  for (int f = 0; f < NumberOfPeakFrames; ++f, ++SilentLeadingFrames) 
  {
    if (PeakValues[f] > sSilenceFloor) 
    {
      break;
    }
  }

  int SilentTrailingFrames = 0;
  for (int f = NumberOfPeakFrames - 1; f > SilentLeadingFrames; --f, ++SilentTrailingFrames) 
  {
    if (PeakValues[f] > sSilenceFloor) 
    {
      break;
    }
  }

  // create a stripped peak envelope
  const int TotalSilentFrames = SilentLeadingFrames + SilentTrailingFrames;

  TArray<double> Envelope(NumberOfPeakFrames - TotalSilentFrames); 
  for (int i = 0; i < Envelope.Size(); ++i) 
  {
    Envelope[i] = PeakValues[i + SilentLeadingFrames];
  }

  // create a dummy decay peak envelope to calculate correlation
  TArray<double> FadeOutEnvelope(Envelope.Size());
  for (int i = 0; i < Envelope.Size(); ++i) 
  {
    FadeOutEnvelope[i] = std::pow(1.0 - (double)i / (double)(Envelope.Size() - 1), 4.0);
  }
  
  // calc cross correlation from sample's envelope with the fade out envelpe 
  const double EnvelopeCorrelation = TStatistics::Correlation(
    FadeOutEnvelope.FirstRead(), Envelope.FirstRead(), Envelope.Size());

  // treat negative correlation (Fade-Ins) as a good OneShot match too 
  const double EnvelopeConfidence = std::min(1.0, std::abs(EnvelopeCorrelation)); 

  
  // . combine both confidences, giving the envelope more weight
  
  Confidence = LengthConfidence * 0.3 + EnvelopeConfidence * 0.7;

  return (Confidence > 0.7);
}

// -------------------------------------------------------------------------------------------------

bool TClassificationHeuristics::IsLoop(
  const TSampleDescriptors& LowLevelDescriptors,
  double&                   Confidence) 
{
  // . Skip when there are less than 8 (percussive) onsets

  if (LowLevelDescriptors.mRhythmPercussiveOnsetCount.mValue < 8) 
  {
    Confidence = 0.0;
    return false;
  }


  // . Skip when the input is highly temporarily steady (sorts out low frequency wavetable sounds)

  if (LowLevelDescriptors.mSpectralFlux.mMean > 0.9)
  {
    Confidence = 0.0;
    return false;
  }


  // . LengthConfidence (longer 1 seconds = more likely a Loop)

  const double LengthConfidence = std::pow(std::max(0.0, std::min(4.0, 
    LowLevelDescriptors.mEffectiveLength24dB.mValue - 1.0) / 4.0), 0.5); 

  
  // . RhythmConfidence (more stable BPM = more likely a Loop)

  double RhythmConfidence = 0.0;
  if (LowLevelDescriptors.mRhythmPercussiveTempoConfidence.mValue > 0.25 &&
      LowLevelDescriptors.mRhythmComplexTempoConfidence.mValue > 0.25) 
  {
    // first ensure that both, amplitude and spectrum flux based rhythm onset detection
    // agreed on that the sample somehow has a tempo, but use the "simpler" 
    // PercussiveTempoConfidence for our isLoop confidence measure
    RhythmConfidence = std::min(1.0, 
      LowLevelDescriptors.mRhythmPercussiveTempoConfidence.mValue * 2.0); 
  }

  
  // . combine confidences, giving RhythmConfidence more weight
  
  Confidence = LengthConfidence * 0.3 + RhythmConfidence * 0.7;

  return (Confidence > 0.7);
}

