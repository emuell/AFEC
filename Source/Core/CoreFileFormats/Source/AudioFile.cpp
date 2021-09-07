#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/ValueChanger.h"
#include "CoreTypes/Export/Directory.h"
#include "CoreTypes/Export/System.h"
#include "CoreTypes/Export/Timer.h"

#include "AudioTypes/Export/AudioMath.h"

// =================================================================================================

// NB: this is based on M16BitSampleRange!
#define MMinAudibleSignal 0.25f

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static TReadableException SChannelConversionException()
{
  return TReadableException( MText( "Invalid src/dest channel setup. "
    "Supporting mono<>stereo conversions only, "
    "else channel layouts must be read as they are present in the file." ) );
}

// -------------------------------------------------------------------------------------------------

static TReadableException SUnsupportedChannelLayoutException(
  int                     SrcChannels, 
  int                     DestChannels,
  TAudioFile::TSampleType SampleType)
{
  return TReadableException( MText( 
    "Unsupported audio file channel layout: src: %s dest: %s, sample-format: %s",
    ToString(SrcChannels), ToString( DestChannels ), ToString( (int)SampleType ) ) );
}

// =================================================================================================

float TAudioFile::sImportProgress = 0.0f;
bool TAudioFile::sCancelImport = false;
  
// -------------------------------------------------------------------------------------------------

TList<TString> TAudioFile::SSupportedExtensions()
{
  TList<TString> SupportedExtensions = 
    // always available
       TWaveFile::SSupportedExtensions() 
     + TAifFile::SSupportedExtensions()
     + TFlacFile::SSupportedExtensions() 
  
  #if !defined(MNoOggVorbis)
     + TOggVorbisFile::SSupportedExtensions()
  #endif
  
  #if !defined(MNoMp3LibMpg)
     + TMp3File::SSupportedExtensions()
  #endif
  
  #if defined(MMac)
     + TCoreAudioAudioFile::SSupportedExtensions()
  #endif
  
  #if defined(MWindows)
     + TMediaFoundationAudioFile::SSupportedExtensions()
  #endif
  ;

  // sort
  SupportedExtensions.Sort();
  
  // remove duplicates
  for (int i = 0; i < SupportedExtensions.Size(); ++i)
  {
    const bool SearchBackwards = false;
    if (SupportedExtensions.Find(SupportedExtensions[i], i + 1, SearchBackwards) != -1)
    {
      SupportedExtensions.Delete(i);
    }
  }

  return SupportedExtensions;
}

// -------------------------------------------------------------------------------------------------

bool TAudioFile::SContainsAudio(
  const TString&  FileNameAndPath, 
  TList<TString>* pContainedAudioTracks)
{
  if (!TFile(FileNameAndPath).Exists())
  {
    // not a valid file at all...
    if (pContainedAudioTracks)
    {
      pContainedAudioTracks->Empty();
    }
  
    return false;
  }
  
  // one file by default with no special identifier
  if (pContainedAudioTracks)
  {
    *pContainedAudioTracks = MakeList<TString>(
      gCutExtensionAndPath(FileNameAndPath));
  }

  //  TWaveFile (all platforms)
  if (gFileMatchesExtension(FileNameAndPath, 
        TWaveFile::SSupportedExtensions()))
  {
    return true;
  }

  // TAifFile (all platforms)
  else if (gFileMatchesExtension(FileNameAndPath, 
              TAifFile::SSupportedExtensions()))
  {
    return true;
  }

  // TFlacFile (all platforms)
  else if (gFileMatchesExtension(FileNameAndPath, 
              TFlacFile::SSupportedExtensions()))
  {
    return true;
  }

  // TOggVorbisFile (all platforms)
  #if !defined(MNoOggVorbis)
    else if (gFileMatchesExtension(FileNameAndPath, 
                TOggVorbisFile::SSupportedExtensions()))
    {
      return true;
    }
  #endif


  // TMp3File (Linux only)
  #if !defined(MNoMp3LibMpg)
    else if (gFileMatchesExtension(FileNameAndPath, 
                TMp3File::SSupportedExtensions()))
    {
      return true;
    }
  #endif
  
  // TCoreAudioAudioFile (OSX only)
  #if defined(MMac)
    else if (gFileMatchesExtension(FileNameAndPath, 
                TCoreAudioAudioFile::SSupportedExtensions()))
    {
      return true;
    }
  #endif

  // TMediaFoundationAudioFile (Windows only)
  #if defined(MWindows)
    else if (TMediaFoundationAudioFile::SContainsAudio(
                FileNameAndPath, pContainedAudioTracks))
    {
      return true;
    }
  #endif

  return false;    
}
  
// -------------------------------------------------------------------------------------------------

bool TAudioFile::SCreateFromFile(
  TPtr<TAudioFile>& pAudioFile,
  const TString& Filename,
  TString&       ErrorMessage)
{
  const int FileSubTrackIndex = -1;
  const bool UseOrCreateDecompressedFile = false;
  
  return SCreateFromFile(
    pAudioFile, Filename, FileSubTrackIndex, 
  UseOrCreateDecompressedFile, ErrorMessage);
}

