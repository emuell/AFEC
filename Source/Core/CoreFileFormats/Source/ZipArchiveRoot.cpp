#include "CoreFileFormatsPrecompiledHeader.h"

#include <cstring>

// =================================================================================================

#define MZipZentralDirSize  22
#define MZipTempBufferSize 32768

// =================================================================================================

const char TZipArchiveRoot::sGZSignature[4] = {0x50, 0x4b, 0x05, 0x06};

// -------------------------------------------------------------------------------------------------

TZipArchiveRoot::TZipArchiveRoot()
{
  mpFile = NULL;
  mpOpenedFile = NULL;
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::Init()
{
  mOnDisk = false;
  mBytesBeforeZip = mCentrDirPos = 0;
  mpOpenedFile = NULL;
  mComment.Empty();
}

// -------------------------------------------------------------------------------------------------

TZipArchiveRoot::~TZipArchiveRoot()
{
  Clear();
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::Read()
{
  MAssert(mpFile, "");
  
  mCentrDirPos = Locate();
  mpFile->SetPosition(mCentrDirPos);
  
  mpFile->Read(mSignature, sizeof(mSignature));
  mpFile->Read(mThisDiskNumber);
  mpFile->Read(mDiskWithCD);
  mpFile->Read(mNumberOfDiskEntries);
  mpFile->Read(mNumberOfEntries);
  mpFile->Read(mSize);
  mpFile->Read(mOffset);
  
  TUInt16 CommentSize;
  mpFile->Read(CommentSize);

  // if mThisDiskNumber is not zero, it is enough to say that it is multi archive
  MAssert((!mThisDiskNumber && (mNumberOfEntries == mNumberOfDiskEntries) && 
    !mDiskWithCD) || mThisDiskNumber, "");

  if (CommentSize)
  {
    mComment.SetSize(CommentSize);
    mpFile->Read(mComment.FirstWrite(), CommentSize);
  }
  
  mBytesBeforeZip = mCentrDirPos - mSize - mOffset;

  if ((! mSize && mNumberOfEntries) || (! mNumberOfEntries && mSize))
  {
    throw TReadableException(MText("Invalid Zip file!"));
  }

  mOnDisk = true;
  
  if (!mSize)
  {
    return;
  }

  ReadHeaders();
}

// -------------------------------------------------------------------------------------------------

size_t TZipArchiveRoot::Locate()
{
  // maximum size of end of central dir record
  size_t MaxRecordSize = 0xffff + MZipZentralDirSize;
  const size_t FileSize = mpFile->SizeInBytes();

  if (MaxRecordSize > FileSize)
  {
    MaxRecordSize = FileSize;
  }

  TArray<char> TempBuffer(MZipTempBufferSize);

  size_t PosInFile = 0;
  size_t Read = 0;

  // backward reading
  while (PosInFile < MaxRecordSize)
  {
    PosInFile = Read + MZipTempBufferSize;
  
    if (PosInFile > MaxRecordSize)
    {
      PosInFile = MaxRecordSize;
    }

    const size_t ToRead = PosInFile - Read;

    if (!mpFile->SetPosition(FileSize - PosInFile))
    {
      throw TReadableException(MText("Error while seeking the Zip file!"));
    }

    mpFile->Read(TempBuffer.FirstWrite(), ToRead);
    
    // search from the very last bytes to prevent an error if inside archive 
    // there are packed other archives
    for (int i = (int)ToRead - 4; i >=0 ; i--)
    {
      if (! memcmp(TempBuffer.FirstRead() + i, sGZSignature, 4))  
      {
        return FileSize - (PosInFile - i);
      }
    }

    Read += ToRead - 3;
  }
  
  throw TReadableException(MText("Zip file location not found!"));
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::ReadHeaders()
{
  if (! mpFile->SetPosition(mOffset + mBytesBeforeZip))
  {
    throw TReadableException(MText("Error while seeking the Zip file!"));
  }
  
  RemoveHeaders();

  for (int i = 0; i < mNumberOfEntries; i++)
  {
    TZipFileHeader* pHeader = new TZipFileHeader;
    mHeaders.Append(pHeader);
    
    if (! pHeader->Read(mpFile))
    {
      throw TReadableException(MText("Invalid Zip file!"));
    }

    const bool FromZip = true;
    ConvertFileName(FromZip, pHeader);
  }
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::Clear(bool Everything)
{
  mpOpenedFile = NULL;
  mLocalExtraField.Empty();

  if (Everything)
  {
    RemoveHeaders();
    mComment.Empty();
  }
}

// -------------------------------------------------------------------------------------------------

bool TZipArchiveRoot::IsValidIndex(int Index)const
{
  return Index >= 0 && Index < mHeaders.Size();
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::OpenFile(
  const TZipArchive::THeaderSignature&  HeaderSignature, 
  int                                   Index)
{
  mpOpenedFile = mHeaders[Index];

  if (! mpFile->SetPosition(mpOpenedFile->mOffset + mBytesBeforeZip))  
  {
    throw TReadableException(MText("Error while seeking the Zip file!"));
  }

  TInt16 LocalExtraFieldSize = 0;
  
  if (! mpOpenedFile->CheckLocal(HeaderSignature, mpFile, LocalExtraFieldSize))
  {
    throw TReadableException(MText("Invalid Zip file!"));
  }

  if (LocalExtraFieldSize)
  {
    mLocalExtraField.SetSize(LocalExtraFieldSize);
    mpFile->Read(mLocalExtraField.FirstWrite(), LocalExtraFieldSize);
  }
  else
  {
    mLocalExtraField.Empty();
  }
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::CloseFile()
{
  if (!mpOpenedFile)
  {
    return;
  }
  
  mLocalExtraField.Empty();
  
  if (mpOpenedFile->HasDataDescr())
  {
    // in span mode, files that are divided between disks have bit 3 of flag set
    // which tell about the presence of the data descriptor after the compressed data
    // This signature may be in the disk spanning archive that is one volume only
    // (it is detected as a non disk spanning archive)
    static const char ExtHeaderSignat[] = {0x50, 0x4b, 0x07, 0x08};

    TInt8 ExtHeader[4];
    mpFile->Read(ExtHeader, sizeof(ExtHeader));

    if (memcmp(ExtHeader, ExtHeaderSignat, 4) != 0) // there is no signature
    {
      if (!mpFile->SetPosition(mpFile->Position() - 4))
      {
        throw TReadableException(MText("Error while seeking the Zip file!"));
      }
    }
    
    const bool ExpectNull = false;
    if (!mpOpenedFile->CheckCrcAndSizes(mpFile, ExpectNull))
    {
      throw TReadableException(MText("CRC error in Zip file!"));
    }
  }

  mpOpenedFile = NULL;
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::AddNewFile(TZipFileHeader& header)
{
  TZipFileHeader* pHeader = new TZipFileHeader(header);
  mpOpenedFile = pHeader;
  mHeaders.Append(pHeader);
  
  RemoveFromDisk();
  
  if (!mpFile->SetPosition(mpFile->SizeInBytes()))
  {
    throw TReadableException(MText("Error while seeking the Zip file!"));
  }
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::RemoveFromDisk()
{
  if (mOnDisk)
  {
    //mpFile->SetLength(mBytesBeforeZip + mOffset); FIXME
    mOnDisk = false;
  }
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::CloseNewFile()
{
  if (!mpFile->SetPosition(mpOpenedFile->mOffset + 14))
  {
    throw TReadableException(MText("Error while seeking the Zip file!"));
  }
  
  mpOpenedFile->WriteCrcAndSizes(mpFile);
  
  // offset set during Writing the local header
  mpOpenedFile->mOffset = (TUInt32)(mpOpenedFile->mOffset - mBytesBeforeZip);
  
  mpFile->Flush();
  mpOpenedFile = NULL;
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::Write()
{
  if (mOnDisk)
  {
    return;
  }
  
  mpFile->Flush();
  mpFile->SetPosition(mpFile->SizeInBytes());

  mNumberOfEntries = (TUInt16)mHeaders.Size();
  mSize = 0;
  
  WriteHeaders();
  mThisDiskNumber = 0;//(int)mpFile->GetCurrentDisk();
  WriteCentralEnd();

  mpFile->Flush();
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::WriteHeaders()
{
  mNumberOfDiskEntries = 0;
  mDiskWithCD = 0;
  mOffset = (TUInt32)(mpFile->Position() - mBytesBeforeZip);

  if (! mNumberOfEntries)
  {
    return;
  }

  for (int i = 0; i < mNumberOfEntries; i++)
  {
    TZipFileHeader* pHeader = mHeaders[i];
    
    const bool FromZip = false;
    ConvertFileName(FromZip, pHeader);
    
    mSize = (TUInt32)(mSize + pHeader->Write(mpFile));

    ++mNumberOfDiskEntries;
  }
}

// -------------------------------------------------------------------------------------------------

size_t TZipArchiveRoot::WriteCentralEnd()
{
  mpFile->Write(sGZSignature, sizeof(sGZSignature));
  mpFile->Write(mThisDiskNumber);
  mpFile->Write(mDiskWithCD);
  mpFile->Write(mNumberOfDiskEntries);
  mpFile->Write(mNumberOfEntries);
  mpFile->Write(mSize);
  mpFile->Write(mOffset);

  TInt16 CommentSize = (TInt16)mComment.Size();
  mpFile->Write(CommentSize);
  mpFile->Write(mComment.FirstRead(), CommentSize);

  return GetSize();
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::RemoveFile(int Index)
{
  TZipFileHeader* pHeader = mHeaders[Index];
  delete pHeader;

  mHeaders.Delete(Index);
}

// -------------------------------------------------------------------------------------------------

unsigned int TZipArchiveRoot::GetSize(bool Whole)const
{
  unsigned int uHeaders = 0;

  if (Whole)
  {
    for (int i = 0; i < mHeaders.Size(); i++)
    {
      uHeaders += (unsigned int)mHeaders[i]->Size();
    }
  }

  return MZipZentralDirSize + mComment.Size() + uHeaders;
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::RemoveHeaders()
{
  for (int i = 0; i < mHeaders.Size(); i++)
  {
    delete mHeaders[i];
    mHeaders[i] = NULL;
  }

  mHeaders.Empty();
}

// -------------------------------------------------------------------------------------------------

void TZipArchiveRoot::ConvertFileName(bool FromZip, TZipFileHeader* pHeader)
{
  if (! pHeader)
  {
    pHeader = mpOpenedFile;
    MAssert(pHeader, "");
  }

  pHeader->ChangeSlash();
}

