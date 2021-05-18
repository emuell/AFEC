#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/ByteOrder.h"

#include "../../3rdParty/Flac/Export/Flac.h"

#include <cstring> // memcmp

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static TAudioFile::TSampleType SSampleTypeFromFlacNumBits(int NumberOfBits)
{
  switch (NumberOfBits)
  {
  default:
    MInvalid("unknown Sample Width")
    return TAudioFile::k32BitInt;
  
  case 8:
    return TAudioFile::k8BitSigned;
  case 16:
    return TAudioFile::k16Bit;
  case 24:
    return TAudioFile::k24Bit;
  case 32:
    return TAudioFile::k32BitInt;
  }
}

// -------------------------------------------------------------------------------------------------

static int SNumBitsFromFlacSampleType(TAudioFile::TSampleType SampleType)
{
  switch (SampleType)
  {
  default:
    MInvalid("unsupported Sample Type");
    return 0;
  
  case TAudioFile::k8BitSigned:
    return 8;
  case TAudioFile::k16Bit:
    return 16;
  case TAudioFile::k24Bit:
    return 24;
  case TAudioFile::k32BitInt:
    return 32;
  }
}

// =================================================================================================

class TFlacDecoderFile : private FLAC::Decoder::Stream, public TReferenceCountable
{
public:
  TFlacDecoderFile(TFile* pFile);
  ~TFlacDecoderFile();

  //! @throw TReadableException
  void Open();
  void Close();

  const TList< TArray<char> >& RiffMetaDataBlocks()const;

  long long TotalNumberOfSamples()const;
  int NumberOfChannels()const;
  int NumberOfBits()const;
  
  TAudioFile::TSampleType SampleType()const;

  int SampleRate()const;
  
  int BlockSize()const;

  bool SeekTo(long long SampleFrame);
  void ReadBuffer(void* pData, int NumSampleFrames);

private:
  virtual FLAC__StreamDecoderReadStatus read_callback(
    FLAC__byte  buffer[], 
    size_t*     bytes);

  virtual FLAC__StreamDecoderSeekStatus seek_callback(
    FLAC__uint64 absolute_byte_offset);

  virtual FLAC__StreamDecoderTellStatus tell_callback(
    FLAC__uint64 *absolute_byte_offset);
  
  virtual FLAC__StreamDecoderLengthStatus length_callback(
    FLAC__uint64 *stream_length);
  
  virtual bool eof_callback();
  
  virtual FLAC__StreamDecoderWriteStatus write_callback(
    const FLAC__Frame*        frame, 
    const FLAC__int32* const  buffer[]);

  virtual void metadata_callback(const FLAC__StreamMetadata *metadata);
  virtual void error_callback(FLAC__StreamDecoderErrorStatus status);
  
  TFile* mpFile;

  TList< TArray<char> > mRiffMetaDataBlocks;

  long long mTotalNumberOfSamples;
  int mBlockSize;
  int mSampleRate;
  int mNumberOfBits;
  int mNumberOfChannels;
  
  long long mFlacSampleFramePos;
  long long mBufferSampleFramePos;

  void* mpSampleBuffer;
  int mSampleBufferSizeInBytes;
  int mSampleBufferSamplesWritten;
};

// =================================================================================================

class TFlacEncoderFile : private FLAC::Encoder::Stream, public TReferenceCountable
{
public:
  TFlacEncoderFile(
    TFile* pFile, 
    int NumberOfChannels, 
    int SampleRate, 
    int NumberOfBits);

  ~TFlacEncoderFile();

  //! optional, but to be added before its opened
  void AddRiffMetaDataChunk(const TArray<char>& RiffMetaDataBlock);

  //! @throw TReadableException
  void Open();
  void Close();

  int NumberOfChannels()const;
  int NumberOfBits()const;
  
  TAudioFile::TSampleType SampleType()const;
  
  int SampleRate()const;
  int BlockSize()const;

  void WriteBuffer(const void* pData, int NumSampleFrames);

private:
  virtual ::FLAC__StreamEncoderWriteStatus write_callback(
    const FLAC__byte  buffer[],
    size_t            bytes, 
    unsigned          samples, 
    unsigned          current_frame);

  virtual FLAC__StreamEncoderSeekStatus seek_callback(
    FLAC__uint64 absolute_byte_offset);
  
  virtual FLAC__StreamEncoderTellStatus tell_callback(
    FLAC__uint64 *absolute_byte_offset);

  TFile* mpFile;

  int mSampleRate;
  int mNumberOfBits;
  int mNumberOfChannels;

  long long mFramesWritten;
  TArray<TInt32> mTempSampleBuffer;

  TList<FLAC::Metadata::Application> mMetaDataBlocks;
};

// =================================================================================================

class TFlacFileStream : public TAudioStream
{
public:
  TFlacFileStream(TFlacDecoderFile* pFlacFile);
  TFlacFileStream(TFlacEncoderFile* pFlacFile);
  
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

  TFlacEncoderFile* mpFlacEncoderFile;
  TFlacDecoderFile* mpFlacDecoderFile;
  