bool TAudioFile::SCreateFromFile(
  TPtr<TAudioFile>& pAudioFile,
  const TString&    Filename,
  int               FileSubTrackIndex,
  bool              UseOrCreateDecompressedFile,
  TString&          ErrorMessage)
{
  pAudioFile.Delete();
  ErrorMessage.Empty();
  
  
  // ... TWaveFile (all platforms)

  if (gFileMatchesExtension(Filename, TWaveFile::SSupportedExtensions()))
  {
    try
    {
      TPtr<TAudioFile> pWaveAudioFile(new TWaveFile());
      pAudioFile = pWaveAudioFile;
      pAudioFile->OpenForRead(Filename, FileSubTrackIndex);

      return true;
    }
    catch(const TReadableException& Exception)
    {
      pAudioFile.Delete();
      ErrorMessage = Exception.Message();
    }
  }


  // ... TAifFile (all platforms)

  if (gFileMatchesExtension(Filename, TAifFile::SSupportedExtensions()))
  {
    try
    {
      TPtr<TAudioFile> pAifAudioFile(new TAifFile());
      pAudioFile = pAifAudioFile;
      pAudioFile->OpenForRead(Filename, FileSubTrackIndex);

      return true;
    }
    catch (const TReadableException& Exception)
    {
      pAudioFile.Delete();
      ErrorMessage = Exception.Message();
    }
  }
  

  // ... TFlacFile (all platforms)

  if (gFileMatchesExtension(Filename, TFlacFile::SSupportedExtensions()))
  {
    try
    {
      TPtr<TAudioFile> pFlacAudioFile(new TFlacFile());
      pAudioFile = pFlacAudioFile;
      
      if (UseOrCreateDecompressedFile)
      {
        pAudioFile->OpenForReadAndDecompressIfNeeded(
          Filename, FileSubTrackIndex);
      }
      else
      {
        pAudioFile->OpenForRead(Filename, FileSubTrackIndex);
      }
      
      return true;
    }
    catch (const TReadableException& Exception)
    {
      pAudioFile.Delete();
      ErrorMessage = Exception.Message();
    }
  }
  

  // ... TOggVorbisFile (all platforms)
  
  #if !defined(MNoOggVorbis)
    if (gFileMatchesExtension(Filename, TOggVorbisFile::SSupportedExtensions()))
    {
      try
      {
        TPtr<TAudioFile> pOggAudioFile(new TOggVorbisFile());
        pAudioFile = pOggAudioFile;
        
        if (UseOrCreateDecompressedFile)
        {
          pAudioFile->OpenForReadAndDecompressIfNeeded(
            Filename, FileSubTrackIndex);
        }
        else
        {
          pAudioFile->OpenForRead(Filename, FileSubTrackIndex);
        }
        
        return true;
      }
      catch (const TReadableException& Exception)
      {
        pAudioFile.Delete();
        ErrorMessage = Exception.Message();
      }
    }
  #endif


  // ... TMp3File (Linux only)

  #if !defined(MNoMp3LibMpg)
    if (gFileMatchesExtension(Filename, TMp3File::SSupportedExtensions()))
    {
      try
      {
        TPtr<TAudioFile> pMp3File(new TMp3File());
        pAudioFile = pMp3File;

        if (UseOrCreateDecompressedFile)
        {
          pAudioFile->OpenForReadAndDecompressIfNeeded(
            Filename, FileSubTrackIndex);
        }
        else
        {
          pAudioFile->OpenForRead(Filename, FileSubTrackIndex);
        }

        return true;
      }
      catch (const TReadableException& Exception)
      {
        pAudioFile.Delete();
        ErrorMessage = Exception.Message();
      }
    }
  #endif
  

  // ... TCoreAudioAudioFile (OSX only)
  
  #if defined(MMac)
    if (gFileMatchesExtension(Filename, TCoreAudioAudioFile::SSupportedExtensions()))
    {
      try
      {
        TPtr<TAudioFile> pCoreAudioAudioFile(new TCoreAudioAudioFile());
        pAudioFile = pCoreAudioAudioFile;
        
        if (UseOrCreateDecompressedFile)
        {
          pAudioFile->OpenForReadAndDecompressIfNeeded(
            Filename, FileSubTrackIndex);
        }
        else
        {
          pAudioFile->OpenForRead(Filename, FileSubTrackIndex);
        }
        
        return true;
      }
      catch (const TReadableException& Exception)
      {
        pAudioFile.Delete();
        ErrorMessage = Exception.Message();
      }
    }
  #endif
  

  // ... TMediaFoundationAudioFile (Windows only)
  
  #if defined(MWindows)
    if (gFileMatchesExtension(Filename, TMediaFoundationAudioFile::SSupportedExtensions()))
    {
      try
      {
        TPtr<TAudioFile> pMediaFoundationFile(new TMediaFoundationAudioFile());
        pAudioFile = pMediaFoundationFile;

        if (UseOrCreateDecompressedFile)
        {
          pAudioFile->OpenForReadAndDecompressIfNeeded(
            Filename, FileSubTrackIndex);
        }
        else
        {
          pAudioFile->OpenForRead(Filename, FileSubTrackIndex);
        }

        return true;
      }
      catch (const TReadableException& Exception)
      {
        pAudioFile.Delete();
        ErrorMessage = Exception.Message();
      }
    }
  #endif
  
  if (ErrorMessage.IsEmpty())
  {
    ErrorMessage = MText("The specified file is not a valid audio file!");
  }  
  
  return false;
}

// -------------------------------------------------------------------------------------------------

