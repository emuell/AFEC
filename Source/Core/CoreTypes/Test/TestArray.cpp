#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"

#include "CoreTypes/Test/TestArray.h"
#include "CoreTypes/Export/Array.h"
#include "CoreTypes/Export/InlineMath.h"

#include <algorithm>  // TArray::Sort

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreTypesTest::Array()
{
  // ... MakeArray

  TArray<int> Array0123 = MakeArray<int>(0, 1, 2, 3);
  
  BOOST_CHECK_EQUAL(Array0123[0], 0);
  BOOST_CHECK_EQUAL(Array0123[1], 1);
  BOOST_CHECK_EQUAL(Array0123[2], 2);
  BOOST_CHECK_EQUAL(Array0123[3], 3);

  
  // ... Iterator Access

  for (TArray<int>::TConstIterator Iter = Array0123.Begin(); !Iter.IsEnd(); ++Iter)
  {
    BOOST_CHECK_EQUAL(Array0123[Iter.Index()], Iter.Index());
  }


  // ... Grow
  
  Array0123.Grow(5);
  BOOST_CHECK_EQUAL(Array0123.Size(), 5);
  
  BOOST_CHECK_EQUAL(Array0123[0], 0);
  BOOST_CHECK_EQUAL(Array0123[1], 1);
  BOOST_CHECK_EQUAL(Array0123[2], 2);
  BOOST_CHECK_EQUAL(Array0123[3], 3);
  // BOOST_CHECK_EQUAL(Array0123[4], ??) uninitialized

  BOOST_CHECK(! Array0123.IsEmpty());

  Array0123.Empty();
  BOOST_CHECK_EQUAL(Array0123.Size(), 0);
  BOOST_CHECK(Array0123.IsEmpty());
  

  // ... Sort & Copy

  TArray<int> SortedArray = MakeArray<int>(2, 3, 3, 5, 8, 10);
  TArray<int> UnsortedArray(SortedArray);

  for (int i = 0; i < 10; ++i)
  {
    UnsortedArray.Swap(
      TRandom::Integer(SortedArray.Size() - 1), 
      TRandom::Integer(SortedArray.Size() - 1));
  }
  
  BOOST_CHECK_EQUAL(UnsortedArray.Sort(), SortedArray);


  // ... Create/copy TStaticArrays from a C-Array

  const int TestCArray[4] = {3,2,1,0};
  TStaticArray<int, 4> TestFromCArray(TestCArray);
  BOOST_CHECK_EQUAL(TestFromCArray[0], 3);
  BOOST_CHECK_EQUAL(TestFromCArray[1], 2);
  BOOST_CHECK_EQUAL(TestFromCArray[2], 1);
  BOOST_CHECK_EQUAL(TestFromCArray[3], 0);

  
  // ... Copy TStaticArrays from different, but convertible types

  TStaticArray<int, 4> TestArray1;
  TestArray1[0] = 0;
  TestArray1[1] = 1;
  TestArray1[2] = 2;
  TestArray1[3] = 3;

  TStaticArray<double, 4> TestArray2;
  TestArray2 = TestArray1;

  BOOST_CHECK_EQUAL(TestArray2[0], 0.0);
  BOOST_CHECK_EQUAL(TestArray2[1], 1.0);
  BOOST_CHECK_EQUAL(TestArray2[2], 2.0);
  BOOST_CHECK_EQUAL(TestArray2[3], 3.0);

  TStaticArray<double, 4> TestArray3(TestArray1);
  BOOST_CHECK_EQUAL(TestArray3[0], 0.0);
  BOOST_CHECK_EQUAL(TestArray3[1], 1.0);
  BOOST_CHECK_EQUAL(TestArray3[2], 2.0);
  BOOST_CHECK_EQUAL(TestArray3[3], 3.0);


  // ... Copying TArrays from different, but convertible types

  TArray<const char*> TestDynArray1;
  TestDynArray1.SetSize(2);
  TestDynArray1[0] = "Bla";
  TestDynArray1[1] = "Wurst";

  TArray<TString> TestDynArray2;
  TestDynArray2 = TestDynArray1;

  BOOST_CHECK_EQUAL(TestDynArray2.Size(), 2);
  BOOST_CHECK_EQUAL(TestDynArray2[0], "Bla");
  BOOST_CHECK_EQUAL(TestDynArray2[1], "Wurst");

  TArray<TString> TestDynArray3(TestDynArray1);
  BOOST_CHECK_EQUAL(TestDynArray3.Size(), 2);
  BOOST_CHECK_EQUAL(TestDynArray3[0], "Bla");
  BOOST_CHECK_EQUAL(TestDynArray3[1], "Wurst");
}

