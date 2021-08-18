#include "CoreTypes/Export/Version.h"
#include "CoreTypes/Export/System.h"
#include "CoreTypes/Export/Log.h"

#include "AudioTypes/Export/AudioTypesInit.h"

#include "CoreFileFormats/Export/CoreFileFormatsInit.h"

#include "FeatureExtraction/Export/FeatureExtractionInit.h"
#include "FeatureExtraction/Export/SampleDescriptors.h"
#include "FeatureExtraction/Export/SampleClassificationDescriptors.h"
#include "FeatureExtraction/Export/SqliteSampleDescriptorPool.h"

#include "Classification/Export/ClassificationInit.h"
#include "Classification/Export/ClassificationTestDataSet.h"
#include "Classification/Export/ClassificationTestResults.h"

#include "Classification/Export/DefaultClassificationModel.h"

#include "../../3rdParty/Boost/Export/BoostProgramOptions.h"

// =================================================================================================

namespace TProductDescription 
{ 
  TString ProductName() { return "AFEC ModelCreator"; }
  TString ProductVendorName() { return "AFEC"; }
  TString ProductProjectsLocation() { return "Crawler/XModelCreator"; }

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

// -------------------------------------------------------------------------------------------------

static void STraceResults(const char* pString, ...)
{
  char TempChars[4096];

  va_list ArgList;
  va_start(ArgList, pString);
  vsnprintf(TempChars, sizeof(TempChars),
    pString, ArgList);
  va_end(ArgList);

  gTrace(TempChars);
  TLog::SLog()->AddLineNoVarArgs("Results", TempChars);
}

// =================================================================================================

#include "CoreTypes/Export/MainEntry.h"

// -------------------------------------------------------------------------------------------------

int gMain(const TList<TString>& Arguments)
{
  // don't add the log output to the command line
  TLog::SLog()->SetTraceLogContents(false);

 
  // ... Parse program options

  const std::string ProgramName = gCutPath(Arguments[0]).StdCString();
  const std::string Usage = std::string() + "Usage:\n" + 
    "  " + ProgramName.c_str() + " [options] <input.db>\n" +
    "  " + ProgramName.c_str() + " --help";

  boost::program_options::options_description CommandLineOptions("Options");
  CommandLineOptions.add_options()
    ("help,h", "Show help message.")
    ("repeat,r", boost::program_options::value<int>()->default_value(8),
      "Number of times to repeat the model creation with different training set "
    "folds, to choose the best one along all runs.")
    ("seed,s", boost::program_options::value<int>()->default_value(-1),
      "Random seed, if any, in order to replicate runs.")
    ("dest_model,o", boost::program_options::value<std::string>(),
      "Destination name and path of the resulting model file.\n"
      "When not specified, the model file will be written into the crawler's "
      "resource directory.")
    ("src_database,i", boost::program_options::value<std::string>()->required(),
      "The low level descriptor db file to create the train and test data from. "
      "Can also be passed as last (positional) argument.")
    ;

  boost::program_options::positional_options_description PositionalArguments;
  PositionalArguments.add("src_database", 1);

  // extract options
  TString SourceDatabasePathAndName;
  TString DestModelPathAndName;
  int NumberOfRuns;
  int RandomSeed;

  try
  {
    // parse arguments
    const boost::program_options::parsed_options ParsedOptions =
      CreateBoostCommandLineParser(Arguments).options(
        CommandLineOptions).positional(PositionalArguments).run();
    boost::program_options::variables_map ProgramVariablesMap;
    boost::program_options::store(ParsedOptions, ProgramVariablesMap);

    // show help
    if (Arguments.Size() == 1 ||
        ProgramVariablesMap.find("help") != ProgramVariablesMap.end())
    {
      std::cout << Usage << "\n\n" << CommandLineOptions << "\n";
      return EXIT_SUCCESS;
    }

    // validate arguments
    boost::program_options::notify(ProgramVariablesMap);

    // repeat -> NumberOfRuns
    NumberOfRuns = ProgramVariablesMap["repeat"].as<int>();

    // seed -> RandomSeed
    RandomSeed = ProgramVariablesMap["seed"].as<int>();

    // src_database -> SourceDatabasePathAndName
    if (ProgramVariablesMap.find("src_database") != ProgramVariablesMap.end())
    {
      SourceDatabasePathAndName = ArgumentToString(ProgramVariablesMap["src_database"]);

      if (!TFile(SourceDatabasePathAndName).Exists() &&
          !TFile(gCutExtension(SourceDatabasePathAndName) + ".shark").Exists())
      {
        std::cerr << "ERROR: src_database is not a valid low level database file: "
          << SourceDatabasePathAndName.StdCString().c_str() << std::endl;

        std::cerr << Usage << "\n\n" << CommandLineOptions << "\n";
        return EXIT_FAILURE;
      }
    }
    else
    {
      std::cerr << Usage << "\n\n" << CommandLineOptions << "\n";
      return EXIT_FAILURE;
    }

    // dest_model -> DestModelPathAndName
    if (ProgramVariablesMap.find("dest_model") != ProgramVariablesMap.end())
    {
      DestModelPathAndName = ArgumentToString(ProgramVariablesMap["dest_model"]);
    }
    else
    {
      const TDirectory ModelDestPath = (gRunningInDeveloperEnvironment()) ?
        gDeveloperProjectsDir().Descend("Crawler").Descend("XCrawler").Descend(
          "Resources").Descend("Models") : gApplicationDir();

      DestModelPathAndName = ModelDestPath.Path() +
        gExtractPath(SourceDatabasePathAndName).SplitPathComponents().Last() + ".model";
    }
  }
  catch (const boost::program_options::error& error)
  {
    std::cerr << error.what() << "\n\n" << Usage << "\n\n" << CommandLineOptions << "\n";
    return EXIT_FAILURE;
  }
  catch (const std::exception& exception)
  {
    std::cerr << exception.what() << "\n";
    return EXIT_FAILURE;
  }


  // ... Init

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
    std::cerr << Exception.what();
    return 1;
  }

  
  // ... Run