int TAudioFile::SNumBitsFromSampleType(TSampleType SampleType)
{
  MStaticAssert(kNumOfSampleTypes == 7); // Update me

  switch (SampleType)
  {
  default:
    MInvalid("Unknown/unexpected sample type");

  case k8BitSigned:
  case k8BitUnsigned:
    return 8;

  case k16Bit:
    return 16;

  case k24Bit:
    return 24;

  case k32BitInt:
  case k32BitFloat:
    return 32;

  case k64BitFloat:
    return 64;
  }
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TAudioFile() 
  : mCalculatingOverviewData(0),
    mOverViewDataWritePos(0), 
    mReadOverViewBufferSamplePos(0),
    mReadOverViewBufferSampleSize(0),
    mIsOpen(false),
    mFileSubTrackIndex(-1),
    mDecompressedFileIsInTemp(false),
    mCancelDecompression(false),
    mDecompressionProgress(0.0f),
    mDecompressing(false)
{ 
}

// -------------------------------------------------------------------------------------------------

TAudioFile::~TAudioFile()
{
  if (mpDecompressedWavFile && mpDecompressedWavFile->IsOpen())
  {
    mpDecompressedWavFile->Close();
  }

  if (mpDecompressedWavFile && mDecompressedFileIsInTemp && 
      gFileExists(mpDecompressedWavFile->FileName()))
  {
    TFile(mpDecompressedWavFile->FileName()).Unlink();
  }
}

// -------------------------------------------------------------------------------------------------

TString TAudioFile::TypeString()const
{
  return OnTypeString();
}

// -------------------------------------------------------------------------------------------------

void TAudioFile::OpenForRead(const TString& Filename, int FileSubTrackIndex)
{
  mIsOpen = false;
  mFileName = TString();
  mFileSubTrackIndex = -1;
  
  OnOpenForRead(Filename, FileSubTrackIndex);

  mIsOpen = true;
  mFileName = Filename;
  mFileSubTrackIndex = FileSubTrackIndex;
}

// -------------------------------------------------------------------------------------------------

void TAudioFile::OpenForReadAndDecompressIfNeeded(
  const TString&  Filename, 
  int             FileSubTrackIndex)
{
  mCancelDecompression = false;
  mDecompressionProgress = 0.0f;
  mDecompressing = false;
  
  const bool UseDecompressedWaveFile = TFile(gCutExtension(Filename) + ".wav").Exists();
  
  MAssert(UseDecompressedWaveFile || 
      gFileMatchesExtension(Filename, MakeList<TString>("*.wav")),
    "Dont call this for audio file that already are decompression");
  
  TPtr<TWaveFile> pDecompressedWavFile;
  
  try
  {
    if (UseDecompressedWaveFile)
    {
      mIsOpen = false;
      mFileName = TString();
      mFileSubTrackIndex = -1;
      
      // use the existing (already extracted wav)
      mDecompressedFileIsInTemp = false;
 
      pDecompressedWavFile = new TWaveFile();
      pDecompressedWavFile->OpenForRead(gCutExtension(Filename) + ".wav");
      mpDecompressedWavFile = pDecompressedWavFile;
      
      mIsOpen = true;
      mFileName = Filename;
      mFileSubTrackIndex = FileSubTrackIndex;
    }
    else
    { 
      mDecompressing = true;
      mIsOpen = false;
      mFileName = TString();
      mFileSubTrackIndex = -1;
      
      OpenForRead(Filename, FileSubTrackIndex);
  
      const TDirectory FileSrcDirectory(gExtractPath(Filename));
      
      TString DestFileFileName;

      if (FileSrcDirectory.CanWrite())
      {
        DestFileFileName = gCutExtension(Filename) + ".wav";
        mDecompressedFileIsInTemp = false;
      }
      else
      {
        DestFileFileName = gGenerateTempFileName(gTempDir());
        mDecompressedFileIsInTemp = true;
      }

      pDecompressedWavFile = new TWaveFile();
      pDecompressedWavFile->OpenForWrite(DestFileFileName, 
        SamplingRate(), SampleType(), NumChannels());
      
      const long long NumTotalSamplesToDecode = this->NumSamples();
      long long NumTotalSamplesLeft = NumTotalSamplesToDecode;
      
      TArray< TArray<float> > Buffers(NumChannels());
      TArray<const float*> SrcChannelPtrs(NumChannels()); 
      TArray<float*> DestChannelPtrs(NumChannels()); 
        
      for (int i = 0; i < NumChannels(); ++i)
      {
        Buffers[i].SetSize(Stream()->PreferedBlockSize() * ((BitsPerSample() + 7) / 8));
        SrcChannelPtrs[i] = Buffers[i].FirstRead();
        DestChannelPtrs[i] = Buffers[i].FirstWrite(); 
      }
          
      while (NumTotalSamplesLeft > 0)
      {
        const int NumSamplesToDecode = MMin<int>(
          Stream()->PreferedBlockSize(), (int)NumTotalSamplesLeft);
        
        Stream()->ReadSamples(DestChannelPtrs, NumSamplesToDecode);
        
        const bool Dither = false;
        pDecompressedWavFile->Stream()->WriteSamples(SrcChannelPtrs, NumSamplesToDecode, Dither);
            
        mDecompressionProgress = 1.0f - ((float)NumTotalSamplesLeft / (float)NumTotalSamplesToDecode);
        
        if (mCancelDecompression)
        {
          throw TReadableException(MText("Canceled extraction/decoding."));
        }
        
        NumTotalSamplesLeft -= NumSamplesToDecode;
      }
   
      pDecompressedWavFile->Close();
      pDecompressedWavFile->OpenForRead(DestFileFileName);
      mpDecompressedWavFile = pDecompressedWavFile;
      
      mDecompressing = false;

      mIsOpen = true;
      mFileName = Filename;
      mFileSubTrackIndex = FileSubTrackIndex;
    }
  }
  catch (const TReadableException&)
  {
    if (pDecompressedWavFile)
    {
      if (pDecompressedWavFile->IsOpen())
      {
        pDecompressedWavFile->Close();
      }

      if (!UseDecompressedWaveFile && gFileExists(pDecompressedWavFile->FileName()))
      {
        // unlink the half written file...
        TFile(pDecompressedWavFile->FileName()).Unlink();
      }
    }
    
    if (mDecompressing)
    {
      Close();
      mDecompressing = false;
    }
    
    throw;
  }
}

// -------------------------------------------------------------------------------------------------

void TAudioFile::OpenForWrite(
  const TString&  Filename,
  int             SamplingRate,
  TSampleType     SampleType,
  int             NumChannels)
{
  mIsOpen = false;
  mFileName = TString();
  mFileSubTrackIndex = -1;
  
  OnOpenForWrite(Filename, SamplingRate, SampleType, NumChannels);

  mIsOpen = true;
  mFileName = Filename;
  mFileSubTrackIndex = 0;
}

// -------------------------------------------------------------------------------------------------

bool TAudioFile::NeedCalcOverViewIdleTimer()const
{
  return mOverViewDataWritePos < NumSamples();
}

// -------------------------------------------------------------------------------------------------

float TAudioFile::CalcOverViewIdleTimerProgress()const
{
  if (NeedCalcOverViewIdleTimer())
  {
    return (float)mOverViewDataWritePos / (float)NumSamples();
  }
  else
  {
    return 1.0f;
  }
}
  
// -------------------------------------------------------------------------------------------------
                                                
void TAudioFile::CalcOverViewIdleTimer(
  long long& SamplePosStart, 
  long long& SamplePosEnd, 
  long long  NumSamples)
{
  MAssert(NumSamples > 0, "");

  if (NeedCalcOverViewIdleTimer())
  {
    SamplePosStart = mOverViewDataWritePos;
    CalcOverViewData(mOverViewDataWritePos, MSamplesPerCalcOverViewIdleCall);
    SamplePosEnd = mOverViewDataWritePos;
  }
  else
  {
    SamplePosStart = 0;
    SamplePosEnd = 0;
  }
}

// -------------------------------------------------------------------------------------------------

bool TAudioFile::HaveOverViewData(long long FromSampleFrame, long long SampleFrameRange)const
{
  return (mOverViewDataWritePos >= FromSampleFrame + SampleFrameRange);
}

// -------------------------------------------------------------------------------------------------

void TAudioFile::CalcOverViewData(long long FromSampleFrame, long long SampleFrameRange)
{
  // avoid being reentered
  if (!mCalculatingOverviewData)
  {
    const TIncDecValue<int> IncDec(mCalculatingOverviewData);
    
    if (!mOverViewData.Size())
    {
      mOverViewData.SetSize(NumChannels());
    }

    for (int i = 0; i < mOverViewData.Size(); ++i)
    {
      if (!mOverViewData[i].Size())
      {
        mOverViewData[i].SetSize((int)(NumSamples() / MSampleOverviewWidth + 1));
      }
    }

    const int StartOverViewPos = 
      (int)(mOverViewDataWritePos / MSampleOverviewWidth);
    
    const int EndOverViewPos = 
      (int)((FromSampleFrame + SampleFrameRange) / MSampleOverviewWidth + 1);
    
    const int NumOverViewFrames = 
      EndOverViewPos - StartOverViewPos;

    // avoid seeking overhead, by reading as much samples as possible in one block
    const int TotalSamplesToAnalyze = (int)MMax(0LL, 
      MMin<long long>(NumOverViewFrames * MSampleOverviewWidth, 
        NumSamples() - StartOverViewPos*MSampleOverviewWidth));

    mTempOverViewBuffer.Grow(TotalSamplesToAnalyze);
    mTempOverViewBufferPtrs.SetSize(1); // overview is always mono
    mTempOverViewBufferPtrs[0] = mTempOverViewBuffer.FirstWrite();
    
    // update for "OverViewMinMax"
    mReadOverViewBufferSamplePos = StartOverViewPos * MSampleOverviewWidth;
    mReadOverViewBufferSampleSize = TotalSamplesToAnalyze;
      
    OverviewStream()->SeekTo(StartOverViewPos * MSampleOverviewWidth);

    try
    {
      OverviewStream()->ReadSamples(mTempOverViewBufferPtrs, TotalSamplesToAnalyze);
    }
    catch (const TReadableException&)
    {
      MInvalid("ReadSamples failed. Broken sample file?");
      
      // flush the rest of the overview and bail out...
      mOverViewDataWritePos = NumSamples();

      for (int i = StartOverViewPos; i < mOverViewData[0].Size(); ++i)
      {
        mOverViewData[0][i].mMin = 0;
        mOverViewData[0][i].mMax = 0;
      }

      return;
    }
    
    for (int i = StartOverViewPos, t = 0; i < EndOverViewPos; ++i, ++t)
    {
      const int SamplesToAnalyze = (int)MMin<long long>(MSampleOverviewWidth, 
        NumSamples() - i*MSampleOverviewWidth);

      if (SamplesToAnalyze <= 0)
      {
        break;
      }
        
      float RealMin = mTempOverViewBuffer[t*MSampleOverviewWidth];
      float RealMax = mTempOverViewBuffer[t*MSampleOverviewWidth];

      TMathT<float>::GetMinMax(RealMin, RealMax, 0, 
        SamplesToAnalyze, mTempOverViewBuffer.FirstRead() + t*MSampleOverviewWidth);  

      MAssert(TMath::f2i(RealMin) >= M16BitSampleMin && TMath::f2i(RealMin) <= M16BitSampleMax, "");
      MAssert(TMath::f2i(RealMax) >= M16BitSampleMin && TMath::f2i(RealMax) <= M16BitSampleMax, "");
        
      mOverViewData[0][i].mMin = (short)TMath::f2i(RealMin);
      mOverViewData[0][i].mMax = (short)TMath::f2i(RealMax);
    }

    mOverViewDataWritePos = EndOverViewPos*MSampleOverviewWidth;
  }
}

// -------------------------------------------------------------------------------------------------

void TAudioFile::OverViewMinMax(
  long long SampleFrameOffset, 
  long long SampleFrameRange, 
  short&    Min, 
  short&    Max)
{
  MAssert(IsOpen(), "Must be open for reading or writing!");
  MAssert(SampleFrameRange > 0, "Invalid sample range!");

  const int NumberOfOverViewFrames = (int)(SampleFrameRange / MSampleOverviewWidth);

  // take a look into the precalculated Overview ?
  if (NumberOfOverViewFrames >= 1)
  {
    const int Channel = 0; // TODO

    if (HaveOverViewData(SampleFrameOffset, SampleFrameRange))
    {
      MAssert(SampleFrameOffset / MSampleOverviewWidth < mOverViewData[0].Size(), "");

      const int FramePosInOverView = (int)((SampleFrameOffset + 
        MSampleOverviewWidth / 2) / MSampleOverviewWidth);

      Min = mOverViewData[Channel][FramePosInOverView].mMin;
      Max = mOverViewData[Channel][FramePosInOverView].mMax;

      // Get the Min and Max from the OverviewData
      for (int i = FramePosInOverView + 1; 
                i < FramePosInOverView + NumberOfOverViewFrames; 
              ++i)
      {
        Min = MMin(mOverViewData[Channel][i].mMin, Min);
        Max = MMax(mOverViewData[Channel][i].mMax, Max);
      }
    }
    else
    {
      Min = Max = 0;
    }
  }
  else
  {
    const float* pSampleBuffer = NULL;

    if (SampleFrameOffset + SampleFrameRange < mReadOverViewBufferSamplePos + mReadOverViewBufferSampleSize && 
        SampleFrameOffset >= mReadOverViewBufferSamplePos)
    {
      // use an already loaded block
      pSampleBuffer = mTempOverViewBuffer.FirstRead() + (SampleFrameOffset - mReadOverViewBufferSamplePos);
    }
    else
    {
      mReadOverViewBufferSamplePos = SampleFrameOffset;
      
      // avoid seeking overhead by reading samples in 2 << 14 sample blocks (if possible)
      mReadOverViewBufferSampleSize = (int)MMax<long long>(SampleFrameRange, 2 << 14);
      
      if (mReadOverViewBufferSamplePos + mReadOverViewBufferSampleSize >= OverviewStream()->NumSamples())
      {
        mReadOverViewBufferSampleSize = (int)(OverviewStream()->NumSamples() - 
          mReadOverViewBufferSamplePos - 1);
      }
    
      mTempOverViewBuffer.Grow(mReadOverViewBufferSampleSize);
      mTempOverViewBufferPtrs.SetSize(1); // overview is always mono
      mTempOverViewBufferPtrs[0] = mTempOverViewBuffer.FirstWrite();
      
      OverviewStream()->SeekTo(SampleFrameOffset);
      
      try
      {
        OverviewStream()->ReadSamples(
          mTempOverViewBufferPtrs, mReadOverViewBufferSampleSize);
      }
      catch (const TReadableException&)
      {
        MInvalid("ReadSamples failed. Broken sample file?");
        mTempOverViewBuffer.Init(0.0f);
      }
      
      pSampleBuffer = mTempOverViewBuffer.FirstRead();
    }

    float fMin = pSampleBuffer[0];                            
    float fMax = pSampleBuffer[0];
       
    TMathT<float>::GetMinMax(fMin, fMax, (int)0, (int)SampleFrameRange, pSampleBuffer);

    Min = (short)fMin;
    Max = (short)fMax;
  }
}
  
// =================================================================================================

// -------------------------------------------------------------------------------------------------

TAudioStream::TAudioStream(
  long long               NumSamples, 
  int                     NumChannels, 
  TAudioFile::TSampleType SampleType)
  : mNumSamples(NumSamples), 
    mNumChannels(NumChannels), 
    mSampleType(SampleType),
    mReadSamplesAreAudible(false),
    mWrittenSamplesAreAudible(false)
{
}

// -------------------------------------------------------------------------------------------------

long long TAudioStream::NumSamples()const
{
  return mNumSamples;
}

// -------------------------------------------------------------------------------------------------

int TAudioStream::NumChannels()const
{
  return mNumChannels;
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TAudioStream::SampleType()const
{
  return mSampleType;
}

// -------------------------------------------------------------------------------------------------

int TAudioStream::PreferedBlockSize()const
{
  const int BlockSize = OnPreferedBlockSize();
  MAssert(BlockSize > 0, "");

  return BlockSize;
}

// -------------------------------------------------------------------------------------------------

bool TAudioStream::ReadSamplesAreAudible()const
{
  return mReadSamplesAreAudible;
}

// -------------------------------------------------------------------------------------------------

bool TAudioStream::WrittenSamplesAreAudible()const
{
  return mWrittenSamplesAreAudible;
}

// -------------------------------------------------------------------------------------------------

long long TAudioStream::BytesWritten()const
{
  return OnBytesWritten();
}

// -------------------------------------------------------------------------------------------------

bool TAudioStream::SeekTo(long long SampleIndex)
{
  return OnSeekTo(SampleIndex);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool TAudioFile::IsOpen()const
{
  if (mpDecompressedWavFile)
  {
    MAssert(mIsOpen == mpDecompressedWavFile->IsOpen(), "Expected the same state!");
    return mpDecompressedWavFile->IsOpen();
  }
  else
  {
    return mIsOpen;
  }
}

// -------------------------------------------------------------------------------------------------

TString TAudioFile::FileName()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->FileName();
  }
  else
  {
    return mFileName;
  }
}

// -------------------------------------------------------------------------------------------------

TString TAudioFile::OriginalFileName()const
{
  return mFileName;
}

// -------------------------------------------------------------------------------------------------

int TAudioFile::FileSubTrackIndex()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->FileSubTrackIndex();
  }
  else
  {
    return mFileSubTrackIndex;
  }
}

