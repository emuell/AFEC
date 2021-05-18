#include "CoreFileFormatsPrecompiledHeader.h"

#include "AudioTypes/Export/AudioMath.h"

#include <limits>

// =================================================================================================

#define UnsignedToFloat(u) (((double)((int)(u - 2147483647 - 1))) + 2147483648.0)
#define FloatToUnsigned(f) ((TUInt32)(((int)(f - 2147483648.0)) + 2147483647) + 1)

// =================================================================================================

namespace TIeee80Conversion
{
  //! convert the Apple specific 10 byte floats to doubles and vice versa
  void ToDouble(const TIeee80& Source, double& Dest);
  void FromDouble(const double& Source, TIeee80& Dest);
}

// -------------------------------------------------------------------------------------------------

void TIeee80Conversion::ToDouble(const TIeee80& Ieee80, double& Double)
{
  int expon = ((Ieee80[0] & 0x7F) << 8) | (Ieee80[1] & 0xFF);

  TUInt32 hiMant = 
    ((TUInt32)(Ieee80[2] & 0xFF) << 24) | 
    ((TUInt32)(Ieee80[3] & 0xFF) << 16) | 
    ((TUInt32)(Ieee80[4] & 0xFF) << 8)  | 
    ((TUInt32)(Ieee80[5] & 0xFF));

  TUInt32 loMant = 
    ((TUInt32)(Ieee80[6] & 0xFF) << 24) | 
    ((TUInt32)(Ieee80[7] & 0xFF) << 16) | 
    ((TUInt32)(Ieee80[8] & 0xFF) << 8)  |
    ((TUInt32)(Ieee80[9] & 0xFF));

  if (expon == 0 && hiMant == 0 && loMant == 0) 
  {
    Double = 0;
  }
  else 
  {
    // Infinity or NaN
    if (expon == 0x7FFF) 
    {    
      Double = std::numeric_limits<double>::infinity();
    }
    else 
    {
      expon -= 16383;
      Double  = ldexp(UnsignedToFloat(hiMant), expon -= 31);
      Double += ldexp(UnsignedToFloat(loMant), expon -= 32);
    }
  }

  if (Ieee80[0] & 0x80)
  {
    Double = -Double;
  }
}

// -------------------------------------------------------------------------------------------------

