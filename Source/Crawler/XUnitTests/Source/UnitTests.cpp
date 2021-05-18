#include "CoreTypes/Export/TestHelpers.h"
#include "CoreTypes/Export/Version.h"
#include "CoreTypes/Export/Directory.h"
#include "CoreTypes/Export/System.h"
#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/Exception.h"

#include "AudioTypes/Export/AudioTypesInit.h"

#include "CoreFileFormats/Export/Database.h"
#include "CoreFileFormats/Export/ZipFile.h"

#include "FeatureExtraction/Test/TestStatistics.h"
#include "FeatureExtraction/Export/SqliteSampleDescriptorPool.h"

#include "Classification/Test/TestShark.h"

#include "CoreFileFormats/Export/CoreFileFormatsInit.h"
#include "FeatureExtraction/Export/FeatureExtractionInit.h"
#include "Classification/Export/ClassificationInit.h"

#include "../../3rdParty/Boost/Export/BoostUnitTest.h"

#include "../../../Msgpack/Export/Msgpack.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include <boost/cstdlib.hpp> // boost::exit_exception_failure and co
#include <iostream> // std::cerr

// =================================================================================================

namespace TProductDescription 
{ 
  TString ProductName() { return "AFEC UnitTests"; }
  TString ProductVendorName() { return "AFEC"; }
  TString ProductProjectsLocation() { return "Crawler/XUnitTests"; }

  int MajorVersion() { return 0; }
  int MinorVersion() { return 1; }
  int RevisionVersion() { return 0; }
  
  TString AlphaOrBetaVersionString() { return ""; }
  TDate ExpirationDate() { return TDate(); }
  
  TString BugReportEMailAddress(){ return "<bug@nowhere.com>"; }
  TString SupportEMailAddress(){ return "<support@nowhere.com>"; }
  TString ProductHomeURL(){ return "http://www.nowhere.com"; }
  
  TString CopyrightString() { return ""; }
}

// =================================================================================================

namespace TCrawlerTests
{
  TString CrawlerExePathAndName();
  TString ModelCreatorExePathAndName();

  void Crawler();
  void ClassificationModel();

  boost::unit_test::test_suite* RegisterUnitTests(int, char*[]);
}

// =================================================================================================

#include "CoreTypes/Export/MainEntry.h"

// -------------------------------------------------------------------------------------------------

