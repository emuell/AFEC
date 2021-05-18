#include "CoreFileFormatsPrecompiledHeader.h"



#define WAVE_FORMAT_PCM         1
#define WAVE_FORMAT_IEEE_FLOAT  3
#define WAVE_FORMAT_EXTENSIBLE  0xFFFE

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TWaveFormatChunkData::SSampleType(
  TUInt16 FormatTag, TUInt16 BitsPerSample) 
{
  MStaticAssert(TAudioFile::kNumOfSampleTypes == 7); // Update me when this triggers

  switch (BitsPerSample)
  {
  default:
    MInvalid("Invalid/unexpected bits per sample");

  case 8:
    return TAudioFile::k8BitUnsigned;

  case 16:
    return TAudioFile::k16Bit;

  case 24:
    return TAudioFile::k24Bit;

  case 32:
    // TODO: with FormatTag == WAVE_FORMAT_EXTENSIBLE, it actually could be both 
    // float or integers, but it's not really clear to me how to separate them...
    if (FormatTag == WAVE_FORMAT_EXTENSIBLE || FormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
      return TAudioFile::k32BitFloat;
    }
    else
    {
      MAssert(FormatTag == WAVE_FORMAT_PCM, "Unexpected format");
      return TAudioFile::k32BitInt;
    }

  case 64:
    return TAudioFile::k64BitFloat;
  }
}

// -------------------------------------------------------------------------------------------------

TWaveFormatChunkData::TWaveFormatChunkData()
{
  Config(TAudioFile::k16Bit, 44100, 2);
}

// -------------------------------------------------------------------------------------------------

void TWaveFormatChunkData::Config(
  TAudioFile::TSampleType  SampleType,
  int                      NewSamplingRate,
  int                      NewNumChannels)
{
  const TUInt16 NewBitsPerSample = 
    (TUInt16)TAudioFile::SNumBitsFromSampleType(SampleType);

  MAssert((NewBitsPerSample == 8 || NewBitsPerSample == 16 || 
      NewBitsPerSample == 24 || NewBitsPerSample == 32) && 
    (NewNumChannels == 1 || NewNumChannels == 2), "Invalid parameters");

  if (NewBitsPerSample != 32 || SampleType == TAudioFile::k32BitInt)
  {
    mFormatTag = WAVE_FORMAT_PCM;
  }
  else
  {
    mFormatTag = WAVE_FORMAT_IEEE_FLOAT;
  }

  mSampleRate  = (TUInt32)NewSamplingRate;
  mChannels = (TUInt16)NewNumChannels;
  mBitsPerSample = (TUInt16)NewBitsPerSample;
  mAvgBytesPerSec = (TUInt32)(mChannels * mSampleRate * mBitsPerSample) / 8;
  mBlockAlign = (TUInt16)((mChannels * mBitsPerSample) / 8);
}

// -------------------------------------------------------------------------------------------------

void TWaveFormatChunkData::Read(TFile* pFile)
{
  pFile->Read(mFormatTag);
  pFile->Read(mChannels);

  pFile->Read(mSampleRate);
  pFile->Read(mAvgBytesPerSec);

  pFile->Read(mBlockAlign);
  pFile->Read(mBitsPerSample);
}

// -------------------------------------------------------------------------------------------------

