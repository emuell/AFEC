#pragma once

#ifndef OggVorbisFile_h
#define OggVorbisFile_h

// =================================================================================================

#ifndef MNoOggVOrbis

#include "CoreFileFormats/Export/AudioFile.h"

class TOggVorbisDecoderFile;

// =================================================================================================

class TOggVorbisFile : public TAudioFile
{
public:
  //! return a list of fileextensions that are supported by TOggVorbisFile
  static TList<TString> SSupportedExtensions();

  TOggVorbisFile();

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

  TPtr<TOggVorbisDecoderFile> mpDecoderFile;
  TPtr<TOggVorbisDecoderFile> mpDecoderOverviewFile;
  
  TPtr<TAudioStream> mpStream;
  TPtr<TAudioStream> mpOverviewStream;

  TFile mFile;
  TFile mOverviewFile;
};


#endif //!defined(MNoOggVOrbis)

#endif // OggVorbisFile_h

