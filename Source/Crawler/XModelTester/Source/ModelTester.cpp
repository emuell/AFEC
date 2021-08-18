#include "CoreTypes/Export/Version.h"
#include "CoreTypes/Export/System.h"
#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/Str.h"

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

#include "Classification/Export/Models/ANN.h"
#include "Classification/Export/Models/DNN.h"
#include "Classification/Export/Models/GBDT.h"
#include "Classification/Export/Models/RBM.h"
#include "Classification/Export/Models/SVM.h"
#include "Classification/Export/Models/NaiveBayes.h"
#include "Classification/Export/Models/RandomForest.h"
#include "Classification/Export/Models/KNN.h"
#include "Classification/Export/Models/Bagging.h"

#include "../../3rdParty/Boost/Export/BoostProgramOptions.h"

#include <cstdlib>

// =================================================================================================

namespace TProductDescription 
{ 
  TString ProductName() { return "AFEC ModelTester"; }
  TString ProductVendorName() { return "AFEC"; }
  TString ProductProjectsLocation() { return "Crawler/XModelTester"; }

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

namespace TClassificationTester
{
  //@{ ... Logging

  //! Trace results to log and std out
  void STraceResults(const char* pString, ...);
  //@}


  //@{ ... Dump Class Predictions

  typedef TClassificationTestResults::TPredictionError TPredictionError;

  // Dump prediction errors to "Results" folder
  void SDumpPredictionErrors(
    const TDirectory&               ResultsDir,
    const TList<TString>&           ClassNames,
    const TList<TPredictionError>&  PredictionErrors);
  //@}


  //@{ ... Dump Class Confusion Matrix

  typedef TClassificationTestResults::TConfusionMatrix TConfusionMatrix;

  // Dump a confusion matrix to std and "Results" folder
  void SDumpConfusionMatrix(
    const TDirectory&       ResultsDir,
    const TList<TString>&   ClassNames,
    const TConfusionMatrix& ConfusionMatrix);
  //@}


  //@{ ... Run Tests

  float SRunTest(
    TClassificationModel&             Model,
    const TClassificationTestDataSet& DataSet,
    int                               NumberOfRuns,
    int                               RandomSeed);
  //@}
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
    ("all,a", boost::program_options::value<bool>()->default_value(false),
      "When enabled, test all models instead of just the the default model.")
    ("repeat,r", boost::program_options::value<int>()->default_value(10), 
      "Number of times the test should be repeated.")
    ("seed,s", boost::program_options::value<int>()->default_value(-1),
      "Random seed, if any, in order to replicate tests.")
    ("bagging,b", boost::program_options::value<bool>()->default_value(false),
      "When enabled, test bagging ensemble models instead of 'raw' ones.")
    ("src_database,i", boost::program_options::value<std::string>()->required(),
      "The low level descriptor db file to create the train and test data from. "
      "Can also be passed as last (positional) argument.")
    ;

  boost::program_options::positional_options_description PositionalArguments;
  PositionalArguments.add("src_database", 1);

  // extract options
  bool TestBaggingModels;
  bool TestAllModels;
  int NumberOfTestRuns;
  int RandomSeed;
  TString SourceDatabasePathAndName;

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

    // all -> TestAllModels
    TestAllModels = ProgramVariablesMap["all"].as<bool>();

    // repeat -> NumberOfTestRuns
    NumberOfTestRuns = ProgramVariablesMap["repeat"].as<int>();

    // seed -> RandomSeed
    RandomSeed = ProgramVariablesMap["seed"].as<int>();

    // bagging -> TestBaggingModels
    TestBaggingModels = ProgramVariablesMap["bagging"].as<bool>();
    
    // src_database -> SourceDatabasePathAndName
    if (ProgramVariablesMap.find("src_database") != ProgramVariablesMap.end())
    {
      SourceDatabasePathAndName = ArgumentToString(ProgramVariablesMap["src_database"]);

      if (!TFile(SourceDatabasePathAndName).Exists() &&
          !TFile(gCutExtension(SourceDatabasePathAndName) + ".shark").Exists())
      {
        std::cerr << "ERROR: src_database is not a valid low level database file: "
          << SourceDatabasePathAndName.StdCString() << std::endl;

        std::cerr << Usage << "\n\n" << CommandLineOptions << "\n";
        return EXIT_FAILURE;
      }
    }
    else
    {
      std::cerr << Usage << "\n\n" << CommandLineOptions << "\n";
      return EXIT_FAILURE;
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
    return EXIT_FAILURE;
  }