// -------------------------------------------------------------------------------------------------

int TAudioFile::OriginalFileSubTrackIndex()const
{
  return mFileSubTrackIndex;
}

// -------------------------------------------------------------------------------------------------

void TAudioFile::Close()
{
  MAssert(mIsOpen, "Is not open!");

  OnClose();
  mIsOpen = false;
}

// -------------------------------------------------------------------------------------------------

float TAudioFile::DecompressionProgress()const
{
  return mDecompressionProgress;
}

// -------------------------------------------------------------------------------------------------

void TAudioFile::CancelDecompression()
{
  mCancelDecompression = true;
  
  TStamp Stamp;
  
  while (mDecompressing)
  {
    if (Stamp.DiffInMs() > 1000)
    {
      MInvalid("Timeout!");
      Stamp.Start();
    }
  }
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TAudioFile::Stream()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->Stream();
  }
  else
  {
    return OnStream();
  }
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TAudioFile::OverviewStream()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->OverviewStream();
  }
  else
  {
    return OnOverviewStream();
  }
}

// -------------------------------------------------------------------------------------------------

int TAudioFile::SamplingRate()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->SamplingRate();
  }
  else
  {
    return OnSamplingRate();
  }
}

// -------------------------------------------------------------------------------------------------

int TAudioFile::BitsPerSample()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->BitsPerSample();
  }
  else
  {
    return OnBitsPerSample();
  }
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TAudioFile::SampleType()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->SampleType();
  }
  else
  {
    return OnSampleType();
  }
}

