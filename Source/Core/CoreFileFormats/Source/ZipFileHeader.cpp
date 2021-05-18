#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Directory.h"

#include <cstring> // memcmp
#include <limits>

namespace TZLib {
  #include "../../3rdParty/ZLib/Export/ZLib.h"
}

// =================================================================================================

#define MZipFileHeaderSize  46
#define MLocalZipFileHeaderSize  30
#define MEncryptedHeaderLen 12

#define MWinArchiveAttribute 0x20
#define MRequiredZipVersion 20
  
// =================================================================================================

const char TZipFileHeader::sGZSignature[4] = {0x50, 0x4b, 0x01, 0x02};
const char TZipFileHeader::sPkZipSignature[4] = {0x50, 0x4b, 0x03, 0x04};

// -------------------------------------------------------------------------------------------------

TZipFileHeader::TZipFileHeader()
  : mSignature(),
    mVersionMadeBy(0),
    mVersionNeeded(0),
    mFlags(0),
    mCompressionMethod(Z_DEFLATED),
    mModTime(0),
    mModDate(0),
    mCrc32(0),
    mCompressedSize(0),
    mUncompressedSize(0),
    mDiskStart(0),
    mInternalAttr(0),
    mExternalAttr(0),
    mOffset(0)  
{
}

// -------------------------------------------------------------------------------------------------

bool TZipFileHeader::HasDataDescr()const
{
  return (mFlags & (int)0x8) != 0;
}

// -------------------------------------------------------------------------------------------------

bool TZipFileHeader::HasUtf8FileNameAndComment()const
{
  return (mFlags & 0x800) != 0;;
}

// -------------------------------------------------------------------------------------------------

size_t TZipFileHeader::Size()const
{
  return MZipFileHeaderSize + GetExtraFieldSize() + GetFileNameSize() + GetCommentSize();
}

// -------------------------------------------------------------------------------------------------

TString TZipFileHeader::GetFileName()const
{
  if (HasUtf8FileNameAndComment())
  {
    return TString().SetFromCStringArray(mArchiveFileName, TString::kUtf8).ReplaceChar(
      TDirectory::SWrongPathSeparatorChar(), TDirectory::SPathSeparatorChar());
  }
  else
  {
    return TString().SetFromCStringArray(mArchiveFileName, TString::kFileSystemEncoding).ReplaceChar(
      TDirectory::SWrongPathSeparatorChar(), TDirectory::SPathSeparatorChar());
  }
}

// -------------------------------------------------------------------------------------------------

void TZipFileHeader::SetFileName(const TString& FileName)
{
  if (!HasUtf8FileNameAndComment() && !FileName.IsPureAscii())
  {
    mFlags |= 0x800; // use utf8 
  }

  if (HasUtf8FileNameAndComment())
  {
    TString(FileName).ReplaceChar(TDirectory::SWrongPathSeparatorChar(), 
      TDirectory::SPathSeparatorChar()).CreateCStringArray(mArchiveFileName, TString::kUtf8);
  }
  else
  {
    TString(FileName).ReplaceChar(TDirectory::SWrongPathSeparatorChar(), 
      TDirectory::SPathSeparatorChar()).CreateCStringArray(mArchiveFileName, TString::kFileSystemEncoding);
  }
}

// -------------------------------------------------------------------------------------------------

TString TZipFileHeader::GetComment()const
{
  if (HasUtf8FileNameAndComment())
  {
    return TString().SetFromCStringArray(mComment, TString::kUtf8);
  }
  else
  {
    return TString().SetFromCStringArray(mComment, TString::kPlatformEncoding);
  }
}

// -------------------------------------------------------------------------------------------------

void TZipFileHeader::SetComment(const TString& Comment)
{
  if (!HasUtf8FileNameAndComment() && !Comment.IsPureAscii())
  {
    mFlags |= 0x800; // use utf8 
  }

  if (HasUtf8FileNameAndComment())
  {
    Comment.CreateCStringArray(mComment, TString::kUtf8);
  }
  else
  {
    Comment.CreateCStringArray(mComment, TString::kPlatformEncoding);
  }
}

// -------------------------------------------------------------------------------------------------

TDate TZipFileHeader::GetDate()const
{
  int Year = 1980 + (mModDate >> 9);
  TMathT<int>::Clip(Year, 0, 3000);
  
  int Month = (mModDate >> 5) & 0x0f;
  TMathT<int>::Clip(Month, 1, 12);
  
  int Day = mModDate & 0x1f;
  TMathT<int>::Clip(Day, 1, 32);
   
  return TDate(Day, Month, Year);
}

// -------------------------------------------------------------------------------------------------

void TZipFileHeader::SetDate(const TDate& Date)
{
  int Year = Date.Year();
  
  if (Year <= 1980)
  {
    Year = 0;
  }
  else
  {
    Year -= 1980;
  }

  mModDate = (TUInt16)(Date.Day() + (Date.Month() << 5) + (Year << 9));
}

// -------------------------------------------------------------------------------------------------

