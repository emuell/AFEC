#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/System.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#include <propvarutil.h>

#pragma comment (lib, "mf.lib")
#pragma comment (lib, "mfplat.lib")
#pragma comment (lib, "mfreadwrite.lib")
#pragma comment (lib, "mfuuid.lib")

// =================================================================================================

#define LOG_PREFIX "Media Foundation"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <class T> void SafeRelease(T **ppT)
{
  if (*ppT)
  {
    (*ppT)->Release();
    *ppT = NULL;
  }
}

// -------------------------------------------------------------------------------------------------

#define LogMediaFoundationError(hr, ERROR_STRING) \
  if (FAILED(hr)) \
  { \
    TLog::SLog()->AddLine(LOG_PREFIX, \
      ERROR_STRING " (Error: %x)", \
      HRESULT_CODE(hr)); \
  } \

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TMediaFoundationAudioDecoder::TMediaFoundationAudioDecoder(const TString& FileName)
  : mFileName(FileName),
    mInitializedCom(false),
    mInitializedMF(false),
    mpSourceReader(NULL),
    mpPcmFormat(NULL),
    mSourceReaderDied(false),
    mPreferedBlockSize(0),
    mStreamDuration(0),
    mpSampleBuffer(NULL),
    mSampleBufferSizeInBytes(0),
    mSampleBufferBytesWritten(0)
{
}

// -------------------------------------------------------------------------------------------------

TMediaFoundationAudioDecoder::~TMediaFoundationAudioDecoder()
{
  if (IsOpen())
  {
    Close();
  }
}

// -------------------------------------------------------------------------------------------------

bool TMediaFoundationAudioDecoder::IsOpen()const
{
  return (mpSourceReader != NULL && mpPcmFormat != NULL);
}

// -------------------------------------------------------------------------------------------------

