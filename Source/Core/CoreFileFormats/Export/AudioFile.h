#pragma once

#ifndef _AudioFile_h_
#define _AudioFile_h_

// =================================================================================================

#include "CoreTypes/Export/Array.h"
#include "CoreTypes/Export/File.h"

#include "AudioTypes/Export/AudioTypes.h"

#include <atomic>

class TAudioStream;

// =================================================================================================

#define MSampleOverviewWidth 256
#define MSamplesPerCalcOverViewIdleCall (MSampleOverviewWidth * 100)

// =================================================================================================

class TAudioFile : public TReferenceCountable
{
public:

  // ===============================================================================================

  enum TSampleType
  {
    kInvalidSampleType = -1,
    
    k8BitUnsigned,      //        0 <-> (2<<8)
    k8BitSigned,        //  -(2<<7) <-> (2<<7) - 1 
    k16Bit,             // -(2<<14) <-> (2<<14) - 1 
    k24Bit,             // -(2<<22) <-> (2<<22) - 1 
    k32BitInt,          // -(2<<30) <-> (2<<30) - 1 
    k32BitFloat,        //   -1.0f  <->  1.0f      
    k64BitFloat,        //   -1.0   <->  1.0      
    
    kNumOfSampleTypes
  };

  // ===============================================================================================

  //! return a list of file extensions (for example "*.wav") that SCreateFromFile 
  // is "maybe" able to instantiate. "Maybe", because some system decoders may not 
  // be avilable to open the given file types.
  static TList<TString> SSupportedExtensions();

  //! return true if the given file might have an audio track within a video 
  //! container, or is an audio file. When set, the passed list will be filled 
  //! with information about multi audio tracks. The string is an identifier 
  //! for the track (for example the language name or title).
  static bool SContainsAudio(
    const TString&  FileNameAndPath, 
    TList<TString>* pContainedAudioTracks = NULL);
  
  //! finds an audio file class that can handle the given file and returns success.
  //! Passed pAudioFile will contain the loaded audio file or NULL if there is no 
  //! matching decoder installed or there was an error while opening the file. 
  //! In this case \param error message also is set
  static bool SCreateFromFile(
    TPtr<TAudioFile>& pAudioFile, 
    const TString&    FileNameAndPath,
    TString&          ErrorMessage);
  
  //! Set \param UseOrCreateDecompressedFile to true, if you want to decompress
  //! compressed formats to a WAV file first, or use them when they already exist
  //! The decompressed WAV files will be loaded/saved next to the original files.
  //! \param FileSubTrackIndex can be set to > 0 when there are more than one sound 
  //! tracks present in the audio file (see SContainsAudio's pContainedAudioTracks).
  static bool SCreateFromFile(
    TPtr<TAudioFile>& pAudioFile,
    const TString&    FileNameAndPath,
    int               FileSubTrackIndex,
    bool              UseOrCreateDecompressedFile,
    TString&          ErrorMessage);

  //! return number of bits needed per sample for the given sampletype
  static int SNumBitsFromSampleType(TSampleType SampleType);

  ~TAudioFile();

  //! return string like "WAV" or "AIFF", which shows which audiofile this is
  TString TypeString()const;


  //@{ ... open the file for read/write

  //! return true if open for reading or writing
  bool IsOpen()const;

  //! return the filename of this audio file, if we have one (file was opened)
  TString FileName()const;
  //! same as FileName(), but when this is a decompressed temp file, the original 
  //! file's name is returned instead of the decompressed temp file's name
  TString OriginalFileName()const;
  
  //! return the track index of this audio file, if we have one set (and the 
  //! file was opened for reading)
  int FileSubTrackIndex()const;
  //! same as FileSubTrackIndex(), but when this is a decompressed temp file, the 
  //! original file's track index is returned instead
  int OriginalFileSubTrackIndex()const;

  //! see OpenForReadAndDecompressIfNeeded: return a value between 0.0f and 1.0f
  float DecompressionProgress()const;
  //! see OpenForReadAndDecompressIfNeeded: cancel an already running decompression
  void CancelDecompression();
  
  //! @throw TReadableException
  void OpenForRead(
    const TString&  Filename, 
  int             FileSubTrackIndex = -1);
  