// -------------------------------------------------------------------------------------------------

int TAudioFile::NumChannels()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->NumChannels();
  }
  else
  {
    return OnNumChannels();
  }
}

// -------------------------------------------------------------------------------------------------

long long TAudioFile::NumSamples()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->NumSamples();
  }
  else
  {
    return OnNumSamples();
  }
}

// -------------------------------------------------------------------------------------------------

TAudioTypes::TLoopMode TAudioFile::LoopMode()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->LoopMode();
  }
  else
  {
    return OnLoopMode();
  }
}

// -------------------------------------------------------------------------------------------------

bool TAudioFile::HasLoopProperties()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->HasLoopProperties();
  }
  else
  {
    return (OnLoopMode() != TAudioTypes::kNoLoop);
  }
}

// -------------------------------------------------------------------------------------------------

long long TAudioFile::LoopStart()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->LoopStart();
  }
  else
  {
    MAssert(OnLoopMode() != TAudioTypes::kNoLoop, "Not valid then!");
    return OnLoopStart();
  }
}

// -------------------------------------------------------------------------------------------------

long long TAudioFile::LoopEnd()const
{
  if (mpDecompressedWavFile)
  {
    return mpDecompressedWavFile->LoopEnd();
  }
  else
  {
    MAssert(OnLoopMode() != TAudioTypes::kNoLoop, "Not valid then!");
    return OnLoopEnd();
  }
}

// -------------------------------------------------------------------------------------------------

void TAudioFile::SetLoop(
  TAudioTypes::TLoopMode  LoopMode, 
  long long               LoopStart, 
  long long               LoopEnd)
{
  MAssert(!IsOpen(), "Setup properties before opening the file for writing");
  OnSetLoop(LoopMode, LoopStart, LoopEnd);
}

// -------------------------------------------------------------------------------------------------

TArray<long long> TAudioFile::CuePoints()const
{
  return OnCuePoints();
}

// -------------------------------------------------------------------------------------------------

