#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"
#include "CoreTypes/Export/Directory.h"
#include "CoreFileFormats/Test/TestZipFile.h"
                  
// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreFileFormatsTest::ZipFile()
{
  const char TestString[] =
    "123456789werwerwerqweqwewenbv3uerzvbeurzvqweqweqwe";

  const int CompressionLevel = 1 + TRandom::Integer(8);

  // gzip array
  TArray<TInt8> SrcArray(sizeof(TestString));
  TMemory::Copy(SrcArray.FirstWrite(), TestString, SrcArray.Size());

  TArray<TInt8> DestArray;
  TZipFile::SGZipData(SrcArray, DestArray, CompressionLevel);
  
  BOOST_CHECK(TZipFile::SIsGZipData(DestArray.FirstRead()));
  
  TZipFile::SUnGZipData(DestArray, SrcArray);
  
  BOOST_CHECK_EQUAL(TString(TestString), TString(SrcArray.FirstRead()));


  // gzip file
  const TDirectory TempDir = gTempDir();
  
  TFile SrcFile(TempDir.Path() + "GZTipTest.txt");
  SrcFile.Open(TFile::kWrite);
  
  SrcFile.Write(TestString, sizeof(TestString));
  SrcFile.Close();
  
  TFile DstFile(TempDir.Path() + "GZTipTest.txt.gz");
  
  TZipFile::SGZipFile(SrcFile.FileName(),
    DstFile.FileName(), CompressionLevel);
    
  SrcFile.Unlink();
  
  BOOST_CHECK(TZipFile::SIsGZipFile(DstFile.FileName()));

  TZipFile::SUnGZipFile(DstFile.FileName(),
    SrcFile.FileName());
                                            
  SrcFile.Open(TFile::kRead);
  
  char TempBuffer[sizeof(TestString)];
  SrcFile.Read(TempBuffer, sizeof(TestString));
  SrcFile.Close();
  
  BOOST_CHECK_EQUAL(TString(TempBuffer), TString(TestString));
}