void TIeee80Conversion::FromDouble(const double& Double, TIeee80& Ieee80)
{
  double Temp = Double;

  int sign;

  if (Temp < 0) 
  {
    sign = 0x8000;
    Temp *= -1;
  } 
  else 
  {
    sign = 0;
  }

  int expon;
  double fMant, fsMant;
  TUInt32 hiMant, loMant;

  if (Temp == 0) 
  {
    expon = 0; 
    hiMant = 0; 
    loMant = 0;
  }
  else 
  {
    fMant = frexp(Temp, &expon);
    
    /* Infinity or NaN */
    if ((expon > 16384) || !(fMant < 1)) 
    {    
      expon = sign | 0x7FFF; 
      hiMant = 0; 
      loMant = 0; /* infinity */
    }
    /* Finite */
    else 
    {    
      expon += 16382;
      
      /* denormalized */
      if (expon < 0) 
      {    
        fMant = ldexp(fMant, expon);
        expon = 0;
      }
      
      expon |= sign;
      fMant = ldexp(fMant, 32);          
      fsMant = floor(fMant); 
      hiMant = FloatToUnsigned(fsMant);
      fMant = ldexp(fMant - fsMant, 32); 
      fsMant = floor(fMant); 
      loMant = FloatToUnsigned(fsMant);
    }
  }
  
  Ieee80[0] = (char)(expon >> 8);
  Ieee80[1] = (char)(expon);
  Ieee80[2] = (char)(hiMant >> 24);
  Ieee80[3] = (char)(hiMant >> 16);
  Ieee80[4] = (char)(hiMant >> 8);
  Ieee80[5] = (char)(hiMant);
  Ieee80[6] = (char)(loMant >> 24);
  Ieee80[7] = (char)(loMant >> 16);
  Ieee80[8] = (char)(loMant >> 8);
  Ieee80[9] = (char)(loMant);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TAifCommChunk::TAifCommChunk()
{
  mChannels = 0;
  mSampleFrames = 0;
  mSampleBits = 0;
  TMemory::Zero(mSampleFreq, sizeof(mSampleFreq));
}

// -------------------------------------------------------------------------------------------------

bool TAifCommChunk::VerifyValidity()const
{
  if (mSampleBits == 8 || mSampleBits == 16 || 
      mSampleBits == 24 || mSampleBits == 32 || 
      mSampleBits == 64)
  {
    return true;
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

void TAifCommChunk::Read(TFile* pFile)
{
  pFile->Read(mChannels);
  pFile->Read(mSampleFrames);
  pFile->Read(mSampleBits);
  pFile->Read(mSampleFreq, 10);
}

// -------------------------------------------------------------------------------------------------

void TAifCommChunk::Write(TFile* pFile)
{
  pFile->Write(mChannels);
  pFile->Write(mSampleFrames);
  pFile->Write(mSampleBits);
  pFile->Write(mSampleFreq, MCountOf(mSampleFreq));
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TAifcCommChunkExt::TAifcCommChunkExt()
{
  mCompressionType = 0; 
}

// -------------------------------------------------------------------------------------------------

bool TAifcCommChunkExt::VerifyValidity()const
{
  return
    // Quicktime extensions: uncompressed, possibly byteswapped linear PCM data
    mCompressionType == gFourCC("ENON") || // linear PCM, little endian
    mCompressionType == gFourCC("enon") || 
    mCompressionType == gFourCC("raw ") || // linear PCM, 8-bit unsigned data
    mCompressionType == gFourCC("RAW ") ||
    mCompressionType == gFourCC("twos") || // linear PCM, 16-bit data, big endian
    mCompressionType == gFourCC("TWOS") ||
    mCompressionType == gFourCC("sowt") || // linear PCM, 16-bit data, little endian
    mCompressionType == gFourCC("SOWT") ||
    mCompressionType == gFourCC("in24") || // linear PCM, 24-bit data, big endian
    mCompressionType == gFourCC("IN24") ||
    mCompressionType == gFourCC("in32") || // linear PCM, 32-bit data, big endian
    mCompressionType == gFourCC("IN32") ||
    mCompressionType == gFourCC("23lf") || // 32-bit floating point PCM, IEEE byte order
    mCompressionType == gFourCC("23LF") || 
    mCompressionType == gFourCC("46lf") || // 64-bit floating point PCM, IEEE byte order
    mCompressionType == gFourCC("46LF");
}

// -------------------------------------------------------------------------------------------------
  
void TAifcCommChunkExt::Read(TFile* pFile)
{
  pFile->Read(mCompressionType);
  
  TUInt8 Size;
  pFile->Read(Size);
  
  mCompressionName.SetSize(Size);
  pFile->Read(mCompressionName.FirstWrite(), Size); 
}

// -------------------------------------------------------------------------------------------------

void TAifcCommChunkExt::Write(TFile* pFile)
{
  pFile->Write(mCompressionType);
  
  pFile->Write((char)mCompressionName.Size());
  pFile->Write(mCompressionName.FirstRead(), mCompressionName.Size());
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TAifSndChunk::TAifSndChunk()
{
  mOffset = 0;
  mBlockSize = 0;
}

// -------------------------------------------------------------------------------------------------

void TAifSndChunk::Read(TFile* pFile)
{
  pFile->Read(mOffset);
  pFile->Read(mBlockSize);
}

// -------------------------------------------------------------------------------------------------

void TAifSndChunk::Write(TFile* pFile)
{
  pFile->Write(mOffset);
  pFile->Write(mBlockSize);
}

// =================================================================================================

TList<TString> TAifFile::SSupportedExtensions()
{
  TList<TString> Ret;
  
  Ret.Append("*.aif");
  Ret.Append("*.aiff");
  Ret.Append("*.aifc");
  Ret.Append("*.snd");

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TAifFile::TAifFile()
  : TAudioFile(), 
    TRiffFile(gFourCC("FORM")), 
    mSampleDataOffset(0), 
    mNumOfSamples(0)
{
}

// -------------------------------------------------------------------------------------------------

TString TAifFile::OnTypeString()const
{
  return "AIFF";
}

// -------------------------------------------------------------------------------------------------

void TAifFile::OnOpenForRead(
  const TString&  FileName,
  int             FileSubTrackIndex)
{
  MUnused(FileSubTrackIndex);

  TRiffFile::Open(FileName, TFile::kRead, TByteOrder::kMotorola);

  ReadChunks();

  if (!(FindChunkByName("AIFF") || FindChunkByName("AIFC")))
  {
    throw TReadableException(MText("This is not a valid AIFF file!"));
  }

  if (!FindChunkByName("COMM") || !FindChunkByName("SSND"))
  {
    throw TReadableException(MText("This is not a valid AIFF file!"));
  }

  TRiffChunkHeaderInfo* pCommChunk = FindChunkByName("COMM");

  TFile& File = TRiffFile::File();

  File.SetPosition(pCommChunk->Offset());
  mCommChunk.Read(&File);

  if (!mCommChunk.VerifyValidity())
  {
    throw TReadableException(MText("Unsupported AIFF file type."));
  }

  if (FindChunkByName("AIFC"))
  {
    mCommChunkExt.Read(&File);

    if (!mCommChunkExt.VerifyValidity())
    {
      throw TReadableException(MText("Unsupported compressed AIFC file type."));
    }
  }

  TRiffChunkHeaderInfo* pDataChunk = FindChunkByName("SSND");
  File.SetPosition(pDataChunk->Offset());
  mSndChunk.Read(&File);

  mSampleDataOffset = pDataChunk->Offset() + sizeof(mSndChunk) + mSndChunk.mOffset;

  const size_t BytesPerSample = (BitsPerSample() % 8 == 0) ?
    BitsPerSample() / 8 : BitsPerSample() / 8 + 1;

  const size_t RestSamplesInFile = (File.SizeInBytes() - (size_t)mSampleDataOffset) /
    (NumChannels() * BytesPerSample);

  mNumOfSamples = MMin<size_t>(RestSamplesInFile, mCommChunk.mSampleFrames);

  TByteOrder::TByteOrder StreamByteOrder = TByteOrder::kMotorola; // little endian
  if (MakeList(gFourCC("twos"), gFourCC("TWOS"), gFourCC("in24"), gFourCC("IN24"), 
        gFourCC("in32"), gFourCC("IN32")).Contains(mCommChunkExt.mCompressionType))
  { 
    // Quicktime extensions: linear 16,24 or 32-bit int data in big endian format
    StreamByteOrder = TByteOrder::kIntel;
  }

  mpStream = new TAudioFileStream(&File, mSampleDataOffset,
    OnNumSamples(), OnNumChannels(), OnSampleType(), StreamByteOrder);

  mpOverviewStream = new TAudioFileStream(&File, mSampleDataOffset,
    OnNumSamples(), OnNumChannels(), OnSampleType(), StreamByteOrder);
}

// -------------------------------------------------------------------------------------------------

void TAifFile::OnOpenForWrite(
  const TString&  Filename,
  int             SamplingRate,
  TSampleType     SampleType,
  int             NumChannels)
{
  throw TReadableException(MText("Writing AIF files is currently not supported"));
}

// -------------------------------------------------------------------------------------------------

void TAifFile::OnClose()
{
  // close and finalize the RIFF file
  TRiffFile::CloseFile();
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TAifFile::OnStream()const
{
  return mpStream;
}

// -------------------------------------------------------------------------------------------------

TAudioStream* TAifFile::OnOverviewStream()const
{
  return mpOverviewStream;
}

// -------------------------------------------------------------------------------------------------

int TAifFile::OnSamplingRate()const
{
  double Double = 0.0;
  TIeee80Conversion::ToDouble(mCommChunk.mSampleFreq, Double);

  return (int)Double;
}

// -------------------------------------------------------------------------------------------------

int TAifFile::OnBitsPerSample()const
{
  return (int)mCommChunk.mSampleBits;
}

// -------------------------------------------------------------------------------------------------

int TAifFile::OnNumChannels()const
{
  return (int)mCommChunk.mChannels;
}

// -------------------------------------------------------------------------------------------------

long long TAifFile::OnNumSamples()const
{
  return mNumOfSamples;
}

// -------------------------------------------------------------------------------------------------

TAudioFile::TSampleType TAifFile::OnSampleType()const
{
  MStaticAssert(TAudioFile::kNumOfSampleTypes == 7); //Update me

  switch (OnBitsPerSample())
  {
  default:
    MInvalid("");

  case 8:
    if (mCommChunkExt.mCompressionType == gFourCC("raw ") ||
        mCommChunkExt.mCompressionType == gFourCC("RAW "))
    {
      return TAudioFile::k8BitUnsigned;
    }
    else
    {
      return TAudioFile::k8BitSigned;
    }

  case 16:
    return TAudioFile::k16Bit;

  case 24:
    return TAudioFile::k24Bit;

  case 32:
    if (mCommChunkExt.mCompressionType == gFourCC("23lf") ||
        mCommChunkExt.mCompressionType == gFourCC("23LF"))
    {
      return TAudioFile::k32BitFloat;
    }
    else
    {
      return TAudioFile::k32BitInt;
    }

  case 64:
    MAssert(mCommChunkExt.mCompressionType == gFourCC("46lf") || 
      mCommChunkExt.mCompressionType == gFourCC("46LF"), "");
      
    return TAudioFile::k64BitFloat;
  }
}

// -------------------------------------------------------------------------------------------------

TAudioTypes::TLoopMode TAifFile::OnLoopMode()const
{
  // Not yet implemented
  return TAudioTypes::kNoLoop;
}

// -------------------------------------------------------------------------------------------------

long long TAifFile::OnLoopStart()const
{
  return -1; // Not yet implemented
}

// -------------------------------------------------------------------------------------------------

long long TAifFile::OnLoopEnd()const
{
  return -1; // Not yet implemented
}

// -------------------------------------------------------------------------------------------------

void TAifFile::OnSetLoop(
  TAudioTypes::TLoopMode LoopMode, 
  long long              LoopStart, 
  long long              LoopEnd)
{
  // Not yet implemented
}

// -------------------------------------------------------------------------------------------------

TArray<long long> TAifFile::OnCuePoints()const 
{ 
  // Not yet implemented
  return TArray<long long>(); 
}

// -------------------------------------------------------------------------------------------------

void TAifFile::OnSetCuePoints(const TArray<long long>& CuePoints) 
{ 
  // Not yet implemented
}

// -------------------------------------------------------------------------------------------------

bool TAifFile::OnIsParentChunk(const TString &FourccName)
{
  return FourccName == "AIFF" || FourccName == "AIFC";
}