  bool GotError = false;

  try
  {
    const TString TestDataSetFilename =
      gCutExtension(SourceDatabasePathAndName) + ".shark";

    TOwnerPtr<TClassificationTestDataSet> pTestSet;

    if (TFile(TestDataSetFilename).Exists() &&
        (TFile(TestDataSetFilename).ModificationStatTime() >
           TFile(SourceDatabasePathAndName).ModificationStatTime() ||
         !TFile(SourceDatabasePathAndName).Exists()))
    {
      STraceResults("Loading data set from '%s'", TestDataSetFilename.StdCString().c_str());

      pTestSet = TOwnerPtr<TClassificationTestDataSet>(new TClassificationTestDataSet());
      pTestSet->LoadFromFile(TestDataSetFilename);
    }
    else
    {
      STraceResults("Creating data set from '%s'",
        SourceDatabasePathAndName.StdCString().c_str());

      const bool ReadOnlyPool = true;
      TSqliteSampleDescriptorPool Pool(TSampleDescriptors::kLowLevelDescriptors);
      if (!Pool.Open(SourceDatabasePathAndName, ReadOnlyPool))
      {
        throw TReadableException(
          MText("Failed to open database: '%s'", SourceDatabasePathAndName));
      }

      const int NumberOfSamples = Pool.NumberOfSamples();
      TList<TSampleClassificationDescriptors> Descriptors;
      Descriptors.PreallocateSpace(NumberOfSamples);
      bool ExtractedFeatureNames = false;
      for (int i = 0; i < NumberOfSamples; ++i)
      {
        if (TOwnerPtr<TSampleDescriptors> pDescriptors = Pool.Sample(i))
        {
          // extract feature values by default
          TSampleClassificationDescriptors::TFeatureExtractionFlags Flags =
            TSampleClassificationDescriptors::kExtractFeatureValues;
          if (!ExtractedFeatureNames)
          {
            // extract feature names in/with the first entry
            ExtractedFeatureNames = true;
            Flags |= TSampleClassificationDescriptors::kExtractFeatureNames;
          }
          // add descriptor to the data set
          Descriptors.Append(TSampleClassificationDescriptors(*pDescriptors, Flags));
        }
      }

      // convert to classification test data set
      pTestSet = TOwnerPtr<TClassificationTestDataSet>(
        new TClassificationTestDataSet(SourceDatabasePathAndName, Descriptors));

      STraceResults("Saving data set to '%s'", TestDataSetFilename.StdCString().c_str());

      // cache set, to quickly rerun next times
      pTestSet->SaveToFile(TestDataSetFilename);

      // export to CSV too to allow using the data in other applications
      pTestSet->ExportToCsvFile(gCutExtension(TestDataSetFilename) + ".csv");
    }

    const TClassificationTestDataSet& TestSet = *pTestSet;

    STraceResults("Set has %d samples, %dx%d features, %d classes\n",
      TestSet.NumberOfSamples(),
      TestSet.InputFeaturesSize().mX,
      TestSet.InputFeaturesSize().mY,
      TestSet.NumberOfClasses());

    STraceResults("Will write model to '%s'\n", DestModelPathAndName.StdCString().c_str());

    TClassificationTestResults BestResults;
    TPtr<TDefaultBaggingClassificationModel> pBestModel;

    if (RandomSeed != -1)
    {
      // apply passed RandomSeed
      shark::Rng::seed(RandomSeed);
    }
    else
    {
      // or make sure we're creating new shuffled test data every new run
      shark::Rng::seed((unsigned int)TSystem::TimeInMsSinceStartup());
    }

    // train (up to NumberOfRuns times)
    for (int i = 0; i < NumberOfRuns; ++i)
    {
      TPtr<TDefaultBaggingClassificationModel> pNewModel(
         new TDefaultBaggingClassificationModel());

      STraceResults("Training new %s model (run %d/%d)...", 
        pNewModel->Name().StdCString().c_str(), i + 1, NumberOfRuns);

      const TClassificationTestResults Results = pNewModel->Train(TestSet);

      STraceResults("  Error: %.2f (Accuracy: %.2f%%)", Results.mFinalError,
        100.0f * (1.0f - Results.mFinalError));

      if (Results.mFinalError < 0.0)
      {
        STraceResults("  ERROR: Model training failed");
        GotError = true;
        break;
      }

      if (!pBestModel || Results.mFinalError < BestResults.mFinalError)
      {
        BestResults = Results;
        pBestModel = pNewModel;

        // save best model
        STraceResults("  Writing best model so far...");
        pBestModel->Save(DestModelPathAndName);
      }
    }

    // dump final, best result
    if (!GotError)
    { 
      STraceResults("  Best Error: %.2f (Accuracy: %.2f%%)", BestResults.mFinalError,
        100.0f * (1.0f - BestResults.mFinalError));
    }
  }

  // covers shark::Exception and TReadableException
  catch (const std::exception& Exception)
  {
    STraceResults("ERROR: %s", Exception.what());
    GotError = true;
  }

  M__EnableFloatingPointAssertions


  // ... Cleanup

  ClassificationExit();
  FeatureExtractionExit();
  CoreFileFormatsExit();
  AudioTypesExit();

  return (GotError) ? 1 : 0;
}