  // ... Run

  M__DisableFloatingPointAssertions

  bool GotError = false;

  try
  {
    float BestError = std::numeric_limits<float>::max();
    TString BestModel = TString();

    const TString TestDataSetFilename =
      gCutExtension(SourceDatabasePathAndName) + ".shark";

    TOwnerPtr<TClassificationTestDataSet> pTestSet;

    if (TFile(TestDataSetFilename).Exists() &&
        (TFile(TestDataSetFilename).ModificationStatTime() >
           TFile(SourceDatabasePathAndName).ModificationStatTime() ||
         !TFile(SourceDatabasePathAndName).Exists()))
    {
      TClassificationTester::STraceResults("Loading data set from '%s'",
        TestDataSetFilename.StdCString().c_str());

      pTestSet = TOwnerPtr<TClassificationTestDataSet>(new TClassificationTestDataSet());
      pTestSet->LoadFromFile(TestDataSetFilename);
    }
    else
    {
      TClassificationTester::STraceResults("Creating data set from '%s'",
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

      TClassificationTester::STraceResults("Saving data set to '%s'", 
        TestDataSetFilename.StdCString().c_str());

      // cache set, to quickly rerun next times
      pTestSet->SaveToFile(TestDataSetFilename);

      // export to CSV too to allow using the data in other applications
      pTestSet->ExportToCsvFile(gCutExtension(TestDataSetFilename) + ".csv");
    }

    const TClassificationTestDataSet& TestSet = *pTestSet;

    TClassificationTester::STraceResults(
      "Set has %d samples, %dx%d features and %d classes.\n",
      TestSet.NumberOfSamples(),
      TestSet.InputFeaturesSize().mX,
      TestSet.InputFeaturesSize().mY,
      TestSet.NumberOfClasses());


    #define MRunTest(MODEL_TYPE, DATA_SET) \
      { \
        TPtr<MODEL_TYPE> pModel(new MODEL_TYPE()); \
        \
        const float Error = TClassificationTester::SRunTest( \
          *pModel, DATA_SET, NumberOfTestRuns, RandomSeed); \
          \
        if (Error >= 0.0f && Error < BestError) \
        { \
          BestError = Error; \
          BestModel = pModel->Name(); \
        } \
      }

    // currently disabled
    // MRunTest(TRandomForestClassificationModel, TestSet);
    // MRunTest(TNaiveBayesClassificationModel, TestSet);
    // MRunTest(TKnnClassificationModel, TestSet);
    // MRunTest(TRbmClassificationModel, TestSet);
    // MRunTest(TDnnClassificationModel, TestSet);

    if (TestBaggingModels)
    {
      if (TestAllModels) 
      {
        MRunTest(TBaggingClassificationModel<TAnnClassificationModel>, TestSet);
        MRunTest(TBaggingClassificationModel<TSvmClassificationModel>, TestSet);
        MRunTest(TBaggingClassificationModel<TGbdtClassificationModel>, TestSet);
      }
      else 
      {
        MRunTest(TDefaultBaggingClassificationModel, TestSet);
      }
    }
    else
    {
      if (TestAllModels) 
      {
        MRunTest(TAnnClassificationModel, TestSet);
        MRunTest(TSvmClassificationModel, TestSet);
        MRunTest(TGbdtClassificationModel, TestSet);
      }
      else 
      {
        MRunTest(TDefaultClassificationModel, TestSet);
      }
    }

    TClassificationTester::STraceResults(
      "-> Best: '%s' Error: %g (Accuracy %.2f%%)", BestModel.StdCString().c_str(),
      BestError, (1.0f - BestError)*100.0f);
  }

  // covers shark::Exception and TReadableException
  catch (const std::exception& Exception)
  {
    TClassificationTester::STraceResults("ERROR: %s", Exception.what());
    GotError = true;
  }

  M__EnableFloatingPointAssertions


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
    ::perror(Exception.what());
    return EXIT_FAILURE;
  }