void TWaveFormatChunkData::Write(TFile* pFile)
{
  pFile->Write(mFormatTag);
  pFile->Write(mChannels);

  pFile->Write(mSampleRate);
  pFile->Write(mAvgBytesPerSec);

  pFile->Write(mBlockAlign);
  pFile->Write(mBitsPerSample);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TWaveFormatChunk::TWaveFormatChunk()
{
  mHeader.mChunckId  = gFourCC("fmt");
  mHeader.mChunkSize = sizeof (TWaveFormatChunkData);
}

// -------------------------------------------------------------------------------------------------

bool TWaveFormatChunk::VerifyValidity()
{
  if (mHeader.mChunckId == gFourCC("fmt"))
  {
    if (mData.mFormatTag == WAVE_FORMAT_PCM || 
        mData.mFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
        mData.mFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
      if (mData.mBitsPerSample == 8 || mData.mBitsPerSample == 16 ||
          mData.mBitsPerSample == 24 || mData.mBitsPerSample == 32 || 
          mData.mBitsPerSample == 64)
      {
        const bool AlignTest1 = (mData.mAvgBytesPerSec == (mData.mChannels * 
          mData.mSampleRate * mData.mBitsPerSample) / 8);

        // incorrect with some wavs, currently unused
        //const bool AlignTest2 = (mData.mBlockAlign == 
        //(mData.mChannels * mData.mBitsPerSample) / 8);

        return AlignTest1;
      }
    }
  }

  return false;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TSampleLoop::TSampleLoop()
{
  mIdentifier[0] = 0;
  mIdentifier[1] = 0;
  mIdentifier[2] = 0;
  mIdentifier[3] = 0;

  mType = 0;
  mStart = 0;
  mEnd = 0;
  mFraction = 0;
  mPlayCount = 0;
}

// -------------------------------------------------------------------------------------------------

void TSampleLoop::Read(TFile* pFile)
{
  pFile->Read(mIdentifier, 4);

  pFile->Read(mType);
  pFile->Read(mStart);
  pFile->Read(mEnd);
  pFile->Read(mFraction);
  pFile->Read(mPlayCount);
}

// -------------------------------------------------------------------------------------------------

void TSampleLoop::Write(TFile* pFile)
{
  pFile->Write(mIdentifier, 4);

  pFile->Write(mType);
  pFile->Write(mStart);
  pFile->Write(mEnd);
  pFile->Write(mFraction);
  pFile->Write(mPlayCount);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TSamplerChunk::TSamplerChunk()
{
  mManufacturer = 0;
  mProduct = 0;
  mSamplePeriod = 0;
  mMIDIUnityNote = 60;
  mMIDIPitchFraction = 0;
  mSMPTEFormat = 0;
  mSMPTEOffset = 0;
  mSampleLoops = 0;
  mSamplerData = 0;
}

// -------------------------------------------------------------------------------------------------

void TSamplerChunk::Read(TFile* pFile)
{
  pFile->Read(mManufacturer);
  pFile->Read(mProduct);
  pFile->Read(mSamplePeriod);
  pFile->Read(mMIDIUnityNote);
  pFile->Read(mMIDIPitchFraction);
  pFile->Read(mSMPTEFormat);
  pFile->Read(mSMPTEOffset);
  pFile->Read(mSampleLoops);
  pFile->Read(mSamplerData);
}

// -------------------------------------------------------------------------------------------------

void TSamplerChunk::Write(TFile* pFile)
{
  pFile->Write(mManufacturer);
  pFile->Write(mProduct);
  pFile->Write(mSamplePeriod);
  pFile->Write(mMIDIUnityNote);
  pFile->Write(mMIDIPitchFraction);
  pFile->Write(mSMPTEFormat);
  pFile->Write(mSMPTEOffset);
  pFile->Write(mSampleLoops);
  pFile->Write(mSamplerData);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TCuePoint::TCuePoint()
{
  mUniqueId = 0;
  mPosition = 0;
  mDataChunkId = gFourCC("data");
  mChunkStart = 0;
  mBlockStart = 0;
  mSampleOffset = 0;
}

// -------------------------------------------------------------------------------------------------

void TCuePoint::Read(TFile* pFile)
{
  pFile->Read(mUniqueId);
  pFile->Read(mPosition);
  pFile->Read((char*)&mDataChunkId, 4);
  pFile->Read(mChunkStart);
  pFile->Read(mBlockStart);
  pFile->Read(mSampleOffset);
}

// -------------------------------------------------------------------------------------------------

void TCuePoint::Write(TFile* pFile)
{
  pFile->Write(mUniqueId);
  pFile->Write(mPosition);
  pFile->Write((char*)&mDataChunkId, 4);
  pFile->Write(mChunkStart);
  pFile->Write(mBlockStart);
  pFile->Write(mSampleOffset);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TCueChunk::TCueChunk()
{
  mNumCuePoints = 0;
}

// -------------------------------------------------------------------------------------------------

void TCueChunk::Read(TFile* pFile)
{
  pFile->Read(mNumCuePoints);
}

// -------------------------------------------------------------------------------------------------

void TCueChunk::Write(TFile* pFile)
{
  pFile->Write(mNumCuePoints);
}

// =================================================================================================

TList<TString> TWaveFile::SSupportedExtensions()
{
  TList<TString> Ret;
  Ret.Append("*.wav");

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TWaveFile::SSampleTypeFromBitDepth(
  int BitDepth, 
  bool PrefereFloatFormats)
{
  switch (BitDepth)
  {
  case 8:
    return TAudioFile::k8BitUnsigned;
  
  case 16:
    return TAudioFile::k16Bit;

  case 24:
    return TAudioFile::k24Bit;

  case 32:
    if (PrefereFloatFormats)
    {
      return TAudioFile::k32BitFloat;
    }
    else
    {
      return TAudioFile::k32BitInt;
    }

  case 64:
    return TAudioFile::k64BitFloat;

  default:
    MInvalid("");
    return TAudioFile::kInvalidSampleType;
  }
}
  
// -------------------------------------------------------------------------------------------------

TWaveFile::TWaveFile()
  : TAudioFile(), 
    TRiffFile(gFourCC("RIFF")),
    mSampleDataChunkOffset(0),
    mNumOfSamples(0)
{
   mPcmDataHeader.mChunckId = gFourCC("data");
   mPcmDataHeader.mChunkSize = 0;
}

// -------------------------------------------------------------------------------------------------

TString TWaveFile::OnTypeString()const
{
  return "WAV";
}

// -------------------------------------------------------------------------------------------------

void TWaveFile::OnOpenForRead(
  const TString&  Filename, 
  int             FileSubTrackIndex)
{
  MUnused(FileSubTrackIndex);

  TFile& File = TRiffFile::Open(Filename, TFile::kRead, TByteOrder::kIntel);

  ReadChunks();

  if (!FindChunkByName("WAVE") || 
      !FindChunkByName("data") || 
      !FindChunkByName("fmt "))
  {
    throw TReadableException(MText("Not a valid WAV file."));
  }

  TRiffChunkHeaderInfo* pFormatChunk = FindChunkByName("fmt ");

  File.SetPosition(pFormatChunk->Offset());
  
  mWaveFormat.mData.Read(&File);
  
  if (!mWaveFormat.VerifyValidity())
  {
    throw TReadableException(MText("Unsupported file format."));
  }
    
  TRiffChunkHeaderInfo* pDataChunk = FindChunkByName("data");
  mSampleDataChunkOffset = pDataChunk->Offset();
  mNumOfSamples = pDataChunk->Size() / (NumChannels() * (BitsPerSample() / 8));
  
  if (mNumOfSamples <= 0)
  {
    throw TReadableException(MText("Unsupported file format or corrupt file."));
  }

  TRiffChunkHeaderInfo* pSmplChunk = FindChunkByName("smpl");

  mSamplerChunk = TSamplerChunk();
  mSampleLoop = TSampleLoop();
    
  if (pSmplChunk)
  {
    try
    {
      File.SetPosition(pSmplChunk->Offset());
      mSamplerChunk.Read(&File);

      if (mSamplerChunk.mSampleLoops > 0)
      {
        // read only the first loopdatablock
        mSampleLoop.Read(&File);
      }
    }
    catch (TReadableException&)
    {
      // silently ignore bogus loops
      mSamplerChunk = TSamplerChunk();
      mSampleLoop = TSampleLoop();
    }
  }

  TRiffChunkHeaderInfo* pCueChunk = FindChunkByName("cue ");

  mCueChunk = TCueChunk();
  mCuePoints.Empty();

  if (pCueChunk)
  {
    try
    {
      File.SetPosition(pCueChunk->Offset());
      mCueChunk.Read(&File);

      if (mCueChunk.mNumCuePoints > 0)
      {
        mCuePoints.SetSize(mCueChunk.mNumCuePoints);

        for (int i = 0; i < mCuePoints.Size(); ++i)
        {
          mCuePoints[i].Read(&File);
        }
      }
    }
    catch (TReadableException&)
    {
      // silently ignore bogus cues
      mCueChunk = TCueChunk();
      mCuePoints.Empty();
    }
  }

  // create streams in read-only mode
  mpStream = new TAudioFileStream(&File, mSampleDataChunkOffset, 
    OnNumSamples(), OnNumChannels(), OnSampleType(), TByteOrder::kIntel);

  mpOverviewStream = new TAudioFileStream(&File, mSampleDataChunkOffset, 
    OnNumSamples(), OnNumChannels(), OnSampleType(), TByteOrder::kIntel);
}

// -------------------------------------------------------------------------------------------------

void TWaveFile::OnOpenForWrite(
  const TString&  Filename,
  int             SamplingRate,
  TSampleType     SampleType,
  int             NumChannels)
{
  MAssert(SampleType != k8BitSigned, "Not supported. Use Unsigned Samples instead."); 

  // this will throw when the format is not accepted, so do this early
  mWaveFormat.mData.Config(SampleType, SamplingRate, NumChannels);

  TFile& File = TRiffFile::Open(Filename, TFile::kWrite, TByteOrder::kIntel);

  // behind the RIFF header
  File.SetPosition(8);

  File.Write("WAVE", 4);
  mWaveFormat.mHeader.Write(&File);
  mWaveFormat.mData.Write(&File);

  mPcmDataHeader.Write(&File);
  mSampleDataChunkOffset = File.Position();

  // create stream to write PCM data
  mpStream = new TAudioFileStream(&File, mSampleDataChunkOffset, 
    OnNumSamples(), OnNumChannels(), OnSampleType(), TByteOrder::kIntel);
  
  // mpOverviewStream should not be accessed in write mode 
}

// -------------------------------------------------------------------------------------------------

void TWaveFile::OnClose()
{
  // update PCM chunk size and append other chunks
  TFile& File = TRiffFile::File();
  if (File.IsOpen() && File.AccessMode() == TFile::kWrite)
  {
    // . update the PCM chunk
    
    mPcmDataHeader.mChunkSize = (TUInt32)Stream()->BytesWritten();

    File.SetPosition((int)(mSampleDataChunkOffset - 4));
    File.Write(mPcmDataHeader.mChunkSize);
    
    File.SetPosition(File.SizeInBytes());
    
    // add a padding byte if needed (chunks must be word aligned)
    if ((mPcmDataHeader.mChunkSize % 2) != 0)
    {
      char PaddingByte = 0;
      File.Write(PaddingByte);
    }

    // . CueChunk (if we got cues points set)

    if (mCuePoints.Size() > 0)
    {
      TRiffChunkHeader CueHeader;
      CueHeader.mChunckId = gFourCC("cue ");
      CueHeader.mChunkSize = (TUInt32)(sizeof(TCueChunk) + 
        mCuePoints.Size() * sizeof(TCuePoint));

      CueHeader.Write(&File);

      mCueChunk.mNumCuePoints = mCuePoints.Size();
      mCueChunk.Write(&File);
      
      for (int i = 0; i < mCuePoints.Size(); ++i)
      {
        mCuePoints[i].Write(&File);
      }
      
      // add a padding byte if needed (chunks must be word aligned)
      if ((CueHeader.mChunkSize % 2) != 0)
      {
        char PaddingByte = 0;
        File.Write(PaddingByte);
      }
    }


    // . SamplerChunk (if we got loop points set)

    if (mSamplerChunk.mSampleLoops > 0)
    {
      MAssert(mSamplerChunk.mSampleLoops == 1, 
        "Expecting exactly one loop here...");
      
      TRiffChunkHeader SamplerHeader;
      SamplerHeader.mChunckId = gFourCC("smpl");
      SamplerHeader.mChunkSize = (TUInt32)(sizeof(mSamplerChunk) + 
        mSamplerChunk.mSamplerData);
        
      SamplerHeader.Write(&File);
      mSamplerChunk.Write(&File);
      mSampleLoop.Write(&File);
      
      // add a padding byte if needed (chunks must be word aligned)
      if ((SamplerHeader.mChunkSize % 2) != 0)
      {
        char PaddingByte = 0;
        File.Write(PaddingByte);
      }
    }
  }

  // update RIFF chunk size
  TRiffFile::CloseFile();
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TWaveFile::OnStream()const
{
  return mpStream;
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TWaveFile::OnOverviewStream()const
{
  return mpOverviewStream;
}

// -------------------------------------------------------------------------------------------------

int TWaveFile::OnSamplingRate()const
{
  return (int)mWaveFormat.mData.mSampleRate;
}

// -------------------------------------------------------------------------------------------------

int TWaveFile::OnBitsPerSample()const
{
  return (int)mWaveFormat.mData.mBitsPerSample;
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TWaveFile::OnSampleType()const
{
  return TWaveFormatChunkData::SSampleType(
    mWaveFormat.mData.mFormatTag, mWaveFormat.mData.mBitsPerSample);
}

// -------------------------------------------------------------------------------------------------

int TWaveFile::OnNumChannels()const
{
  return (int)mWaveFormat.mData.mChannels;
}

// -------------------------------------------------------------------------------------------------

long long TWaveFile::OnNumSamples() const
{
  return mNumOfSamples;
}

// -------------------------------------------------------------------------------------------------

TAudioTypes::TLoopMode TWaveFile::OnLoopMode()const
{
  // > check to ignore invalid loops (sometimes the loop start 
  // is loop end if no loop is present)...

  if (mSamplerChunk.mSampleLoops > 0 && 
      mSampleLoop.mEnd > mSampleLoop.mStart)
  {
    switch (mSampleLoop.mType)
    {
    case 0:
      return TAudioTypes::kLoopForward;

    case 1:
      return TAudioTypes::kLoopPingPong;

    case 2:
      return TAudioTypes::kLoopBackward;
    }

    return TAudioTypes::kLoopForward;
  }
  else
  {
    // no sampler chunk present
    return TAudioTypes::kNoLoop; 
  }
}

// -------------------------------------------------------------------------------------------------

long long TWaveFile::OnLoopStart()const
{
  return mSampleLoop.mStart;
}

// -------------------------------------------------------------------------------------------------

long long TWaveFile::OnLoopEnd()const
{
  return mSampleLoop.mEnd;
}

// -------------------------------------------------------------------------------------------------

void TWaveFile::OnSetLoop(
  TAudioTypes::TLoopMode LoopMode, 
  long long              LoopStart, 
  long long              LoopEnd)
{
  if (LoopMode == TAudioTypes::kNoLoop)
  {
    mSamplerChunk.mSampleLoops = 0;
    mSamplerChunk.mSamplerData = 0;
  }
  else
  {
    switch (LoopMode)
    {
    default: 
      MInvalid("Unepected loop mode");

    case TAudioTypes::kLoopForward:
      mSampleLoop.mType = 0;
      break;

    case TAudioTypes::kLoopPingPong:
      mSampleLoop.mType = 1;
      break;

    case TAudioTypes::kLoopBackward:
      mSampleLoop.mType = 2;
      break;
    }

    mSampleLoop.mStart = (TUInt32)LoopStart;
    mSampleLoop.mEnd = (TUInt32)LoopEnd;
    mSampleLoop.mFraction = 0;
    mSampleLoop.mPlayCount = 0;

    mSamplerChunk.mSampleLoops = 1;
    mSamplerChunk.mSamplerData = 
      (TInt32)(mSamplerChunk.mSampleLoops * sizeof(TSampleLoop));
  }
}

// -------------------------------------------------------------------------------------------------

TArray<long long> TWaveFile::OnCuePoints()const
{
  TArray<long long> Ret;
  Ret.SetSize(mCuePoints.Size());

  for (int i = 0; i < mCuePoints.Size(); ++i)
  {
    if (mCuePoints[i].mPosition != 0)
    {
      Ret[i] = mCuePoints[i].mPosition;
    }
    else
    {
      Ret[i] = mCuePoints[i].mSampleOffset;
    }
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

void TWaveFile::OnSetCuePoints(const TArray<long long>& CuePoints)
{
  mCuePoints.SetSize(CuePoints.Size());

  for (int i = 0; i < CuePoints.Size(); ++i)
  {
    mCuePoints[i].mUniqueId = i;
    mCuePoints[i].mPosition = (TUInt32)CuePoints[i];
    mCuePoints[i].mDataChunkId = gFourCC("data");
    mCuePoints[i].mChunkStart = 0;
    mCuePoints[i].mBlockStart = 0;
    mCuePoints[i].mSampleOffset = (TUInt32)CuePoints[i];
  }
}

// -------------------------------------------------------------------------------------------------

bool TWaveFile::OnIsParentChunk(const TString &FourccName)
{
  return (FourccName == "WAVE");
}

