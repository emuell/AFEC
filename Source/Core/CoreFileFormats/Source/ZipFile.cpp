#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/System.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Directory.h"

#include <cstdio>
#include <string>

#if defined(MLinux) || defined(MMac)
  #include <unistd.h>  // for unlink
#endif

#include "../../3rdParty/ZLib/Export/ZLib.h"

// =================================================================================================

#define MGZipBufferSize 4096

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool TZipFile::SIsGZipData(const void* pData)
{
  return ((unsigned char*)pData)[0] == 0x1F && ((unsigned char*)pData)[1] == 0x8B;
}

// -------------------------------------------------------------------------------------------------

void TZipFile::SGZipData(
  const TArray<TInt8>&  UncompressedData,
  TArray<TInt8>&        CompressedData,
  int                   CompressionLevel)
{
  MAssert(CompressionLevel >= 1 && CompressionLevel <= 9, "Invalid compression level");

  z_stream zs; // z_stream is zlib's control structure
  TMemory::Zero(&zs, sizeof(zs));

  const int MemoryLevel = 8;
  const int WindowBits = MAX_WBITS + 16; // + 16 to force writing gzip headers

  if (deflateInit2(&zs, CompressionLevel,  
        Z_DEFLATED, WindowBits, MemoryLevel, Z_DEFAULT_STRATEGY) != Z_OK)
  {
    throw TReadableException(
      MText("Failed to compress data (gzip deflateInit failed)"));
  }

  // set the z_stream's input
  zs.next_in = (Bytef*)UncompressedData.FirstRead();
  zs.avail_in = (uInt)UncompressedData.Size();

  MStaticAssert(sizeof(char) == sizeof(TInt8));
  char TempBuffer[MGZipBufferSize];
  std::string TempOutputStream;

  // retrieve the compressed bytes blockwise
  int ret;
  do
  {
    zs.next_out = reinterpret_cast<Bytef*>(TempBuffer);
    zs.avail_out = sizeof(TempBuffer);

    ret = deflate(&zs, Z_FINISH);

    // append the block to the output stream
    if (zs.total_out > TempOutputStream.size())
    {
      TempOutputStream.append(TempBuffer, zs.total_out - TempOutputStream.size());
    }
  }
  while (ret == Z_OK);

  deflateEnd(&zs);

  if (ret != Z_STREAM_END)
  {
    // an error occurred that was not EOF
    throw TReadableException(
      MText("Exception during zlib compression: (%s)", TString(zs.msg)));
  }

  CompressedData.SetSize((int)TempOutputStream.size());
  TMemory::Copy(CompressedData.FirstWrite(), 
    TempOutputStream.data(), TempOutputStream.size());
}

// -------------------------------------------------------------------------------------------------

void TZipFile::SUnGZipData(
  const TArray<TInt8> CompressedData,
  TArray<TInt8>&      UncompressedData)
{
  z_stream zs; // z_stream is zlib's control structure
  TMemory::Zero(&zs, sizeof(zs));

  const int WindowBits = MAX_WBITS + 16; // + 16 to force looking for gzip headers

  if (inflateInit2(&zs, WindowBits) != Z_OK)
  {
    throw TReadableException(
      MText("Failed to uncompress data (gzip inflateInit failed)"));
  }
  
  zs.next_in = (Bytef*)CompressedData.FirstRead();
  zs.avail_in = (uInt)CompressedData.Size();

  MStaticAssert(sizeof(char) == sizeof(TInt8));
  char TempBuffer[MGZipBufferSize];
  std::string TempOutputStream;

  // get the decompressed bytes blockwise using repeated calls to inflate
  int ret;
  do
  {
    zs.next_out = reinterpret_cast<Bytef*>(TempBuffer);
    zs.avail_out = sizeof(TempBuffer);

    ret = inflate(&zs, 0);
    if (zs.total_out > TempOutputStream.size())
    {
      TempOutputStream.append(TempBuffer, zs.total_out - TempOutputStream.size());
    }
  }
  while (ret == Z_OK);

  inflateEnd(&zs);

  if (ret != Z_STREAM_END)
  { 
    // an error occurred that was not EOF
    throw TReadableException(
      MText("Exception during zlib decompression: (%s)", TString(zs.msg)));
  }

  UncompressedData.SetSize((int)TempOutputStream.size());
  TMemory::Copy(UncompressedData.FirstWrite(), 
    TempOutputStream.data(), TempOutputStream.size());
}

// -------------------------------------------------------------------------------------------------

