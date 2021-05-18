#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"
#include "CoreTypes/Export/WeakReferenceable.h"
#include "CoreTypes/Export/Pointer.h"
#include "CoreTypes/Test/TestWeakRef.h"

// =================================================================================================

class TWeakObject : public TWeakReferenceable
{
public:
  TWeakObject(int Number) 
    : mNumber(Number) { }
    
  operator int()const { return mNumber; }
  
private:
  int mNumber;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreTypesTest::WeakRef()
{
  // ... Test dying TWeakReferenceables

  TWeakPtr<TWeakObject> WeakPtr;
  {
    TWeakObject Object(1);
    WeakPtr = &Object;
    BOOST_CHECK(WeakPtr == &Object);
  }

  BOOST_CHECK(WeakPtr == NULL);


  // ... Test TWeakReferenceable copy constructors

  TWeakPtr<TWeakObject> WeakPtr2;
  TWeakObject* pObservable1 = new TWeakObject(1);
  TWeakObject* pObservable2 = new TWeakObject(2);

  WeakPtr2 = pObservable1;
  *pObservable1 = *pObservable2;

  BOOST_CHECK(WeakPtr2 == pObservable1);


  // ... Test TWeakPtr operators

  WeakPtr = WeakPtr2;
  BOOST_CHECK(WeakPtr == WeakPtr2);
  BOOST_CHECK_EQUAL(*WeakPtr, 2);
  BOOST_CHECK_EQUAL(*WeakPtr2, 2);

  delete pObservable1;
  delete pObservable2;

  BOOST_CHECK(WeakPtr == NULL);
  BOOST_CHECK(WeakPtr2 == NULL);
}

