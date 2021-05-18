#pragma once

#ifndef AifFile_h
#define AifFile_h

// =================================================================================================

#include "CoreTypes/Export/Array.h"

#include "CoreFileFormats/Export/AudioFile.h"
#include "CoreFileFormats/Export/RiffFile.h"

typedef char TIeee80[10];

// =================================================================================================

struct TAifCommChunk
{
  TAifCommChunk();
  bool VerifyValidity()const;

  void Read(TFile* pFile);
  void Write(TFile* pFile);

  TInt16 mChannels;
  TUInt32 mSampleFrames;
  TInt16 mSampleBits;
  TIeee80 mSampleFreq;
};

// =================================================================================================

struct TAifcCommChunkExt
{
  TAifcCommChunkExt();
  
  void Read(TFile* pFile);
  void Write(TFile* pFile);

  bool VerifyValidity()const;

  TUInt32 mCompressionType;
  TArray<char> mCompressionName; 
};

// =================================================================================================

struct TAifSndChunk
{
  TAifSndChunk();

  void Read(TFile* pFile);
  void Write(TFile* pFile);

  TUInt32 mOffset;
  TUInt32 mBlockSize;
};
  
// =================================================================================================

class TAifFile : public TAudioFile, public TRiffFile
{
public:
  //! return a list of file extensions that are supported by AWavFile
  static TList<TString> SSupportedExtensions();

  TAifFile();
    
private:
  virtual TString OnTypeString()const;

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
  
  virtual int OnNumChannels()const;
  virtual long long OnNumSamples()const;
  virtual TSampleType OnSampleType()const;
  
  virtual TAudioTypes::TLoopMode OnLoopMode()const;
  virtual long long OnLoopStart()const;
  virtual long long OnLoopEnd()const;

  virtual void OnSetLoop(
    TAudioTypes::TLoopMode LoopMode, 
    long long               LoopStart, 
    long long               LoopEnd);

  virtual TArray<long long> OnCuePoints()const;
  virtual void OnSetCuePoints(const TArray<long long>& CuePoints);

  virtual bool OnIsParentChunk(const TString &FourccName);

  TAifCommChunk mCommChunk;
  TAifcCommChunkExt mCommChunkExt;
  TAifSndChunk mSndChunk;

  long long mSampleDataOffset;
  long long mNumOfSamples;

  TPtr<TAudioFileStream> mpStream;
  TPtr<TAudioFileStream> mpOverviewStream;
};


#endif 

