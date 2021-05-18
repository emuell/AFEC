#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"
#include "CoreTypes/Export/ProductVersion.h"

#include "CoreTypes/Test/TestProductVersion.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreTypesTest::ProductVersion()
{
  // . test comparison

  BOOST_CHECK(TProductVersion(6,7,8, "b1") == TProductVersion(6,7,8, "b1"));
  BOOST_CHECK(TProductVersion(6,7,8, "b1") != TProductVersion(6,7,8, "b2"));
  BOOST_CHECK(TProductVersion(6,7,8, "b1") != TProductVersion(6,7,8));

  BOOST_CHECK(TProductVersion(1,0,0) > TProductVersion(0,9,0));
  BOOST_CHECK(TProductVersion(2,6,9) >= TProductVersion(2,6,9));
  BOOST_CHECK(TProductVersion(2,6,9) <= TProductVersion(2,6,9));
  BOOST_CHECK(TProductVersion(0,9,0) < TProductVersion(0,9,9));

  BOOST_CHECK(TProductVersion(1,2,5) > TProductVersion(1,2,5, "rc1"));
  BOOST_CHECK(TProductVersion(1,2,5) > TProductVersion(1,2,5, "b8"));
  BOOST_CHECK(TProductVersion(1,2,5, "rc2") >= TProductVersion(1,2,5, "rc2"));
  BOOST_CHECK(TProductVersion(1,2,5, "rc2") > TProductVersion(1,2,5, "rc1"));
  BOOST_CHECK(TProductVersion(1,2,5, "rc1") > TProductVersion(1,2,5, "b6"));
  BOOST_CHECK(TProductVersion(1,2,5, "b2") > TProductVersion(1,2,5, "a1"));
  BOOST_CHECK(TProductVersion(1,2,5, "a1") <= TProductVersion(1,2,5, "a2"));
 

  // . test serialization

  TProductVersion Version1(5,3,0);
  TString VersionString = ToString(Version1);
  BOOST_CHECK_EQUAL(VersionString, "V5.3.0");

  TProductVersion Version2;
  BOOST_CHECK(StringToValue(Version2, VersionString));
  BOOST_CHECK_EQUAL(Version1, Version2);

  Version1 = TProductVersion(1,2,5, "a1");
  VersionString = ToString(Version1);
  BOOST_CHECK_EQUAL(VersionString, "V1.2.5 a1");

  BOOST_CHECK(StringToValue(Version2, VersionString));
  BOOST_CHECK_EQUAL(Version1, Version2);

  BOOST_CHECK(! StringToValue(Version2, "V222 rc1"));
  BOOST_CHECK(! StringToValue(Version2, "V222"));
  BOOST_CHECK(! StringToValue(Version2, "V2.2"));

  BOOST_CHECK_EQUAL(ToString(TProductVersion()), "");
  BOOST_CHECK(StringToValue(Version2, ""));
  BOOST_CHECK_EQUAL(Version2, TProductVersion());
}

