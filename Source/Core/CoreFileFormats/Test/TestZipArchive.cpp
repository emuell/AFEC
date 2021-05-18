#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"
#include "CoreTypes/Export/Directory.h"
#include "CoreTypes/Export/System.h"

#include "CoreFileFormats/Test/TestZipArchive.h"
                  
// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreFileFormatsTest::ZipArchive()
{
  const TDirectory TempDir = gTempDir(); 

  const TDirectory SrcDir = gTempDir(); 
  const TDirectory ResultDir = gTempDir().Path() + "Result"; 
  
  const char ArchiveComment[] = "Best Archive ever made!";

  const char TestString1[] = "123456789werwerwerqweqwewenbv3uerzvbeurzvqweqweqwe";
  const char TestString2[] = "sdfwererte erterbervtertvert";
  const char TestString3[] = "This is TestString3";
  
  const char TestComment[] = "This Entry Rocks.";
  
  const wchar_t UniCharFileName[] = L"UnicodeFile\u00c4\u00Dc\u3487\u1234\u2345.txt";
  
  TArray<char> TestExtraField;
  TString("An Extra Filed. )(").CreateCStringArray(TestExtraField);

  TFile SrcFile1(SrcDir.Path() + "File1.txt");
  SrcFile1.Open(TFile::kWrite);
  SrcFile1.Write(TestString1, sizeof(TestString1));
  SrcFile1.Close();

  TFile SrcFile2(SrcDir.Path() + "File2.txt");
  SrcFile2.Open(TFile::kWrite);
  SrcFile2.Write(TestString2, sizeof(TestString2));
  SrcFile2.Close();

  TFile SrcFile3(SrcDir.Path() + "File3.txt");
  SrcFile3.Open(TFile::kWrite);
  SrcFile3.Write(TestString3, sizeof(TestString3));
  SrcFile3.Close();
  
  TFile SrcFile4(SrcDir.Path() + TString(UniCharFileName));
  SrcFile4.Open(TFile::kWrite);
  SrcFile4.Write(TestString3, sizeof(TestString3));
  SrcFile4.Close();
  
  TZipArchive Archive(TempDir.Path() + "TestArchive.zip");
  BOOST_CHECK(Archive.Open(TFile::kWrite));
  Archive.SetArchiveComment(ArchiveComment);
  BOOST_CHECK(Archive.AddNewFile(SrcFile1.FileName(), "File1.txt"));
  BOOST_CHECK(Archive.AddNewFile(SrcFile2.FileName()));
  BOOST_CHECK(Archive.AddNewFile(SrcFile3.FileName(), "TestFolder/File3.txt", 9));
  Archive.SetEntryComment(TestComment);
  BOOST_CHECK(Archive.AddNewFile(SrcFile4.FileName(), TString(UniCharFileName)));
  Archive.SetEntryExtraField(TestExtraField, 0);
  Archive.Close();

  // ... test entry properties

  TDirectory(ResultDir).DeleteAllFiles();

  TZipArchive Archive2(TempDir.Path() + "TestArchive.zip");
  Archive2.Open(TFile::kRead);

  Archive2.TestArchive();
    
  BOOST_CHECK_EQUAL(Archive2.Entry(0).mArchiveLocalName, "File1.txt");
  BOOST_CHECK_EQUAL(Archive2.Entry(0).mExtraField, TestExtraField);

  BOOST_CHECK_EQUAL(Archive2.Entry(1).mArchiveLocalName, "File2.txt");

  BOOST_CHECK_EQUAL(Archive2.Entry(2).mArchiveLocalName, "TestFolder" + 
    TDirectory::SPathSeparator() + "File3.txt");
  
  BOOST_CHECK_EQUAL(Archive2.Entry(2).mComment, TestComment);

  BOOST_CHECK_EQUAL(Archive2.Entry(3).mArchiveLocalName, TString(UniCharFileName));
  
  Archive2.Close();
  

  // ... test extracting a single file

  TZipArchive Archive3(TempDir.Path() + "TestArchive.zip");
  Archive3.Open(TFile::kRead);
  BOOST_CHECK_EQUAL(Archive3.ArchiveComment(), ArchiveComment);
  BOOST_CHECK_EQUAL(Archive3.NumberOfEntries(), 4);
  BOOST_CHECK(Archive3.ExtractEntry(1, ResultDir));
  Archive3.Close();

  BOOST_CHECK_FILES_EQUAL(
    TString(SrcDir.Path() + "File2.txt"), 
    TString(ResultDir.Path() + "File2.txt"));

  // ... test extracting the whole archive

  TDirectory(ResultDir).DeleteAllFiles();

  TZipArchive Archive4(TempDir.Path() + "TestArchive.zip");
  Archive4.Open(TFile::kRead);
  Archive4.Extract(ResultDir);
  Archive4.Close();
  
  BOOST_CHECK_FILES_EQUAL(
    TString(SrcDir.Path() + "File1.txt"), 
    TString(ResultDir.Path() + "File1.txt"));

  BOOST_CHECK_FILES_EQUAL(
    TString(SrcDir.Path() + "File2.txt"), 
    TString(ResultDir.Path() + "File2.txt"));

  BOOST_CHECK_FILES_EQUAL(
    TString(SrcDir.Path() + "File3.txt"), 
    TString(ResultDir.Path() + "TestFolder/File3.txt"));
  
  BOOST_CHECK_FILES_EQUAL(
    TString(SrcDir.Path() + TString(UniCharFileName)), 
    TString(ResultDir.Path() + TString(UniCharFileName)));
  
    
  // ... test with the systems unzip tool

  #if defined(MMac)
    const bool WaitTillFinished = true;
    
    BOOST_CHECK(TSystem::LaunchProcess(
      "/usr/bin/unzip", 
      MakeList<TString>("-t", TempDir.Path() + "TestArchive.zip"), 
      WaitTillFinished) == EXIT_SUCCESS);
  #endif  
}