  //! @throw TReadableException
  //! Will, when this file is a compressed audio file create an uncompressed WAV file next 
  //! to the compressed file in WAV format, if possible.
  //! If we cant write next to the original file, we will save one into the temp dir
  //! If this file is uncompressed, or a previous call to OpenForReadAndDecompressIfNeeded
  //! already decompressed the file, we will reuse this file and do a simple OpenForRead
  void OpenForReadAndDecompressIfNeeded(
    const TString&  Filename, 
    int             FileSubTrackIndex = -1);
  
  //! @throw TReadableException
  void OpenForWrite(
    const TString&  Filename,
    int             SamplingRate  = 44100,
    TSampleType     SampleType    = k16Bit,
    int             NumChannels   = 2);

  //! close the file
  void Close();
  //@}

  
  //@{ ... Sample data stream

  //! Read write samples or sample buffers via TAudioStream 
  TAudioStream* Stream()const;

  //! a second independent stream that can be used parallel to the main stream
  TAudioStream* OverviewStream()const;
  //@}


  //@{ ... Basic properties

  int SamplingRate()const;
  int BitsPerSample()const;
  TSampleType SampleType()const;
  
  int NumChannels()const;
  long long NumSamples()const;
  //@}


  //@{ ... Optional properties

  //! all loop properties are only valid if HasLoopProperties returns true!
  bool HasLoopProperties()const;

  // TAudioTypes::kNoLoop implies that LoopStart&End are invalid!
  TAudioTypes::TLoopMode LoopMode()const;

  long long LoopStart()const;
  long long LoopEnd()const;

  //! setup loop points when (re)saving a file
  void SetLoop(
    TAudioTypes::TLoopMode  LoopMode, 
    long long               LoopStart, 
    long long               LoopEnd);

  //! get set a list of cue points present in the audio file
  TArray<long long> CuePoints()const;
  void SetCuePoints(const TArray<long long>& CuePoints);
  //@}

  
  //@{ ... Overview Data

  //! return true if we need to call CalcOverViewIdleTimer to calc the overview
  bool NeedCalcOverViewIdleTimer()const;
  
  //! returns the overview Progress from 0.0f - 1.0f
  float CalcOverViewIdleTimerProgress()const;
  
  //! Calculate a partition of the overview
  void CalcOverViewIdleTimer(
    long long& SamplePosStart, 
    long long& SamplePosEnd, 
    long long  NumSamples = MSamplesPerCalcOverViewIdleCall);
  

  bool HaveOverViewData(
    long long StartSampleFrame, 
    long long SampleFrameRange)const;

  void CalcOverViewData(
    long long StartSampleFrame, 
    long long SampleFrameRange);

  void OverViewMinMax(
    long long SampleFrameOffset, 
    long long SampleFrameRange, 
    short&    Min, 
    short&    Max);
  //@}

protected:
  TAudioFile();

  static float sImportProgress;
  static bool sCancelImport;

private:
  virtual TString OnTypeString()const = 0;

  virtual void OnOpenForRead(
    const TString&  Filename, 
    int             FileSubTrackIndex) = 0;
  
  virtual void OnOpenForWrite(
    const TString&  Filename,
    int             SamplingRate,
    TSampleType     SampleType,
    int             NumChannels) = 0;

  virtual void OnClose() = 0;

  virtual TAudioStream* OnStream()const = 0;
  virtual TAudioStream* OnOverviewStream()const = 0;

  virtual int OnSamplingRate()const = 0;
  virtual int OnBitsPerSample()const = 0;
  virtual TSampleType OnSampleType()const = 0;
  
  virtual int OnNumChannels()const = 0;
  virtual long long OnNumSamples()const = 0;

  virtual TAudioTypes::TLoopMode OnLoopMode()const = 0;
  virtual long long OnLoopStart()const = 0;
  virtual long long OnLoopEnd()const = 0;

  virtual void OnSetLoop(
    TAudioTypes::TLoopMode LoopMode, 
    long long               LoopStart, 
    long long               LoopEnd) = 0;

  virtual TArray<long long> OnCuePoints()const = 0;
  virtual void OnSetCuePoints(const TArray<long long>& CuePoints) = 0;

  int mCalculatingOverviewData;
  long long mOverViewDataWritePos;

