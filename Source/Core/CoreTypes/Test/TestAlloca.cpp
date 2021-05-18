#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"

#include "CoreTypes/Test/TestAlloca.h"
#include "CoreTypes/Export/Alloca.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreTypesTest::Alloca()
{
  // ... TAllocaList

  TAllocaList<int> AllocaList12(2); 
  MInitAllocaList(AllocaList12);
  
  BOOST_CHECK_EQUAL(AllocaList12.MaximumSize(), 2);
  
  AllocaList12.Append(1);
  AllocaList12.Append(2);
  
  TAllocaList<int> AllocaList0123(4);
  MInitAllocaList(AllocaList0123);
  
  BOOST_CHECK_EQUAL(AllocaList0123.MaximumSize(), 4);

  AllocaList0123.Append(2);
  AllocaList0123.Append(3);
  AllocaList0123.Prepend(1);
  AllocaList0123.Prepend(0);         

  BOOST_CHECK_EQUAL(AllocaList0123.Size(), 4);
  BOOST_CHECK_EQUAL(AllocaList0123[0], 0);
  BOOST_CHECK_EQUAL(AllocaList0123[1], 1);
  BOOST_CHECK_EQUAL(AllocaList0123[2], 2);
  BOOST_CHECK_EQUAL(AllocaList0123[3], 3);

  // empty alloca lists are allowed as well
  TAllocaList<int> EmptyAllocaList(0);
  MInitAllocaList(EmptyAllocaList);

  BOOST_CHECK(EmptyAllocaList.IsEmpty());

  // all const methods should work without asserting
  BOOST_CHECK_EQUAL(EmptyAllocaList.MaximumSize(), 0);
  BOOST_CHECK_EQUAL(EmptyAllocaList.Size(), 0);
  BOOST_CHECK_EQUAL(EmptyAllocaList.Find(0), -1);


  // ... TAllocaArray

  TAllocaArray<double> AllocaArray(12);
  BOOST_CHECK_EQUAL(AllocaArray.Size(), 12);
  MInitAllocaArray(AllocaArray);
  BOOST_CHECK_EQUAL(AllocaArray.Size(), 12);

  AllocaArray.Init(55.0);
  BOOST_CHECK_EQUAL(AllocaArray[TRandom::Integer(AllocaArray.Size())], 55.0);

  TAllocaArray<double> EmptyAllocaArray(0);
  MInitAllocaArray(EmptyAllocaArray);

  char ForceUnalignedStack = 1; MUnused(ForceUnalignedStack);

  TAllocaArray<double> AlignedAllocaArray1(1, 16);
  MInitAllocaArray(AlignedAllocaArray1);
  BOOST_CHECK(((ptrdiff_t)AlignedAllocaArray1.FirstWrite() & 15) == 0);

  TAllocaArray<double> AlignedAllocaArray2(1, 32);
  MInitAllocaArray(AlignedAllocaArray2);
  BOOST_CHECK(((ptrdiff_t)AlignedAllocaArray2.FirstWrite() & 31) == 0);
  
  TAllocaArray<double> AlignedAllocaArray3(1, 128);
  MInitAllocaArray(AlignedAllocaArray3);
  BOOST_CHECK(((ptrdiff_t)AlignedAllocaArray3.FirstWrite() & 127) == 0);
}