bool TZipFile::SIsGZipFile(const TString& FileNameAndPath)
{
  TFile File(FileNameAndPath);

  if (File.Open(TFile::kRead))
  {
    char Buffer[2];
    
    if (File.ReadByte(Buffer[0]) && File.ReadByte(Buffer[1]))
    {
      return (unsigned char)Buffer[0] == 0x1F && (unsigned char)Buffer[1] == 0x8B;
    }
    else
    {
      return false;
    }
  }
  else
  {
    MInvalid("File should exist!");
    return false;
  }
}

// -------------------------------------------------------------------------------------------------

void TZipFile::SGZipFile(
  const TString&    SrcFileNameAndPath, 
  const TString&    DstFileNameAndPath,
  int               CompressionLevel)
{
  MAssert(CompressionLevel >= 1 && CompressionLevel <= 9, "Invalid compression level");
  
  const std::string CSrcFileName = SrcFileNameAndPath.StdCString(TString::kFileSystemEncoding);
  const std::string CDstFileName = DstFileNameAndPath.StdCString(TString::kFileSystemEncoding);
  
  FILE* pInFile = ::fopen(CSrcFileName.c_str(), "rb");
  
  const char Mode[] = { 'w', 'b', (char)('0' + CompressionLevel), ' ', '\0' };
  gzFile pOutFile = gzopen(CDstFileName.c_str(), Mode);
     
  try
  {
    if (pInFile == NULL) 
    {
      throw TReadableException(
        MText("Failed to compress '%s'!", SrcFileNameAndPath));
    }
    
    if (pOutFile == NULL) 
    {
      throw TReadableException(
        MText("Failed to compress '%s'!", SrcFileNameAndPath));
    }
    
    for (;;)
    {
      char Buffer[MGZipBufferSize];
      const int BytesRead = (int)::fread(Buffer, 1, sizeof(Buffer), pInFile);
      
      if (BytesRead <= 0)
      {
        break;
      }

      if (gzwrite(pOutFile, Buffer, (unsigned)BytesRead) != BytesRead) 
      {
        throw TReadableException(
          MText("Failed to compress '%s'!", SrcFileNameAndPath));
      }
    }
    
    ::fclose(pInFile);
    
    if (gzclose(pOutFile) != Z_OK)
    {
      MInvalid("gzclose failed");
    }
  }
  catch (const TReadableException&)
  {
    if (pInFile)
    {
      ::fclose(pInFile);      
    }
    
    if (pOutFile)
    {
      gzclose(pOutFile);
      const int Ret = ::unlink(CSrcFileName.c_str());
      MAssert(Ret == 0, ""); MUnused(Ret);
    }
    
    throw;
  }
}

// -------------------------------------------------------------------------------------------------

void TZipFile::SUnGZipFile(
  const TString&    SrcFileNameAndPath, 
  const TString&    DstFileNameAndPath)
{
  const std::string CSrcFileName = SrcFileNameAndPath.StdCString(TString::kFileSystemEncoding);
  const std::string CDstFileName = DstFileNameAndPath.StdCString(TString::kFileSystemEncoding);
  
  gzFile pInFile = gzopen(CSrcFileName.c_str(), "rb");
  FILE* pOutFile = fopen(CDstFileName.c_str(), "wb");
  
  try
  {
    if (pInFile == NULL) 
    {
      throw TReadableException(MText(
        "Failed to uncompress '%s'!", SrcFileNameAndPath));
    }

    if (pOutFile == NULL) 
    {
      throw TReadableException(MText(
        "Failed to uncompress '%s'!", SrcFileNameAndPath));
    }
    
    for (;;)
    {
      char Buffer[MGZipBufferSize];
      const int BytesRead = gzread(pInFile, Buffer, sizeof(Buffer));
      
      if (BytesRead <= 0)
      {
        break;
      }

      if (fwrite(Buffer, 1, BytesRead, pOutFile) != (unsigned)BytesRead) 
      {
        throw TReadableException(MText(
          "Failed to uncompress '%s'!", SrcFileNameAndPath));
      }
    }
    
    if (gzclose(pInFile) != Z_OK) 
    {
      MInvalid("gzclose failed");
    }
    
    ::fclose(pOutFile);
  }
  catch (const TReadableException&)
  {
    if (pInFile)
    {
      gzclose(pInFile);
    }
    
    if (pOutFile)
    {
      ::fclose(pOutFile);
      const int Ret = ::unlink(CSrcFileName.c_str());
      MAssert(Ret == 0, ""); MUnused(Ret);
    }
    
    throw;
  }
}

