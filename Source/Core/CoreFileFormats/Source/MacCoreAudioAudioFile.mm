#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/Directory.h"

#include <Foundation/Foundation.h>
#include <AudioToolbox/AudioToolbox.h>

// =================================================================================================

#define MCoreAudioDecoderBitDepth 16

// =================================================================================================

class TCoreAudioAudioFileStream : public TAudioStream
{
public:
  TCoreAudioAudioFileStream(TCoreAudioAudioDecoder* pCoreAudioFile);
  
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

  TCoreAudioAudioDecoder* mpCoreAudioDecoderFile;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TCoreAudioAudioDecoder::TCoreAudioAudioDecoder(const TString& FileName)
  : mFileName(FileName),
    mTotalNumberOfSamples(0),
    mSampleRate(0),
    mNumberOfBits(0),
    mNumberOfChannels(0),
    mPreferredBlockSize(0),
    mBufferSampleFramePos(0),
    mpExtAudioFileRef(NULL)
{
  MAssert(TFile(FileName).Exists(), "File must exist");
}

// -------------------------------------------------------------------------------------------------

TCoreAudioAudioDecoder::~TCoreAudioAudioDecoder()
{
  if (mpExtAudioFileRef != NULL)
  {
    Close();
  }
}

// -------------------------------------------------------------------------------------------------

bool TCoreAudioAudioDecoder::IsOpen()const
{
  return (mpExtAudioFileRef != NULL);
}

// -------------------------------------------------------------------------------------------------

void TCoreAudioAudioDecoder::Open() 
{
  M__DisableFloatingPointAssertions

  OSStatus err = noErr;
  
  
  // ... Open the file
  
  NSString* pParentPath = gCreateNSString(
    gExtractPath(mFileName).Path());
    
  FSRef ParentRef;
  err = ::FSPathMakeRef((const UInt8*)[[NSFileManager defaultManager]
    fileSystemRepresentationWithPath: pParentPath], &ParentRef, NULL);
   
  if (err != noErr)
  {
    throw TReadableException(MText(
      "CoreAudio: Failed to open the file for reading."));
  }
       
  // core service treats ':' as '/'
  const TString BaseFileName = gCutPath(mFileName).ReplaceChar(':', '/');
  
  FSRef ref; 
  err = ::FSMakeFSRefUnicode(&ParentRef, BaseFileName.Size(), 
    BaseFileName.Chars(), kTextEncodingDefaultFormat, &ref);

  err = ::ExtAudioFileOpen(&ref, &mpExtAudioFileRef);
  
  if (err != noErr)
  {
    M__EnableFloatingPointAssertions
  
    throw TReadableException(MText(
      "CoreAudio: Failed to open the file for reading, or no decoder is "
      "installed that can handle this kind of audio."));
  }
    
    
  // ... Get input data formats
  
  AudioStreamBasicDescription StreamDescription;
  TMemory::Zero(&StreamDescription, sizeof(StreamDescription));

  UInt32 PropertySize = sizeof(AudioStreamBasicDescription);
  
  err = ::ExtAudioFileGetProperty(mpExtAudioFileRef, 
    kExtAudioFileProperty_FileDataFormat, &PropertySize, &StreamDescription);
    
  if (err != noErr)
  {
    ::ExtAudioFileDispose(mpExtAudioFileRef);
    mpExtAudioFileRef = NULL;
    
    M__EnableFloatingPointAssertions
    
    throw TReadableException(MText(
      "CoreAudio: failed to query the input format."));
  }
  
  // Set block size from the formats mFramesPerPacket
  if (StreamDescription.mFramesPerPacket > 0 && 
      StreamDescription.mFramesPerPacket < 16384)
  {
    mPreferredBlockSize = StreamDescription.mFramesPerPacket;

    if (mPreferredBlockSize < 8192) // use multiples of mPreferredBlockSize
    {
      mPreferredBlockSize *= 8192 / mPreferredBlockSize;
    }
  }
  else
  {
    mPreferredBlockSize = 16384;
  }
  
  
  // ... Set output data format

  // Convert to return interleaved 16-bit PCM
  MStaticAssert(MCoreAudioDecoderBitDepth == 16);

  StreamDescription.mFormatID = kAudioFormatLinearPCM;
  StreamDescription.mFormatFlags = kAudioFormatFlagIsSignedInteger | 
    kAudioFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
  StreamDescription.mChannelsPerFrame = 
    MMin(2, StreamDescription.mChannelsPerFrame);
  StreamDescription.mBitsPerChannel = 16;
  StreamDescription.mBytesPerFrame = 2 * StreamDescription.mChannelsPerFrame;
  StreamDescription.mBytesPerPacket = StreamDescription.mBytesPerFrame;
  StreamDescription.mFramesPerPacket = 1;

  // apply the new format
  err = ::ExtAudioFileSetProperty(mpExtAudioFileRef, 
    kExtAudioFileProperty_ClientDataFormat, 
    sizeof(StreamDescription), &StreamDescription);
    
  if (err != noErr)
  {
    ::ExtAudioFileDispose(mpExtAudioFileRef);
    mpExtAudioFileRef = NULL;
    
    M__EnableFloatingPointAssertions
    
    throw TReadableException(MText(
      "CoreAudio: Failed to set the decoder output format."));
  }
  
  
  // ... Set internal frame/channel properties
  
  mSampleRate = (int)(StreamDescription.mSampleRate + 0.5f);
  mNumberOfBits = MCoreAudioDecoderBitDepth;
  mNumberOfChannels = StreamDescription.mChannelsPerFrame;

  SInt64 FrameCount = 0;
  PropertySize = sizeof(SInt64);
  err = ::ExtAudioFileGetProperty(mpExtAudioFileRef, 
    kExtAudioFileProperty_FileLengthFrames, &PropertySize, &FrameCount);
  
  if (err != noErr)
  {
    ::ExtAudioFileDispose(mpExtAudioFileRef);
    mpExtAudioFileRef = NULL;
    
    M__EnableFloatingPointAssertions
    
    throw TReadableException(MText(
      "CoreAudio: Failed to query the file's lenght."));
  }
  
  mTotalNumberOfSamples = (long long)FrameCount;

  if (mTotalNumberOfSamples == 0)
  {
    throw TReadableException(MText(
      "CoreAudio: File is empty or not a valid audio file."));
  }
    
  M__EnableFloatingPointAssertions
}

// -------------------------------------------------------------------------------------------------

void TCoreAudioAudioDecoder::Close()
{
  M__DisableFloatingPointAssertions

  if (mpExtAudioFileRef)
  {
    ::ExtAudioFileDispose(mpExtAudioFileRef);
    mpExtAudioFileRef = NULL;
  }
  
  M__EnableFloatingPointAssertions
}

// -------------------------------------------------------------------------------------------------

long long TCoreAudioAudioDecoder::TotalNumberOfSamples()const 
{ 
  return mTotalNumberOfSamples; 
}

// -------------------------------------------------------------------------------------------------

int TCoreAudioAudioDecoder::NumberOfChannels()const 
{ 
  return mNumberOfChannels; 
}

// -------------------------------------------------------------------------------------------------

int TCoreAudioAudioDecoder::SampleRate()const 
{ 
  return mSampleRate; 
}

// -------------------------------------------------------------------------------------------------

int TCoreAudioAudioDecoder::BlockSize()const
{
  return mPreferredBlockSize;
}

// -------------------------------------------------------------------------------------------------

int TCoreAudioAudioDecoder::NumberOfBits()const 
{ 
  return mNumberOfBits; 
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TCoreAudioAudioDecoder::SampleType()const
{
  MStaticAssert(MCoreAudioDecoderBitDepth == 16);
  return TAudioFile::k16Bit;
}

// -------------------------------------------------------------------------------------------------

bool TCoreAudioAudioDecoder::SeekTo(long long SampleFrame)
{
  MAssert(mpExtAudioFileRef, "Decoder not open!");
  
  if (mBufferSampleFramePos == SampleFrame)
  {
    // no relocation needed
    return true;
  }

  M__DisableFloatingPointAssertions
  
  OSStatus err = ::ExtAudioFileSeek(mpExtAudioFileRef, SampleFrame);

  if (err == noErr)
  {
    mBufferSampleFramePos = SampleFrame;

    M__EnableFloatingPointAssertions
    return true;
  }
  else
  {
    M__EnableFloatingPointAssertions
    return false;
  }
}

// -------------------------------------------------------------------------------------------------

void TCoreAudioAudioDecoder::ReadBuffer(void* pData, int NumSampleFrames)
{
  MAssert(mpExtAudioFileRef, "Decoder not open!");

  const int BytesPerFrame = ((mNumberOfBits + 7) / 8) * mNumberOfChannels;
  const int TotalNumBytesInBuffer = NumSampleFrames * BytesPerFrame; 
  
  char BufferListMemory[sizeof(UInt32) + sizeof(AudioBuffer)];
  AudioBufferList* pBufferList = (AudioBufferList*)BufferListMemory;
  
  pBufferList->mNumberBuffers = 1;
  pBufferList->mBuffers[0].mNumberChannels = mNumberOfChannels;
  pBufferList->mBuffers[0].mDataByteSize = TotalNumBytesInBuffer;
  pBufferList->mBuffers[0].mData = pData;
  
  UInt32 LoadedFrames = NumSampleFrames;
  OSStatus err = ::ExtAudioFileRead(mpExtAudioFileRef, &LoadedFrames, pBufferList);

  if (err) 
  {
    MInvalid("Error while decompressing!");
    TMemory::Zero(pData, TotalNumBytesInBuffer);
  }
  else
  {
    // Note: last chunk can be smaller in some compressed formats: 
    // mTotalNumberOfSamples may be quantized by a "packet" size here
    MAssert(LoadedFrames == (unsigned)NumSampleFrames || 
      mBufferSampleFramePos + NumSampleFrames == mTotalNumberOfSamples, "");

    if (LoadedFrames < (unsigned)NumSampleFrames)
    {
      const int RemainingSamples = NumSampleFrames - LoadedFrames;

      TMemory::Zero((char*)pData + LoadedFrames * BytesPerFrame, 
        RemainingSamples * BytesPerFrame);
    }
  }
  
  mBufferSampleFramePos += NumSampleFrames;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TCoreAudioAudioFileStream::TCoreAudioAudioFileStream(
  TCoreAudioAudioDecoder* pAudioFile)
  : TAudioStream(
      pAudioFile->TotalNumberOfSamples(), 
      pAudioFile->NumberOfChannels(), 
      pAudioFile->SampleType()
    ),
    mpCoreAudioDecoderFile(pAudioFile)
{
}

// -------------------------------------------------------------------------------------------------

int TCoreAudioAudioFileStream::OnPreferedBlockSize()const
{
  return mpCoreAudioDecoderFile->BlockSize();
}

// -------------------------------------------------------------------------------------------------

long long TCoreAudioAudioFileStream::OnBytesWritten()const
{
  MCodeMissing;
  return 0;
}

// -------------------------------------------------------------------------------------------------

bool TCoreAudioAudioFileStream::OnSeekTo(long long Frame)
{
  return mpCoreAudioDecoderFile->SeekTo(Frame);
}

// -------------------------------------------------------------------------------------------------

void TCoreAudioAudioFileStream::OnReadBuffer(
  void* pDestBuffer, 
  int   NumberOfSampleFrames)
{
  mpCoreAudioDecoderFile->ReadBuffer(pDestBuffer, NumberOfSampleFrames);
}

// -------------------------------------------------------------------------------------------------

void TCoreAudioAudioFileStream::OnWriteBuffer(
  const void* pSrcBuffer, 
  int         NumberOfSampleFrames)
{
  MCodeMissing;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TList<TString> TCoreAudioAudioFile::SSupportedExtensions()
{
  // NB: keep this in sync with TQuickTimeAudioFile::SSupportedExtensions()
  TList<TString> AudioFileExtensions;
  AudioFileExtensions.Append("*.aac");
  AudioFileExtensions.Append("*.aif");
  AudioFileExtensions.Append("*.aifc");
  AudioFileExtensions.Append("*.aiff");
  AudioFileExtensions.Append("*.au");
  AudioFileExtensions.Append("*.caf");
  AudioFileExtensions.Append("*.mp2");
  AudioFileExtensions.Append("*.mp3");
  AudioFileExtensions.Append("*.mp4a");
  AudioFileExtensions.Append("*.m4a");
  AudioFileExtensions.Append("*.snd");
  AudioFileExtensions.Append("*.wav");

  return AudioFileExtensions;
}

// -------------------------------------------------------------------------------------------------

TCoreAudioAudioFile::TCoreAudioAudioFile()
  : TAudioFile()
{
}

// -------------------------------------------------------------------------------------------------

TString TCoreAudioAudioFile::OnTypeString()const
{
  return "CA";
}

// -------------------------------------------------------------------------------------------------

void TCoreAudioAudioFile::OnOpenForRead(
  const TString&  Filename, 
  int             FileSubTrackIndex)
{
  MUnused(FileSubTrackIndex); // not implemented

  mpDecoderFile = new TCoreAudioAudioDecoder(Filename);
  mpDecoderFile->Open();
  mpStream = new TCoreAudioAudioFileStream(mpDecoderFile);  

  // the overview file and stream is created on the fly, when needed
}

// -------------------------------------------------------------------------------------------------

void TCoreAudioAudioFile::OnOpenForWrite(
  const TString&  Filename,
  int             SamplingRate,
  TSampleType     SampleType,
  int             NumChannels)
{
  throw TReadableException(
    MText("Encoding CoreAudio files is not supported."));
}

// -------------------------------------------------------------------------------------------------

void TCoreAudioAudioFile::OnClose()
{
  if (mpDecoderFile)
  {
    mpDecoderFile->Close();
  }

  if (mpDecoderOverviewFile)
  {
    mpDecoderOverviewFile->Close();
  }

  mpStream.Delete();
  mpOverviewStream.Delete();
  
  mpDecoderFile.Delete();
  mpDecoderOverviewFile.Delete();
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TCoreAudioAudioFile::OnStream()const
{
  return mpStream;
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TCoreAudioAudioFile::OnOverviewStream()const
{
  if (! mpOverviewStream)
  {
    TCoreAudioAudioFile* pMutableThis = const_cast<TCoreAudioAudioFile*>(this);
    
    pMutableThis->mpDecoderOverviewFile = 
      new TCoreAudioAudioDecoder(pMutableThis->FileName());
    
    pMutableThis->mpDecoderOverviewFile->Open();

    pMutableThis->mpOverviewStream = 
      new TCoreAudioAudioFileStream(pMutableThis->mpDecoderOverviewFile);
  }

  return mpOverviewStream;
}

// -------------------------------------------------------------------------------------------------

int TCoreAudioAudioFile::OnSamplingRate()const
{
  return mpDecoderFile->SampleRate();
}

// -------------------------------------------------------------------------------------------------

int TCoreAudioAudioFile::OnBitsPerSample()const
{
  return mpDecoderFile->NumberOfBits();
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TCoreAudioAudioFile::OnSampleType()const
{
  return mpDecoderFile->SampleType();
}

// -------------------------------------------------------------------------------------------------

int TCoreAudioAudioFile::OnNumChannels()const
{
  return mpDecoderFile->NumberOfChannels();
}

// -------------------------------------------------------------------------------------------------

long long TCoreAudioAudioFile::OnNumSamples() const
{
  return mpDecoderFile->TotalNumberOfSamples();
}

// -------------------------------------------------------------------------------------------------

TAudioTypes::TLoopMode TCoreAudioAudioFile::OnLoopMode()const
{
  return TAudioTypes::kNoLoop; // Not supported...
}

// -------------------------------------------------------------------------------------------------

long long TCoreAudioAudioFile::OnLoopStart()const
{
  return -1; // Not supported...
}

// -------------------------------------------------------------------------------------------------

long long TCoreAudioAudioFile::OnLoopEnd()const
{
  return -1; // Not supported...
}

// -------------------------------------------------------------------------------------------------

void TCoreAudioAudioFile::OnSetLoop(
  TAudioTypes::TLoopMode LoopMode, 
  long long              LoopStart, 
  long long              LoopEnd)
{
  // Not supported...
}

// -------------------------------------------------------------------------------------------------

TArray<long long> TCoreAudioAudioFile::OnCuePoints()const
{ 
  return TArray<long long>(); // Not supported...
}

// -------------------------------------------------------------------------------------------------

void TCoreAudioAudioFile::OnSetCuePoints(const TArray<long long>& CuePoints) 
{ 
  // Not supported...
}

