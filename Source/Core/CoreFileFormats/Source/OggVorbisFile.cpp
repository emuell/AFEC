#include "CoreFileFormatsPrecompiledHeader.h"

#if !defined(MNoOggVorbis)

#include "CoreTypes/Export/ByteOrder.h"
#include "../../3rdParty/OggVorbis/Export/OggVorbis.h"

// =================================================================================================

#define MOggVorbisBlockSize 4096
#define MOggVorbisDecoderBitDepth 16

// =================================================================================================

class TOggVorbisDecoderFile : public TReferenceCountable
{
public:
  TOggVorbisDecoderFile(TFile* pFile);
  ~TOggVorbisDecoderFile();

  //! @throw TReadableException
  void Open();

  long long TotalNumberOfSamples()const;
  int NumberOfChannels()const;
  int NumberOfBits()const;
  
  TAudioFile::TSampleType SampleType()const;

  int SampleRate()const;
  
  int BlockSize()const;

  bool SeekTo(long long SampleFrame);
  void ReadBuffer(void* pData, int NumSampleFrames);

private:
  static size_t SReadCallback(void *ptr, size_t size, size_t nmemb, void *datasource);
  static int SSeekCallback(void *datasource, ogg_int64_t offset, int whence);
  static int SCloseCallback(void *datasource);
  static long STellCallback(void *datasource);

  TFile* mpFile;

  long long mTotalNumberOfSamples;
  int mSampleRate;
  int mNumberOfBits;
  int mNumberOfChannels;
  
  long long mBufferSampleFramePos;

  OggVorbis_File mVorbisFile;
};

// =================================================================================================

class TOggVorbisFileStream : public TAudioStream
{
public:
  TOggVorbisFileStream(TOggVorbisDecoderFile* pOggVorbisFile);
  
private:
  virtual int OnPreferedBlockSize()const;

  virtual long long OnBytesWritten()const;

  virtual bool OnSeekTo(long long Frame);

  virtual void OnReadBuffer(
    void* pDestBuffer, 
    int   NumberOfSampleFrames);

  virtual void OnWriteBuffer(
    const void* pSrcBuffer, 
    int         NumberOfSampleFrames);

  TOggVorbisDecoderFile* mpOggVorbisDecoderFile;
  
