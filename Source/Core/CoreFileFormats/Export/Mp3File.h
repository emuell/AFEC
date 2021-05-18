#pragma once

#ifndef TMp3File_h
#define TMp3File_h

// =================================================================================================

#ifndef MNoMp3LibMpg

#include "CoreFileFormats/Export/AudioFile.h"

class TMp3DecoderFile;

// =================================================================================================

class TMp3File : public TAudioFile
{
public:
  //! return a list of fileextensions that are supported by TMp3File
  static TList<TString> SSupportedExtensions();

  TMp3File();

  //! Init/Exit statics
  static void SInit();
  static void SExit();

  static bool SIsSampleRateSupported(int SampleRate);
  static bool SIsBitDepthSupported(int BitDepths);

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

  TPtr<TMp3DecoderFile> mpDecoderFile;
  TPtr<TMp3DecoderFile> mpDecoderOverviewFile;

  TPtr<TAudioStream> mpStream;
  TPtr<TAudioStream> mpOverviewStream;

  TFile mFile;
  TFile mOverviewFile;
};


#endif //!defined(MNoMp3LibMpg)

#endif // TMp3File_h

