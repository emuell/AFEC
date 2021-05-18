#pragma once

#ifndef __DDC_RIFF_H
#define __DDC_RIFF_H

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Pointer.h"
#include "CoreTypes/Export/File.h"

#include "CoreFileFormats/Export/AudioFile.h"

class TRiffFile;

// =================================================================================================

TUInt32 gFourCC(const TString &ChunkName);

// =================================================================================================

struct TRiffChunkHeader
{
  //! @throw TReadableException
  void Read(TFile* pFile);
  //! @throw TReadableException
  void Write(TFile* pFile);

  TUInt32 mChunckId;
  TUInt32 mChunkSize;     
};

// =================================================================================================

class TRiffChunkHeaderInfo : public TReferenceCountable
{
public:
  TRiffChunkHeaderInfo();

  //! @throw TReadableException
  void Read(TFile* pFile);
  //! @throw TReadableException
  void Write(TFile* pFile);

  TString Name()const;
  TUInt32 Size()const;
  TUInt32 Offset()const;

private:
  TRiffChunkHeader mHeader;
  TUInt32 mOffset;
};

// =================================================================================================

class TRiffFile
{
public:
   virtual ~TRiffFile();
   
protected:
  TRiffFile(TUInt32 MainChunkID);
   
  const TFile& File()const { return mFile; }
  TFile& File() { return mFile; }
  
  TRiffChunkHeader Header()const {return mRiffHeader;}

  TRiffChunkHeaderInfo* FindChunkByName(const TString &FourCCName)const;

  //! @throw TReadableException
  TFile& Open(
    const TString&          FileName, 
    TFile::TAccessMode      ReadOrWrite, 
    TByteOrder::TByteOrder  ByteOrder);

  void CloseFile();

  void ReadChunks();
  
  virtual bool OnIsParentChunk(const TString &FourccName) = 0;

private:
  // header for whole file
  TRiffChunkHeader mRiffHeader;    
  TList< TPtr<TRiffChunkHeaderInfo> > mChunks;

  TFile mFile;
};

#endif 

