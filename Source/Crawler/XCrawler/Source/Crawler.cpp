#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Directory.h"
#include "CoreTypes/Export/Version.h"
#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/Pointer.h"
#include "CoreTypes/Export/System.h"
#include "CoreTypes/Export/Cpu.h"

#include "AudioTypes/Export/AudioTypesInit.h"

#include "CoreFileFormats/Export/AudioFile.h"
#include "CoreFileFormats/Export/CoreFileFormatsInit.h"

#include "FeatureExtraction/Export/FeatureExtractionInit.h"
#include "FeatureExtraction/Export/SampleAnalyser.h"
#include "FeatureExtraction/Export/SqliteSampleDescriptorPool.h"

#include "Classification/Export/ClassificationInit.h"

#include "../../3rdParty/Boost/Export/BoostProgramOptions.h"
#include "../../3rdParty/Ctpl/Export/Ctpl.h"

#include <string>
#include <iostream>
#include <set>
#include <list>
#include <stdexcept>
#include <cstdlib>

#if defined(MWindows)
  #include <windows.h> // for SConsoleCtrlHandler only
#endif

#if defined(MLinux) || defined(MMac)
  #include <signal.h>
#endif

// =================================================================================================

#define MDefaultSampleRate 44100
#define MDefaultFFTFrameSize 2048
#define MDefaultHopFrameSize 1024

#define MDefaultLowLevelDatabaseName "afec-ll.db"
#define MDefaultHighLevelDatabaseName "afec.db"

#define MDefaultClassificationModelName "OneShot-vs-Loops.model"
#define MDefaultOneShotCategorizationModelName "OneShot-Categories.model"

#define MLogPrefix "Crawler"

// =================================================================================================

static volatile bool sAbortProcessing = false;

// -------------------------------------------------------------------------------------------------

#if defined(MWindows)

  static BOOL WINAPI SConsoleCtrlHandler(DWORD dwCtrlType)
  {
    sAbortProcessing = true;
    return 1;
  }

#elif defined(MLinux) || defined(MMac)

  static void SSigIntHandler(int sig) 
  {
    ::fprintf(stdout, "Caught signal %d. Requesting to abort processing...\n", sig);
    sAbortProcessing = true;
  }

#endif

// =================================================================================================

/*!
 * Needed for CoreTypes/Export/Version.h and co
!*/

namespace TProductDescription
{
  TString ProductName() { return "AFEC Crawler"; }
  TString ProductVendorName() { return "AFEC"; }
  TString ProductProjectsLocation() { return "Crawler/XCrawler"; }

  // use the AFEC GUI's version
  int MajorVersion() { return 1; }
  int MinorVersion() { return 0; }
  int RevisionVersion() { return 0; }

  TString AlphaOrBetaVersionString() { return ""; }
  TDate ExpirationDate(){ return TDate(); }

  TString SupportEMailAddress() { return "<support@afec.io>"; }
  TString ProductHomeURL() { return "https://afec.io/"; }
  TString CopyrightString() { return "2021"; }
}

// =================================================================================================

static void SShowVersionInfo();

static int SRunExtractor(
  const TList<TString>&               DirectoriesOrFiles,
  const TString&                      DbNameAndPath,
  const TDirectory&                   DbBasePath,
  const TString&                      ClassificationModelNameAndPath,
  const TString&                      OneShotCategorizationModelNameAndPath,
  TSampleDescriptors::TDescriptorSet  DescriptorSet,
  int                                 MaxAnalyzeThreads);

static bool SIgnoreRootDirectory(const TDirectory& BaseDirectory);
static bool SIgnoreSubDirectory(const TString& SubDirName);
static bool SIgnoreFile(const TString& Filename);

static void SCollectFiles(
  const TString&                      DirectoryOrFileNames,
  TDirectory::TSymLinkRecursionTest&  RecursionTester,
  TList<TString>&                     AudioFiles);

