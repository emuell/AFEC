#pragma once

#ifndef _ReferenceCountable_h_
#define _ReferenceCountable_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"
#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/Memory.h" // MDeclareDebugNewAndDelete

#include <atomic>

// =================================================================================================

/*!
 * Stores and manages an object's reference counter for TPtr<>. If an object's
 * reference count reaches zero, the object will be destroyed via the global
 * delete operator. 
!*/

class TReferenceCountable
{
public:
  TReferenceCountable();
  virtual ~TReferenceCountable();

  //! Preserves refcounts (copies nothing).
  TReferenceCountable(const TReferenceCountable& Other);
  TReferenceCountable& operator =(const TReferenceCountable& Other);
  
  
  //!@{ ... Reference counting

  //! return how many object have references to this object
  //! (-1 while we are about to die - no one refs the object then anymore)
  int RefCount()const;

  //! Establish ownership and prevent deletion.
  void IncRef();

  //! Release ownership. The object gets deleted as soon as the refcount 
  //! reaches zero.
  void DecRef();
  //!@}
  

  //!@{ ... new/delete

  MDeclareDebugNewAndDelete
  //!@}

protected:
  //! Called right before we are getting dec-refed to death. 
  //! return true if the global "delete" operator should be called, which 
  //! is the default. 
  //! When returning false, the object is no longer valid, RefCount() is -1, 
  //! and you have take care about deleting the object by your own.
  virtual bool OnPrepareDestruction() { return true; }

private:
  //! Deals with "OnPrepareDestruction", then deletes "this" or sends a 
  //! command to the GUI to do so (see \function OnDeleteInGui).
  //! private: clients should use \function DecRef instead.
  void Delete();

  std::atomic<int> mRefCount;
};  

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline int TReferenceCountable::RefCount()const        
{
  return mRefCount; 
} 

// -------------------------------------------------------------------------------------------------

MForceInline void TReferenceCountable::IncRef()
{
  MAssert(mRefCount >= 0, "Trying to incref a deleted object!");
  ++mRefCount; 
}

// -------------------------------------------------------------------------------------------------

MForceInline void TReferenceCountable::DecRef()
{
  MAssert(mRefCount > 0, "Trying to decref a non increfed object!");

  if (--mRefCount == 0)
  {
    Delete();
  }
}


#endif  // _ReferenceCountable_h_

