#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"

#include "CoreTypes/Test/TestList.h"
#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/InlineMath.h"

#include <algorithm>  // TList::Sort

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreTypesTest::List()
{
  // ... Append/Prepend

  TList<int> List12;
  List12.Append(1);
  List12.Append(2);
  
  TList<int> List0123;
  List0123.Append(2);
  List0123.Append(3);
  List0123.Prepend(1);
  List0123.Prepend(0);         

  BOOST_CHECK_EQUAL(List0123.Size(), 4);
  
  BOOST_CHECK_EQUAL(List0123[0], 0);
  BOOST_CHECK_EQUAL(List0123[1], 1);
  BOOST_CHECK_EQUAL(List0123[2], 2);
  BOOST_CHECK_EQUAL(List0123[3], 3);


  // ... Reverse

  List0123.Reverse();      

  BOOST_CHECK_EQUAL(List0123[0], 3);
  BOOST_CHECK_EQUAL(List0123[1], 2);
  BOOST_CHECK_EQUAL(List0123[2], 1);
  BOOST_CHECK_EQUAL(List0123[3], 0);

  List0123.Reverse();
  
  
  // ... Iterator Access

  for (TList<int>::TConstIterator Iter = List0123.Begin(); !Iter.IsEnd(); ++Iter)
  {
    BOOST_TEST_CONTEXT("index " << Iter.Index())
    BOOST_CHECK_EQUAL(List0123[Iter.Index()], Iter.Index());
  }
  
  
  // ... Delete Range / Insert List

  List0123.DeleteRange(1, 1);
  List0123.DeleteRange(1, 1);
               
  BOOST_CHECK_EQUAL(List0123[0], 0);
  BOOST_CHECK_EQUAL(List0123[1], 3);
  
  List0123.Insert(List12, 1);

  BOOST_CHECK_EQUAL(List0123[0], 0);
  BOOST_CHECK_EQUAL(List0123[1], 1);
  BOOST_CHECK_EQUAL(List0123[2], 2);
  BOOST_CHECK_EQUAL(List0123[3], 3);

  List0123.DeleteRange(0, 3);
  BOOST_CHECK_EQUAL(List0123.Size(), 0);
  
  List0123.Insert(List12, 0);
  List0123.Insert(0, 0);
  List0123.Insert(3, 3);

  BOOST_CHECK_EQUAL(List0123[0], 0);
  BOOST_CHECK_EQUAL(List0123[1], 1);
  BOOST_CHECK_EQUAL(List0123[2], 2);
  BOOST_CHECK_EQUAL(List0123[3], 3);
  
  
  // ... IndexOf
  
  BOOST_CHECK_EQUAL(List0123.IndexOf(List0123[0]), 0);
  BOOST_CHECK_EQUAL(List0123.IndexOf(List0123[1]), 1);
  BOOST_CHECK_EQUAL(List0123.IndexOf(List0123[2]), 2);
  BOOST_CHECK_EQUAL(List0123.IndexOf(List0123[3]), 3);

  
  // ... Sort / LowerBound / Binary Search

  TList<int> SortedList = MakeList<int>(2, 3, 3, 5, 8, 10, 20);

  TList<int> UnsortedList(SortedList);

  for (int i = 0; i < 10; ++i)
  {
    UnsortedList.Swap(
      TRandom::Integer(SortedList.Size() - 1), 
      TRandom::Integer(SortedList.Size() - 1));
  }
  
  BOOST_CHECK_EQUAL(UnsortedList.Sort(), SortedList);

  BOOST_CHECK_EQUAL(SortedList.FindLowerBound(1), 0);
  BOOST_CHECK_EQUAL(SortedList.FindLowerBound(3), 1);
  BOOST_CHECK_EQUAL(SortedList.FindLowerBound(4), 3);
  BOOST_CHECK_EQUAL(SortedList.FindLowerBound(100), 7);
  
  BOOST_CHECK_EQUAL(TList<int>().FindLowerBound(3), 0);
  BOOST_CHECK(TList<int>().FindLowerBoundIter(-1).IsEnd());

  BOOST_CHECK_EQUAL(SortedList.FindIterWithBinarySearch(3).Index(), 1);
  BOOST_CHECK_EQUAL(SortedList.FindWithBinarySearch(10), 5);
  
  BOOST_CHECK(TList<int>().FindIterWithBinarySearch(-1).IsEnd());
  BOOST_CHECK(TList<int>().FindWithBinarySearch(999) == -1);


  // ... InternalCopy (TArrayInitializer<T>::SInitialize use)

  TList<TString> StringListA = MakeList<TString>("a1", "a2", "a3");
  StringListA.DeleteLast(); // keep two entries only, but space for 3

  TList<TString> StringListB = MakeList<TString>("b1", "b2", "b3");

  BOOST_CHECK(StringListA.PreallocatedSpace() == 3);
  BOOST_CHECK(StringListB.PreallocatedSpace() == 3);

  // StringListB will not allocate a new buffer here, but 
  // must clear its last item:
  StringListB = StringListA;

  BOOST_CHECK(StringListB.PreallocatedSpace() == 3);

  BOOST_CHECK(*(StringListB.FirstRead() + 0) == "a1");
  BOOST_CHECK(*(StringListB.FirstRead() + 1) == "a2");
  BOOST_CHECK(*(StringListB.FirstRead() + 2) == "");


  // ... Copying/converting lists from other lists with convertible types

  TList<const char*> CStringList = MakeList<const char*>("1", "2", "3");
  TList<TString> TStringList(CStringList);
  TStringList = CStringList;

  BOOST_CHECK(TStringList.Size() == 3);

  BOOST_CHECK_EQUAL(TStringList[0], "1");
  BOOST_CHECK_EQUAL(TStringList[1], "2");
  BOOST_CHECK_EQUAL(TStringList[2], "3");
}

