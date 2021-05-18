#pragma once

#ifndef _MacCoreAudioFile_h
#define _MacCoreAudioFile_h

// =================================================================================================

#if defined(MMac)

#include "CoreFileFormats/Export/AudioFile.h"

// avoid including <AudioToolBox.h> here:
typedef struct OpaqueExtAudioFile* ExtAudioFileRef; 

class TCoreAudioAudioDecoder;

// =================================================================================================

/*!
 * A wrapper around not only ".caf" core audio files, but actually everything
 * that the AudioToolbox files wrappers on the Mac can load (mp3, aac, caf 
 * and more). 
 * 
 * Only decoding (reading) is supported at the moment.
!*/

class TCoreAudioAudioFile : public TAudioFile
{
public:
  static TList<TString> SSupportedExtensions();

  TCoreAudioAudioFile();
  
private:
  virtual TString OnTypeString()const;

  virtual void OnOpenForRead(
    const TString&  FileName, 
    int             FileSubTrackIndex);
  
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

  TPtr<TCoreAudioAudioDecoder> mpDecoderFile;
  TPtr<TCoreAudioAudioDecoder> mpDecoderOverviewFile;
    
  TPtr<TAudioStream> mpStream;
  TPtr<TAudioStream> mpOverviewStream;
};

// =================================================================================================

/*!
 * Decodes an audio file (or movie) on request (via ReadBuffer calls)
!*/

class TCoreAudioAudioDecoder : public TReferenceCountable
{
public:
  TCoreAudioAudioDecoder(const TString& FileName);
  ~TCoreAudioAudioDecoder();

  //! returns true when successfully opened
  bool IsOpen()const;
  //! @throw TReadableException
  void Open();
  //! called in the thread that we got our open call from...
  void Close();

  long long TotalNumberOfSamples()const;
  int NumberOfChannels()const;
  int NumberOfBits()const;
  
  TAudioFile::TSampleType SampleType()const;

  int SampleRate()const;
  int BlockSize()const;

  bool SeekTo(long long SampleFrame);
  void ReadBuffer(void* pData, int NumSampleFrames);

private:
  TString mFileName;

  long long mTotalNumberOfSamples;
  int mSampleRate;
  int mNumberOfBits;
  int mNumberOfChannels;
  
  int mPreferredBlockSize;
  
  long long mBufferSampleFramePos;

  ExtAudioFileRef mpExtAudioFileRef;
};


#endif // defined(MMac)

#endif // _MacCoreAudioFile_h