TDayTime TZipFileHeader::GetDayTime()const
{
  int Hours = mModTime >> 11;
  TMathT<int>::Clip(Hours, 0, 23);
  
  int Minutes = (mModTime >> 5) & 0x3f;
  TMathT<int>::Clip(Minutes, 0, 59);
  
  int Seconds = (mModTime& 0x1f) << 1;
  TMathT<int>::Clip(Seconds, 0, 59);
   
  return TDayTime(Hours, Minutes, Seconds);
}

// -------------------------------------------------------------------------------------------------

void TZipFileHeader::SetDayTime(const TDayTime& DayTime)
{
  mModTime = (TUInt16)((DayTime.Seconds() >> 1) + 
    (DayTime.Minutes() << 5) + (DayTime.Hours() << 11));
}

// -------------------------------------------------------------------------------------------------

bool TZipFileHeader::Read(TFile* pStorage)
{
//   // just in case
  mComment.Empty();
  mArchiveFileName.Empty();

  TUInt16 FileNameSize, CommentSize, ExtraFieldSize;
  
  pStorage->Read(mSignature, sizeof(mSignature));
  pStorage->Read(mVersionMadeBy);
  pStorage->Read(mVersionNeeded);
  pStorage->Read(mFlags);
  pStorage->Read(mCompressionMethod);
  pStorage->Read(mModTime);
  pStorage->Read(mModDate);
  pStorage->Read(mCrc32);
  pStorage->Read(mCompressedSize);
  pStorage->Read(mUncompressedSize);
  pStorage->Read(FileNameSize);
  pStorage->Read(ExtraFieldSize);
  pStorage->Read(CommentSize);
  pStorage->Read(mDiskStart);
  pStorage->Read(mInternalAttr);
  pStorage->Read(mExternalAttr);
  pStorage->Read(mOffset);

  if (::memcmp(mSignature, sGZSignature, 4) != 0)
  {
    return false;
  }

  mArchiveFileName.SetSize(FileNameSize); // don't add NULL at the end
  pStorage->Read(mArchiveFileName.FirstWrite(), FileNameSize);
  
  #if defined(MWindows)
    // fix file name characters not allowed on windows but on other OSs
    for (int i = 0; i < mArchiveFileName.Size(); ++i)
    {
      if (mArchiveFileName[i] >= 1 && mArchiveFileName[i] <= 0x1f)
      {
        mArchiveFileName[i] = ' ';
      }
    }
  #endif

  if (ExtraFieldSize)
  {
    MAssert(!mExtraField.Size(), "");
    mExtraField.SetSize(ExtraFieldSize);
    pStorage->Read(mExtraField.FirstWrite(), ExtraFieldSize);
  }

  if (CommentSize)
  {
    mComment.SetSize(CommentSize);
    pStorage->Read(mComment.FirstWrite(), CommentSize);
  }

  return true; 
}

// -------------------------------------------------------------------------------------------------

size_t TZipFileHeader::Write(TFile* pStorage)
{
  const TUInt16 FileNameSize = (TUInt16)GetFileNameSize();
  const TUInt16 CommentSize = (short)GetCommentSize();
  const short ExtraFieldSize = (short)GetExtraFieldSize();
  
  pStorage->Write(mSignature, sizeof(mSignature));
  pStorage->Write(mVersionMadeBy);
  pStorage->Write(mVersionNeeded);
  pStorage->Write(mFlags);
  pStorage->Write(mCompressionMethod);
  pStorage->Write(mModTime);
  pStorage->Write(mModDate);
  pStorage->Write(mCrc32);
  pStorage->Write(mCompressedSize);
  pStorage->Write(mUncompressedSize);
  pStorage->Write(FileNameSize);
  pStorage->Write(ExtraFieldSize);
  pStorage->Write(CommentSize);
  pStorage->Write(mDiskStart);
  pStorage->Write(mInternalAttr);
  pStorage->Write(mExternalAttr);
  pStorage->Write(mOffset);
  
  pStorage->Write(mArchiveFileName.FirstRead(), FileNameSize);
  pStorage->Write(mExtraField.FirstRead(), ExtraFieldSize);
   pStorage->Write(mComment.FirstRead(), CommentSize);
 
  return Size();
}

// -------------------------------------------------------------------------------------------------

bool TZipFileHeader::CheckLocal(
  const TZipArchive::THeaderSignature&  Header,
  TFile*                                pStorage, 
  TInt16&                               LocExtrFieldSize)const
{
  TInt8 Signature[4];
  pStorage->Read(Signature, sizeof(Signature));
  
  if (::memcmp(Signature, Header, 4) != 0)
  {
    return false;
  }
  
  pStorage->Skip(sizeof(mVersionNeeded));
  
  TInt16 Flags;
  pStorage->Read(Flags);

  if (Flags != mFlags)
  {
    return false;
  }
  
  TInt16 CompressionMethod;
  pStorage->Read(CompressionMethod);

  if (CompressionMethod != mCompressionMethod || 
      (mCompressionMethod && mCompressionMethod != Z_DEFLATED))
  {
    return false;
  }
  
  pStorage->Skip(sizeof(mModTime));
  pStorage->Skip(sizeof(mModDate));

  // CRC and sizes are in the data description behind the compressed data
  const bool ExpectNull = (Flags & 8) != 0;
  
  if (!CheckCrcAndSizes(pStorage, ExpectNull))
  {
    return false;
  }

  TInt16 FileNameSize;
  pStorage->Read(FileNameSize);
  pStorage->Read(LocExtrFieldSize);
  
  if (FileNameSize == 0)
  {
    return false;
  }
  
  pStorage->Skip(FileNameSize);

  return true;
}

