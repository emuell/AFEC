#pragma once

#ifndef _BoostUnitTest_h_
#define _BoostUnitTest_h_

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("BoostUnitTest")
  #define BOOST_ALL_NO_LIB
#endif

// =================================================================================================

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#endif // _BoostUnitTest_h_

