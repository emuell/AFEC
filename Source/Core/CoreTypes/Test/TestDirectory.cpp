#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"
#include "CoreTypes/Export/Str.h"

#include "CoreTypes/Test/TestDirectory.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreTypesTest::Directory()
{
  // . test exists

  #if !defined(MWindows)
    BOOST_CHECK(gApplicationDir().Exists());
  #endif

  BOOST_CHECK(gApplicationDir().ExistsIgnoreCase());


  // . test leading, trailing MPathSeperators

  TDirectory Trailing = gApplicationDir().Path().RemoveLast(TDirectory::SPathSeparator());
  BOOST_CHECK_EQUAL(Trailing, gApplicationDir());

  TDirectory Leading = gApplicationDir().Path() + TDirectory::SPathSeparator();
  BOOST_CHECK_EQUAL(Trailing, gApplicationDir());
  
  TDirectory Leading4 = gApplicationDir().Path() + 4*TDirectory::SPathSeparator();
  BOOST_CHECK_EQUAL(Leading4, gApplicationDir());


  // . test ascend ParentDirCount
  
  const TString AppDirRoot = gApplicationDir().Path().SplitAt(
    TDirectory::SPathSeparatorChar()).First() + TDirectory::SPathSeparator();
  
  BOOST_CHECK_EQUAL(TDirectory(AppDirRoot).ParentDirCount(), 0);
  BOOST_CHECK_NE(gApplicationDir().ParentDirCount(), 0);
  
  TDirectory AscentDir(gApplicationDir());
  int AscentCount = 0;

  while (AscentDir.ParentDirCount() > 0)
  {
    const TDirectory Previous = AscentDir;
    AscentDir.Ascend();
    ++AscentCount;

    #if !defined(MWindows)
      BOOST_CHECK(AscentDir.Exists());
    #endif

    BOOST_CHECK(AscentDir.ExistsIgnoreCase());

    BOOST_CHECK(AscentDir.IsParentDirOf(Previous));
    BOOST_CHECK(Previous.IsSubDirOf(AscentDir));
  }

  BOOST_CHECK_EQUAL(AscentDir.Path(), AppDirRoot);
  BOOST_CHECK_EQUAL(AscentCount, gApplicationDir().ParentDirCount());
  BOOST_CHECK_EQUAL(AscentDir.ParentDirCount(), 0);


  // . test descend and SubDir Find

  TDirectory DescendDir = AppDirRoot;
  int DescentCount = 0;

  while (!DescendDir.FindSubDirNames().IsEmpty())
  {
    const TDirectory Previous = DescendDir;
    DescendDir.Descend(DescendDir.FindSubDirNames().First());
    ++DescentCount;

    #if !defined(MWindows)
      BOOST_CHECK(DescendDir.Exists());
    #endif

    BOOST_CHECK(DescendDir.ExistsIgnoreCase());

    BOOST_CHECK(Previous.IsParentDirOf(DescendDir));
    BOOST_CHECK(DescendDir.IsSubDirOf(Previous));
  }

  BOOST_CHECK_EQUAL(DescentCount, DescendDir.ParentDirCount());
  

  // . test create and unlink

  BOOST_CHECK(gTempDir().Descend("CreateTest1").Create());
  BOOST_CHECK(gTempDir().Descend("CreateTest1").Exists());
  BOOST_CHECK(gTempDir().Descend("CreateTest1").Unlink());
  BOOST_CHECK(! gTempDir().Descend("CreateTest1").Exists());

  bool CreateParentDirs = false;
  BOOST_CHECK(! gTempDir().Descend("CreateTest2").Descend("CreateTest21").Create(CreateParentDirs));
  BOOST_CHECK(! gTempDir().Descend("CreateTest2").Exists());

  CreateParentDirs = true;
  BOOST_CHECK(gTempDir().Descend("CreateTest3").Descend("CreateTest31").Create(CreateParentDirs));
  BOOST_CHECK(gTempDir().Descend("CreateTest3").Exists());
  BOOST_CHECK(gTempDir().Descend("CreateTest3").Descend("CreateTest31").Exists());
  BOOST_CHECK(gTempDir().Descend("CreateTest3").Unlink());
  BOOST_CHECK(! gTempDir().Descend("CreateTest3").Exists());


  // . test normalization

  BOOST_CHECK_EQUAL(TDirectory("/Olla").Descend(".."), TDirectory("/"));

  BOOST_CHECK_EQUAL(TDirectory("C:/Schmu/.."), TDirectory("C:"));
  BOOST_CHECK_EQUAL(TDirectory("C:/Schmu/Olla/../../"), TDirectory("C:/"));
  BOOST_CHECK_EQUAL(TDirectory("C:/Schmu/../Olla/../Womb"), TDirectory("C:/Womb"));
  BOOST_CHECK_EQUAL(TDirectory("C:/Schmu/Olla/../../Womb"), TDirectory("C:/Womb"));
  BOOST_CHECK_EQUAL(TDirectory("/Olla/.."), TDirectory("/"));
}

