#include "CoreTypes/Export/TestHelpers.h"
#include "CoreTypes/Export/Version.h"
#include "CoreTypes/Export/Directory.h"
#include "CoreTypes/Export/System.h"
#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/Exception.h"

#include "CoreTypes/Test/TestAlloca.h"
#include "CoreTypes/Test/TestArray.h"
#include "CoreTypes/Test/TestList.h"
#include "CoreTypes/Test/TestInlineMath.h"
#include "CoreTypes/Test/TestWeakRef.h"
#include "CoreTypes/Test/TestString.h"
#include "CoreTypes/Test/TestStringConverter.h"
#include "CoreTypes/Test/TestDirectory.h"
#include "CoreTypes/Test/TestMemory.h"
#include "CoreTypes/Test/TestProductVersion.h"

#include "AudioTypes/Export/AudioTypesInit.h"
#include "AudioTypes/Test/TestFourier.h"

#include "CoreFileFormats/Export/CoreFileFormatsInit.h"
#include "CoreFileFormats/Test/TestZipFile.h"
#include "CoreFileFormats/Test/TestZipArchive.h"
#include "CoreFileFormats/Test/TestAudioFile.h"
#include "CoreFileFormats/Test/TestDatabase.h"

#include "../../3rdParty/Boost/Export/BoostUnitTest.h"

#include <boost/cstdlib.hpp>
#include <iostream>

#if defined(MCompiler_VisualCPP)
  // unreachable code (in the exception handlers)
  #pragma warning(disable: 4702)
#endif

// =================================================================================================

namespace TProductDescription 
{ 
  TString ProductName() { return "UnitTests"; }
  TString ProductVendorName() { return "ACME"; }
  TString ProductProjectsLocation() { return "Core/XCoreUnitTests"; }

  int MajorVersion() { return 0; }
  int MinorVersion() { return 1; }
  int RevisionVersion() { return 0; }
  
  TString AlphaOrBetaVersionString() { return ""; }
  TDate ExpirationDate() { return TDate(); }
  
  TString BugReportEMailAddress(){ return "<bug@acme.com>"; }
  TString SupportEMailAddress(){ return "<support@acme.com>"; }
  TString ProductHomeURL(){ return "http://www.acme.com"; }
  
  TString CopyrightString() { return ""; }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

boost::unit_test::test_suite* RegisterUnitTests(int, char*[])
{
  // give master a name for the reports
  boost::unit_test::framework::master_test_suite().p_name.value = "Core";


  // . CoreTypes

  boost::unit_test::test_suite* pCoreTypesTest = BOOST_TEST_SUITE("CoreTypes");
  {
    pCoreTypesTest->add(BOOST_TEST_CASE([]() {
      BOOST_TEST_MESSAGE("  Testing CoreTypes..."); BOOST_CHECK(true);
    }));

    pCoreTypesTest->add(BOOST_TEST_CASE(TCoreTypesTest::Alloca));
    pCoreTypesTest->add(BOOST_TEST_CASE(TCoreTypesTest::Array));
    pCoreTypesTest->add(BOOST_TEST_CASE(TCoreTypesTest::List));
    pCoreTypesTest->add(BOOST_TEST_CASE(TCoreTypesTest::InlineMath));
    pCoreTypesTest->add(BOOST_TEST_CASE(TCoreTypesTest::WeakRef));
    pCoreTypesTest->add(BOOST_TEST_CASE(TCoreTypesTest::String));
    pCoreTypesTest->add(BOOST_TEST_CASE(TCoreTypesTest::StringConverter));
    pCoreTypesTest->add(BOOST_TEST_CASE(TCoreTypesTest::Directory));
    pCoreTypesTest->add(BOOST_TEST_CASE(TCoreTypesTest::Memory));
    pCoreTypesTest->add(BOOST_TEST_CASE(TCoreTypesTest::ProductVersion));
  }
  boost::unit_test::framework::master_test_suite().add(pCoreTypesTest);


  // . AudioTypes

  boost::unit_test::test_suite* pAudioTypesTests = BOOST_TEST_SUITE("AudioTypes");
  {
    pAudioTypesTests->add(BOOST_TEST_CASE([]() {
      BOOST_TEST_MESSAGE("  Testing AudioTypes..."); BOOST_CHECK(true);
    }));

    pAudioTypesTests->add(BOOST_TEST_CASE(TAudioTypesTest::Fourier));
  }
  boost::unit_test::framework::master_test_suite().add(pAudioTypesTests);


  // . CoreFileFormats

  boost::unit_test::test_suite* pCoreFileFormatsTests = BOOST_TEST_SUITE("CoreFileFormats");
  {
    pCoreFileFormatsTests->add(BOOST_TEST_CASE([]() {
      BOOST_TEST_MESSAGE("  Testing CoreFileFormats..."); BOOST_CHECK(true);
    }));

    pCoreFileFormatsTests->add(BOOST_TEST_CASE(TCoreFileFormatsTest::ZipFile));
    pCoreFileFormatsTests->add(BOOST_TEST_CASE(TCoreFileFormatsTest::ZipArchive));
    pCoreFileFormatsTests->add(BOOST_TEST_CASE(TCoreFileFormatsTest::AudioFile));
    pCoreFileFormatsTests->add(BOOST_TEST_CASE(TCoreFileFormatsTest::Database));
  }
  boost::unit_test::framework::master_test_suite().add(pCoreFileFormatsTests);

  return 0;
}

// =================================================================================================

#include "CoreTypes/Export/MainEntry.h"

// -------------------------------------------------------------------------------------------------

int gMain(const TList<TString>& Arguments)
{
  // ... Init

  // Don't want to dump info to the std out when running tests
  const TSystem::TSuppressInfoMessages SupressInfos;

  // Hack: Disable FPU exceptions for the test runner in linux builds, 
  // cause we can not reliably turn them off there...
  #if defined(MLinux)
    gEnableFloatingPointExceptions = false;
  #endif

  try
  {
    // don't mix up our output with the regular log output...
    TLog::SLog()->SetTraceLogContents(false);

    AudioTypesInit();
    CoreFileFormatsInit();
  }
  catch (const TReadableException& Exception)
  {
    std::cerr << Exception.Message().CString();
    return boost::exit_exception_failure;
  }


  // ... Run Tests

  const int ExitCode = boost::unit_test::unit_test_main(
    RegisterUnitTests, gArgc, gArgv);


  // ... Finalize 

  try
  {
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

