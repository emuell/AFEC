#pragma once

#ifndef _WinMediaFoundationAudioFile_h
#define _WinMediaFoundationAudioFile_h

// =================================================================================================

#include "CoreFileFormats/Export/AudioFile.h"

// NB: avoid including windows.h here - it's messing up winsock.h includes

struct IMFSourceReader;
struct IMFMediaType;
struct IMFMediaSource;
struct tWAVEFORMATEX;

class TMediaFoundationAudioDecoder;

// =================================================================================================

class TMediaFoundationAudioFile : public TAudioFile
{
public:
  //! return a list of file extensions (for example "*.mp3") that 
  //! TMediaFoundationAudioFile "maybe" is able to load
  static TList<TString> SSupportedExtensions();
 
  //! return true when the given file is an audio file (see SSupportedExtensions)
  //! or video file and has an audio stream that we maybe can read
  static bool SContainsAudio(
    const TString&  FileNameAndPath,
    TList<TString>* pContainedAudioTracks);

  TMediaFoundationAudioFile();
  
private:
  
  //@{ TAudioFile implementation

  virtual TString OnTypeString()const;

  virtual void OnOpenForRead(
    const TString&  Filename,
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
  //@}

  TPtr<TMediaFoundationAudioDecoder> mpDecoderFile;
  TPtr<TMediaFoundationAudioDecoder> mpOverviewDecoderFile;
    
  TPtr<TAudioStream> mpStream;
  TPtr<TAudioStream> mpOverviewStream;
};

// =================================================================================================

/*!
 * Decodes audio files or audio streams within video files, using the 
 * Windows Media Foundation API.
!*/

class TMediaFoundationAudioDecoder : public TReferenceCountable
{
public:
  TMediaFoundationAudioDecoder(const TString& FileName);
  ~TMediaFoundationAudioDecoder();

  //! returns true when successfully opened
  bool IsOpen()const;
  //! @throw TReadableException
  void Open();
  //! NB: should be called in the thread that we got our open call from...
  void Close();

  long long TotalFrameCount()const;
  
  int SamplesPerSec()const;
  TAudioFile::TSampleType SampleType()const;
  int NumberOfChannels()const;
  int BitsPerSample()const;
  
  int BlockSize()const;

  bool SeekTo(long long SampleFrame);
  void ReadBuffer(void* pData, int NumSampleFrames);

private:
  // overridden from TReferenceCountable
  // important to release the COM objects in the thread they got created
  virtual bool OnShouldBeDeletedInGui()const { return false; }

  // Decoder initialization
  long ConfigureAudioStream();
  
  // Convert a 100ns Media Foundation value to a frame offset.
  long long FrameFromMF(long long mf) const;
  // Convert a frame offset to a 100ns Media Foundation value.
  long long MFFromFrame(long long frame) const;

  TString mFileName;
  
  bool mInitializedCom;
  bool mInitializedMF;

  IMFSourceReader *mpSourceReader;
  tWAVEFORMATEX *mpPcmFormat;

  bool mSourceReaderDied;
  int mPreferedBlockSize;
  long long mStreamDuration;

  void* mpSampleBuffer;
  int mSampleBufferSizeInBytes;
  int mSampleBufferBytesWritten;
};


#endif // _WinMediaFoundationAudioFile_h

