#pragma once

#ifndef ZipFile_h
#define ZipFile_h

// =================================================================================================

#include "CoreTypes/Export/Array.h"

// =================================================================================================

class TString;

// =================================================================================================

namespace TZipFile
{
  //! Returns true if the passed data looks like gzip'ed data.
  //! This only checks if the first two bytes match the gzip magic (0x1F, 0x8B).
  bool SIsGZipData(const void* pData);

  //! gzip the given raw array content in memory.
  //! \param compression level can be [1 - 9], where 9 is the best, 1 is poor
  //! \throws TReadableException
  void SGZipData(
    const TArray<TInt8>&  UncompressedData,
    TArray<TInt8>&        CompressedData,
    int                   CompressionLevel);

  //! GUnzip the zipped src array buffer in memory.
  //! \throws TReadableException
  void SUnGZipData(
    const TArray<TInt8> CompressedData,
    TArray<TInt8>&      UncompressedData);


  //! Returns true if the passed file looks like a gzip compressed file. 
  //! See also \function SIsGZipData;
  bool SIsGZipFile(const TString& FileNameAndPath);
  
  //! gzip the file at SrcFileName and path, writing the resulting gzipped file into 
  //! the given DestFileName and Path.
  //! The SrcFile will not be deleted, the DestFile will be overwritten if it already 
  //! exists. The DestFileName has to include the wanted extension: We will not add 
  //! ".gz" to the given file.
  //! \param compression level can be [1 - 9], where 9 is the best, 1 is poor
  //! \throws TReadableException
  void SGZipFile(
    const TString&    SrcFileNameAndPath, 
    const TString&    DstFileNameAndPath, 
    int               CompressionLevel);
  
  //! GUnzip the zipped src file to the given dest file. 
  //! The destfile will be overwritten if it already exists. 
  //! \throws TReadableException
  void SUnGZipFile(
    const TString&    SrcFileNameAndPath, 
    const TString&    DstFileNameAndPath);
}


#endif 