static void SBuildChangeList(
  const TList<TString>&               AudioFiles,
  TSqliteSampleDescriptorPool*        pSamplePool,
  TList<TString>&                     AudioFilesToAdd,
  TList<TString>&                     AudioFilesToRemove);

// =================================================================================================

#include "CoreTypes/Export/MainEntry.h"

// -------------------------------------------------------------------------------------------------

int gMain(const TList<TString>& Arguments)
{
  // ... install Control + C / SIGINT handler

  #if defined(MWindows)
    ::SetConsoleCtrlHandler(SConsoleCtrlHandler, TRUE);

  #elif defined(MLinux) || defined(MMac)
    struct sigaction AbortSigAction;
    ::memset(&AbortSigAction, 0, sizeof(struct sigaction));
    AbortSigAction.sa_handler = (void(*)(int))SSigIntHandler;
    AbortSigAction.sa_flags = SA_NODEFER | SA_ONSTACK;
    ::sigaction(SIGINT, &AbortSigAction, NULL);

  #else
    #error "Unexpected platform"
  #endif


  // ... Parse program options

  const std::string ProgramName = gCutPath(Arguments[0]).StdCString();
  const std::string Usage = std::string() + "Usage:\n" +
    "  " + ProgramName.c_str() + " [options] -o /path_to/database.db <paths...>\n" +
    "  " + ProgramName.c_str() + " --help";

  boost::program_options::options_description CommandLineOptions("Options");
  CommandLineOptions.add_options()
    ("help,h", "Show help message.")
    ("version,v", "Show version, build and other infos.")
    ("level,l", boost::program_options::value<std::string>()->default_value("high"),
      "Create a 'high' or 'low' level database.")
    ("model,m", boost::program_options::value<std::vector<std::string>>()->multitoken(),
      "Specify the 'Classifiers' and 'OneShot-Categories' model files that should be used "
      "for level='high'. When not specified, the default models from the crawler's "
      "resource dir are used. Set to 'none' to explicitely avoid loading the "
      "a default model - e.g. --model \"None\" --model \"None\" will disable both.")
    ("jobs,j", boost::program_options::value<int>()->default_value(-1),
      "Maximum number of samples that are analyzed simultaneously. "
      "By default all available concurrent CPU threads in the system.")
    ("out,o", boost::program_options::value<std::string>(), (std::string() +
      "Set destination directory/db_name.db or just a directory. When only a directory "
      "is specified, the database filename will be: '" + std::string(MDefaultLowLevelDatabaseName) + 
      "' or '" + std::string(MDefaultHighLevelDatabaseName) + "', depending on the level. "
      "When no directory or file is specified, the database will be written into the current "
      "working dir.").c_str())
    ("paths", boost::program_options::value<std::vector<std::string>>(),
      "One or more paths to a folder or single audio file which should be analyzed. "
      "Can also be passed as last (positional) argument.\n"
      "When all given paths are sub paths of the 'out' db path, all sample paths "
      "within the database will be relative to the out dir, else absolute.")
    ;

  boost::program_options::positional_options_description PositionalArguments;
  PositionalArguments.add("paths", -1);

  // ... parse arguments

  TString DbNameAndPath;
  TString ClassificationModelNameAndPath, CategorizationModelNameAndPath;
  TList<TString> DirectoriesOrFiles;

  TSampleDescriptors::TDescriptorSet DescriptorSet =
    TSampleDescriptors::kLowLevelDescriptors;

  int MaxAnalyzeThreads = -1;

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

    // show version
    if (ProgramVariablesMap.find("version") != ProgramVariablesMap.end())
    {
      SShowVersionInfo();
      return EXIT_SUCCESS;
    }

    // validate arguments
    boost::program_options::notify(ProgramVariablesMap);

    // level -> DescriptorSet
    if (ProgramVariablesMap.find("level") != ProgramVariablesMap.end())
    {
      const TString Level = ArgumentToString(ProgramVariablesMap["level"]);
      if (gStringsEqualIgnoreCase(Level, "low"))
      {
        DescriptorSet = TSampleDescriptors::kLowLevelDescriptors;
      }
      else if (gStringsEqualIgnoreCase(Level, "high"))
      {
        DescriptorSet = TSampleDescriptors::kHighLevelDescriptors;
      }
      else
      {
        std::stringstream Error;
        Error << "ERROR: invalid -l argument: expected 'low' or 'high', got: " <<
          Level.StdCString() << ".";
        throw boost::program_options::error(Error.str());
      }
    }

    // model -> ClassificationModelNameAndPath, CategorizationModelNameAndPath
    if (ProgramVariablesMap.find("model") != ProgramVariablesMap.end())
    {
      const TList<TString> Models = ArgumentToStringList(ProgramVariablesMap["model"]);
      if (Models.Size() > 2)
      {
        std::stringstream Error;
        Error << "model argument was specified more than 2 times: " <<
          "will use first model as 'Classifiers' model and second one " <<
          "as 'OneShot-Categories' models - all others are undefined.";
        throw boost::program_options::error(Error.str());
      }

      // ClassificationModelNameAndPath
      ClassificationModelNameAndPath = (Models.Size() > 0) ? Models[0] : "";
      if (ClassificationModelNameAndPath.IsNotEmpty())
      {
        if (!gStringsEqualIgnoreCase(ClassificationModelNameAndPath, "none") &&
            !TFile(ClassificationModelNameAndPath).Exists())
        {
          std::stringstream Error;
          Error << "class model argument does not point to an existing model file: '" <<
            ClassificationModelNameAndPath.StdCString() << "'.";
          throw boost::program_options::error(Error.str());
        }
      }
  
      // CategorizationModelNameAndPath
      CategorizationModelNameAndPath = (Models.Size() > 1) ? Models[1] : "";
      if (CategorizationModelNameAndPath.IsNotEmpty())
      {
        if (!gStringsEqualIgnoreCase(CategorizationModelNameAndPath, "none") &&
            !TFile(CategorizationModelNameAndPath).Exists())
        {
          std::stringstream Error;
          Error << "category model does not point to an existing model file: '" <<
            ClassificationModelNameAndPath.StdCString() << "'.";
          throw boost::program_options::error(Error.str());
        }
      }
    }
    
    // jobs -> MaxAnalyzeThreads
    if (ProgramVariablesMap.find("jobs") != ProgramVariablesMap.end()) 
    {
      MaxAnalyzeThreads = ProgramVariablesMap["jobs"].as<int>();
      if (MaxAnalyzeThreads != -1 && MaxAnalyzeThreads <= 0)
      {
        std::stringstream Error;
        Error << "jobs must be a number > 0 or -1.";
        throw boost::program_options::error(Error.str());
      }
    }

    // out -> DbNameAndPath
    if (ProgramVariablesMap.find("out") != ProgramVariablesMap.end())
    {
      TString DbName = ArgumentToString(ProgramVariablesMap["out"]);
      
      if (TDirectory(DbName).Exists())
      {
        // arg is a path, append default db name
        DbName = TDirectory(DbName).Path() +
          ((DescriptorSet == TSampleDescriptors::kLowLevelDescriptors) ?
            MDefaultLowLevelDatabaseName : MDefaultHighLevelDatabaseName);
      }

      if (!DbName.Contains(TDirectory::SPathSeparator()) &&
          !DbName.Contains(TDirectory::SWrongPathSeparator()))
      {
        // arg is a db name only, make DB an abs path
        DbNameAndPath = gCurrentWorkingDir().Path() + DbName;
      }
      else
      {
        // DB is an abs path, check if it's valid
        DbNameAndPath = DbName;
        if (! TDirectory(DbNameAndPath).IsAbsolute())
        {
          std::stringstream Error;
          Error << "expected an abs path [and name] or just "
            "a filename with the 'out' option.";
          throw boost::program_options::error(Error.str());
        }
      }
    }
    
    // file -> DirectoriesOrFiles
    if (ProgramVariablesMap.find("paths") != ProgramVariablesMap.end())
    {
      const TList<TString> FilesOrDirectories = 
        ArgumentToStringList(ProgramVariablesMap["paths"]);
      
      for (int i = 0; i < FilesOrDirectories.Size(); ++i)
      {
        const TString FileOrDirectory = FilesOrDirectories[i];
        const bool IsDirectory = TDirectory(FileOrDirectory).Exists();

        if (IsDirectory)
        {
          const TDirectory Directory = TDirectory(FileOrDirectory);

          if (DirectoriesOrFiles.Contains(Directory.Path()))
          {
            std::stringstream Error;
            Error << "Directory '" << Directory.Path().StdCString() << "' got specified multiple "
              "times in 'files'. You probably forgot to prefix it with an 'out' option?";
            throw boost::program_options::error(Error.str());
          }

          DirectoriesOrFiles.Append(Directory.Path());
        }
        else if (TFile(FileOrDirectory).Exists())
        {
          DirectoriesOrFiles.Append(FileOrDirectory);
        }
        else
        {
          std::stringstream Error;
          Error << "invalid arguments - argument '" << FileOrDirectory.StdCString() <<
            "' is neither an existing file nor directory.";
          throw boost::program_options::error(Error.str());
        }
      }
    }
    else
    {
      // check if we got any paths to crawl
      if (DirectoriesOrFiles.IsEmpty())
      {
        std::stringstream Error;
        Error << "invalid arguments - got no directories or files to analyze." ;
        throw boost::program_options::error(Error.str());
      }
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

  
  // ... Get DbBasePath from current options
  
  // check if sample paths can be relative, else make all sample paths absolute
  bool UseRelativePaths = false;
  if (! DbNameAndPath.IsEmpty())
  {
    const TDirectory DbPath = gExtractPath(DbNameAndPath);

    UseRelativePaths = true;
    for (int i = 0; i < DirectoriesOrFiles.Size(); ++i)
    {
      const TDirectory Directory = TDirectory(DirectoriesOrFiles[i]).Exists() ?
        TDirectory(DirectoriesOrFiles[i]) : gExtractPath(DirectoriesOrFiles[i]);

      if (Directory.IsRelative())
      {
        // make dirs absolute, if necessary
        DirectoriesOrFiles[i] = gCurrentWorkingDir().Descend(Directory.Path()).Path();
      }

      if (! Directory.IsSameOrSubDirOf(DbPath))
      {
        // all passed directories must be child dirs of the db path
        UseRelativePaths = false;
      }
    }
  }

  // set base sample path
  TDirectory DbBasePath;
  if (UseRelativePaths)
  {
    DbBasePath = gExtractPath(DbNameAndPath);

    TLog::SLog()->AddLine(MLogPrefix,
      "Will use relative paths for samples in the assets db.");
  }
  else
  {
    TLog::SLog()->AddLine(MLogPrefix,
      "NOTE: Will use absolute paths for samples in the assets db.");
  }

  // set db path
  if (DbNameAndPath.IsEmpty())
  {
    DbNameAndPath = gCurrentWorkingDir().Path() +
      ((DescriptorSet == TSampleDescriptors::kLowLevelDescriptors) ?
        MDefaultLowLevelDatabaseName : MDefaultHighLevelDatabaseName);
  }


  // ... Init

  // setup TLog to trace the log too.
  TLog::SLog()->SetTraceLogContents(true);

  // ignore all FPU exceptions from libxtract
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
    return EXIT_FAILURE;
  }


  // ... Run

  const int Result = SRunExtractor(
    DirectoriesOrFiles, DbNameAndPath, DbBasePath,
    ClassificationModelNameAndPath, CategorizationModelNameAndPath,
    DescriptorSet, 
    MaxAnalyzeThreads);


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
    std::cerr << Exception.what();
    return EXIT_FAILURE;
  }

  return Result;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void SShowVersionInfo()
{
  // don't mix log content with the version std out
  TLog::SLog()->SetTraceLogContents(false);

  // version and built date
  std::cout << MProductString.StdCString() << " " << 
    MBuildNumberString.StdCString() << "\n";

  // supported audio files
  std::cout << "Supported Audio files:" << "\n";
  
  std::cout << "  ";
  const TList<TString> AudioFileExtensions = TAudioFile::SSupportedExtensions();
  for (int i = 0; i < AudioFileExtensions.Size(); ++i)
  {
    std::cout << AudioFileExtensions[i].StdCString();
    if (i < AudioFileExtensions.Size() - 1)
    {
      std::cout << ", ";
    }
  }
  std::cout << "\n";

  // assets database version
  std::cout << "Pool Version:\n";
  std::cout << "  " << std::to_string(
    TSampleDescriptorPool::kCurrentVersion) << "\n";

  // class model
  const TString DefaultClassModelPathAndName =
    gApplicationResourceDir().Descend("Models").Path() +
    MDefaultClassificationModelName;
  
  std::cout << "Class Model:\n";
  if (TFile(DefaultClassModelPathAndName).Exists())
  {
    std::cout << "  " << DefaultClassModelPathAndName.StdCString() << "\n";
  }
  else
  {
    std::cout << "  not found\n";
  }

  // category model
  const TString DefaultCategoryModelPathAndName =
    gApplicationResourceDir().Descend("Models").Path() +
    MDefaultOneShotCategorizationModelName;

  std::cout << "Category Model:\n";
  if (TFile(DefaultCategoryModelPathAndName).Exists())
  {
    std::cout << "  " << DefaultCategoryModelPathAndName.StdCString() << "\n";
  }
  else
  {
    std::cout << "  not found\n";
  }
}

