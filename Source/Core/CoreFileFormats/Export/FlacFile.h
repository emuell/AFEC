#pragma once

#ifndef FlacFile_h
#define FlacFile_h

// =================================================================================================

#include "CoreFileFormats/Export/AudioFile.h"
#include "CoreFileFormats/Export/WaveFile.h"

class TFlacDecoderFile;
class TFlacEncoderFile;

// =================================================================================================

class TFlacFile : public TAudioFile
{
public:
  //! return a list of fileextensions that are supported by FLAC
  static TList<TString> SSupportedExtensions();

  //! return the sampletype that FLAC needs for the given bit depth 
  static TAudioFile::TSampleType SSampleTypeFromBitDepth(int BitDepth);

  //! returns true, if we can read/write FLAC files with the given sample rate
  static bool SIsSampleRateSupported(int SampleRate);
  //! returns true, if we can read/write FLAC files with the given sample depth
  static bool SIsBitDepthSupported(int BitDepths);
  
  TFlacFile();

private:
  virtual TString OnTypeString()const;

  //! @throw TReadableException
  virtual void OnOpenForRead(
    const TString&  FileName, 
    int             FileSubTrackIndex);
  
  //! @throw TReadableException
  virtual void OnOpenForWrite(
    const TString&  Filename,
    int             SamplingRate,
    TSampleType     SampleType,
    int             NumChannels);

  virtual void OnClose();

  virtual TAudioStream* OnStream()const;
  virtual TAudioStream* OnOverviewStream()const;
  
  virtual int OnSamplingRate()const;
  virtual int OnBitsPerSample()const;
  virtual TSampleType OnSampleType()const;
  
  virtual int OnNumChannels()const;
  virtual long long OnNumSamples()const;

  virtual TAudioTypes::TLoopMode OnLoopMode()const;

  virtual long long OnLoopStart()const;
  virtual long long OnLoopEnd()const;
  
  virtual void OnSetLoop(
    TAudioTypes::TLoopMode LoopMode, 
    long long              LoopStart, 
    long long              LoopEnd);

  virtual TArray<long long> OnCuePoints()const;
  virtual void OnSetCuePoints(const TArray<long long>& CuePoints);

  TPtr<TFlacDecoderFile> mpDecoderFile;
  TPtr<TFlacDecoderFile> mpDecoderOverviewFile;
  
  TPtr<TFlacEncoderFile> mpEncoderFile;
  
  TPtr<TAudioStream> mpStream;
  TPtr<TAudioStream> mpOverviewStream;

  TSamplerChunk mRiffSamplerChunk;
  TSampleLoop mRiffSampleLoop;

  TFile mFile;
  TFile mOverviewFile;
};


#endif 