void TMediaFoundationAudioDecoder::Open() 
{
  MAssert(! IsOpen(), "Already open");


  // . Initialize COM

  HRESULT hr;
  if (FAILED(hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
  {
    MInvalid("COM failed to init. "
      "Already initialized in a different mode?");

    TLog::SLog()->AddLine(LOG_PREFIX,
      "COM FAILED to initialize (Error: %d)!", hr);

    mInitializedCom = false;
  }
  else
  {
    mInitializedCom = true;
  }


  // . Initialize the Media Foundation platform

  try 
  {
    hr = ::MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
      throw TReadableException(MText("Failed to initialize Media Foundation [%s]", 
        ToString(HRESULT_CODE(hr), "%x")));
    }
    else 
    {
      mInitializedMF = true;
    }

    // Create the source reader to read the input file.
    hr = ::MFCreateSourceReaderFromURL(mFileName.Chars(), NULL, &mpSourceReader);
    if (FAILED(hr))
    {
      throw TReadableException(
        MText("Error opening input file [%s]", ToString(HRESULT_CODE(hr), "%x")));
    }

    hr = ConfigureAudioStream();
    if (FAILED(hr))
    {
      throw TReadableException(
        MText("Stream configuration failed [%s]", ToString(HRESULT_CODE(hr), "%x")));
    }

    if (TotalFrameCount() == 0)
    {
      throw TReadableException(
        MText("Failed to get audio stream duration or stream is empty"));
    }

    // Seek to position 0 to force the source reader to to skip over all header frames
    SeekTo(0);
  }
  catch (const TReadableException& Exception)
  {
    Close();

    throw TReadableException(MText(
      "Media Foundation Error: no decoder installed which can handle the given "
      "file, or decoding failed (Internal Error: '%s').", Exception.Message()));
  }
}

// -------------------------------------------------------------------------------------------------

void TMediaFoundationAudioDecoder::Close()
{
  // release temp buffer
  if (mpSampleBuffer) 
  { 
    TMemory::Free(mpSampleBuffer);
    mpSampleBuffer = NULL;
  }
  mSampleBufferSizeInBytes = 0;
  mSampleBufferBytesWritten = 0;

  mSourceReaderDied = true;
  mPreferedBlockSize = 0;
  mStreamDuration = 0;

  // release media foundation resources
  if (mpPcmFormat) 
  {
    ::CoTaskMemFree(mpPcmFormat);
    mpPcmFormat = NULL;
  }

  ::SafeRelease(&mpSourceReader);

  if (mInitializedMF)
  {
    ::MFShutdown();
    mInitializedMF = false;
  }

  // uninitialize COM
  if (mInitializedCom)
  {
    ::CoUninitialize();
    mInitializedCom = false;
  }
}

// -------------------------------------------------------------------------------------------------

long long TMediaFoundationAudioDecoder::TotalFrameCount()const 
{ 
  if (!mpPcmFormat)
  {
    MInvalid("Not initialized");
    return 0;
  }

  return FrameFromMF(mStreamDuration);
}

// -------------------------------------------------------------------------------------------------

int TMediaFoundationAudioDecoder::NumberOfChannels()const 
{ 
  if (mpPcmFormat == NULL)
  {
    MInvalid("Not initialized");
    return 0;
  }

  return mpPcmFormat->nChannels;
}

// -------------------------------------------------------------------------------------------------

int TMediaFoundationAudioDecoder::SamplesPerSec()const 
{ 
  if (mpPcmFormat == NULL)
  {
    MInvalid("Not initialized");
    return 0;
  }

  return mpPcmFormat->nSamplesPerSec;
}

// -------------------------------------------------------------------------------------------------

int TMediaFoundationAudioDecoder::BitsPerSample()const 
{ 
  if (mpPcmFormat == NULL)
  {
    MInvalid("Not initialized");
    return 0;
  }

  return mpPcmFormat->wBitsPerSample;
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TMediaFoundationAudioDecoder::SampleType()const
{
  if (mpPcmFormat == NULL)
  {
    MInvalid("Not initialized");
    return TAudioFile::k16Bit;
  }

  return TWaveFormatChunkData::SSampleType(
    mpPcmFormat->wFormatTag, mpPcmFormat->wBitsPerSample);
}

// -------------------------------------------------------------------------------------------------

int TMediaFoundationAudioDecoder::BlockSize()const
{
  if (mpPcmFormat == NULL)
  {
    MInvalid("Not initialized");
    return 0;
  }

  return (mPreferedBlockSize == 0) ? 8192 : mPreferedBlockSize;
}

// -------------------------------------------------------------------------------------------------

bool TMediaFoundationAudioDecoder::SeekTo(long long SampleFrame)
{
  if (! IsOpen())
  {
    MInvalid("Not initialized");
    return false;
  }

  if (mSourceReaderDied)
  {
    return false;
  }

  HRESULT hr = S_OK;

  // flush source reader
  hr = mpSourceReader->Flush((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM);
  if (FAILED(hr))
  {
    TLog::SLog()->AddLine(LOG_PREFIX, "Failed to flush before seek");
    return false;
  }

  // flush internal buffer
  mSampleBufferBytesWritten = 0;

  // minus 1 here seems to make our seeking work properly, otherwise we will
  // (more often than not, maybe always) seek a bit too far
  const long long SeekTarget = MMax(0LL, MFFromFrame(SampleFrame) - 1);

  PROPVARIANT prop;
  hr = InitPropVariantFromInt64(SeekTarget, &prop);
  // this doesn't fail, see MS's implementation
  MUnused(hr);

  // seek source reader
  hr = mpSourceReader->SetCurrentPosition(GUID_NULL, prop);
  if (FAILED(hr))
  {
    TLog::SLog()->AddLine(LOG_PREFIX, "Failed to seek %s",
      hr == MF_E_INVALIDREQUEST ? "Sample requests still pending" : "");
    PropVariantClear(&prop);
    return false;
  }

  PropVariantClear(&prop);
  return true;
}

// -------------------------------------------------------------------------------------------------

void TMediaFoundationAudioDecoder::ReadBuffer(void* pData, int NumSampleFrames)
{
  const int BufferByteSize = (mpPcmFormat->wBitsPerSample + 7) / 8 *
    NumSampleFrames * mpPcmFormat->nChannels;

  // read new samples until we got enough sample to fill the requested buffer
  while (! mSourceReaderDied && mSampleBufferBytesWritten < BufferByteSize)
  {
    HRESULT hr = S_OK;

    // Read the next sample.
    long long CurrentTime = 0;
    DWORD dwFlags = 0;
    IMFSample *pSample = NULL;

    hr = mpSourceReader->ReadSample(
      (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
      0,
      NULL,
      &dwFlags,
      &CurrentTime,
      &pSample
    );

    if (FAILED(hr))
    {
      TLog::SLog()->AddLine(LOG_PREFIX,
        "ReadSample failed - Error: %x", HRESULT_CODE(hr));
      break;
    }

    if (dwFlags & MF_SOURCE_READERF_ERROR)
    {
      TLog::SLog()->AddLine(LOG_PREFIX,
        "ReadSample set ERROR, SourceReader is now dead");
      // our source reader is now dead - don't try to access it again
      mSourceReaderDied = true;
      break;
    }
    else if (dwFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
    {
      TLog::SLog()->AddLine(LOG_PREFIX, "Type change in stream - not supported");
      break;
    }
    else if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
    {
      // Expected: End of input file
      break;
    }

    if (pSample == NULL)
    {
      TLog::SLog()->AddLine(LOG_PREFIX, "No sample present at current position");
      continue;
    }

    // Get a pointer to the audio data in the sample.
    IMFMediaBuffer *pBuffer = NULL;
    hr = pSample->ConvertToContiguousBuffer(&pBuffer);
    if (FAILED(hr))
    {
      TLog::SLog()->AddLine(LOG_PREFIX,
        "ConvertToContiguousBuffer failed - Error: %x", HRESULT_CODE(hr));
      ::SafeRelease(&pSample);
      break;
    }

    BYTE *pAudioData = NULL;
    DWORD AudioDataNumberOfBytes = 0;

    hr = pBuffer->Lock(&pAudioData, NULL, &AudioDataNumberOfBytes);
    if (FAILED(hr))
    {
      TLog::SLog()->AddLine(LOG_PREFIX,
        "Buffer lock failed - Error: %x", HRESULT_CODE(hr));
      ::SafeRelease(&pSample);
      break;
    }
    
    // Ensure temp buffer can hold the data
    const int NeededSize = mSampleBufferSizeInBytes + AudioDataNumberOfBytes;
    const int AllocSize = TMath::NextPow2(64 * NeededSize);
    
    if (mpSampleBuffer == NULL)
    {
      mpSampleBuffer = TMemory::Alloc("#MFSampleBuffer", AllocSize);
      mSampleBufferSizeInBytes = AllocSize;
    }
    else if (mSampleBufferBytesWritten + 
        (int)AudioDataNumberOfBytes > mSampleBufferSizeInBytes)
    {
      mpSampleBuffer = TMemory::Realloc(mpSampleBuffer, AllocSize);
      mSampleBufferSizeInBytes = AllocSize;
    }

    // append current sample data to the temp buffer
    TMemory::Copy(
      (char*)mpSampleBuffer + mSampleBufferBytesWritten,
      pAudioData, 
      (int)AudioDataNumberOfBytes
    );
    
    mSampleBufferBytesWritten += (int)AudioDataNumberOfBytes;

    // Unlock the buffer (ignoring errors).
    pBuffer->Unlock();
    pAudioData = NULL;

    ::SafeRelease(&pSample);
    ::SafeRelease(&pBuffer);
  }

  // copy data from temp buffer into dest buffer
  if (mSampleBufferBytesWritten >= BufferByteSize)
  {
    // consume up to BufferByteSize from the temp buffer
    TMemory::Copy(pData, mpSampleBuffer, BufferByteSize);

    // shunt remaining temp data
    const int RemainingSamples = mSampleBufferBytesWritten - BufferByteSize;

    TMemory::Move(
      mpSampleBuffer,
      (char*)mpSampleBuffer + BufferByteSize,
      RemainingSamples
    );

    mSampleBufferBytesWritten -= BufferByteSize;
  }
  else
  {
    // consume data that could be read
    TMemory::Copy(pData, mpSampleBuffer, mSampleBufferBytesWritten);

    // and clear the rest of the buffer
    TMemory::Zero(
      (TInt8*)pData + mSampleBufferBytesWritten,
      BufferByteSize - mSampleBufferBytesWritten
    );

    mSampleBufferBytesWritten = 0;
  }
}

// -------------------------------------------------------------------------------------------------

HRESULT TMediaFoundationAudioDecoder::ConfigureAudioStream()
{
  HRESULT hr = S_OK;

  IMFMediaType *pUncompressedAudioType = NULL;
  IMFMediaType *pPartialType = NULL;

  // Create a partial media type that specifies uncompressed PCM audio.
  hr = ::MFCreateMediaType(&pPartialType);
  LogMediaFoundationError(hr, "Failed to create media type")

  if (SUCCEEDED(hr))
  {
    hr = pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    LogMediaFoundationError(hr, "Failed to set media main type")
  }

  if (SUCCEEDED(hr))
  {
    hr = pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    LogMediaFoundationError(hr, "Failed to set media sub type")
  }

  // Set this type on the source reader. The source reader will load the necessary decoder.
  if (SUCCEEDED(hr))
  {
    hr = mpSourceReader->SetCurrentMediaType(
      (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
      NULL,
      pPartialType
    );
    LogMediaFoundationError(hr, "Failed to select media type")
  }

  // Get the complete uncompressed format.
  if (SUCCEEDED(hr))
  {
    hr = mpSourceReader->GetCurrentMediaType(
      (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
      &pUncompressedAudioType
    );
    LogMediaFoundationError(hr, "Failed to fetch the uncompressed format")
  }

  // Seselect all streams, we only want the first only
  if (SUCCEEDED(hr))
  {
    hr = mpSourceReader->SetStreamSelection(
      (DWORD)MF_SOURCE_READER_ALL_STREAMS, 
      FALSE
    );
    LogMediaFoundationError(hr, "Failed to deselect all streams")
  }

  // Ensure the first audio stream is selected
  if (SUCCEEDED(hr))
  {
    hr = mpSourceReader->SetStreamSelection(
      (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
      TRUE
    );
    LogMediaFoundationError(hr, "Failed to select the first audio stream")
  }

  // Fetch PCM format 
  if (SUCCEEDED(hr))
  {
    // Convert the PCM audio format into a WAVEFORMATEX structure.
    UINT32 SizeOfFormat = 0;
    hr = ::MFCreateWaveFormatExFromMFMediaType(
      pUncompressedAudioType,
      &mpPcmFormat,
      &SizeOfFormat
    );
    LogMediaFoundationError(hr, "Failed to convert audio format")
  }

  // Get preferred block size
  if (SUCCEEDED(hr))
  {
    TUInt32 HasFixedSampleSize = 0;
    hr = pUncompressedAudioType->GetUINT32(
      MF_MT_FIXED_SIZE_SAMPLES,
      &HasFixedSampleSize
    );
    LogMediaFoundationError(hr, "Failed to get buffer size")

    if (!!HasFixedSampleSize) 
    { 
      TUInt32 SampleSize = 0;

      hr = pUncompressedAudioType->GetUINT32(
        MF_MT_SAMPLE_SIZE,
        &SampleSize
      );
      // This will usually fail, don't log an error: 
      // LogMediaFoundationError(hr, "Failed to get buffer size")
      if (SUCCEEDED(hr)) 
      {
        mPreferedBlockSize = (int)SampleSize;
      }
    }

    // errors from MF_MT_SAMPLE_SIZE are not fatal
    hr = S_OK;
  }

  // Get total stream duration
  if (SUCCEEDED(hr))
  {
    PROPVARIANT prop;

    hr = mpSourceReader->GetPresentationAttribute(
      (DWORD)MF_SOURCE_READER_MEDIASOURCE,
      MF_PD_DURATION, 
      &prop
    );
    LogMediaFoundationError(hr, "Failed to fetch duration")
    
    if (SUCCEEDED(hr))
    {
      mStreamDuration = prop.hVal.QuadPart;
      PropVariantClear(&prop);
    }
  }

  // clean up all temp handles
  ::SafeRelease(&pUncompressedAudioType);
  ::SafeRelease(&pPartialType);
  
  return hr;
}

// -------------------------------------------------------------------------------------------------

long long TMediaFoundationAudioDecoder::FrameFromMF(long long mf) const
{
  if (!mpPcmFormat)
  {
    MInvalid("Not initialized");
    return 0;
  }

  return (long long)((double)mf * mpPcmFormat->nSamplesPerSec / 1e7);
}

// -------------------------------------------------------------------------------------------------

long long TMediaFoundationAudioDecoder::MFFromFrame(long long frame) const
{
  if (!mpPcmFormat)
  {
    MInvalid("Not initialized");
    return 0;
  }
  
  return (long long)((double)frame / mpPcmFormat->nSamplesPerSec * 1e7);
}

// =================================================================================================

class TMediaFoundationAudioFileStream : public TAudioStream
{
public:
  TMediaFoundationAudioFileStream(TMediaFoundationAudioDecoder* pMediaFoundationFile);
  
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

  TMediaFoundationAudioDecoder* mpMediaFoundationDecoderFile;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TMediaFoundationAudioFileStream::TMediaFoundationAudioFileStream(
  TMediaFoundationAudioDecoder* pAudioFile)
  : TAudioStream(
      pAudioFile->TotalFrameCount(), 
      pAudioFile->NumberOfChannels(), 
      pAudioFile->SampleType()
    ),
    mpMediaFoundationDecoderFile(pAudioFile)
{
}

// -------------------------------------------------------------------------------------------------

int TMediaFoundationAudioFileStream::OnPreferedBlockSize()const
{
  return mpMediaFoundationDecoderFile->BlockSize();
}

// -------------------------------------------------------------------------------------------------

long long TMediaFoundationAudioFileStream::OnBytesWritten()const
{
  MCodeMissing;
  return 0;
}

// -------------------------------------------------------------------------------------------------

bool TMediaFoundationAudioFileStream::OnSeekTo(long long Frame)
{
  return mpMediaFoundationDecoderFile->SeekTo(Frame);
}

// -------------------------------------------------------------------------------------------------

void TMediaFoundationAudioFileStream::OnReadBuffer(
  void* pDestBuffer, 
  int   NumberOfSampleFrames)
{
  mpMediaFoundationDecoderFile->ReadBuffer(pDestBuffer, NumberOfSampleFrames);
}

// -------------------------------------------------------------------------------------------------

void TMediaFoundationAudioFileStream::OnWriteBuffer(
  const void* pSrcBuffer, 
  int         NumberOfSampleFrames)
{
  MCodeMissing;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TList<TString> TMediaFoundationAudioFile::SSupportedExtensions()
{
  TList<TString> AudioFileExtensions;
  AudioFileExtensions.Append("*.aac");
  AudioFileExtensions.Append("*.ac3");
  AudioFileExtensions.Append("*.mp3");
  AudioFileExtensions.Append("*.mp4a");
  AudioFileExtensions.Append("*.m4a");
  AudioFileExtensions.Append("*.wav");
  AudioFileExtensions.Append("*.wma");

  return AudioFileExtensions;
}

// -------------------------------------------------------------------------------------------------

bool TMediaFoundationAudioFile::SContainsAudio(
  const TString&  FileNameAndPath, 
  TList<TString>* pContainedAudioTracks)
{
  if (pContainedAudioTracks) // TODO
  {
    *pContainedAudioTracks = MakeList<TString>(
      gCutExtensionAndPath(FileNameAndPath));
  }

  const bool AlreadyExtracted = 
    TFile(gCutExtension(FileNameAndPath) + ".wav").Exists();
    
  if (AlreadyExtracted)
  {
    return true;
  }

  if (gFileMatchesExtension(FileNameAndPath, 
        MakeList<TString>("*.mpv", "*.m2v", "*.m1v")))
  {
    // demuxed video never has audio...
    return false;
  }
 
  // TODO: check file for audio streams
  return true; //  gFileMatchesExtension(FileNameAndPath, SSupportedExtensions());
}

// -------------------------------------------------------------------------------------------------

TMediaFoundationAudioFile::TMediaFoundationAudioFile()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

TString TMediaFoundationAudioFile::OnTypeString()const
{
  return "MF";
}

// -------------------------------------------------------------------------------------------------

void TMediaFoundationAudioFile::OnOpenForRead(
  const TString&  Filename, 
  int             FileSubTrackIndex)
{
  MUnused(FileSubTrackIndex); // not implemented

  mpDecoderFile = new TMediaFoundationAudioDecoder(Filename);
  mpDecoderFile->Open();
  
  mpStream = new TMediaFoundationAudioFileStream(mpDecoderFile);  

  // the overview file and stream is created on the fly, when needed
}

// -------------------------------------------------------------------------------------------------

void TMediaFoundationAudioFile::OnOpenForWrite(
  const TString&  Filename,
  int             SamplingRate,
  TSampleType     SampleType,
  int             NumChannels)
{
  throw TReadableException(
    MText("Writing MediaFoundation files currently is not supported."));
}

// -------------------------------------------------------------------------------------------------

void TMediaFoundationAudioFile::OnClose()
{
  if (mpDecoderFile)
  {
    mpDecoderFile->Close();
  }

  if (mpOverviewDecoderFile)
  {
    mpOverviewDecoderFile->Close();
  }

  mpStream.Delete();
  mpOverviewStream.Delete();
  
  mpDecoderFile.Delete();
  mpOverviewDecoderFile.Delete();
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TMediaFoundationAudioFile::OnStream()const
{
  return mpStream;
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TMediaFoundationAudioFile::OnOverviewStream()const
{
  if (! mpOverviewStream)
  {
    TMediaFoundationAudioFile* pMutableThis = 
      const_cast<TMediaFoundationAudioFile*>(this);
    
    pMutableThis->mpOverviewDecoderFile = 
      new TMediaFoundationAudioDecoder(pMutableThis->FileName());
    
    pMutableThis->mpOverviewDecoderFile->Open();

    pMutableThis->mpOverviewStream = 
      new TMediaFoundationAudioFileStream(pMutableThis->mpOverviewDecoderFile);
  }

  return mpOverviewStream;
}

// -------------------------------------------------------------------------------------------------

int TMediaFoundationAudioFile::OnSamplingRate()const
{
  return mpDecoderFile->SamplesPerSec();
}

// -------------------------------------------------------------------------------------------------

int TMediaFoundationAudioFile::OnBitsPerSample()const
{
  return mpDecoderFile->BitsPerSample();
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TMediaFoundationAudioFile::OnSampleType()const
{
  return mpDecoderFile->SampleType();
}

// -------------------------------------------------------------------------------------------------

int TMediaFoundationAudioFile::OnNumChannels()const
{
  return mpDecoderFile->NumberOfChannels();
}

// -------------------------------------------------------------------------------------------------

long long TMediaFoundationAudioFile::OnNumSamples()const
{
  return mpDecoderFile->TotalFrameCount();
}

// -------------------------------------------------------------------------------------------------

TAudioTypes::TLoopMode TMediaFoundationAudioFile::OnLoopMode()const
{
  return TAudioTypes::kNoLoop; // Not supported...
}

// -------------------------------------------------------------------------------------------------

long long TMediaFoundationAudioFile::OnLoopStart()const
{
  return -1; // Not supported...
}

// -------------------------------------------------------------------------------------------------

long long TMediaFoundationAudioFile::OnLoopEnd()const
{
  return -1; // Not supported...
}

// -------------------------------------------------------------------------------------------------

void TMediaFoundationAudioFile::OnSetLoop(
  TAudioTypes::TLoopMode LoopMode, 
  long long              LoopStart, 
  long long              LoopEnd)
{
  // Not supported...
}

// -------------------------------------------------------------------------------------------------

TArray<long long> TMediaFoundationAudioFile::OnCuePoints()const
{ 
  return TArray<long long>(); // Not supported...
}

// -------------------------------------------------------------------------------------------------

void TMediaFoundationAudioFile::OnSetCuePoints(const TArray<long long>& CuePoints) 
{ 
  // Not supported...
}

