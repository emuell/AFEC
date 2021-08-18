#include "CoreFileFormatsPrecompiledHeader.h"

#if !defined(MNoMp3LibMpg)

#include "CoreTypes/Export/ByteOrder.h"
#include "CoreTypes/Export/Log.h"

#include "../../3rdParty/Mpg123/Export/Mpg123.h"

#include <mutex>

// =================================================================================================

extern int libmpg123_is_present;
extern int libmpg123_is_using_64;
extern "C" int libmpg123_symbol_is_present(char *s);

// =================================================================================================

class TMp3DecoderFile : public TReferenceCountable
{
public:
  TMp3DecoderFile(TFile* pFile);
  ~TMp3DecoderFile();

  //! @throw TReadableException
  void Open();
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
  TFile* mpFile;

  long long mTotalNumberOfSamples;
  int mBlockSize;
  int mSampleRate;
  int mNumberOfBits;
  int mNumberOfChannels;
  
  long long mBufferSampleFramePos;

  mpg123_handle* mMp3File;
};

// =================================================================================================

class TMp3FileStream : public TAudioStream
{
public:
  TMp3FileStream(TMp3DecoderFile* pMp3File);

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

  TMp3DecoderFile* mpMp3DecoderFile;

  int mBytesWritten;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TMp3DecoderFile::TMp3DecoderFile(TFile* pFile)
  : mpFile(pFile),
    mTotalNumberOfSamples(0),
    mBlockSize(0),
    mSampleRate(0),
    mNumberOfBits(0),
    mNumberOfChannels(0),
    mBufferSampleFramePos(0),
    mMp3File(NULL)
{
  MAssert(pFile->IsOpen(), "File must be open!");
}

// -------------------------------------------------------------------------------------------------

TMp3DecoderFile::~TMp3DecoderFile()
{
  Close();
}

// -------------------------------------------------------------------------------------------------

void TMp3DecoderFile::Open()
{
  int Mpeg123Error = MPG123_OK;

  MAssert(mMp3File == NULL, "File is already open");
  mMp3File = ::mpg123_new(NULL, &Mpeg123Error);

  if (mMp3File == NULL || Mpeg123Error != MPG123_OK)
  {
    mMp3File = NULL;

    throw TReadableException(MText("Failed to initialize libmpg123 "
      "(mpg123_new: '%s').", ToString(Mpeg123Error)));
  }

  const std::string FileNameCString = 
    mpFile->FileName().StdCString(TString::kFileSystemEncoding);

  if (libmpg123_is_using_64)
  {
    Mpeg123Error = ::mpg123_open_64(mMp3File, FileNameCString.c_str());
  }
  else
  {
    Mpeg123Error = ::mpg123_open(mMp3File, FileNameCString.c_str());
  }

  if (Mpeg123Error != MPG123_OK)
  {
    ::mpg123_delete(mMp3File);
    mMp3File = NULL;

    throw TReadableException(MText("File does not appear to be an MP3 "
      "bitstream (mpg123_open: '%s').", ToString(Mpeg123Error)));
  }

  long Rate = 0;
  int Channels = 0;
  int Encoding = 0;

  if ((Mpeg123Error = ::mpg123_getformat(
        mMp3File, &Rate, &Channels, &Encoding)) != MPG123_OK)
  {
    ::mpg123_delete(mMp3File);
    mMp3File = NULL;

    throw TReadableException(MText("There was error while opening the "
      "MP3 file (mp123_getformat: '%s').", ToString(Mpeg123Error)));
  }

  mSampleRate = (int)Rate;
  mNumberOfChannels = Channels;

  switch (Encoding)
  {
  case MPG123_ENC_8:
  case MPG123_ENC_UNSIGNED_8:
  case MPG123_ENC_SIGNED_8:
  case MPG123_ENC_ULAW_8:
  case MPG123_ENC_ALAW_8:
    mNumberOfBits = 8;
    break;
  case MPG123_ENC_ANY:
  case MPG123_ENC_16:
  case MPG123_ENC_SIGNED_16:
  case MPG123_ENC_UNSIGNED_16:
    mNumberOfBits = 16;
    break;
  case MPG123_ENC_SIGNED:
  case MPG123_ENC_FLOAT:
  case MPG123_ENC_32:
  case MPG123_ENC_SIGNED_32:
  case MPG123_ENC_UNSIGNED_32:
  case MPG123_ENC_FLOAT_32:
    mNumberOfBits = 32;
    break;
  case MPG123_ENC_FLOAT_64:
    mNumberOfBits = 64;
    break;

  default:
    ::mpg123_close(mMp3File);
    ::mpg123_delete(mMp3File);
    mMp3File = NULL;

    throw TReadableException(MText("Unsupported bit depth (encoding: '%s') "
      "in the MP3 file.", ToString(Encoding)));
  }

  if (libmpg123_is_using_64)
  {
    ::mpg123_seek_64(mMp3File, 0L, SEEK_END);
    mTotalNumberOfSamples = ::mpg123_tell_64(mMp3File);
    ::mpg123_seek_64(mMp3File, 0L, SEEK_SET);
  }
  else
  {
    ::mpg123_seek(mMp3File, 0L, SEEK_END);
    mTotalNumberOfSamples = ::mpg123_tell(mMp3File);
    ::mpg123_seek(mMp3File, 0L, SEEK_SET);
  }

  if (mTotalNumberOfSamples <= 0)
  {
    // try querying the filesize with mpg123_position if mpg123_seek failed
    // this will at least give us an good approximation...
    double CurrentSeconds = 0.0f, SecondsLeft = 0.0;
    
    if (libmpg123_is_using_64)
    {
      // mpg123_position might be missing in final mpg123 libs 
      if (libmpg123_symbol_is_present((char*)"mpg123_position_64")) 
      {
        if ((Mpeg123Error = ::mpg123_position_64(mMp3File,
              0, 0, NULL, NULL, &CurrentSeconds, &SecondsLeft)) == MPG123_OK)
        {
          mTotalNumberOfSamples = (long long)((CurrentSeconds + SecondsLeft) * mSampleRate);
        }
      }
    }
    else
    {
      // mpg123_position might be missing in final mpg123 libs 
      if (libmpg123_symbol_is_present((char*)"mpg123_position"))
      {
        if ((Mpeg123Error = ::mpg123_position(mMp3File,
              0, 0, NULL, NULL, &CurrentSeconds, &SecondsLeft)) == MPG123_OK)
        {
          mTotalNumberOfSamples = (long long)((CurrentSeconds + SecondsLeft) * mSampleRate);
        }
      }
    }
  }
   
  if (mTotalNumberOfSamples <= 0)
  {  
    ::mpg123_close(mMp3File);
    ::mpg123_delete(mMp3File);
    mMp3File = NULL;

    throw TReadableException(MText(
      "Failed to query the MP3 file's total sample frame duration (mpg123_tell: '%s')."
      " This MP3 file might not be seekable and thus can not be used in $MProductString...",
      ToString((int)mTotalNumberOfSamples)));
  }
  
  mBlockSize = (int)::mpg123_outblock(mMp3File);

  if (mBlockSize <= 0)
  {
    ::mpg123_close(mMp3File);
    ::mpg123_delete(mMp3File);
    mMp3File = NULL;

    throw TReadableException(MText("Failed to query the MP3 file's "
      "decoding properties (mpg123_outblock: '%s').", ToString(mBlockSize)));
  }
}

// -------------------------------------------------------------------------------------------------

void TMp3DecoderFile::Close()
{
  if (mMp3File)
  {
    ::mpg123_close(mMp3File);
    ::mpg123_delete(mMp3File);
    mMp3File = NULL;
  }
}

// -------------------------------------------------------------------------------------------------

long long TMp3DecoderFile::TotalNumberOfSamples()const
{ 
  return mTotalNumberOfSamples; 
}

// -------------------------------------------------------------------------------------------------

int TMp3DecoderFile::NumberOfChannels()const
{ 
  return mNumberOfChannels; 
}

// -------------------------------------------------------------------------------------------------

int TMp3DecoderFile::SampleRate()const
{ 
  return mSampleRate; 
}

// -------------------------------------------------------------------------------------------------

int TMp3DecoderFile::BlockSize()const
{
  return mBlockSize;
}

// -------------------------------------------------------------------------------------------------

int TMp3DecoderFile::NumberOfBits()const
{ 
  return mNumberOfBits; 
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TMp3DecoderFile::SampleType()const
{
  return TAudioFile::k16Bit;
}

// -------------------------------------------------------------------------------------------------

bool TMp3DecoderFile::SeekTo(long long SampleFrame)
{
  MAssert(mMp3File, "Open was not called!");

  if (mBufferSampleFramePos == SampleFrame)
  {
    // no relocation needed
    return true;
  }

  if (libmpg123_is_using_64)
  {
    // seeking necessary?
    if (::mpg123_tell_64(mMp3File) == SampleFrame)
    {
      mBufferSampleFramePos = SampleFrame;
      return true;
    }

    // seeking succeeded?
    if (::mpg123_seek_64(mMp3File, SampleFrame, SEEK_SET) >= 0)
    {
      mBufferSampleFramePos = SampleFrame;
      return true;
    }
  }
  else
  {
    // seeking necessary?
    if (::mpg123_tell(mMp3File) == (off_t)SampleFrame)
    {
      mBufferSampleFramePos = SampleFrame;
      return true;
    }

    if (::mpg123_seek(mMp3File, SampleFrame, SEEK_SET) >= 0)
    {
      mBufferSampleFramePos = SampleFrame;
      return true;
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

void TMp3DecoderFile::ReadBuffer(void* pData, int NumSampleFrames)
{
  MAssert(mMp3File, "Open was not called!");

  const int TotalNumBytesInBuffer = NumSampleFrames *
    ((mNumberOfBits + 7) / 8) * mNumberOfChannels;

  const int BytesPerSample = ((mNumberOfBits + 7) / 8);

  int TotalSamplesRead = 0;
  int BufferOffset = 0;
  size_t BytesRead = 0;

  // Note: avoid seeking when not neccessary. some mpg123 version will fade
  // volume in/out when seeking, even when seeking to the current position
  if (libmpg123_is_using_64)
  {
    if (::mpg123_tell_64(mMp3File) != mBufferSampleFramePos)
    {
      ::mpg123_seek_64(mMp3File, mBufferSampleFramePos, SEEK_SET);
    }
  }
  else
  {
    if (::mpg123_tell(mMp3File) != (off_t)mBufferSampleFramePos)
    {
      ::mpg123_seek(mMp3File, (off_t)mBufferSampleFramePos, SEEK_SET);
    }
  }

  while (TotalSamplesRead < NumSampleFrames)
  {
    const int NumBytesToDecode = (NumSampleFrames - TotalSamplesRead) * 
      BytesPerSample * mNumberOfChannels;

    int ReadError = 0;
    
    {
      M__DisableFloatingPointAssertions
  
      // mpg123_read is not thread safe
      static std::mutex sMpg123ReadLock; 
      const std::lock_guard<std::mutex> Lock(sMpg123ReadLock);
      
      ReadError = ::mpg123_read(mMp3File, (unsigned char*)pData + BufferOffset,
        NumBytesToDecode, &BytesRead);
    
      M__EnableFloatingPointAssertions
    } 
    
    if (ReadError == MPG123_NEED_MORE)
    {
      // all fine, just wants more input...
      MAssert(BytesRead % (BytesPerSample * mNumberOfChannels) == 0, "");
      TotalSamplesRead += (int)BytesRead / BytesPerSample / mNumberOfChannels;
      BufferOffset += (int)BytesRead;
    }
    else if (ReadError != MPG123_OK || BytesRead == 0)
    {
      MInvalid("Unexpected End of File");
      TMemory::Zero(pData, TotalNumBytesInBuffer);
      break;
    } 
    else 
    {
      // all fine, done for now
      MAssert(ReadError == MPG123_OK, "Unexpected read result");
      break;
    }
  }

  mBufferSampleFramePos += NumSampleFrames;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TMp3FileStream::TMp3FileStream(TMp3DecoderFile* pMp3File)
  : TAudioStream((int)pMp3File->TotalNumberOfSamples(),
      pMp3File->NumberOfChannels(), pMp3File->SampleType()),
    mpMp3DecoderFile(pMp3File),
    mBytesWritten(0)
{
}

// -------------------------------------------------------------------------------------------------

int TMp3FileStream::OnPreferedBlockSize()const
{
  return mpMp3DecoderFile->BlockSize();
}

// -------------------------------------------------------------------------------------------------

long long TMp3FileStream::OnBytesWritten()const
{
  return mBytesWritten;
}

// -------------------------------------------------------------------------------------------------

bool TMp3FileStream::OnSeekTo(long long Frame)
{
  return mpMp3DecoderFile->SeekTo(Frame);
}

// -------------------------------------------------------------------------------------------------

void TMp3FileStream::OnReadBuffer(
  void* pDestBuffer, 
  int   NumberOfSampleFrames)
{
  mpMp3DecoderFile->ReadBuffer(pDestBuffer, NumberOfSampleFrames);
}

// -------------------------------------------------------------------------------------------------

void TMp3FileStream::OnWriteBuffer(
  const void* pSrcBuffer, 
  int         NumberOfSampleFrames)
{
  MCodeMissing;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TList<TString> TMp3File::SSupportedExtensions()
{
  TList<TString> Ret;

  Ret.Append("*.mp2");
  Ret.Append("*.mp3");

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TMp3File::TMp3File()
  : TAudioFile()
{
}

// -------------------------------------------------------------------------------------------------

TString TMp3File::OnTypeString()const
{
  return "MP3";
}

// -------------------------------------------------------------------------------------------------

void TMp3File::SInit()
{
  if (libmpg123_is_present)
  {
    TLog::SLog()->AddLine("File-IO",
      "Enabling MP3 decoding support using system mpg123 library...");

    ::mpg123_init();
  }
}

// -------------------------------------------------------------------------------------------------

void TMp3File::SExit()
{
  if (libmpg123_is_present)
  {
    ::mpg123_exit();
  }
}

// -------------------------------------------------------------------------------------------------

bool TMp3File::SIsSampleRateSupported(int SampleRate)
{
  return (SampleRate == 11025 || SampleRate == 22050 ||
    SampleRate == 44100 || SampleRate == 48000);
}

// -------------------------------------------------------------------------------------------------

bool TMp3File::SIsBitDepthSupported(int BitDepth)
{
  return (BitDepth == 8 || BitDepth == 16 ||
    BitDepth == 24 || BitDepth == 32);
}

// -------------------------------------------------------------------------------------------------

void TMp3File::OnOpenForRead(
  const TString&  Filename, 
  int             FileSubTrackIndex)
{
  MUnused(FileSubTrackIndex);

  if (!libmpg123_is_present)
  {
    throw TReadableException(MText(
      "libmpg123 was not found, but must be installed to load MP3 files in $MProductString."));
  }

  mFile.SetFileName(Filename);
  mOverviewFile.SetFileName(Filename);

  if (!mFile.Open(TFile::kRead) || !mOverviewFile.Open(TFile::kRead))
  {
    throw TReadableException(MText("Failed to open the file for reading."));
  }

  mpDecoderFile = new TMp3DecoderFile(&mFile);
  mpDecoderFile->Open();
  mpStream = new TMp3FileStream(mpDecoderFile);

  // the overview file and stream is created when needed
}

// -------------------------------------------------------------------------------------------------

void TMp3File::OnOpenForWrite(
  const TString&  Filename,
  int             SamplingRate,
  TSampleType     SampleType,
  int             NumChannels)
{
  throw TReadableException(
    MText("Encoding MP3 files is currently not supported."));
}

// -------------------------------------------------------------------------------------------------

void TMp3File::OnClose()
{
  mpStream.Delete();
  mpOverviewStream.Delete();

  if (mpDecoderFile)
  {
    mpDecoderFile->Close();
    mpDecoderFile.Delete();
  }

  if (mpDecoderOverviewFile)
  {
    mpDecoderOverviewFile->Close();
    mpDecoderOverviewFile.Delete();
  }
  
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

TAudioStream* TMp3File::OnStream()const
{
  return mpStream;
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TMp3File::OnOverviewStream()const
{
  if (!mpOverviewStream)
  {
    TMp3File* pMutableThis = const_cast<TMp3File*>(this);
    
    pMutableThis->mpDecoderOverviewFile = 
      new TMp3DecoderFile(&pMutableThis->mOverviewFile);
    
    pMutableThis->mpDecoderOverviewFile->Open();

    pMutableThis->mpOverviewStream = 
      new TMp3FileStream(pMutableThis->mpDecoderOverviewFile);
  }

  return mpOverviewStream;
}

// -------------------------------------------------------------------------------------------------

int TMp3File::OnSamplingRate()const
{
  return mpDecoderFile->SampleRate();
}

// -------------------------------------------------------------------------------------------------

int TMp3File::OnBitsPerSample()const
{
  return mpDecoderFile->NumberOfBits();
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TMp3File::OnSampleType()const
{
  return mpDecoderFile->SampleType();
}

// -------------------------------------------------------------------------------------------------

int TMp3File::OnNumChannels()const
{
  return mpDecoderFile->NumberOfChannels();
}

// -------------------------------------------------------------------------------------------------

long long TMp3File::OnNumSamples() const
{
  return mpDecoderFile->TotalNumberOfSamples();
}

// -------------------------------------------------------------------------------------------------

TAudioTypes::TLoopMode TMp3File::OnLoopMode()const
{
  // Not supported
  return TAudioTypes::kNoLoop;
}

// -------------------------------------------------------------------------------------------------

long long TMp3File::OnLoopStart()const
{
  // Not supported
  return -1;
}

// -------------------------------------------------------------------------------------------------

long long TMp3File::OnLoopEnd()const
{
  // Not supported
  return -1;
}

// -------------------------------------------------------------------------------------------------

void TMp3File::OnSetLoop(
  TAudioTypes::TLoopMode LoopMode, 
  long long              LoopStart, 
  long long              LoopEnd)
{
  // Not supported
}

// -------------------------------------------------------------------------------------------------

TArray<long long> TMp3File::OnCuePoints()const 
{ 
  // Not supported 
  return TArray<long long>(); 
}

// -------------------------------------------------------------------------------------------------

void TMp3File::OnSetCuePoints(const TArray<long long>& CuePoints) 
{ 
  // Not supported 
}


#endif // !defined(MNoMp3LibMpg)

