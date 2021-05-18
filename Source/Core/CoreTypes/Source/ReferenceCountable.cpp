#include "CoreTypesPrecompiledHeader.h"

// =================================================================================================

MImplementDebugNewAndDelete(TReferenceCountable)

// -------------------------------------------------------------------------------------------------

TReferenceCountable::TReferenceCountable()
  : mRefCount(0)
{              
}

TReferenceCountable::TReferenceCountable(const TReferenceCountable& Other)
  : mRefCount(0)
{
  // never copy a refcount...
  MUnused(Other);
}

// -------------------------------------------------------------------------------------------------

TReferenceCountable::~TReferenceCountable()
{
  MAssert(mRefCount == 0 || mRefCount == -1, 
    "Object is still beeing referenced!");
}

// -------------------------------------------------------------------------------------------------

TReferenceCountable& TReferenceCountable::operator=(
  const TReferenceCountable& Other)
{
  // never copy a refcount...
  MUnused(Other);

  return *this;
}

// -------------------------------------------------------------------------------------------------

void TReferenceCountable::Delete()
{
  MAssert(mRefCount == 0, 
    "Trying to delete an object which is still being referenced!");

  mRefCount = -1;  // mark as "dead"
      
  // When OnPrepareDestruction returns false and is an observer, the object 
  // has pending thread notifications. TObservable will then take care of 
  // deleting it as soon as it received its last command.

  if (OnPrepareDestruction())
  {
    delete this;
  }
}