void TAudioFile::SetCuePoints(const TArray<long long>& CuePoints)
{
  OnSetCuePoints(CuePoints);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TAudioStream::ReadSamples(
  const TArray<float*>& DestChannelPtrs, 
  int                   NumberOfSampleFrames,
  bool                  CheckForAudibleSamples)
{
  MAssert(DestChannelPtrs.Size() > 0, "Need at least one channel");

  const int SrcChaCount = mNumChannels;
  const int DestChaCount = DestChannelPtrs.Size();
                  
  const int SrcBytesPerFrame = TAudioFile::SNumBitsFromSampleType(mSampleType) / 8;
  

  // ... read src data into a temp buffer
  
  // mTempBuffer is a member to avoid reallocation when calling ReadSamples
  // frequently with the same number of frames. Available for every thread because
  // a stream can be accessed from several threads at once
  mTempBuffer.Grow(
    NumberOfSampleFrames * SrcChaCount * SrcBytesPerFrame);
  
  // this will take care about endianess
  OnReadBuffer(mTempBuffer.FirstWrite(), NumberOfSampleFrames);
  

  // ... Convert to the dest format

  MStaticAssert(TAudioFile::kNumOfSampleTypes == 7); // Update me

  switch (mSampleType)  
  {

  // ....................................................... k8BitSigned

  case TAudioFile::k8BitSigned:
  {
    if (SrcChaCount == 1)
    {          
      for (int Cha = 0; Cha < DestChaCount; ++Cha)
      {               
        TSampleConverter::Int8BitSignedToInternalFloat(
          (const TInt8*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[Cha], NumberOfSampleFrames);
      }
    }                      
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 1)
      {
        TSampleConverter::Int8BitSignedInterleavedStereoToInternalFloatMono(
          (const TInt8*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], NumberOfSampleFrames);
      }
      else if (DestChaCount == 2)
      {              
        TSampleConverter::Int8BitSignedInterleavedToInternalFloat(
          (const TInt8*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], DestChannelPtrs[1], NumberOfSampleFrames);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == DestChaCount)
    {
      TSampleConverter::Int8BitSignedInterleavedToInternalFloat(
        (const TInt8*)mTempBuffer.FirstRead(),
        DestChannelPtrs, NumberOfSampleFrames );
    }
    else
    {
      throw SUnsupportedChannelLayoutException(
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;


  // ....................................................... k8BitUnsigned

  case TAudioFile::k8BitUnsigned:
  {
    if (SrcChaCount == 1)
    {          
      for (int Cha = 0; Cha < DestChaCount; ++Cha)
      {               
        TSampleConverter::Int8BitUnsignedToInternalFloat(
          (const TUInt8*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[Cha], NumberOfSampleFrames);
      }
    }                      
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 1)
      {
        TSampleConverter::Int8BitUnsignedInterleavedStereoToInternalFloatMono(
          (const TUInt8*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], NumberOfSampleFrames);
      }
      else if (DestChaCount == 2)
      {              
        TSampleConverter::Int8BitUnsignedInterleavedToInternalFloat(
          (const TUInt8*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], DestChannelPtrs[1], NumberOfSampleFrames);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == DestChaCount)
    {
      TSampleConverter::Int8BitUnsignedInterleavedToInternalFloat(
        (const TUInt8*)mTempBuffer.FirstRead(),
        DestChannelPtrs, NumberOfSampleFrames);
    }
    else
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;


  // ....................................................... k16Bit

  case TAudioFile::k16Bit:
  {
    if (SrcChaCount == 1)
    {                 
      for (int Cha = 0; Cha < DestChaCount; ++Cha)
      {               
        TSampleConverter::Int16BitToInternalFloat(
          (const TInt16*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[Cha], NumberOfSampleFrames);
      }
    }
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 1)
      {               
        TSampleConverter::Int16BitInterleavedStereoToInternalFloatMono(
          (const TInt16*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], NumberOfSampleFrames);
      }
      else if (DestChaCount == 2)
      {
        TSampleConverter::Int16BitInterleavedToInternalFloat(
          (const TInt16*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], DestChannelPtrs[1], NumberOfSampleFrames);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == DestChaCount)
    {
      TSampleConverter::Int16BitInterleavedToInternalFloat(
        (const TInt16*)mTempBuffer.FirstRead(),
        DestChannelPtrs, NumberOfSampleFrames);
    }
    else 
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;


  // ....................................................... k24Bit

  case TAudioFile::k24Bit:
  {
    if (SrcChaCount == 1)
    {
      for (int Cha = 0; Cha < DestChaCount; ++Cha)
      {               
        TSampleConverter::Int24BitToInternalFloat(
          (const T24*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[Cha], NumberOfSampleFrames);
      }
    }
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 1)
      {
        TSampleConverter::Int24BitInterleavedStereoToInternalFloatMono(
          (const T24*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], NumberOfSampleFrames);
      }
      else if (DestChaCount == 2)
      {
        TSampleConverter::Int24BitInterleavedToInternalFloat(
          (const T24*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], DestChannelPtrs[1], NumberOfSampleFrames);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == DestChaCount)
    {
      TSampleConverter::Int24BitInterleavedToInternalFloat(
        (const T24*)mTempBuffer.FirstRead(),
        DestChannelPtrs, NumberOfSampleFrames);
    }
    else
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;


  // ....................................................... k32BitInt

  case TAudioFile::k32BitInt:
  {
    if (SrcChaCount == 1)
    {              
      for (int Cha = 0; Cha < DestChaCount; ++Cha)
      {               
        TSampleConverter::Int32BitToInternalFloat(
          (const TInt32*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[Cha], NumberOfSampleFrames);
      }
    }
    else if (SrcChaCount == 2)
    {            
      if (DestChaCount == 1)
      { 
        TSampleConverter::Int32BitInterleavedStereoToInternalFloatMono(
          (const TInt32*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], NumberOfSampleFrames);
      }
      else if (DestChaCount == 2)
      {
        TSampleConverter::Int32BitInterleavedToInternalFloat(
          (const TInt32*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], DestChannelPtrs[1], NumberOfSampleFrames);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == DestChaCount)
    {
      TSampleConverter::Int32BitInterleavedToInternalFloat(
        (const TInt32*)mTempBuffer.FirstRead(),
        DestChannelPtrs, NumberOfSampleFrames);
    }
    else
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;


  // ....................................................... k32BitFloat

  case TAudioFile::k32BitFloat:
  {
    if (SrcChaCount == 1)
    {           
      for (int Cha = 0; Cha < DestChaCount; ++Cha)
      {               
        TSampleConverter::Float32BitToInternalFloat(
          (const float*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[Cha], NumberOfSampleFrames);
      }
    }
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 1)
      {                    
        TSampleConverter::Float32BitInterleavedStereoToInternalFloatMono(
          (const float*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], NumberOfSampleFrames);
      }
      else if (DestChaCount == 2)
      {   
        TSampleConverter::Float32BitInterleavedToInternalFloat(
          (const float*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], DestChannelPtrs[1], NumberOfSampleFrames);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == DestChaCount)
    {
      TSampleConverter::Float32BitInterleavedToInternalFloat(
        (const float*)mTempBuffer.FirstRead(),
        DestChannelPtrs, NumberOfSampleFrames);
    }
    else
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;


  // ....................................................... k64BitFloat

  case TAudioFile::k64BitFloat:
  {
    if (SrcChaCount == 1)
    {
      for (int Cha = 0; Cha < DestChaCount; ++Cha)
      { 
        TSampleConverter::Float64BitToInternalFloat(
          (const double*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[Cha], NumberOfSampleFrames);
      }
    }
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 1)
      {               
        TSampleConverter::Float64BitInterleavedStereoToInternalFloatMono(
          (const double*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], NumberOfSampleFrames);
      }
      else if (DestChaCount == 2)
      {
        TSampleConverter::Float64BitInterleavedToInternalFloat(
          (const double*)mTempBuffer.FirstRead(), 
          DestChannelPtrs[0], DestChannelPtrs[1], NumberOfSampleFrames);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == DestChaCount)
    {
      TSampleConverter::Float64BitInterleavedToInternalFloat(
        (const double*)mTempBuffer.FirstRead(),
        DestChannelPtrs, NumberOfSampleFrames );
    }
    else
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;

  default:
    throw TReadableException(MText("Unknown or unsupported audio file format: %s", 
      ToString((int)mSampleType)));
  }


  // ... Check if there is some audible content

  if (CheckForAudibleSamples && !mReadSamplesAreAudible)
  {
    for (int Channel = 0; Channel < DestChannelPtrs.Size(); ++Channel)        
    {
      const float* pChannelSamples = DestChannelPtrs[Channel];
      
      for (int SampleFrame = 0; SampleFrame < NumberOfSampleFrames; ++SampleFrame)
      {
        if (pChannelSamples[SampleFrame] > MMinAudibleSignal ||
            pChannelSamples[SampleFrame] < -MMinAudibleSignal)
        {
          mReadSamplesAreAudible = true;
          break;
        }
      }
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TAudioStream::WriteSamples(
  const TArray<const float*>& SrcChannelPtrs, 
  int                         NumberOfSampleFrames,
  bool                        Dither,
  bool                        CheckForAudibleSamples)
{
  MAssert(SrcChannelPtrs.Size() > 0, "Need at least one channel");

  const int SrcChaCount = SrcChannelPtrs.Size();
  const int DestChaCount = mNumChannels;
                  
  const int DestBytesPerFrame = TAudioFile::SNumBitsFromSampleType(mSampleType) / 8;

  
  // ... Check if there is some audible content

  if (CheckForAudibleSamples && !mWrittenSamplesAreAudible)
  {
    for (int Channel = 0; Channel < SrcChannelPtrs.Size(); ++Channel)        
    {
      const float* pChannelSamples = SrcChannelPtrs[Channel];

      for (int SampleFrame = 0; SampleFrame < NumberOfSampleFrames; ++SampleFrame)
      {
        if (pChannelSamples[SampleFrame] > MMinAudibleSignal ||
            pChannelSamples[SampleFrame] < -MMinAudibleSignal)
        {
          mWrittenSamplesAreAudible = true;
          break;
        }
      }
    }
  }


  // ... Convert to the dest format

  MStaticAssert(TAudioFile::kNumOfSampleTypes == 7); // Update me


  // ... write dest data into a temp buffer

  // mTempBuffer is a member to avoid reallocation when calling WriteSamples
  // frequently with the same number of frames
  mTempBuffer.Grow(
    NumberOfSampleFrames * DestChaCount * DestBytesPerFrame);
  

  switch (mSampleType)  
  {
  
  // ....................................................... k32BitFloat

  case TAudioFile::k32BitFloat:
  {
    if (SrcChaCount == 1)
    { 
      if (DestChaCount == 1)
      { 
        TSampleConverter::InternalFloatToFloat32(
          SrcChannelPtrs[0], 
          (float*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames);
      }
      else if (DestChaCount == 2)
      {   
        TSampleConverter::InternalFloatToFloat32Interleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[0],
          (float*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 2)
      {
        TSampleConverter::InternalFloatToFloat32Interleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[1],
          (float*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;


  // ....................................................... k32BitInt

  case TAudioFile::k32BitInt:
  {
    if (SrcChaCount == 1)
    { 
      if (DestChaCount == 1)
      { 
        TSampleConverter::InternalFloatToInt32Bit(
          SrcChannelPtrs[0], 
          (TInt32*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames);
      }
      else if (DestChaCount == 2)
      {   
        TSampleConverter::InternalFloatToInt32BitInterleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[0],
          (TInt32*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 2)
      {
        TSampleConverter::InternalFloatToInt32BitInterleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[1],
          (TInt32*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;


  // ....................................................... k24Bit

  case TAudioFile::k24Bit:
  {
    if (SrcChaCount == 1)
    { 
      if (DestChaCount == 1)
      { 
        TSampleConverter::InternalFloatToInt24Bit(
          SrcChannelPtrs[0], 
          (T24*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else if (DestChaCount == 2)
      {   
        TSampleConverter::InternalFloatToInt24BitInterleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[0],
          (T24*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 2)
      {
        TSampleConverter::InternalFloatToInt24BitInterleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[1],
          (T24*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;


  // ....................................................... k16Bit

  case TAudioFile::k16Bit:
  {
    if (SrcChaCount == 1)
    { 
      if (DestChaCount == 1)
      { 
        TSampleConverter::InternalFloatToInt16Bit(
          SrcChannelPtrs[0], 
          (TInt16*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else if (DestChaCount == 2)
      {   
        TSampleConverter::InternalFloatToInt16BitInterleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[0],
          (TInt16*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 2)
      {
        TSampleConverter::InternalFloatToInt16BitInterleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[1],
          (TInt16*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;


  // ....................................................... k8BitSigned

  case TAudioFile::k8BitSigned:
  {
    if (SrcChaCount == 1)
    { 
      if (DestChaCount == 1)
      { 
        TSampleConverter::InternalFloatToInt8BitSigned(
          SrcChannelPtrs[0], 
          (TInt8*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else if (DestChaCount == 2)
      {   
        TSampleConverter::InternalFloatToInt8BitSignedInterleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[0],
          (TInt8*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 2)
      {
        TSampleConverter::InternalFloatToInt8BitSignedInterleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[1],
          (TInt8*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType);
    }
  } break;
  
  
  // ....................................................... k8BitUnsigned

  case TAudioFile::k8BitUnsigned:
  {
    if (SrcChaCount == 1)
    { 
      if (DestChaCount == 1)
      { 
        TSampleConverter::InternalFloatToInt8BitUnsigned(
          SrcChannelPtrs[0], 
          (TUInt8*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else if (DestChaCount == 2)
      {   
        TSampleConverter::InternalFloatToInt8BitUnsignedInterleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[0],
          (TUInt8*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else if (SrcChaCount == 2)
    {
      if (DestChaCount == 2)
      {
        TSampleConverter::InternalFloatToInt8BitUnsignedInterleaved(
          SrcChannelPtrs[0],
          SrcChannelPtrs[1],
          (TUInt8*)mTempBuffer.FirstWrite(), 
          NumberOfSampleFrames,
          Dither);
      }
      else
      {
        throw SChannelConversionException();
      }
    }
    else
    {
      throw SUnsupportedChannelLayoutException( 
        SrcChaCount, DestChaCount, mSampleType );
    }
  } break;
  

  default:
    throw TReadableException( MText("Unknown or unsupported audio file format: %s",
      ToString((int)mSampleType )));
  }

  
  // this will take care about endianess
  OnWriteBuffer(mTempBuffer.FirstRead(), NumberOfSampleFrames);
}
  
// =================================================================================================

// -------------------------------------------------------------------------------------------------

TAudioFileStream::TAudioFileStream(
  TFile*                  pFile,
  long long               SampleDataOffset,
  long long               NumSamples,
  int                     NumChannels,
  TAudioFile::TSampleType SampleType,
  TByteOrder::TByteOrder  SampleDataByteOrder)
  : TAudioStream(NumSamples, NumChannels, SampleType),
    mpFile(pFile),
    mSampleDataOffset(SampleDataOffset),
    mBytesWritten(0),
    mSampleDataByteOrder(SampleDataByteOrder)
{
  // seek to the PCM chunk start
  mpFile->SetPosition((size_t)mSampleDataOffset);
}

// -------------------------------------------------------------------------------------------------

int TAudioFileStream::OnPreferedBlockSize()const
{
  return 8192;
}
  
// -------------------------------------------------------------------------------------------------

long long TAudioFileStream::OnBytesWritten()const
{
  return mBytesWritten;
}

// -------------------------------------------------------------------------------------------------

bool TAudioFileStream::OnSeekTo(long long Frame)
{
  MAssert(mpFile->IsOpenForReading(),
    "Should only seek when the file is open for reading");

  MAssert(Frame >= 0 && Frame < NumSamples(), "SampleIndex out of Range");
  
  const int BytesPerSampleFrame = 
    TAudioFile::SNumBitsFromSampleType(SampleType()) / 8;
  
  const bool Succeeded = mpFile->SetPosition(
    (size_t)(mSampleDataOffset + (long long)BytesPerSampleFrame * NumChannels() * Frame));
  
  return Succeeded;
}

// -------------------------------------------------------------------------------------------------

void TAudioFileStream::OnReadBuffer(
  void* pDestBuffer, 
  int   NumberOfSampleFrames)
{
  MAssert(mpFile->IsOpenForReading(),
    "File stream is not or no longer open");

  MStaticAssert(TAudioFile::kNumOfSampleTypes == 7); // Update me

  // Set PCM byte order
  const TFileSetAndRestoreByteOrder SetPcmByteOrder(*mpFile, mSampleDataByteOrder);

  // Read from file stream
  switch (SampleType())
  {
  case TAudioFile::k8BitSigned:
  case TAudioFile::k8BitUnsigned:
    mpFile->Read((char*)pDestBuffer, (size_t)NumberOfSampleFrames * NumChannels());
    break;

  case TAudioFile::k16Bit:
    mpFile->Read((TInt16*)pDestBuffer, (size_t)NumberOfSampleFrames * NumChannels());
    break;

  case TAudioFile::k24Bit:
    mpFile->Read((T24*)pDestBuffer, (size_t)NumberOfSampleFrames * NumChannels());
    break;

  case TAudioFile::k32BitInt:
  case TAudioFile::k32BitFloat:
    MStaticAssert(sizeof(TInt32) == sizeof(float));
    MStaticAssert(sizeof(TInt32) == 4);
    mpFile->Read((TInt32*)pDestBuffer, (size_t)NumberOfSampleFrames * NumChannels());
    break;

  case TAudioFile::k64BitFloat:
    MStaticAssert(sizeof(double) == 8);
    mpFile->Read((double*)pDestBuffer, (size_t)NumberOfSampleFrames * NumChannels());
    break;

  default:
    MInvalid("");
    break;
  }
}

// -------------------------------------------------------------------------------------------------

void TAudioFileStream::OnWriteBuffer(
  const void* pSrcBuffer, 
  int         NumberOfSampleFrames)
{
  MAssert(mpFile->IsOpenForWriting(),
    "File stream is not or no longer open for writing");

  MStaticAssert(TAudioFile::kNumOfSampleTypes == 7); // Update me

  // Set PCM byte order
  const TFileSetAndRestoreByteOrder SetPcmByteOrder(*mpFile, mSampleDataByteOrder);

  // Write to file stream
  switch (SampleType())
  {
  case TAudioFile::k8BitUnsigned:
  case TAudioFile::k8BitSigned:
    mpFile->Write((const char*)pSrcBuffer, (size_t)NumberOfSampleFrames * NumChannels());
    mBytesWritten += sizeof(char) * (long long)NumberOfSampleFrames * NumChannels();
    break;

  case TAudioFile::k16Bit:
    mpFile->Write((const short*)pSrcBuffer, (size_t)NumberOfSampleFrames * NumChannels());
    mBytesWritten += sizeof(short) * (long long)NumberOfSampleFrames * NumChannels();
    break;

  case TAudioFile::k24Bit:
    mpFile->Write((const T24*)pSrcBuffer, (size_t)NumberOfSampleFrames * NumChannels());
    mBytesWritten += sizeof(T24) * (long long)NumberOfSampleFrames * NumChannels();
    break;

  case TAudioFile::k32BitInt:
  case TAudioFile::k32BitFloat:
    MStaticAssert(sizeof(TInt32) == sizeof(float));
    MStaticAssert(sizeof(TInt32) == 4);
    mpFile->Write((const TInt32*)pSrcBuffer, (size_t)NumberOfSampleFrames * NumChannels());
    mBytesWritten += sizeof(TInt32) * (long long)NumberOfSampleFrames * NumChannels();
    break;

  case TAudioFile::k64BitFloat:
    MStaticAssert(sizeof(double) == 8);
    mpFile->Write((const double*)pSrcBuffer, (size_t)NumberOfSampleFrames * NumChannels());
    mBytesWritten += sizeof(double) * (long long)NumberOfSampleFrames * NumChannels();
    break;

  default:
    MInvalid("");
    break;
  }
}