int gMain(const TList<TString>& Arguments)
{
  // ... Init

  // don't want to dump any info to the std out when running tests
  const TSystem::TSuppressInfoMessages SupressInfos;

  // don't mix up our output with the regular log output...
  TLog::SLog()->SetTraceLogContents(false);

  M__DisableFloatingPointAssertions

  try
  {
    AudioTypesInit();
    CoreFileFormatsInit();
    FeatureExtractionInit();
    ClassificationInit();
  }
  catch (const TReadableException& Exception)
  {
    std::cerr << Exception.Message().CString();
    return boost::exit_exception_failure;
  }


  // ... Run Tests

  const int ExitCode = boost::unit_test::unit_test_main(
    TCrawlerTests::RegisterUnitTests, gArgc, gArgv);


  // ... Finalize 

  try
  {
    ClassificationExit();
    FeatureExtractionExit();
    CoreFileFormatsExit();
    AudioTypesExit();
  }
  catch (const TReadableException& Exception)
  {
    std::cerr << Exception.Message().CString();
    return boost::exit_exception_failure;
  }

  return ExitCode;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

#if defined(MWindows)
  #define MAppExtension ".exe"
#else
  #define MAppExtension ""
#endif

// -------------------------------------------------------------------------------------------------

TString TCrawlerTests::CrawlerExePathAndName()
{
  return gApplicationDir().Path() + "Crawler" + MAppExtension;
}

// -------------------------------------------------------------------------------------------------

TString TCrawlerTests::ModelCreatorExePathAndName()
{
  return gApplicationDir().Path() + "ModelCreator" + MAppExtension;
}

// -------------------------------------------------------------------------------------------------

void TCrawlerTests::Crawler()
{
  BOOST_TEST_MESSAGE("  Testing Crawler...");

  const bool WaitTilProcessFinished = true;
  int LaunchResult;

  auto VerifyColumnContent = [] (
    TDatabase::TStatement&  DbStatement,
    int                     ColumnIndex) {

    const TString ColumnType = DbStatement.ColumnTypeString(ColumnIndex);
    const TString ColumnName = DbStatement.ColumnName(ColumnIndex);
    BOOST_TEST_CONTEXT("column: " << ColumnName.CString())
    {
      int RawColumnContentSize = 0;
      const void* pRawColumnContent = DbStatement.ColumnBlob(
        ColumnIndex, RawColumnContentSize);

      // content should not be empty for "suceeded" samples 
      BOOST_CHECK(RawColumnContentSize > 0);
      if (RawColumnContentSize > 0)
      {
        TString ColumnContent;

        if (ColumnType == "BLOB")
        {
          // deserialize msgpack blob
          msgpack::object_handle MsgPackObjectHandle = msgpack::unpack(
            (const char*)pRawColumnContent, (size_t)RawColumnContentSize);

          // convert deserialized object to JSON, so we can always test with JSON below
          std::stringstream TempStream;
          TempStream << MsgPackObjectHandle.get();

          ColumnContent = TString(TempStream.str().c_str(), TString::kUtf8);
        }
        else
        {
          ColumnContent = DbStatement.ColumnText(ColumnIndex);
        }

        // create a string stream for boost property_tree
        const char* pConlumnContentCString = ColumnContent.CString(TString::kUtf8);
        boost::iostreams::stream<boost::iostreams::array_source> ColumnContentStream(
          pConlumnContentCString, ::strlen(pConlumnContentCString));

        if (ColumnName.EndsWith("_S"))
        {
          // nothing to check for strings
        }
        else if (ColumnName.EndsWith("_VS"))
        {
          // check if the content is a valid JSON array
          boost::property_tree::ptree Tree;
          BOOST_CHECK_NO_THROW(boost::property_tree::read_json(
            ColumnContentStream, Tree));
        }
        else if (ColumnName.EndsWith("_R"))
        {
          // check if the content is a valid number
          BOOST_CHECK_NO_THROW(std::stod(pConlumnContentCString));
        }
        else if (ColumnName.EndsWith("_VR"))
        {
          // check if the content is a valid JSON array
          boost::property_tree::ptree Tree;
          BOOST_CHECK_NO_THROW(boost::property_tree::read_json(
            ColumnContentStream, Tree));

          // check if the content is an array of numbers
          for (auto ChildIter : Tree)
          {
            if (!ChildIter.first.empty() ||
                !ChildIter.second.get_value_optional<double>().is_initialized())
            {
              BOOST_CHECK_MESSAGE(ChildIter.first.empty(),
                "expecting an array element (with no name)");
              BOOST_CHECK_NO_THROW(std::stod(ChildIter.second.data()));
              break;
            }
          }
        }
        else if (ColumnName.EndsWith("_VVR"))
        {
          // check if the content is valid JSON
          boost::property_tree::ptree Tree;
          BOOST_CHECK_NO_THROW(boost::property_tree::read_json(
            ColumnContentStream, Tree));

          // check if the content is an array of arrays of numbers
          for (auto ChildIter : Tree)
          {
            BOOST_CHECK_MESSAGE(ChildIter.first.empty(),
              "expecting an array element (with no name)");
            for (auto ChildChildIter : ChildIter.second)
            {
              if (!ChildChildIter.first.empty() ||
                  !ChildChildIter.second.get_value_optional<double>().is_initialized())
              {
                BOOST_CHECK_MESSAGE(ChildChildIter.first.empty(),
                  "expecting an array element (with no name)");
                BOOST_CHECK_NO_THROW(std::stod(ChildChildIter.second.data()));
                break;
              }
            }
          }
        }
        else
        {
          // unexpected column name
          BOOST_CHECK_MESSAGE(ColumnName == "filename" ||
              ColumnName == "modtime" || ColumnName == "status",
            "Unexpected column name");
        }
      }
    }
  };


  // ... resolve crawler exe path and "train" folder

  const TString CrawlerExePath = CrawlerExePathAndName();
  BOOST_CHECK(TFile(CrawlerExePath).ExistsIgnoreCase());

  const TDirectory SampleTrainFolder =
    gApplicationResourceDir().Descend("Kicks-vs-Snare-Train");

  BOOST_CHECK(SampleTrainFolder.ExistsIgnoreCase());


  // ... create low level db

  BOOST_TEST_INFO("    Creating low level descriptors...");

  const TString LowLevelDescriptorDb =
    SampleTrainFolder.Path() + "afec-ll.db";

  if (TFile(LowLevelDescriptorDb).Exists())
  {
    BOOST_CHECK(TFile(LowLevelDescriptorDb).Unlink());
  }

  // wrong arguments -> must fail
  LaunchResult = TSystem::LaunchProcess(CrawlerExePath,
    MakeList<TString>(
      "-l", "wrong_level"
      ),
    WaitTilProcessFinished);

  BOOST_CHECK(LaunchResult != EXIT_SUCCESS);

  LaunchResult = TSystem::LaunchProcess(CrawlerExePath,
    MakeList<TString>(
      "-l", "low",
      "!!invalid_path"
      ),
    WaitTilProcessFinished);

  BOOST_CHECK(LaunchResult != EXIT_SUCCESS);

  LaunchResult = TSystem::LaunchProcess(CrawlerExePath,
    MakeList<TString>(
      "-l", "low",
      "-o", LowLevelDescriptorDb,
      SampleTrainFolder.Path()
      ),
    WaitTilProcessFinished);

  BOOST_CHECK(LaunchResult == EXIT_SUCCESS);
  BOOST_CHECK(TFile(LowLevelDescriptorDb).Exists());


  // ... check low level db content

  BOOST_TEST_INFO("    Checking low level database...");
  {
    TDatabase Database;
    BOOST_CHECK(Database.Open(LowLevelDescriptorDb));

    // expecting one failed sample in test set ("_Not A Wavefile.wav")
    {
      TDatabase::TStatement DbStatement(Database,
        "SELECT COUNT(*) FROM assets WHERE status!='succeeded';");
      BOOST_CHECK(DbStatement.Step());

      const int NumberOfFailedSamples = DbStatement.ColumnInt(0);
      BOOST_CHECK(NumberOfFailedSamples == 1);
    }

    // make sure that all columns are filled with valid content
    {
      TDatabase::TStatement DbStatement(Database,
        "SELECT * FROM assets WHERE status='succeeded';");

      BOOST_REQUIRE(DbStatement.Step()); // check first asset only

      BOOST_CHECK(DbStatement.ColumnCount() > 0);
      for (int Column = 0; Column < DbStatement.ColumnCount(); ++Column)
      {
        VerifyColumnContent(DbStatement, Column);
      }
    }
  }


  // ... create high level db

  BOOST_TEST_INFO("    Creating high level descriptors...");

  const TString HighLevelDescriptorDb =
    SampleTrainFolder.Path() + "afec.db";

  if (TFile(HighLevelDescriptorDb).Exists())
  {
    BOOST_CHECK(TFile(HighLevelDescriptorDb).Unlink());
  }

  LaunchResult = TSystem::LaunchProcess(CrawlerExePath,
    MakeList<TString>(
      "-l", "high",
      "-o", HighLevelDescriptorDb,
      SampleTrainFolder.Path()
    ),
    WaitTilProcessFinished);

  BOOST_CHECK(LaunchResult == EXIT_SUCCESS);
  BOOST_CHECK(TFile(HighLevelDescriptorDb).Exists());


  // ... check high level db content

  BOOST_TEST_INFO("    Checking high level database...");
  {
    TDatabase Database;
    BOOST_CHECK(Database.Open(HighLevelDescriptorDb));

    // expecting one failed sample in test set ("_Not A Wavefile.wav")
    {
      TDatabase::TStatement DbStatement(Database,
        "SELECT COUNT(*) FROM assets WHERE status!='succeeded';");
      BOOST_CHECK(DbStatement.Step());

      const int NumberOfFailedSamples = DbStatement.ColumnInt(0);
      BOOST_CHECK(NumberOfFailedSamples == 1);
    }

    // make sure that all columns are filled with valid content
    {
      TDatabase::TStatement DbStatement(Database,
        "SELECT * FROM assets WHERE status='succeeded';");

      BOOST_REQUIRE(DbStatement.Step()); // check first asset only

      BOOST_CHECK(DbStatement.ColumnCount() > 0);
      for (int Column = 0; Column < DbStatement.ColumnCount(); ++Column)
      {
        VerifyColumnContent(DbStatement, Column);
      }
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TCrawlerTests::ClassificationModel()
{
  BOOST_TEST_MESSAGE("  Testing ClassificationModel...");

  const bool WaitTilProcessFinished = true;
  int LaunchResult;


  // ... resolve crawler and model creator exe

  const TString CrawlerExePath = CrawlerExePathAndName();
  BOOST_CHECK(TFile(CrawlerExePath).ExistsIgnoreCase());

  const TString ModelCreatorExePath = ModelCreatorExePathAndName();
  BOOST_CHECK(TFile(ModelCreatorExePath).ExistsIgnoreCase());

  // ... create model (from TRAIN folder)

  BOOST_TEST_INFO("    Creating model from low level descriptors...");

  const TDirectory SampleTrainFolder =
    gApplicationResourceDir().Descend("Kicks-vs-Snare-Train");
  BOOST_CHECK(SampleTrainFolder.ExistsIgnoreCase());

  // created by "Crawler" test:
  const TString LowLevelDescriptorDb =
    SampleTrainFolder.Path() + "afec-ll.db";
  BOOST_CHECK(TFile(LowLevelDescriptorDb).ExistsIgnoreCase());

  // will create that now:
  const TString ModelDestPathAndName =
    SampleTrainFolder.Path() + "Kicks-vs-Snares.model";

  if (TFile(ModelDestPathAndName).Exists())
  {
    BOOST_CHECK(TFile(ModelDestPathAndName).Unlink());
  }

  // wrong arguments (missing src db) - must fail
  LaunchResult = TSystem::LaunchProcess(ModelCreatorExePath,
    MakeList<TString>(
      "-o", ModelDestPathAndName
    ),
    WaitTilProcessFinished);

  BOOST_CHECK(LaunchResult != EXIT_SUCCESS);
  BOOST_CHECK(!TFile(ModelDestPathAndName).Exists());

  // correct arguments - must succeed
  LaunchResult = TSystem::LaunchProcess(ModelCreatorExePath,
    MakeList<TString>(
      "-o", ModelDestPathAndName,
      LowLevelDescriptorDb
    ),
    WaitTilProcessFinished);

  BOOST_CHECK(LaunchResult == EXIT_SUCCESS);
  BOOST_CHECK(TFile(ModelDestPathAndName).Exists());


  // ... apply model - high level db (from TEST folder)

  BOOST_TEST_INFO("    Creating high level descriptors from model...");

  const TDirectory SampleTestFolder =
    gApplicationResourceDir().Descend("Kicks-vs-Snare-Test");

  BOOST_CHECK(SampleTrainFolder.ExistsIgnoreCase());

  const TString HighLevelDescriptorDb =
    SampleTestFolder.Path() + "afec.db";

  if (TFile(HighLevelDescriptorDb).Exists())
  {
    BOOST_CHECK(TFile(HighLevelDescriptorDb).Unlink());
  }
  
  LaunchResult = TSystem::LaunchProcess(CrawlerExePath,
    MakeList<TString>(
      "-l", "high",
      "-m", "none", // Classifiers
      "-m", ModelDestPathAndName, // OneShot-Categories
      "-o", HighLevelDescriptorDb,
      SampleTestFolder.Path()
    ),
    WaitTilProcessFinished);

  BOOST_CHECK(LaunchResult == EXIT_SUCCESS);
  BOOST_CHECK(TFile(HighLevelDescriptorDb).Exists());


  // ... check high level db content

  BOOST_TEST_INFO("    Checking database...");

  TDatabase Database;
  BOOST_CHECK(Database.Open(HighLevelDescriptorDb));

  // check 'class' table content
  {
    TDatabase::TStatement DbStatement(Database,
      "SELECT * FROM classes WHERE classifier='OneShot-Categories';");
    BOOST_CHECK(DbStatement.Step());

    BOOST_CHECK(DbStatement.ColumnName(0) == "classifier");
    const TString Classifier = DbStatement.ColumnText(0);
    BOOST_CHECK(Classifier == "OneShot-Categories");

    BOOST_CHECK(DbStatement.ColumnName(1) == "classes");

    TString ClassNameBlob = DbStatement.ColumnText(1);
    ClassNameBlob.RemoveFirst("["); ClassNameBlob.RemoveLast("]");
    BOOST_CHECK(!ClassNameBlob.IsEmpty());

    const TList<TString> Classnames = ClassNameBlob.SplitAt(",");
    BOOST_CHECK_EQUAL(Classnames.Size(), 2);
    BOOST_CHECK(Classnames[0].Contains("Kicks"));
    BOOST_CHECK(Classnames[1].Contains("Snares"));
  }

  // check 'assets' table content
  {
    TDatabase::TStatement DbStatement(Database,
      "SELECT COUNT(*) FROM assets WHERE status!='succeeded';");
    BOOST_CHECK(DbStatement.Step());

    // expecting no failed samples
    int NumberOfFailedSamples = DbStatement.ColumnInt(0);
    BOOST_CHECK_EQUAL(NumberOfFailedSamples, 0);
  }

  // check classificaton results
  {
    TDatabase::TStatement DbStatement(Database,
      "SELECT filename, classes_VS, categories_VS FROM assets;");

    int Total = 0;
    int MispredictedCategories = 0;
    while (DbStatement.Step())
    {
      BOOST_CHECK_EQUAL(DbStatement.ColumnName(0), "filename");
      const TString FileName = DbStatement.ColumnText(0);
      BOOST_CHECK(!FileName.IsEmpty());

      // check evaluated classes
      BOOST_CHECK_EQUAL(DbStatement.ColumnName(1), "classes_VS");
      {
        std::stringstream TempStream; 
        TempStream << DbStatement.ColumnText(1).CString();
        
        boost::property_tree::ptree PredictedClassesTree;
        BOOST_CHECK_NO_THROW(boost::property_tree::read_json(
          TempStream, PredictedClassesTree));
        // we've disabled classes via "-mclass none" - so it should be empty
        BOOST_CHECK(PredictedClassesTree.size() == 0); 
      }

      // check evaluated category
      BOOST_CHECK_EQUAL(DbStatement.ColumnName(2), "categories_VS");
      {
        std::stringstream TempStream; 
        TempStream << DbStatement.ColumnText(2).CString();
        
        boost::property_tree::ptree PredictedCategoriesTree;
        BOOST_CHECK_NO_THROW(boost::property_tree::read_json(
          TempStream, PredictedCategoriesTree));
        
        const TString PredictedCategory = (PredictedCategoriesTree.size()) ?
          PredictedCategoriesTree.front().second.get_value<std::string>().c_str() : 
          "";
        
        if (!FileName.StartsWith(PredictedCategory))
        {
          BOOST_TEST_MESSAGE("    Mispredicted: " + FileName);
          ++MispredictedCategories;
        }
      }

      ++Total;
    }

    // a simple classification problem. should have good results:
    const double CategorizationError = ((double)MispredictedCategories / Total);
    BOOST_CHECK(CategorizationError <= 0.2);

    BOOST_TEST_MESSAGE("    Categorization accuracy: " +
      ToString((1.0 - CategorizationError) * 100.0, "%.02lf"));
  }
}

// -------------------------------------------------------------------------------------------------

boost::unit_test::test_suite* TCrawlerTests::RegisterUnitTests(int, char*[])
{
  // give master a name for the reports
  boost::unit_test::framework::master_test_suite().p_name.value = "Crawler";


  // . FeatureExtraction

  boost::unit_test::test_suite* pFeatureExtractionTest = BOOST_TEST_SUITE("FeatureExtraction");
  {
    pFeatureExtractionTest->add(BOOST_TEST_CASE(TFeatureExtractionTest::Statistics));
  }
  boost::unit_test::framework::master_test_suite().add(pFeatureExtractionTest);

  // . Classification

  boost::unit_test::test_suite* pCoreMachinelearningTests = BOOST_TEST_SUITE("Classification");
  {
    pCoreMachinelearningTests->add(BOOST_TEST_CASE([]() {
      BOOST_TEST_MESSAGE("  Testing Classification..."); BOOST_CHECK(true);
      }));

    pCoreMachinelearningTests->add(BOOST_TEST_CASE(TClassificationTest::Shark));
  }
  boost::unit_test::framework::master_test_suite().add(pCoreMachinelearningTests);

  // . Crawler

  boost::unit_test::test_suite* pCrawlerTest = BOOST_TEST_SUITE("Crawler");
  {
    pCrawlerTest->add(BOOST_TEST_CASE(Crawler));
  }
  boost::unit_test::framework::master_test_suite().add(pCrawlerTest);

  // . ClassificationModels

  boost::unit_test::test_suite* pClassificationModelTest = BOOST_TEST_SUITE("ClassificationModels");
  {
    pClassificationModelTest->add(BOOST_TEST_CASE(ClassificationModel));
  }
  boost::unit_test::framework::master_test_suite().add(pClassificationModelTest);

  return NULL;
}