  long long mBytesWritten;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TFlacDecoderFile::TFlacDecoderFile(TFile* pFile)
  : mpFile(pFile),
    mTotalNumberOfSamples(0),
    mBlockSize(0),
    mSampleRate(0),
    mNumberOfBits(0),
    mNumberOfChannels(0),
    mFlacSampleFramePos(0),
    mBufferSampleFramePos(0),
    mpSampleBuffer(NULL),
    mSampleBufferSizeInBytes(0),
    mSampleBufferSamplesWritten(0)
{
  MAssert(pFile->IsOpen(), "File must be open!");
}

// -------------------------------------------------------------------------------------------------

TFlacDecoderFile::~TFlacDecoderFile()
{
  Close();

  if (mpSampleBuffer)
  {
    TMemory::Free(mpSampleBuffer);
  }
}

// -------------------------------------------------------------------------------------------------

void TFlacDecoderFile::Open() 
{
  // make sure we get all meta data
  FLAC::Decoder::Stream::set_metadata_respond_all();

  // initialize and parse the stream
  const FLAC__StreamDecoderInitStatus Status = FLAC::Decoder::Stream::init();
  
  if (Status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
  {
    throw TReadableException(
      MText("Failed to open the FLAC file (Error: %s).", 
        FLAC__StreamDecoderInitStatusString[Status]));
  }

  if (! FLAC::Decoder::Stream::process_until_end_of_metadata())
  {
    throw TReadableException(MText(
      "Failed to open the FLAC file (Failed to read the meta data)."));
  }

  // validate format
  if (! (mNumberOfBits == 8 || mNumberOfBits == 16 || 
         mNumberOfBits == 24 || mNumberOfBits == 32))
  {
    throw TReadableException(MText("Failed to open the FLAC file "
      "(Only 8, 16, 24 or 32 bit samples are currently supported)."));
  }
  
  // find out how many frames we've got
  if (mTotalNumberOfSamples == 0)
  {
    // don't have TotalNumberOfSamples set in the meta data? 
    // Then calc it the hard way... 
    while (true)
    {
      const FLAC__StreamDecoderState State = FLAC::Decoder::Stream::get_state();
      
      if (State == FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC ||
          State == FLAC__STREAM_DECODER_READ_FRAME ) 
      {
        if (!FLAC::Decoder::Stream::process_single())
        {
          break;
        }
      } 
      else if (State == FLAC__STREAM_DECODER_SEEK_ERROR)
      {
        if (!FLAC::Decoder::Stream::flush())
        {
          break;
        }
      }
      else
      {
        MAssert(State != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA &&
          State != FLAC__STREAM_DECODER_READ_METADATA, 
          "Metadata should already be parsed here!");
      
        break;
      }
    }

    mTotalNumberOfSamples = mSampleBufferSamplesWritten;
    
    // completely reset state (we start reading again now)
    if (mTotalNumberOfSamples > 0)
    {
      FLAC::Decoder::Stream::reset();

      mRiffMetaDataBlocks.Empty();

      mFlacSampleFramePos = 0;
      mBufferSampleFramePos = 0;

      mSampleBufferSizeInBytes = 0;
      mSampleBufferSamplesWritten = 0;

      if (mpSampleBuffer)
      {
        TMemory::Free(mpSampleBuffer);
        mpSampleBuffer = NULL;
      }

      // retain manually parsed TotalNumberOfSamples
      const long long TotalNumberOfSamples = mTotalNumberOfSamples;

      if (! FLAC::Decoder::Stream::process_until_end_of_metadata())
      {
        MInvalid("Expected no error here "
          "(previous 'process_until_end_of_metadata' succeeded)");
      }

      mTotalNumberOfSamples = TotalNumberOfSamples;
    }
  }
  
  if (mTotalNumberOfSamples == 0)
  {
    throw TReadableException(MText("Failed to open the FLAC file "
      "(file cannot be accessed or is no valid FLAC file)."));
  }
}

// -------------------------------------------------------------------------------------------------

void TFlacDecoderFile::Close()
{
  // nothing to do...
}

// -------------------------------------------------------------------------------------------------

const TList< TArray<char> >& TFlacDecoderFile::RiffMetaDataBlocks()const
{
  return mRiffMetaDataBlocks;
}

// -------------------------------------------------------------------------------------------------

long long TFlacDecoderFile::TotalNumberOfSamples()const 
{ 
  return mTotalNumberOfSamples; 
}

// -------------------------------------------------------------------------------------------------

int TFlacDecoderFile::NumberOfChannels()const 
{ 
  return mNumberOfChannels; 
}

// -------------------------------------------------------------------------------------------------

int TFlacDecoderFile::SampleRate()const 
{ 
  return mSampleRate; 
}

// -------------------------------------------------------------------------------------------------

int TFlacDecoderFile::BlockSize()const
{
  if (mBlockSize > 0)
  {
    return mBlockSize;
  }
  else
  {
    return 4096;
  }
}

// -------------------------------------------------------------------------------------------------

int TFlacDecoderFile::NumberOfBits()const 
{ 
  return mNumberOfBits; 
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TFlacDecoderFile::SampleType()const
{
  return SSampleTypeFromFlacNumBits(mNumberOfBits);
}

// -------------------------------------------------------------------------------------------------

bool TFlacDecoderFile::SeekTo(long long SampleFrame)
{
  MAssert(mpFile->IsOpen() && FLAC::Decoder::Stream::is_valid(), 
    "Open was not called!");

  bool Succeeded = true;
  
  if (SampleFrame != mFlacSampleFramePos)
  {
    Succeeded = FLAC::Decoder::Stream::seek_absolute(SampleFrame);
    mFlacSampleFramePos = SampleFrame;
  }

  mBufferSampleFramePos = SampleFrame;
  
  return Succeeded;
}

// -------------------------------------------------------------------------------------------------

void TFlacDecoderFile::ReadBuffer(void* pData, int NumSampleFrames)
{
  MAssert(mpFile->IsOpen() && FLAC::Decoder::Stream::is_valid(), 
    "Open was not called!");

  if (mFlacSampleFramePos != mBufferSampleFramePos)
  {
    FLAC::Decoder::Stream::seek_absolute(mBufferSampleFramePos);
    mFlacSampleFramePos = mBufferSampleFramePos;
  }

  const int BufferByteSize = (mNumberOfBits + 7) / 8 * 
    NumSampleFrames * mNumberOfChannels;
  
  while (mSampleBufferSamplesWritten / mNumberOfChannels < NumSampleFrames)
  {
    const FLAC__StreamDecoderState State = FLAC::Decoder::Stream::get_state();
            
    if (State == FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC ||
        State == FLAC__STREAM_DECODER_READ_FRAME ) 
    {
      if (!FLAC::Decoder::Stream::process_single())
      {
        break;
      }
    } 
    else if (State == FLAC__STREAM_DECODER_SEEK_ERROR)
    {
      if (!FLAC::Decoder::Stream::flush())
      {
        break;
      }
    }
    else
    {
      MAssert(State != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA &&
        State != FLAC__STREAM_DECODER_READ_METADATA, 
        "Metadata should already be parsed here!");
    
      break;
    }
  }

  const bool Failed = 
    (mSampleBufferSamplesWritten / mNumberOfChannels < NumSampleFrames);

  if (Failed)
  {
    // copy what could be read
    const int SuccessfullyReadBytes = 
    (mNumberOfBits + 7) / 8 * mSampleBufferSamplesWritten;
    TMemory::Copy(pData, mpSampleBuffer, SuccessfullyReadBytes);

    // clear the rest
    TMemory::Zero((TInt8*)pData + SuccessfullyReadBytes, 
    BufferByteSize - SuccessfullyReadBytes);
  }
  else
  {
    TMemory::Copy(pData, mpSampleBuffer, BufferByteSize);
  }

  mBufferSampleFramePos += NumSampleFrames;
  mSampleBufferSamplesWritten = 0;
}

// -------------------------------------------------------------------------------------------------

FLAC__StreamDecoderReadStatus TFlacDecoderFile::read_callback(
  FLAC__byte  buffer[], 
  size_t*     bytes)
{
  if (mpFile->IsOpen() && mpFile->ReadBytes((char*)buffer, bytes))
  {
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
  }
  else
  {
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
  }
}

// -------------------------------------------------------------------------------------------------

FLAC__StreamDecoderSeekStatus TFlacDecoderFile::seek_callback(
  FLAC__uint64 absolute_byte_offset)
{
  if (mpFile->IsOpen() && mpFile->SetPosition((int)absolute_byte_offset))
  {
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
  }
  else
  {
    return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
  }
}

// -------------------------------------------------------------------------------------------------

FLAC__StreamDecoderTellStatus TFlacDecoderFile::tell_callback(
  FLAC__uint64* absolute_byte_offset)
{
  if (mpFile->IsOpen())
  {
    *absolute_byte_offset = mpFile->Position();
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
  }
  else
  {
    return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
  }
}

// -------------------------------------------------------------------------------------------------

FLAC__StreamDecoderLengthStatus TFlacDecoderFile::length_callback(
  FLAC__uint64* stream_length)
{
  if (mpFile->IsOpen())
  {
    *stream_length = mpFile->SizeInBytes();
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
  }
  else
  {
    return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
  }
}

// -------------------------------------------------------------------------------------------------

bool TFlacDecoderFile::eof_callback()
{
  // TODO
  return false;
}

// -------------------------------------------------------------------------------------------------

FLAC__StreamDecoderWriteStatus TFlacDecoderFile::write_callback(
  const FLAC__Frame*        frame, 
  const FLAC__int32* const  buffer[])
{
  const int FrameSize = frame->header.blocksize;
  const int BitsPerSampleFrame = frame->header.bits_per_sample;

  // support up to two NumberOfChannels only
  const int NumberOfChannels = MMin(frame->header.channels, 2u); 

  const int ByteSize = (BitsPerSampleFrame + 7) / 8 * 
    FrameSize * NumberOfChannels;

  const int SampleBufferBytePos = (BitsPerSampleFrame + 7) / 8 * 
    mSampleBufferSamplesWritten * NumberOfChannels;

  // preallocate in batches to avoid too many reallocations
  const int AllocSize = mSampleBufferSizeInBytes + 
    TMath::NextPow2(128*ByteSize);
  
  if (mpSampleBuffer == NULL)
  {
    mpSampleBuffer = TMemory::Alloc("#FLACSampleBuffer", AllocSize);
    mSampleBufferSizeInBytes = AllocSize;
  }
  else if (SampleBufferBytePos + ByteSize > mSampleBufferSizeInBytes)
  {
    mpSampleBuffer = TMemory::Realloc(mpSampleBuffer, AllocSize);
    mSampleBufferSizeInBytes = AllocSize;
  }

  for (int j = 0; j < FrameSize; ++j) 
  {
    for (int i = 0; i < NumberOfChannels; ++i) 
    {
      switch (BitsPerSampleFrame)
      {
      case 8:
        ((TInt8*)mpSampleBuffer)[mSampleBufferSamplesWritten++] = 
          (TInt8)buffer[i][j];
        break;

      case 16:
        ((TInt16*)mpSampleBuffer)[mSampleBufferSamplesWritten++] = 
          (TInt16)buffer[i][j];
        break;

      case 24:
      {
        T24* pSrc = (T24*)((char*)mpSampleBuffer + 
          3*mSampleBufferSamplesWritten++);

        #if (MSystemByteOrder == MMotorolaByteOrder)
          (*pSrc)[2] = (char)((TInt32)buffer[i][j] >> 0);
          (*pSrc)[1] = (char)((TInt32)buffer[i][j] >> 8);
          (*pSrc)[0] = (char)((TInt32)buffer[i][j] >> 16);
        #else
          (*pSrc)[0] = (char)((TInt32)buffer[i][j] >> 0);
          (*pSrc)[1] = (char)((TInt32)buffer[i][j] >> 8);
          (*pSrc)[2] = (char)((TInt32)buffer[i][j] >> 16);
        #endif  
      }  break;
      
      case 32:
        ((TInt32*)mpSampleBuffer)[mSampleBufferSamplesWritten++] = 
          (TInt32)buffer[i][j];
        break;

      default:
        MInvalid("");
      }
    }
  }

  mFlacSampleFramePos += FrameSize;

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

// -------------------------------------------------------------------------------------------------

void TFlacDecoderFile::metadata_callback(
  const FLAC__StreamMetadata* metadata)
{
  if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) 
  {
    mBlockSize = metadata->data.stream_info.max_blocksize;
    mTotalNumberOfSamples = metadata->data.stream_info.total_samples;
    mSampleRate =  metadata->data.stream_info.sample_rate;
    mNumberOfBits = metadata->data.stream_info.bits_per_sample;
    // supporting up to 2 NumberOfChannels only
    mNumberOfChannels = MMin(metadata->data.stream_info.channels, 2u);  
  }
  else if (metadata->type == FLAC__METADATA_TYPE_APPLICATION)
  {
    const FLAC__byte* pId = metadata->data.application.id;
    
    // copy riff app data only (for now)
    if (pId[0] == 'r' && pId[1] == 'i' && pId[2] == 'f' && pId[3] == 'f')
    {
      const FLAC__byte* pData = metadata->data.application.data;
      
      const int DataLength = metadata->length - 
        FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8; 

      mRiffMetaDataBlocks.Append(TArray<char>());
      mRiffMetaDataBlocks.Last().SetSize(DataLength);

      TMemory::Copy(mRiffMetaDataBlocks.Last().FirstWrite(), pData, DataLength);
    }
  }
  else
  {
    // ignore all others
  }
}

// -------------------------------------------------------------------------------------------------

void TFlacDecoderFile::error_callback(
  FLAC__StreamDecoderErrorStatus status)
{
  switch (status) 
  {
  case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
    TLog::SLog()->AddLine("FLAC", "Decoder Error: LOST_SYNC");
    break;

  case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
    TLog::SLog()->AddLine("FLAC", "Decoder Error: BAD_HEADER");
    break;

  case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
    TLog::SLog()->AddLine("FLAC", "Decoder Error: FRAME_CRC_MISMATCH");
    break;

  case FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM:
    TLog::SLog()->AddLine("FLAC", "Decoder Error: UNPARSEABLE_STREAM");
    break;
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TFlacEncoderFile::TFlacEncoderFile(
  TFile*  pFile, 
  int     NumberOfChannels, 
  int     SampleRate, 
  int     NumberOfBits)
  : mpFile(pFile),
    mSampleRate(SampleRate),
    mNumberOfBits(NumberOfBits),
    mNumberOfChannels(NumberOfChannels),
    mFramesWritten(0)
{
  MAssert(mpFile->AccessMode() == TFile::kWrite || 
    mpFile->AccessMode() == TFile::kReadWrite, "File must be open!");
}

// -------------------------------------------------------------------------------------------------

TFlacEncoderFile::~TFlacEncoderFile()
{
  Close();
}

// -------------------------------------------------------------------------------------------------

void TFlacEncoderFile::AddRiffMetaDataChunk(const TArray<char>& RiffMetaDataBlock)
{
  #if defined(MDebug)
    for (int i = 0; i < mMetaDataBlocks.Size(); ++i)
    {
      MAssert(::memcmp(mMetaDataBlocks[i].get_data(), 
        RiffMetaDataBlock.FirstRead(), 4) != 0, "Riff chunk is already present");
    }
  #endif

  mMetaDataBlocks.Append(FLAC::Metadata::Application());
  
  FLAC__byte Id[4];
  Id[0] = 'r'; Id[1] = 'i'; Id[2] = 'f'; Id[3] = 'f';
  mMetaDataBlocks.Last().set_id(Id);
  
  const bool CopyData = true;
  
  mMetaDataBlocks.Last().set_data(
    (FLAC__byte*)RiffMetaDataBlock.FirstWrite(), 
    RiffMetaDataBlock.Size(), 
    CopyData);
}

// -------------------------------------------------------------------------------------------------

void TFlacEncoderFile::Open()
{
  #if defined(MDebug)
    //FLAC::Encoder::Stream::set_verify(true);
  #endif

  if (!(mNumberOfBits == 8 || mNumberOfBits == 16 || 
        mNumberOfBits == 24 || mNumberOfBits == 32))
  {
    throw TReadableException(MText("Failed to write the FLAC file "
      "(Only 8, 16, 24 or 32 bit samples are currently supported)."));
  }

  FLAC::Encoder::Stream::set_channels(mNumberOfChannels);
  FLAC::Encoder::Stream::set_bits_per_sample(mNumberOfBits);
  FLAC::Encoder::Stream::set_sample_rate(mSampleRate);
  
  // allow all sample rates that the FLAC format can handle
  FLAC::Encoder::Stream::set_streamable_subset(false);
  
  // setup the encoder (5 - the default of the command line encoder)
  FLAC::Encoder::Stream::set_blocksize(4096);
  FLAC::Encoder::Stream::set_compression_level(5);
  
  // add application metadata, if present
  TArray<FLAC__StreamMetadata*> MetaDataPtrs(mMetaDataBlocks.Size());

  if (!mMetaDataBlocks.IsEmpty())
  {
    for (int i = 0; i < mMetaDataBlocks.Size(); ++i)
    {
      MetaDataPtrs[i] = const_cast<FLAC__StreamMetadata*>(
        mMetaDataBlocks[i].operator const FLAC__StreamMetadata*());
    }

    // Note: set_metadata(FLAC::Metadata::Prototype**) is buggy and wont work! 
    if (!FLAC::Encoder::Stream::set_metadata(
          MetaDataPtrs.FirstWrite(), MetaDataPtrs.Size()))
    {
      throw TReadableException(MText("Failed to write the FLAC file "
        "(Metadata could not be written)."));
    }
  }


  // init/create the flac file
  const FLAC__StreamEncoderInitStatus Status = FLAC::Encoder::Stream::init();
  
  if (Status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
  {
    throw TReadableException(MText("Failed to create a FLAC file (Error: %s).", 
      TString(FLAC__StreamEncoderInitStatusString[Status])));
  }
}

// -------------------------------------------------------------------------------------------------

void TFlacEncoderFile::Close()
{
  if (FLAC::Encoder::Stream::is_valid())
  {
    FLAC::Encoder::Stream::finish();
  }
}

// -------------------------------------------------------------------------------------------------

int TFlacEncoderFile::NumberOfChannels()const
{
  return mNumberOfChannels;
}

// -------------------------------------------------------------------------------------------------

int TFlacEncoderFile::NumberOfBits()const
{
  return mNumberOfBits;
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TFlacEncoderFile::SampleType()const
{
  return SSampleTypeFromFlacNumBits(mNumberOfBits);
}

// -------------------------------------------------------------------------------------------------

int TFlacEncoderFile::SampleRate()const
{
  return mSampleRate;
}

// -------------------------------------------------------------------------------------------------

int TFlacEncoderFile::BlockSize()const
{
  const int BlockSize = FLAC::Encoder::Stream::get_blocksize();
  
  if (BlockSize > 0)
  {
    return BlockSize;
  }
  else
  {
    return 4096;
  }
}

// -------------------------------------------------------------------------------------------------

void TFlacEncoderFile::WriteBuffer(const void* pSrcBuffer, int NumSampleFrames)
{
  MAssert(mpFile->IsOpen() && FLAC::Encoder::Stream::is_valid(), 
    "File is not open!");
  
  const int SamplesToWrite = NumSampleFrames * mNumberOfChannels;
  mTempSampleBuffer.Grow(SamplesToWrite);
  
  TInt32* pDestBuffer = mTempSampleBuffer.FirstWrite();

  int SourceReadPos = 0;

  for (int i = 0; i < SamplesToWrite; ++i)
  {
    switch (mNumberOfBits)
    {
    default:
      MInvalid("");

    case 8:
    {
      const TInt8* pSrc = &((TInt8*)pSrcBuffer)[SourceReadPos++];
      *pDestBuffer++ = (TInt8)(*pSrc);
    } break;

    case 16:
    {
      const TInt16* pSrc = &((TInt16*)pSrcBuffer)[SourceReadPos++];
      *pDestBuffer++ = (TInt16)(*pSrc);
    } break;

    case 24:
    {
      const T24* pSrc = &((const T24*)pSrcBuffer)[SourceReadPos++];
    
      #if (MSystemByteOrder == MMotorolaByteOrder)
        const TInt32 Int32 = (((unsigned char)(*pSrc)[2] | 
          ((unsigned char)(*pSrc)[1] << 8) | 
          ((unsigned char)(*pSrc)[0] << 16)));
      #else
        const TInt32 Int32 = (((unsigned char)(*pSrc)[0] | 
          ((unsigned char)(*pSrc)[1] << 8) | 
          ((unsigned char)(*pSrc)[2] << 16)));
      #endif
      
      *pDestBuffer++ = (Int32 << 8) >> 8; // be sign aware
    } break;

    case 32:
    {
      const TInt32* pSrc = &((TInt32*)pSrcBuffer)[SourceReadPos++];
      *pDestBuffer = (TInt32)(*pSrc);

      // hackaround for an overflow bug in Flac with -2147483648 values
      if (*pDestBuffer < -2147483647)
      {
        *pDestBuffer = -2147483647;
      }
      
      ++pDestBuffer;
    } break;
    }
  }

  // FLAC is not FPU exception safe...
  M__DisableFloatingPointAssertions

  if (!FLAC::Encoder::Stream::process_interleaved(
        mTempSampleBuffer.FirstRead(), NumSampleFrames))
  {
    M__EnableFloatingPointAssertions
    
    throw TReadableException(MText(
      "File I/O error: Failed to save a FLAC file.\n"
      "Please make sure that there is enough space available on the device!"));
  }
  
  M__EnableFloatingPointAssertions

  mFramesWritten += NumSampleFrames;
}

// -------------------------------------------------------------------------------------------------

FLAC__StreamEncoderWriteStatus TFlacEncoderFile::write_callback(
  const FLAC__byte  buffer[],
  size_t            bytes, 
  unsigned          samples, 
  unsigned          current_frame)
{
  size_t NumBytes = bytes;

  if (mpFile->IsOpen() && mpFile->WriteBytes((const char*)buffer, &NumBytes))
  {
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
  }
  else
  {
    // do not throw: handle the error in the write/init calls instead
    return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
  }
}

// -------------------------------------------------------------------------------------------------

FLAC__StreamEncoderSeekStatus TFlacEncoderFile::seek_callback(
  FLAC__uint64 absolute_byte_offset)
{
  if (mpFile->IsOpen() && mpFile->SetPosition((int)absolute_byte_offset))
  {
    return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
  }
  else
  {
    return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
  }
}

// -------------------------------------------------------------------------------------------------

FLAC__StreamEncoderTellStatus TFlacEncoderFile::tell_callback(
  FLAC__uint64 *absolute_byte_offset)
{
  if (mpFile->IsOpen())
  {
    *absolute_byte_offset = mpFile->Position();
    return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
  }
  else
  {
    return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TFlacFileStream::TFlacFileStream(TFlacDecoderFile* pFlacFile)
  : TAudioStream((int)pFlacFile->TotalNumberOfSamples(), 
      pFlacFile->NumberOfChannels(), pFlacFile->SampleType()),
    mpFlacEncoderFile(NULL),
    mpFlacDecoderFile(pFlacFile),
    mBytesWritten(0)
{
}

TFlacFileStream::TFlacFileStream(TFlacEncoderFile* pFlacFile)
  : TAudioStream(0, pFlacFile->NumberOfChannels(), pFlacFile->SampleType()),
    mpFlacEncoderFile(pFlacFile),
    mpFlacDecoderFile(NULL),
    mBytesWritten(0)
{
}

// -------------------------------------------------------------------------------------------------

int TFlacFileStream::OnPreferedBlockSize()const
{
  if (mpFlacEncoderFile)
  {
    return mpFlacEncoderFile->BlockSize();
  }
  else
  {
    return mpFlacDecoderFile->BlockSize();
  }
}

// -------------------------------------------------------------------------------------------------

long long TFlacFileStream::OnBytesWritten()const
{
  return mBytesWritten;
}

// -------------------------------------------------------------------------------------------------

bool TFlacFileStream::OnSeekTo(long long Frame)
{
  MAssert(mpFlacDecoderFile, "Encoder does not support seeking!");
  return mpFlacDecoderFile->SeekTo(Frame);
}

// -------------------------------------------------------------------------------------------------

void TFlacFileStream::OnReadBuffer(
  void* pDestBuffer, 
  int   NumberOfSampleFrames)
{
  MAssert(mpFlacDecoderFile, "Not opened for reading!");
  mpFlacDecoderFile->ReadBuffer(pDestBuffer, NumberOfSampleFrames);
}

// -------------------------------------------------------------------------------------------------

void TFlacFileStream::OnWriteBuffer(
  const void* pSrcBuffer, 
  int         NumberOfSampleFrames)
{
  MAssert(mpFlacEncoderFile, "Not opened for writing!");
  mpFlacEncoderFile->WriteBuffer(pSrcBuffer, NumberOfSampleFrames);
  mBytesWritten += NumberOfSampleFrames;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TList<TString> TFlacFile::SSupportedExtensions()
{
  TList<TString> Ret;
  Ret.Append("*.flac");
  Ret.Append("*.fla");

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TFlacFile::SSampleTypeFromBitDepth(int BitDepth)
{
  return SSampleTypeFromFlacNumBits(BitDepth);
}

// -------------------------------------------------------------------------------------------------

bool TFlacFile::SIsSampleRateSupported(int SampleRate)
{
  return (::FLAC__format_sample_rate_is_valid(SampleRate) != 0);
}

// -------------------------------------------------------------------------------------------------

bool TFlacFile::SIsBitDepthSupported(int BitDepth)
{
  return (BitDepth == 4 || BitDepth == 8 || 
    BitDepth == 16 || BitDepth == 24 || BitDepth == 32);
}

// -------------------------------------------------------------------------------------------------

TFlacFile::TFlacFile()
  : TAudioFile()
{
}

// -------------------------------------------------------------------------------------------------

TString TFlacFile::OnTypeString()const
{
  return "FLAC";
}

// -------------------------------------------------------------------------------------------------

void TFlacFile::OnOpenForRead(
  const TString&  FileName, 
  int             FileSubTrackIndex)
{
  MUnused(FileSubTrackIndex);


  // . open and init the file
  
  mFile.SetFileName(FileName);
  mOverviewFile.SetFileName(FileName);
  
  if (!mFile.Open(TFile::kRead) || !mOverviewFile.Open(TFile::kRead))
  {
    throw TReadableException(MText("Failed to open the file for reading."));
  }
  
  
  // . open and init the decoder
  
  mpDecoderFile = new TFlacDecoderFile(&mFile);
  mpDecoderFile->Open();
  
  
  // . parse RIFF chunk data
  
  mRiffSamplerChunk = TSamplerChunk();
  mRiffSampleLoop = TSampleLoop();

  const TList< TArray<char> >& RiffMetaDataBlocks = 
    mpDecoderFile->RiffMetaDataBlocks();

  for (int i = 0; i < RiffMetaDataBlocks.Size(); ++i)
  {
    const size_t NumRiffHeaderBytes = 8; // ID + size

    // copy the first loop struct from smpl chunks
    if ((size_t)RiffMetaDataBlocks[i].Size() >= 
          NumRiffHeaderBytes + sizeof(TSamplerChunk) + sizeof(TSampleLoop) && 
        RiffMetaDataBlocks[i].FirstRead()[0] == 's' &&
        RiffMetaDataBlocks[i].FirstRead()[1] == 'm' &&
        RiffMetaDataBlocks[i].FirstRead()[2] == 'p' &&
        RiffMetaDataBlocks[i].FirstRead()[3] == 'l')
    {
      TSamplerChunk* pSamplerChunk = (TSamplerChunk*)
        ((char*)RiffMetaDataBlocks[i].FirstRead() + NumRiffHeaderBytes);

      TSampleLoop* pSampleLoop = (TSampleLoop*)
        ((char*)pSamplerChunk + sizeof(TSamplerChunk)); 

      mRiffSamplerChunk = *pSamplerChunk;
      mRiffSampleLoop = *pSampleLoop;
      
      // WAVE RIFFS are intel byte ordered
      #if (MSystemByteOrder == MMotorolaByteOrder)
        TByteOrder::Swap(mRiffSamplerChunk.mManufacturer);
        TByteOrder::Swap(mRiffSamplerChunk.mProduct);
        TByteOrder::Swap(mRiffSamplerChunk.mSamplePeriod);
        TByteOrder::Swap(mRiffSamplerChunk.mMIDIUnityNote);
        TByteOrder::Swap(mRiffSamplerChunk.mMIDIPitchFraction);
        TByteOrder::Swap(mRiffSamplerChunk.mSMPTEFormat);
        TByteOrder::Swap(mRiffSamplerChunk.mSMPTEOffset);
        TByteOrder::Swap(mRiffSamplerChunk.mSampleLoops);
        TByteOrder::Swap(mRiffSamplerChunk.mSamplerData);
        
        // mRiffSampleLoop.mIdentifier[4]
        TByteOrder::Swap(mRiffSampleLoop.mType);
        TByteOrder::Swap(mRiffSampleLoop.mStart);
        TByteOrder::Swap(mRiffSampleLoop.mEnd);
        TByteOrder::Swap(mRiffSampleLoop.mFraction);
        TByteOrder::Swap(mRiffSampleLoop.mPlayCount);
      #endif
      
      MAssert((size_t)RiffMetaDataBlocks[i].Size() == NumRiffHeaderBytes +
        sizeof(TSamplerChunk) + mRiffSamplerChunk.mSampleLoops * sizeof(TSampleLoop), 
        "Unexpected chunk size");
        
      MAssert((size_t)mRiffSamplerChunk.mSamplerData == 
        mRiffSamplerChunk.mSampleLoops * sizeof(TSampleLoop), 
        "Unexpected samplerdata size");
    }
  }

  
  // . open the file stream
  
  mpStream = new TFlacFileStream(mpDecoderFile);  
}

// -------------------------------------------------------------------------------------------------

void TFlacFile::OnOpenForWrite(
  const TString&  Filename,
  int             SamplingRate,
  TSampleType     SampleType,
  int             NumChannels)
{
  // . validate format

  if (!SIsBitDepthSupported(SNumBitsFromFlacSampleType(SampleType)))
  {
    throw TReadableException(MText("FLAC only supports "
      "8, 16, 24 or 32 bit integer formats."));
  }
  
  if (!SIsSampleRateSupported(SamplingRate))
  {
    throw TReadableException(MText("FLAC only supports sample rates "
      "8, 16, 22.05, 24, 32, 44.1, 48 or 96 kHz."));
  }
  
  
  // . open the file
  
  mFile.SetFileName(Filename);
  
  if (!mFile.Open(TFile::kWrite))
  {
    throw TReadableException(MText("Failed to open the file for writing."));
  }

  
  // . create the encoder
  
  mpEncoderFile = new TFlacEncoderFile(&mFile, NumChannels, SamplingRate, 
    TAudioFile::SNumBitsFromSampleType(SampleType));
  

  // . add RIFF chunks (if needed)

  if (mRiffSamplerChunk.mSampleLoops > 0)
  {
    MAssert(mRiffSamplerChunk.mSampleLoops == 1, 
      "Expecting exactly one loop here");

    const size_t NumRiffHeaderBytes = 8; // ID + Size
    
    TArray<char> RiffSamplerCunk(NumRiffHeaderBytes + 
      sizeof(TSamplerChunk) + sizeof(TSampleLoop));

    // RIFF Chunk Header
    TUInt32* pChunkId = (TUInt32*)&(RiffSamplerCunk.FirstWrite()[0]);
    TUInt32* pChunkSize = (TUInt32*)&(RiffSamplerCunk.FirstWrite()[4]);
    *pChunkId = gFourCC("smpl");
    *pChunkSize = sizeof(TSamplerChunk) + sizeof(TSampleLoop);
   
    // WAVE RIFFS are intel byte ordered
    #if (MSystemByteOrder == MMotorolaByteOrder)
      TByteOrder::Swap(*pChunkSize);
    #endif
    
    // RIFF Chunk Data
    TSamplerChunk* pSamplerChunk = 
      (TSamplerChunk*)&(RiffSamplerCunk.FirstWrite()[8]);
    
    TSampleLoop* pSampleLoop = 
      (TSampleLoop*)((char*)pSamplerChunk + sizeof(TSamplerChunk));

    *pSamplerChunk = mRiffSamplerChunk;
    *pSampleLoop = mRiffSampleLoop;

    // WAVE RIFFS are intel byte ordered
    #if (MSystemByteOrder == MMotorolaByteOrder)
      TByteOrder::Swap(pSamplerChunk->mManufacturer);
      TByteOrder::Swap(pSamplerChunk->mProduct);
      TByteOrder::Swap(pSamplerChunk->mSamplePeriod);
      TByteOrder::Swap(pSamplerChunk->mMIDIUnityNote);
      TByteOrder::Swap(pSamplerChunk->mMIDIPitchFraction);
      TByteOrder::Swap(pSamplerChunk->mSMPTEFormat);
      TByteOrder::Swap(pSamplerChunk->mSMPTEOffset);
      TByteOrder::Swap(pSamplerChunk->mSampleLoops);
      TByteOrder::Swap(pSamplerChunk->mSamplerData);
      
      // pSampleLoop->mIdentifier[4]
      TByteOrder::Swap(pSampleLoop->mType);
      TByteOrder::Swap(pSampleLoop->mStart);
      TByteOrder::Swap(pSampleLoop->mEnd);
      TByteOrder::Swap(pSampleLoop->mFraction);
      TByteOrder::Swap(pSampleLoop->mPlayCount);
    #endif
    
    mpEncoderFile->AddRiffMetaDataChunk(RiffSamplerCunk);
  }

  
  // . open the encoder
  
  mpEncoderFile->Open();
  

  // . create the file stream

  mpStream = new TFlacFileStream(mpEncoderFile); 
}

// -------------------------------------------------------------------------------------------------

void TFlacFile::OnClose()
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

  if (mpEncoderFile)
  {
    mpEncoderFile->Close();
    mpEncoderFile.Delete();
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

TAudioStream* TFlacFile::OnStream()const
{
  return mpStream;
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TFlacFile::OnOverviewStream()const
{
  if (!mpOverviewStream)
  {
    TFlacFile* pMutableThis = const_cast<TFlacFile*>(this);
    
    pMutableThis->mpDecoderOverviewFile = 
      new TFlacDecoderFile(&pMutableThis->mOverviewFile);
    
    pMutableThis->mpDecoderOverviewFile->Open();

    pMutableThis->mpOverviewStream = 
      new TFlacFileStream(pMutableThis->mpDecoderOverviewFile);
  }

  return mpOverviewStream;
}

// -------------------------------------------------------------------------------------------------

int TFlacFile::OnSamplingRate()const
{
  return mpDecoderFile->SampleRate();
}

// -------------------------------------------------------------------------------------------------

int TFlacFile::OnBitsPerSample()const
{
  return mpDecoderFile->NumberOfBits();
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TFlacFile::OnSampleType()const
{
  return mpDecoderFile->SampleType();
}

// -------------------------------------------------------------------------------------------------

int TFlacFile::OnNumChannels()const
{
  return mpDecoderFile->NumberOfChannels();
}

// -------------------------------------------------------------------------------------------------

long long TFlacFile::OnNumSamples() const
{
  return mpDecoderFile->TotalNumberOfSamples();
}

// -------------------------------------------------------------------------------------------------

TAudioTypes::TLoopMode TFlacFile::OnLoopMode()const
{
  // > check to ignore invalid loops (sometimes the loop start 
  // is loop end if no loop is present)...

  if (mRiffSamplerChunk.mSampleLoops > 0 && 
      mRiffSampleLoop.mEnd > mRiffSampleLoop.mStart)
  {
    switch (mRiffSampleLoop.mType)
    {
    default:
      // threat unknown, custom loops as forward loops...
      return TAudioTypes::kLoopForward;
  
    case 0:
      return TAudioTypes::kLoopForward;

    case 1:
      return TAudioTypes::kLoopPingPong;

    case 2:
      return TAudioTypes::kLoopBackward;
    }
  }
  else
  {
    // no sampler chunk present
    return TAudioTypes::kNoLoop; 
  }
}

// -------------------------------------------------------------------------------------------------

long long TFlacFile::OnLoopStart()const
{
  return mRiffSampleLoop.mStart;
}

// -------------------------------------------------------------------------------------------------

long long TFlacFile::OnLoopEnd()const
{
  return mRiffSampleLoop.mEnd;
}

// -------------------------------------------------------------------------------------------------

void TFlacFile::OnSetLoop(
  TAudioTypes::TLoopMode LoopMode, 
  long long              LoopStart, 
  long long              LoopEnd)
{
  MAssert(mpEncoderFile == NULL, 
    "Need the loops before opening the file!");

  if (LoopMode == TAudioTypes::kNoLoop)
  {
    mRiffSamplerChunk.mSampleLoops = 0;
    mRiffSamplerChunk.mSamplerData = 0;
  }
  else
  {
    switch (LoopMode)
    {
    default: 
      MInvalid("Unepected loop mode");

    case TAudioTypes::kLoopForward:
      mRiffSampleLoop.mType = 0;
      break;

    case TAudioTypes::kLoopPingPong:
      mRiffSampleLoop.mType = 1;
      break;

    case TAudioTypes::kLoopBackward:
      mRiffSampleLoop.mType = 2;
      break;
    }

    mRiffSampleLoop.mStart = (TUInt32)LoopStart;
    mRiffSampleLoop.mEnd = (TUInt32)LoopEnd;
    mRiffSampleLoop.mFraction = 0;
    mRiffSampleLoop.mPlayCount = 0;

    mRiffSamplerChunk.mSampleLoops = 1;
    mRiffSamplerChunk.mSamplerData = sizeof(TSampleLoop);
  }
}

// -------------------------------------------------------------------------------------------------

TArray<long long> TFlacFile::OnCuePoints()const 
{ 
  // Not yet supported 
  return TArray<long long>(); 
}

// -------------------------------------------------------------------------------------------------

void TFlacFile::OnSetCuePoints(const TArray<long long>& CuePoints) 
{ 
  // Not yet supported 
}

