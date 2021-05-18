#pragma once

#ifndef _WaveFile_h_
#define _WaveFile_h_

// =================================================================================================

#include "CoreFileFormats/Export/AudioFile.h"
#include "CoreFileFormats/Export/RiffFile.h"

// =================================================================================================

struct TWaveFormatChunkData
{
  // get corresponding TSampleType for the given WAV format properties
  static TAudioFile::TSampleType SSampleType(
    TUInt16 FormatTag, 
    TUInt16 BitsPerSample);

  TWaveFormatChunkData();

  void Config(
    TAudioFile::TSampleType SampleType,
    int                     NewSamplingRate,
    int                     NewNumChannels);
  
  void Read(TFile* pFile);
  void Write(TFile* pFile);

  // Format category (PCM=1)
  TUInt16 mFormatTag;
  // Number of channels (mono=1, stereo=2)
  TUInt16 mChannels;
   
  // Sampling rate [Hz]
  TUInt32 mSampleRate;
  TUInt32 mAvgBytesPerSec;
   
  TUInt16 mBlockAlign;
  TUInt16 mBitsPerSample;
};

// =================================================================================================

struct TWaveFormatChunk
{
  TWaveFormatChunk();
  bool VerifyValidity();
  
  TRiffChunkHeader mHeader;
  TWaveFormatChunkData mData;
};

// =================================================================================================

struct TSampleLoop
{
  TSampleLoop();

  void Read(TFile* pFile);
  void Write(TFile* pFile);

  char mIdentifier[4];
  
  TUInt32 mType;
  TUInt32 mStart;
  TUInt32 mEnd;
  TUInt32 mFraction;
  TUInt32 mPlayCount;
};
  
// =================================================================================================

struct TSamplerChunk 
{
  TSamplerChunk();

  void Read(TFile* pFile);
  void Write(TFile* pFile);

  TInt32 mManufacturer;
  TInt32 mProduct;
  TInt32 mSamplePeriod;
  TInt32 mMIDIUnityNote;
  TInt32 mMIDIPitchFraction;
  TInt32 mSMPTEFormat;
  TInt32 mSMPTEOffset;
  TInt32 mSampleLoops;
  TInt32 mSamplerData;
};

// =================================================================================================

struct TCuePoint
{
  TCuePoint();

  void Read(TFile* pFile);
  void Write(TFile* pFile);

  TInt32 mUniqueId;
  TInt32 mPosition;
  TInt32 mDataChunkId;
  TInt32 mChunkStart;
  TInt32 mBlockStart;
  TInt32 mSampleOffset;
};

// =================================================================================================

struct TCueChunk 
{
  TCueChunk();

  void Read(TFile* pFile);
  void Write(TFile* pFile);

  TUInt32 mNumCuePoints;
  // TCuePoint[] mCuePoints;
};

// =================================================================================================

class TWaveFile: public TAudioFile, public TRiffFile
{
public:
  //! return a list of file extensions that are supported by AWavFile
  static TList<TString> SSupportedExtensions();

  //! return the sampletype that WAV needs for the given bit depth
  //! set UseFloat to true, if you prefer using float formats for 32 or 64 bits 
  static TAudioFile::TSampleType SSampleTypeFromBitDepth(
    int BitDepth, 
    bool PrefereFloatFormats);
  
  TWaveFile();
      
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

  virtual bool OnIsParentChunk(const TString &FourccName);

  TRiffChunkHeader mPcmDataHeader;
  TWaveFormatChunk mWaveFormat;
  
  TSamplerChunk mSamplerChunk;
  TSampleLoop mSampleLoop;

  TCueChunk mCueChunk;
  TArray<TCuePoint> mCuePoints;

  // where in the file the sampledata begins
  long long mSampleDataChunkOffset;  
  long long mNumOfSamples;

  TPtr<TAudioFileStream> mpStream;
  TPtr<TAudioFileStream> mpOverviewStream;
};


#endif // _WaveFile_h_