// -------------------------------------------------------------------------------------------------

int SRunExtractor(
  const TList<TString>&               DirectoriesOrFiles,
  const TString&                      DbNameAndPath,
  const TDirectory&                   DbBasePath,
  const TString&                      ClassificationModelNameAndPath,
  const TString&                      CategorizationModelNameAndPath,
  TSampleDescriptors::TDescriptorSet  DescriptorSet,
  int                                 MaxAnalyzeThreads)
{
  bool GotCrawlError = false;

  try
  {
    // ... init

    TLog::SLog()->AddLine(MLogPrefix, "Setting up sqlite extractor");
    TLog::SLog()->AddLine(MLogPrefix, "Writing into db: '%s'", DbNameAndPath.StdCString().c_str());


    // ... start crawling

    TOwnerPtr<TSqliteSampleDescriptorPool> pSamplePool(
      new TSqliteSampleDescriptorPool(DescriptorSet));

    pSamplePool->SetBasePath(DbBasePath);

    if (!pSamplePool->Open(DbNameAndPath))
    {
      throw std::runtime_error("Failed to open or create database");
    }

    TLog::SLog()->AddLine(MLogPrefix, "Setting up feature analyzer...");

    TOwnerPtr<TSampleAnalyser> pAnalyzer(new TSampleAnalyser(
      MDefaultSampleRate, MDefaultFFTFrameSize, MDefaultHopFrameSize));

    if (DescriptorSet == TSampleDescriptors::kHighLevelDescriptors)
    {
      #if defined(MEnableDebugSampleDescriptors)
        TLog::SLog()->AddLine(MLogPrefix, "Please note: "
          "Will create/update a database with 'debug_R/VR/VVR' descriptors enabled. "
          "This changes the default column set and should only be used in local dev-builds...");
      #endif

      // load classification model
      if (!gStringsEqualIgnoreCase(ClassificationModelNameAndPath, "none"))
      {
        const TString DefaultClassificationModelPathAndName =
          gApplicationResourceDir().Descend("Models").Path() +
          MDefaultClassificationModelName;

        if (!ClassificationModelNameAndPath.IsEmpty() ||
          TFile(DefaultClassificationModelPathAndName).Exists())
        {
          TLog::SLog()->AddLine(MLogPrefix, "Loading classification model...");

          pAnalyzer->SetClassificationModel((ClassificationModelNameAndPath.IsEmpty()) ?
            DefaultClassificationModelPathAndName : ClassificationModelNameAndPath);

          // and save model's categories into db
          pSamplePool->InsertClassifier("Classifiers",
            pAnalyzer->ClassificationClasses());
        }
      }

      // load categorization model
      if (!gStringsEqualIgnoreCase(CategorizationModelNameAndPath, "none"))
      {
        const TString DefaultCategorizationModelPathAndName =
          gApplicationResourceDir().Descend("Models").Path() +
          MDefaultOneShotCategorizationModelName;

        if (!CategorizationModelNameAndPath.IsEmpty() ||
          TFile(DefaultCategorizationModelPathAndName).Exists())
        {
          TLog::SLog()->AddLine(MLogPrefix, "Loading categorization model...");

          pAnalyzer->SetOneShotCategorizationModel((CategorizationModelNameAndPath.IsEmpty()) ?
            DefaultCategorizationModelPathAndName : CategorizationModelNameAndPath);

          // and save model's categories into db
          pSamplePool->InsertClassifier("OneShot-Categories",
            pAnalyzer->OneShotCategorizationClasses());
        }
      }
    }

    // collect files
    TLog::SLog()->AddLine(MLogPrefix, "Collecting files...");

    TList<TString> AllAudioFiles;

    for (int i = 0; i < DirectoriesOrFiles.Size() && !sAbortProcessing; ++i)
    {
      TDirectory::TSymLinkRecursionTest RecursionTester;
      SCollectFiles(DirectoriesOrFiles[i], RecursionTester, AllAudioFiles);
    }

    // build change list
    TList<TString> AudioFilesToAdd;
    TList<TString> AudioFilesToRemove;
    if (!sAbortProcessing)
    {
      TLog::SLog()->AddLine(MLogPrefix, "Building change lists...");
      SBuildChangeList(AllAudioFiles, pSamplePool, AudioFilesToAdd, AudioFilesToRemove);
    }

    if (AudioFilesToAdd.IsEmpty() && AudioFilesToRemove.IsEmpty())
    {
      TLog::SLog()->AddLine(MLogPrefix, "Database content is up to date. Nothing to do.");
    }
    else
    {
      // analyze new files
      const int MaxThreads = (MaxAnalyzeThreads == -1) ? 
        TCpu::NumberOfConcurrentThreads() : MaxAnalyzeThreads;

      const int NumberOfAudioFilesToAdd = AudioFilesToAdd.Size();

      std::mutex SamplePoolLock;

      if (MaxThreads == 1)
      {
        for (int i = 0; i < NumberOfAudioFilesToAdd; ++i)
        {
          if (sAbortProcessing)
          {
            throw std::runtime_error("Analyzation aborted...");
          }

          const TString AudioFileToAdd = AudioFilesToAdd[i];

          TLog::SLog()->AddLine(MLogPrefix, "Analyzing '%s' (%d of %d)",
            AudioFileToAdd.StdCString().c_str(), i + 1, NumberOfAudioFilesToAdd);

          pAnalyzer->Extract(AudioFileToAdd, pSamplePool, SamplePoolLock);
        }
      }
      else
      {
        ctpl::thread_pool ThreadPool(
          MMin(MaxThreads, AudioFilesToAdd.Size()),
          AudioFilesToAdd.Size());

        std::list< std::future<void> > JobList;
        for (int i = 0; i < NumberOfAudioFilesToAdd; ++i)
        {
          const TString AudioFileToAdd = AudioFilesToAdd[i];

          JobList.push_back(ThreadPool.push(
            [=, &pAnalyzer, &pSamplePool, &SamplePoolLock](int _ThreadId) {
              if (sAbortProcessing)
              {
                throw std::runtime_error("Analyzation aborted...");
              }

              TLog::SLog()->AddLine(MLogPrefix, "Analyzing '%s' (%d of %d)",
                AudioFileToAdd.StdCString().c_str(), i + 1, NumberOfAudioFilesToAdd);

              pAnalyzer->Extract(AudioFileToAdd, pSamplePool, SamplePoolLock);
            }
          ));
        }

        // wait until all tasks completed
        while (! JobList.empty())
        {
          try
          {
            JobList.front().get();
          }
          catch (const std::exception&)
          {
            // stop thread pool, wait until its down and clear all tasks on errors
            ThreadPool.stop();
            JobList.clear();

            throw;
          }

          // remove successfully finished tasks
          JobList.pop_front();
        }

        // remove no longer existing files
        if (! AudioFilesToRemove.IsEmpty())
        {
          TLog::SLog()->AddLine(MLogPrefix, "Removing %d samples",
            AudioFilesToRemove.Size());

          pSamplePool->RemoveSamples(AudioFilesToRemove);
        }
      }
    }
  }
  catch (const std::exception& Exception)
  {
    if (sAbortProcessing)
    {
      TLog::SLog()->AddLine(MLogPrefix, "Crawler was aborted...");
      GotCrawlError = false;
    }
    else
    {
      TLog::SLog()->AddLine(MLogPrefix, "ERROR: Exception caught: %s", Exception.what());
      GotCrawlError = true;
    }
  }

  return (GotCrawlError) ? EXIT_FAILURE : EXIT_SUCCESS;
}