  long long mReadOverViewBufferSamplePos;
  int mReadOverViewBufferSampleSize;

  struct TOverViewPoint
  {
    short mMin;
    short mMax;
  };

  TArray< TArray<TOverViewPoint> > mOverViewData;

  TArray<float> mTempOverViewBuffer;
  TArray<float*> mTempOverViewBufferPtrs;

  bool mIsOpen;

  TString mFileName;
  int mFileSubTrackIndex;
  
  TPtr<class TWaveFile> mpDecompressedWavFile;
  bool mDecompressedFileIsInTemp;
  
  bool mCancelDecompression;
  float mDecompressionProgress;
  std::atomic<bool> mDecompressing;
};

// =================================================================================================

class TAudioStream : public TReferenceCountable
{
public:
  int NumChannels()const;
  long long NumSamples()const;
  TAudioFile::TSampleType SampleType()const;

  //! return a blocksize at which this stream performs best when reading/writing
  int PreferedBlockSize()const;
  
  //! returns if anything audible was read and the CheckForAudibleSamples was 
  // set in ReadSamples calls  
  bool ReadSamplesAreAudible()const;
  //! returns if anything audible was written and the CheckForAudibleSamples was 
  // set in WriteSamples calls  
  bool WrittenSamplesAreAudible()const;

  //! when opened for writing, returning the number of !bytes! that we 
  //! have written so far
  long long BytesWritten()const;

  //! reposition the current read/write position. 
  //! Returns success - will not throw!
  bool SeekTo(long long SampleFrame);

  //! read a samplebuffer, converting from any number of src channels 
  //! and src format to a floatbuffer with a samplerange of shorts
  void ReadSamples(
    const TArray<float*>& DestChannelPtrs, 
    int                   NumberOfSamples,
    bool                  CheckForAudibleSamples = false);

  //! write a samplebuffer, converting from any number of src channels 
  //! and src format to a floatbuffer with a samplerange of shorts
  //! \param Dither is only used when writing in a non float dest format
  void WriteSamples(
    const TArray<const float*>& SrcChannelPtrs, 
    int                         NumberOfSamples, 
    bool                        Dither,
    bool                        CheckForAudibleSamples = false);

protected:
  TAudioStream(
    long long                NumSamples, 
    int                     NumChannels, 
    TAudioFile::TSampleType SampleType);

private:
  virtual int OnPreferedBlockSize()const = 0;
  
  virtual long long OnBytesWritten()const = 0;

  virtual void OnReadBuffer(
    void* pDestBuffer, 
    int   NumberOfSampleFrames) = 0;

  virtual void OnWriteBuffer(
    const void* pSrcBuffer, 
    int         NumberOfSampleFrames) = 0;

  virtual bool OnSeekTo(long long SampleIndex) = 0;

  long long mNumSamples;
  int mNumChannels;
  TAudioFile::TSampleType mSampleType;

  bool mReadSamplesAreAudible;
  bool mWrittenSamplesAreAudible;
  
  TArray<char> mTempBuffer;
};

// =================================================================================================

/*! 
 * AudioStream which reads content directly from a file 
!*/

class TAudioFileStream : public TAudioStream
{
public:
  // create a file stream, using an existing File handle.
  // NB: SampleDataByteOrder usually should be the File's current byte order, but 
  // can be set to something else to read the PCM data in a different order.
  TAudioFileStream(
    TFile*                  pFile,
    long long               SampleDataOffset,
    long long               NumSamples,
    int                     NumChannels,
    TAudioFile::TSampleType SampleType,
    TByteOrder::TByteOrder  SampleDataByteOrder);

private:
  
  //@{ ... Overridden from TAudioStream
  
  virtual int OnPreferedBlockSize()const;

  virtual long long OnBytesWritten()const;

  virtual bool OnSeekTo(long long Frame);

  virtual void OnReadBuffer(
    void* pDestBuffer, 
    int   NumberOfSampleFrames);

  virtual void OnWriteBuffer(
    const void* pSrcBuffer, 
    int         NumberOfSampleFrames);
  //@}
  
  TFile* mpFile;

  long long mSampleDataOffset;
  long long mBytesWritten;

  TByteOrder::TByteOrder mSampleDataByteOrder;
};


#endif // _AudioFile_h_