  return (GotError) ? EXIT_FAILURE : EXIT_SUCCESS;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static float SCalcClassAccuracy(
  const TArray<unsigned int>& PredictionRow,
  int                         Class)
{
  MAssert(Class >= 0 && Class < PredictionRow.Size(), "");

  int Total = 0, Correct = 0;
  for (int i = 0; i < PredictionRow.Size(); ++i)
  {
    if (i == Class)
    {
      Correct += PredictionRow[i];
    }

    Total += PredictionRow[i];
  }

  return (float)Correct / Total;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TClassificationTester::STraceResults(const char* pString, ...)
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

// -------------------------------------------------------------------------------------------------

void TClassificationTester::SDumpPredictionErrors(
  const TDirectory&               ResultsDir,
  const TList<TString>&           ClassNames,
  const TList<TPredictionError>&  PredictionErrors)
{
  const int NumberOfClasses = ClassNames.Size();

  TFile ResultFile(ResultsDir.Path() + "PredictionErrors.csv");

  if (!ResultFile.Open(TFile::kWrite))
  {
    throw shark::Exception(std::string() +
      "Failed to open '" + ResultFile.FileName().StdCString() + "' for writing");
  }

  // sort errors by name
  std::map<TString, TList<TPredictionError> > SortedErrors;
  for (int i = 0; i < PredictionErrors.Size(); ++i)
  {
    SortedErrors[PredictionErrors[i].mName].Append(PredictionErrors[i]);
  }

  // dump
  TList<TString> Content;

  // header
  TString Header = TString("File, 1st Prediction, 2nd Prediction");
  for (int i = 0; i < NumberOfClasses; ++i)
  {
    Header += ", " + ClassNames[i];
  }
  Content.Append(Header);

  // content rows
  for (auto iter : SortedErrors)
  {
    const TList<TPredictionError> Errors = iter.second;

    for (int i = 0; i < Errors.Size(); ++i)
    {
      // FileName
      TString ErrorRow = TString(Errors[i].mName).ReplaceChar(',', ' ');

      // 1st prediction
      ErrorRow += ", " + ClassNames[Errors[i].mPredictedClass];

      if (Errors[i].mPredictionWeights.IsEmpty())
      {
        // 2nd prediction
        ErrorRow += ", N/A";

        // weights (binary classifiers like SVMs have no weights)
        for (int i = 0; i < NumberOfClasses; ++i)
        {
          ErrorRow += ", N/A";
        }
      }
      else
      {
        // 2nd prediction
        float SecondBestGuessWeight = 0.0f;
        unsigned int SecondBestGuessClass = (unsigned int)-1;

        for (int k = 0; k < Errors[i].mPredictionWeights.Size(); ++k)
        {
          if (k != (int)Errors[i].mPredictedClass)
          {
            if (Errors[i].mPredictionWeights[k] > SecondBestGuessWeight)
            {
              SecondBestGuessWeight = Errors[i].mPredictionWeights[k];
              SecondBestGuessClass = k;
            }
          }
        }

        if (SecondBestGuessClass != (unsigned int)-1)
        {
          ErrorRow += ", " + ClassNames[SecondBestGuessClass];
        }
        else
        {
          ErrorRow += ", N/A";
        }

        // weights
        for (int k = 0; k < Errors[i].mPredictionWeights.Size(); ++k)
        {
          ErrorRow += ", " + ToString(Errors[i].mPredictionWeights[k], "%.2f");
        }
      }

      Content += ErrorRow;

      // gTraceVar("  %s -> %s", Errors[i].mName.StdCString().c_str(),
      //   ClassNames[Errors[i].mPredictedClass].StdCString().c_str());
    }
  }

  ResultFile.WriteText(Content);
}

// -------------------------------------------------------------------------------------------------

void TClassificationTester::SDumpConfusionMatrix(
  const TDirectory&       ResultsDir,
  const TList<TString>&   ClassNames,
  const TConfusionMatrix& ConfusionMatrix)
{
  const int NumberOfClasses = ConfusionMatrix.Size();

  MAssert(ClassNames.Size() == NumberOfClasses &&
    ConfusionMatrix.Size() == ConfusionMatrix[0].Size(), "");

  TFile ResultFile(ResultsDir.Path() + "Confusion.csv");

  if (!ResultFile.Open(TFile::kWrite))
  {
    throw shark::Exception(std::string() +
      "Failed to open '" + ResultFile.FileName().StdCString() + "' for writing");
  }

  TList<TString> CsvContent;

  // dump matrix 
  STraceResults("  Class Accuracy:");

  TString Header;
  Header += "Class, ";
  for (int c = 0; c < NumberOfClasses; ++c)
  {
    Header += ClassNames[c];
    Header += ", ";
  }
  Header += "Accuracy";
  CsvContent.Append(Header);

  for (int c = 0; c < NumberOfClasses; ++c)
  {
    MAssert(ConfusionMatrix[c].Size() == (int)NumberOfClasses, "");

    TString Row = ToString(ClassNames[c]) + ", ";
    for (int j = 0; j < (int)NumberOfClasses; ++j)
    {
      Row += ToString((int)ConfusionMatrix[c][j], "%d");
      if (j < NumberOfClasses - 1)
      {
        Row += ", ";
      }
    }

    const float ClassAccuracy = SCalcClassAccuracy(ConfusionMatrix[c], c);
    Row += ", " + ToString(ClassAccuracy * 100.0f, "%.02f%%");

    STraceResults("  %.02f%% - %s", ClassAccuracy, ClassNames[c].StdCString().c_str());
    CsvContent.Append(Row);
  }

  ResultFile.WriteText(CsvContent);
}

// -------------------------------------------------------------------------------------------------

float TClassificationTester::SRunTest(
  TClassificationModel&             Model,
  const TClassificationTestDataSet& DataSet,
  int                               NumberOfRuns,
  int                               RandomSeed)
{
  TClassificationTester::STraceResults(
    "Training %s model...", Model.Name().StdCString().c_str());

  float BestError = 1.0f;
  TList<float> Errors, SecondaryErrors;
  TArray< TArray<unsigned int> > SummedConfusionMatrix;
  TList<TClassificationTestResults::TPredictionError> SummedPredictionErrors;

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

  for (int i = 0; i < NumberOfRuns; ++i)
  {
    const TClassificationTestResults Result = Model.Train(DataSet);

    if (SummedConfusionMatrix.IsEmpty())
    {
      SummedConfusionMatrix = Result.mConfusionMatrix;
    }
    else
    {
      for (int cx = 0; cx < Result.mConfusionMatrix.Size(); ++cx)
      {
        for (int cy = 0; cy < Result.mConfusionMatrix[cx].Size(); ++cy)
        {
          SummedConfusionMatrix[cx][cy] += Result.mConfusionMatrix[cx][cy];
        }
      }
    }

    SummedPredictionErrors += Result.mPredictionErrors;

    if (Result.mFinalError < 0.0f)
    {
      STraceResults("-> Test failed or not implemented\n");
      return -1.0f;
    }
    else
    {
      STraceResults("  Run %d - Accuracy %.2f%% (2nd: %.2f%%)",
        i + 1, (1.0f - Result.mFinalError)*100.0f,
        (1.0f - Result.mFinalSecondaryError)*100.0f);

      Errors.Append(Result.mFinalError);
      SecondaryErrors.Append(Result.mFinalSecondaryError);

      // save best model
      if (Result.mFinalError < BestError)
      {
        Model.Save(Model.ModelDir().Path() + "Classification.model");
      }
    }
  }

  float ErrorPrecentageMean = 0.0f;
  for (int i = 0; i < Errors.Size(); ++i)
  {
    ErrorPrecentageMean += (Errors[i] / Errors.Size()) * 100;
  }

  float ErrorPrecentageVar = 0.0f;
  for (int i = 0; i < Errors.Size(); ++i)
  {
    ErrorPrecentageVar += TMathT<float>::Square(
      Errors[i] * 100.0f - ErrorPrecentageMean) /
      MMax(1, (Errors.Size() - 1));
  }

  float SecondaryErrorPrecentageMean = 0.0f;
  for (int i = 0; i < Errors.Size(); ++i)
  {
    SecondaryErrorPrecentageMean +=
      (SecondaryErrors[i] / SecondaryErrors.Size()) * 100;
  }

  float SecondaryErrorPrecentageVar = 0.0f;
  for (int i = 0; i < SecondaryErrors.Size(); ++i)
  {
    SecondaryErrorPrecentageVar += TMathT<float>::Square(
      SecondaryErrors[i] * 100.0f - SecondaryErrorPrecentageMean) /
      MMax(1, (SecondaryErrors.Size() - 1));
  }

  SDumpPredictionErrors(
    Model.ResultsDir(), DataSet.ClassNames(), SummedPredictionErrors);

  SDumpConfusionMatrix(
    Model.ResultsDir(), DataSet.ClassNames(), SummedConfusionMatrix);

  STraceResults("-> Accuracy %.2f%% +- %.2f%% (2nd: %.2f%% +- %.2f%%)\n",
    (100.0f - ErrorPrecentageMean), ErrorPrecentageVar,
    (100.0f - SecondaryErrorPrecentageMean), SecondaryErrorPrecentageVar);

  return ErrorPrecentageMean / 100.0f;
}