// -------------------------------------------------------------------------------------------------

bool SIgnoreRootDirectory(const TDirectory& BaseDirectory) 
{
  // ignored root directories (BaseDirectory "startsWith")
  static const wchar_t* const sIgnoredRootDirectories[] = { 
    #if defined(MLinux)
      L"bin",
      L"boot",
      L"dev",
      L"lost+found",
      L"run",
      L"sys",
      L"proc",
      L"sbin",
      L"var"
    #elif defined(MMac)
      L"bin",
      L"etc",
      L"dev",
      L"private",
      L"lost+found",
      L"sbin",
      L"usr",
      L"var" 
    #elif defined(MWindows)
      L"System Volume Information",
      L"$RECYCLE.BIN",
      L"PerfLogs",
      L"Recovery"
    #else
      #error "Unexpected platform"
    #endif
  };

  const TList<TString> PathComponents = BaseDirectory.SplitPathComponents();
  TString RootDirectory;
  if (PathComponents.Size() > 1) // skip drive letter on windows and / on Linux, OSX
  {
    RootDirectory = PathComponents[1];
  }

  if (! RootDirectory.IsEmpty()) 
  {
    for (size_t i = 0; i < MCountOf(sIgnoredRootDirectories); ++i) 
    {
      // NB: be case insensitive here on all platforms
      if (gStringsEqualIgnoreCase(RootDirectory, sIgnoredRootDirectories[i]))
      {
        return true;
      }
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

bool SIgnoreSubDirectory(const TString& SubDirName) 
{
  // check for ignored sub directories
  static const wchar_t* const sIgnoredSubDirectories[] = { 
    L"__MACOSX",
    L".git",
    L".hg",
    L".svn",
    L".npm",
    L".cache",
    L"CVS"
  };

  for (size_t i = 0; i < MCountOf(sIgnoredSubDirectories); ++i) 
  {
    // NB: be case insensitive here on all platforms
    if (gStringsEqualIgnoreCase(SubDirName, TString(sIgnoredSubDirectories[i]))) 
    {
      return true;
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

bool SIgnoreFile(const TString& Filename) 
{
  // ignore OSX resource fork data files
  return Filename.StartsWith("._");
}

// -------------------------------------------------------------------------------------------------

void SCollectFiles(
  const TString&                      DirectoryOrFileName,
  TDirectory::TSymLinkRecursionTest&  RecursionTester,
  TList<TString>&                     DestAudioFiles)
{
  // check if indexing got aborted
  if (sAbortProcessing) 
  {
    return;
  }
  
  if (TDirectory(DirectoryOrFileName).Exists()) // is directory?
  {
    const TDirectory Directory(DirectoryOrFileName);

    // check for ignored root directories
    if (SIgnoreRootDirectory(Directory))
    {
      TLog::SLog()->AddLine(MLogPrefix,
        "Ignoring contents of directory: '%s'", Directory.Path().StdCString().c_str());

      return;
    }

    // analyze all audio files within this path
    const TList<TString> AudioFileNames = Directory.FindFileNames(
      TAudioFile::SSupportedExtensions());
    for (int i = 0; i < AudioFileNames.Size() && !sAbortProcessing; ++i)
    {
      // check for ignored files
      if (!SIgnoreFile(AudioFileNames[i]))
      {
        DestAudioFiles.Append(Directory.Path() + AudioFileNames[i]);
      }
    }

    // collect recursively within all sub paths
    const TList<TString> SubDirNames = Directory.FindSubDirNames("*", &RecursionTester);
    for (int i = 0; i < SubDirNames.Size() && !sAbortProcessing; ++i)
    {
      const TDirectory FullSubDir = TDirectory(Directory).Descend(SubDirNames[i]);

      // check for ignored sub folders
      if (SIgnoreSubDirectory(SubDirNames[i]))
      {
        TLog::SLog()->AddLine(MLogPrefix,
          "Ignoring contents of (sub)directory: '%s'", FullSubDir.Path().StdCString().c_str());

        continue;
      }
      
      SCollectFiles(FullSubDir.Path(), RecursionTester, DestAudioFiles);
    }
  }
  else if (TFile(DirectoryOrFileName).Exists()) // is a file?
  {
    DestAudioFiles.Append(DirectoryOrFileName);
  }
}

// -------------------------------------------------------------------------------------------------

static void SBuildChangeList(
  const TList<TString>&               AudioFiles,
  TSqliteSampleDescriptorPool*        pSamplePool,
  TList<TString>&                     AudioFilesToAdd,
  TList<TString>&                     AudioFilesToRemove)
{
  // . create new content

  if (pSamplePool->IsEmpty())
  {
    AudioFilesToAdd = AudioFiles;
    AudioFilesToRemove.Empty();

    return;
  }


  // . update existing content

  AudioFilesToAdd.Empty();
  AudioFilesToRemove.Empty();

  // check existing samples in db
  const TList< TPair<TString, int> > ExistingFiles =
    pSamplePool->SampleModificationDates();

  std::set<TString> ExistingSamplesMap;

  for (int i = 0; i < ExistingFiles.Size(); ++i)
  {
    const TString AbsFilename = ExistingFiles[i].First();

    // add normalized paths to cache
    ExistingSamplesMap.insert(TString(AbsFilename).ReplaceChar(
      TDirectory::SWrongPathSeparatorChar(), TDirectory::SPathSeparatorChar()));

    if (!TFile(AbsFilename).Exists())
    {
      AudioFilesToRemove.Append(AbsFilename);
    }
    else // file in db still exists
    {
      // does it need to be refreshed?
      const int ActualStatTime = TFile(AbsFilename).ModificationStatTime();
      const int StoredStatTime = ExistingFiles[i].Second();
      if (ActualStatTime > StoredStatTime)
      {
        AudioFilesToAdd.Append(AbsFilename);
      }
    }
  }

  // insert new samples to db
  for (int i = 0; i < AudioFiles.Size(); ++i)
  {
    // always compare normalized paths: different slashes may be used
    const TString NormalizedPath = TString(AudioFiles[i]).ReplaceChar(
      TDirectory::SWrongPathSeparatorChar(), TDirectory::SPathSeparatorChar());

    if (ExistingSamplesMap.find(NormalizedPath) == ExistingSamplesMap.end())
    {
      AudioFilesToAdd.Append(NormalizedPath);
    }
  }
}

