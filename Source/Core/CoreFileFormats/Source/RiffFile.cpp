#include "CoreFileFormatsPrecompiledHeader.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TUInt32 gFourCC(const TString &ChunkName)
{
  // four spaces (padding)
  int retbuf = 0x20202020;   
  char* p = ((char*)&retbuf);

  // Remember, this is Intel format!
  // The first character goes in the LSB
  for (int i = 0; i < 4 && ChunkName[i]; ++i)
  {
    *p++ = (char)ChunkName[i];
  }

  return retbuf;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TRiffChunkHeader::Read(TFile* pFile)
{
  // avoid that the ID gets swapped
  pFile->Read((char*)&mChunckId, 4);
  pFile->Read(mChunkSize);
}

// -------------------------------------------------------------------------------------------------

void TRiffChunkHeader::Write(TFile* pFile)
{
  // avoid that the ID gets swapped
  pFile->Write((char*)&mChunckId, 4);
  pFile->Write(mChunkSize);     
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TRiffChunkHeaderInfo::TRiffChunkHeaderInfo()
{
  mOffset = 0;
  mHeader.mChunkSize = 0;
}

// -------------------------------------------------------------------------------------------------

TString TRiffChunkHeaderInfo::Name()const
{
  TUnicodeChar Temp[5];

  Temp[0] = ((char*)&mHeader.mChunckId)[0];
  Temp[1] = ((char*)&mHeader.mChunckId)[1];
  Temp[2] = ((char*)&mHeader.mChunckId)[2];
  Temp[3] = ((char*)&mHeader.mChunckId)[3];
  Temp[4] = 0;

  return TString(Temp);
}

// -------------------------------------------------------------------------------------------------

TUInt32 TRiffChunkHeaderInfo::Size()const
{
  return mHeader.mChunkSize;
}

// -------------------------------------------------------------------------------------------------

TUInt32 TRiffChunkHeaderInfo::Offset()const
{
  return mOffset;
}

// -------------------------------------------------------------------------------------------------

void TRiffChunkHeaderInfo::Read(TFile* pFile)
{
  mHeader.Read(pFile);
  mOffset = (TUInt32)pFile->Position();
}

// -------------------------------------------------------------------------------------------------

void TRiffChunkHeaderInfo::Write(TFile* pFile)
{
  mHeader.Write(pFile);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TRiffFile::TRiffFile(TUInt32 HeaderChunkID)
{
  mRiffHeader.mChunckId = HeaderChunkID;
  mRiffHeader.mChunkSize = 0;
}

// -------------------------------------------------------------------------------------------------

TRiffFile::~TRiffFile()
{
  MAssert(!mFile.IsOpen() || mFile.AccessMode() != TFile::kWrite, 
    "Riff file was not closed!");
}

// -------------------------------------------------------------------------------------------------

TFile& TRiffFile::Open(
  const TString&          Filename, 
  TFile::TAccessMode      NewMode,
  TByteOrder::TByteOrder  ByteOrder)
{
  mFile.SetFileName(Filename);
  mFile.SetByteOrder(ByteOrder);
  
  // clear any previously loaded chunks
  mChunks.Empty();

  if (! mFile.Open(NewMode))
  {
    throw TReadableException(MText("Failed to open the file '%s'.", Filename));
  }

  switch (NewMode)
  {
  default:
    MInvalid("Invalid Mode");
    break;
    
  case TFile::kWrite:
    // Write the RIFF header...
    // We will have to come back later and patch it to 
    // save the real chunk size !
    mRiffHeader.Write(&mFile);
    break;
    
  case TFile::kRead:
    // Try to read the RIFF header...
    mRiffHeader.Read(&mFile);
    break;
  }

  return mFile;
}

// -------------------------------------------------------------------------------------------------

void TRiffFile::CloseFile()
{
  if (mFile.IsOpen() && mFile.AccessMode() == TFile::kWrite)
  {
    MAssert(mFile.SizeInBytes() % 2 == 0, "RIFF files must be word aligned!");
  
    // update the full "main" chunksize
    mFile.SetPosition(sizeof(TUInt32));
    mFile.Write((TUInt32)(mFile.SizeInBytes() - 8));
  }

  if (mFile.IsOpen())
  {
    mFile.Close();
  }
}

// -------------------------------------------------------------------------------------------------

void TRiffFile::ReadChunks()
{
  try
  {
    const size_t FileSizeInBytes = mFile.SizeInBytes();

    // clear all previously parsed chunks - if any
    mChunks.Empty();

    // skip the main chunk header
    mFile.SetPosition(sizeof(TRiffChunkHeader));

    for (MEver)
    {
      TPtr<TRiffChunkHeaderInfo> pHeaderInfo(new TRiffChunkHeaderInfo);
      pHeaderInfo->Read(&mFile);
      mChunks.Append(pHeaderInfo);
      
      // seek to the next chunk
      if (OnIsParentChunk(pHeaderInfo->Name()))
      {
        // parent chunks have only a name (no size and no data)
        mFile.SetPosition(pHeaderInfo->Offset() - sizeof(TUInt32));
      }
      else
      {
        // & 1 padding: Riff chunks are word aligned, but the chunk may not
        const size_t NextChunkStart = pHeaderInfo->Offset() + 
          pHeaderInfo->Size() + (pHeaderInfo->Size() & 1);

        if (NextChunkStart > mFile.Position() && 
            NextChunkStart + sizeof(TRiffChunkHeader) < FileSizeInBytes)
        {
          // seek to the next header
          if (! mFile.SetPosition(NextChunkStart)) 
          {
            // seeking failed
            break;
          }
        }
        else
        {
          // reached the end or an invalid position
          break;
        }
      }
    }
  }
  catch (const TReadableException&)
  {
    // silently ignore file errors here
  }
}

// -------------------------------------------------------------------------------------------------

TRiffChunkHeaderInfo* TRiffFile::FindChunkByName(const TString &FourCCName)const
{
  for (int i = 0; i < mChunks.Size(); ++i)
  {
    if (mChunks[i]->Name() == FourCCName)
    {
      return mChunks[i];
    }
  }

  return NULL;
}