// -------------------------------------------------------------------------------------------------

void TZipFileHeader::WriteLocal(
  const TZipArchive::THeaderSignature&  Header,
  TFile*                                pStorage)
{
  mDiskStart = 0;
  mOffset = (TUInt32)pStorage->Position();
  
  pStorage->Write(Header, sizeof(Header));
  pStorage->Write(mVersionNeeded);
  pStorage->Write(mFlags);
  pStorage->Write(mCompressionMethod);
  pStorage->Write(mModTime);
  pStorage->Write(mModDate);
  pStorage->Write(mCrc32);
  pStorage->Write(mCompressedSize);
  pStorage->Write(mUncompressedSize);

  const short FileNameSize = (short)GetFileNameSize();
  const short ExtraFieldSize = (short)GetExtraFieldSize();
  
  pStorage->Write(FileNameSize);
  pStorage->Write(ExtraFieldSize);
  pStorage->Write(mArchiveFileName.FirstRead(), FileNameSize);
  pStorage->Write(mExtraField.FirstRead(), ExtraFieldSize);

  // it was only local information, use TZipArchive::SetExtraField to 
  // set the file extra field in the central directory
  mExtraField.Empty();
}

// -------------------------------------------------------------------------------------------------

bool TZipFileHeader::PrepareData(
  int CompressionLevel, 
  bool ExtraHeader, 
  bool Encrypted)
{
  MStaticAssert(sizeof(sGZSignature) == sizeof(mSignature));
  TMemory::Copy(mSignature, sGZSignature, sizeof(mSignature));
  
  mInternalAttr = 0;
  mExternalAttr = MWinArchiveAttribute;
  
  mVersionMadeBy = MRequiredZipVersion;
  mVersionNeeded = MRequiredZipVersion;

  mCrc32 = 0;
  mCompressedSize = 0;
  mUncompressedSize = 0;
  
  if (CompressionLevel == 0)
  {
    mCompressionMethod = 0;
  }

  if ((mCompressionMethod != Z_DEFLATED) && (mCompressionMethod != 0))
  {
    mCompressionMethod = Z_DEFLATED;
  }

  const bool UseUtf8 = HasUtf8FileNameAndComment();

  mFlags = 0;

  if (mCompressionMethod == Z_DEFLATED)
  {
    switch (CompressionLevel)
    {
    case 1:
      mFlags |= 0x6;
      break;

    case 2:
      mFlags |= 0x4;
      break;
    
    case 8:
    case 9:
      mFlags |= 0x2;
      break;
    }
  }

  if (UseUtf8)
  {
    mFlags |= 0x800; // use utf8 for filenames and comments
  }

  if (ExtraHeader)
  {
    mFlags |= 0x8; // data descriptor present
  }

  if (Encrypted)
  {
    mCompressedSize = MEncryptedHeaderLen;  // encrypted header
    mFlags |= 0x9;    // encrypted file
  }

  return !(mComment.Size() > (int)std::numeric_limits<TUInt16>::max() ||
    mArchiveFileName.Size() > (int)std::numeric_limits<TUInt16>::max() ||
    mExtraField.Size() > (int)std::numeric_limits<TUInt16>::max());
}

// -------------------------------------------------------------------------------------------------

bool TZipFileHeader::CheckCrcAndSizes(TFile* pStorage, bool ExpectNull)const
{
  TUInt32 Crc32, CompressedSize, UncompressedSize;
  pStorage->Read(Crc32);
  pStorage->Read(CompressedSize);
  pStorage->Read(UncompressedSize);
  
  if (ExpectNull)
  {
    return 
      Crc32 == 0 && 
      CompressedSize == 0 && 
      UncompressedSize == 0;
  }
  else
  {
    return 
      Crc32 == mCrc32 && 
      CompressedSize == mCompressedSize && 
      UncompressedSize == mUncompressedSize;
  }
}

// -------------------------------------------------------------------------------------------------

void TZipFileHeader::WriteCrcAndSizes(TFile* pStorage)
{
  pStorage->Write(mCrc32);
  pStorage->Write(mCompressedSize);
  pStorage->Write(mUncompressedSize);
}

// -------------------------------------------------------------------------------------------------

void TZipFileHeader::ChangeSlash()
{
  for (int i = 0; i < mArchiveFileName.Size(); i++)
  {
    if (mArchiveFileName[i] == '\\')
    {
      mArchiveFileName[i] = '/';
    }
  }
}