  int mBytesWritten;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TOggVorbisDecoderFile::TOggVorbisDecoderFile(TFile* pFile)
  : mpFile(pFile),
    mTotalNumberOfSamples(0),
    mSampleRate(0),
    mNumberOfBits(0),
    mNumberOfChannels(0),
    mBufferSampleFramePos(0)
{
  TMemory::Zero(&mVorbisFile, sizeof(mVorbisFile));

  MAssert(pFile->IsOpen(), "File must be open!");
}

// -------------------------------------------------------------------------------------------------

TOggVorbisDecoderFile::~TOggVorbisDecoderFile()
{
  if (mVorbisFile.ready_state)
  {
    ::ov_clear(&mVorbisFile);
  }
}

// -------------------------------------------------------------------------------------------------

void TOggVorbisDecoderFile::Open() 
{
  ov_callbacks Callbacks;
  Callbacks.read_func = SReadCallback;
  Callbacks.seek_func = SSeekCallback;
  Callbacks.close_func = SCloseCallback;
  Callbacks.tell_func = STellCallback;

  if (::ov_open_callbacks(this, &mVorbisFile, NULL, 0, Callbacks) < 0) 
  {
    throw TReadableException(MText("File does not appear to be an OGG bitstream."));
  }

  vorbis_info* pVorbisInfo = ::ov_info(&mVorbisFile, -1);

  if (pVorbisInfo == NULL)
  {
    throw TReadableException(MText("There was an unknown error while opening the OGG file."));
  }

  mTotalNumberOfSamples = ::ov_pcm_total(&mVorbisFile, -1);
  // NB: bitrates-sizes in vorbis header are only hints, so we can't use them as block size
  mSampleRate = (int)pVorbisInfo->rate;
  mNumberOfBits = MOggVorbisDecoderBitDepth;
  mNumberOfChannels = pVorbisInfo->channels;
}

// -------------------------------------------------------------------------------------------------

long long TOggVorbisDecoderFile::TotalNumberOfSamples()const 
{ 
  return mTotalNumberOfSamples; 
}

// -------------------------------------------------------------------------------------------------

int TOggVorbisDecoderFile::NumberOfChannels()const 
{ 
  return mNumberOfChannels; 
}

// -------------------------------------------------------------------------------------------------

int TOggVorbisDecoderFile::SampleRate()const 
{ 
  return mSampleRate; 
}

// -------------------------------------------------------------------------------------------------

int TOggVorbisDecoderFile::BlockSize()const
{
  return MOggVorbisBlockSize;
}

// -------------------------------------------------------------------------------------------------

int TOggVorbisDecoderFile::NumberOfBits()const 
{ 
  return mNumberOfBits; 
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TOggVorbisDecoderFile::SampleType()const
{
  MStaticAssert(MOggVorbisDecoderBitDepth == 16);
  return TAudioFile::k16Bit;
}

// -------------------------------------------------------------------------------------------------

bool TOggVorbisDecoderFile::SeekTo(long long SampleFrame)
{
  MAssert(mVorbisFile.ready_state, "Open was not called!");
  
  if (mBufferSampleFramePos == SampleFrame)
  {
    // no relocation needed
    return true;
  }

  if (::ov_pcm_seek(&mVorbisFile, SampleFrame) == 0)
  {
    mBufferSampleFramePos = SampleFrame;
    return true;
  }
  
  return false;
}

// -------------------------------------------------------------------------------------------------

void TOggVorbisDecoderFile::ReadBuffer(void* pData, int NumSampleFrames)
{
  MAssert(mVorbisFile.ready_state, "Open was not called!");

  const int TotalNumBytesInBuffer = NumSampleFrames * 
    ((mNumberOfBits + 7) / 8) * mNumberOfChannels; 
  
  #if (MSystemByteOrder == MIntelByteOrder)
    const int BigEndian = 0;
  #else
    const int BigEndian = 1;
  #endif
  
  const int BytesPerSample = ((mNumberOfBits + 7) / 8);
  const int Signed = 1;
  int Bitstream = 0;

  int TotalSamplesRead = 0;
  int BufferOffset = 0;

  // make sure that we are seeked to the correct samplepos (in case of an stream error)
  if (mVorbisFile.pcm_offset != mBufferSampleFramePos)
  {
    ::ov_pcm_seek(&mVorbisFile, mBufferSampleFramePos);
  }

  while (TotalSamplesRead < NumSampleFrames)
  {
    const int NumBytesToDecode = (NumSampleFrames - TotalSamplesRead) * 
      BytesPerSample * mNumberOfChannels;

    const int BytesRead = (int)::ov_read(&mVorbisFile, (char*)pData + BufferOffset, 
      NumBytesToDecode, BigEndian, BytesPerSample, Signed, &Bitstream);
      
    if (BytesRead == 0) 
    {
      MInvalid("Unexpected End of File");
      TMemory::Zero(pData, TotalNumBytesInBuffer);
      break;
    } 
    else if (BytesRead < 0) 
    {
      MInvalid("Error in the OGG stream");
      TMemory::Zero(pData, TotalNumBytesInBuffer);
      break;
    } 
    else 
    {
      // all fine...
      MAssert(BytesRead % (BytesPerSample * mNumberOfChannels) == 0, "");
      TotalSamplesRead += BytesRead / BytesPerSample / mNumberOfChannels;
      BufferOffset += BytesRead;
    }
  }

  mBufferSampleFramePos += NumSampleFrames;
}

// -------------------------------------------------------------------------------------------------

size_t TOggVorbisDecoderFile::SReadCallback(void* ptr, size_t size, size_t nmemb, void *datasource)
{
  TOggVorbisDecoderFile* pThis = (TOggVorbisDecoderFile*)datasource;
  
  size_t BytesRead = size*nmemb;
  
  if (pThis->mpFile->ReadBytes((char*)ptr, &BytesRead))
  {
    return BytesRead / size;
  }
  else
  {
    return 0;
  }
}

// -------------------------------------------------------------------------------------------------

int TOggVorbisDecoderFile::SSeekCallback(void *datasource, ogg_int64_t offset, int whence)
{
  TOggVorbisDecoderFile* pThis = (TOggVorbisDecoderFile*)datasource;

  switch(whence)
  {
  default: MInvalid("");
  case SEEK_SET:
    if (pThis->mpFile->SetPosition((int)offset))
    {
      return 0;
    }
    else
    {
      return -1;
    }

  case SEEK_END:
    if (pThis->mpFile->SetPosition((int)(pThis->mpFile->SizeInBytes() - offset)))
    {
      return 0;
    }
    else
    {
      return -1;
    }

  case SEEK_CUR:
    if (pThis->mpFile->SetPosition((int)(pThis->mpFile->Position() + offset)))
    {
      return 0;
    }
    else
    {
      return -1;
    }
    break;
  }
}

// -------------------------------------------------------------------------------------------------

int TOggVorbisDecoderFile::SCloseCallback(void *datasource)
{
  // do nothing (the OggFile takes care about closing the file)
  return 0;
}

// -------------------------------------------------------------------------------------------------

long TOggVorbisDecoderFile::STellCallback(void *datasource)
{
  TOggVorbisDecoderFile* pThis = (TOggVorbisDecoderFile*)datasource;
  return (long)pThis->mpFile->Position();
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TOggVorbisFileStream::TOggVorbisFileStream(TOggVorbisDecoderFile* pOggVorbisFile)
  : TAudioStream((int)pOggVorbisFile->TotalNumberOfSamples(), 
      pOggVorbisFile->NumberOfChannels(), pOggVorbisFile->SampleType()),
    mpOggVorbisDecoderFile(pOggVorbisFile),
    mBytesWritten(0)
{
}

// -------------------------------------------------------------------------------------------------

int TOggVorbisFileStream::OnPreferedBlockSize()const
{
  return mpOggVorbisDecoderFile->BlockSize();
}

// -------------------------------------------------------------------------------------------------

long long TOggVorbisFileStream::OnBytesWritten()const
{
  return mBytesWritten;
}

// -------------------------------------------------------------------------------------------------

bool TOggVorbisFileStream::OnSeekTo(long long Frame)
{
  return mpOggVorbisDecoderFile->SeekTo(Frame);
}

// -------------------------------------------------------------------------------------------------

void TOggVorbisFileStream::OnReadBuffer(
  void* pDestBuffer, 
  int   NumberOfSampleFrames)
{
  mpOggVorbisDecoderFile->ReadBuffer(pDestBuffer, NumberOfSampleFrames);
}

// -------------------------------------------------------------------------------------------------

void TOggVorbisFileStream::OnWriteBuffer(
  const void* pSrcBuffer, 
  int         NumberOfSampleFrames)
{
  MCodeMissing;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TList<TString> TOggVorbisFile::SSupportedExtensions()
{
  TList<TString> Ret;
  Ret.Append("*.ogg");

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TOggVorbisFile::TOggVorbisFile()
  : TAudioFile()
{
}

// -------------------------------------------------------------------------------------------------

TString TOggVorbisFile::OnTypeString()const
{
  return "OGG";
}

// -------------------------------------------------------------------------------------------------

void TOggVorbisFile::OnOpenForRead(
  const TString&  FileName, 
  int             FileSubTrackIndex)
{
  MUnused(FileSubTrackIndex); // not implemented

  mFile.SetFileName(FileName);
  mOverviewFile.SetFileName(FileName);
  
  if (!mFile.Open(TFile::kRead) || !mOverviewFile.Open(TFile::kRead))
  {
    throw TReadableException(MText("Failed to open the file for reading."));
  }
    
  mpDecoderFile = new TOggVorbisDecoderFile(&mFile);
  mpDecoderFile->Open();
  mpStream = new TOggVorbisFileStream(mpDecoderFile);  

  // the overview file and stream is created when needed
}

// -------------------------------------------------------------------------------------------------

void TOggVorbisFile::OnOpenForWrite(
  const TString&  Filename,
  int             SamplingRate,
  TSampleType     SampleType,
  int             NumChannels)
{
  throw TReadableException(
    MText("Encoding OGG vorbis files is currently not supported."));
}

// -------------------------------------------------------------------------------------------------

void TOggVorbisFile::OnClose()
{
  mpStream.Delete();
  mpOverviewStream.Delete();
  
  mpDecoderFile.Delete();
  mpDecoderOverviewFile.Delete();

  if (mFile.IsOpen())
  {
    mFile.Close();
  }

  if (mOverviewFile.IsOpen())
  {
    mOverviewFile.Close();
  }
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TOggVorbisFile::OnStream()const
{
  return mpStream;
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TOggVorbisFile::OnOverviewStream()const
{
  if (!mpOverviewStream)
  {
    TOggVorbisFile* pMutableThis = const_cast<TOggVorbisFile*>(this);
    
    pMutableThis->mpDecoderOverviewFile = 
      new TOggVorbisDecoderFile(&pMutableThis->mOverviewFile);
    
    pMutableThis->mpDecoderOverviewFile->Open();

    pMutableThis->mpOverviewStream = 
      new TOggVorbisFileStream(pMutableThis->mpDecoderOverviewFile);
  }

  return mpOverviewStream;
}

// -------------------------------------------------------------------------------------------------

int TOggVorbisFile::OnSamplingRate()const
{
  return mpDecoderFile->SampleRate();
}

// -------------------------------------------------------------------------------------------------

int TOggVorbisFile::OnBitsPerSample()const
{
  return mpDecoderFile->NumberOfBits();
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TOggVorbisFile::OnSampleType()const
{
  return mpDecoderFile->SampleType();
}

// -------------------------------------------------------------------------------------------------

int TOggVorbisFile::OnNumChannels()const
{
  return mpDecoderFile->NumberOfChannels();
}

// -------------------------------------------------------------------------------------------------

long long TOggVorbisFile::OnNumSamples() const
{
  return mpDecoderFile->TotalNumberOfSamples();
}

// -------------------------------------------------------------------------------------------------

TAudioTypes::TLoopMode TOggVorbisFile::OnLoopMode()const
{
  // Not supported
  return TAudioTypes::kNoLoop;
}

// -------------------------------------------------------------------------------------------------

long long TOggVorbisFile::OnLoopStart()const
{
  // Not supported
  return -1;
}

// -------------------------------------------------------------------------------------------------

long long TOggVorbisFile::OnLoopEnd()const
{
  // Not supported
  return -1;
}

// -------------------------------------------------------------------------------------------------

void TOggVorbisFile::OnSetLoop(
  TAudioTypes::TLoopMode LoopMode, 
  long long              LoopStart, 
  long long              LoopEnd)
{
  // Not supported
}

// -------------------------------------------------------------------------------------------------

TArray<long long> TOggVorbisFile::OnCuePoints()const 
{ 
  // Not supported 
  return TArray<long long>(); 
}

// -------------------------------------------------------------------------------------------------

void TOggVorbisFile::OnSetCuePoints(const TArray<long long>& CuePoints) 
{ 
  // Not supported 
};

#endif // !defined(MNoOggVorbis)

