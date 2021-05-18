#pragma once

#ifndef _WeakReferenceable_h_
#define _WeakReferenceable_h_

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/Debug.h"

// =================================================================================================

/* 
 * Interface to something a WeakPtr can point to.
 * As soon as a TWeakReferenceable dies, all WeakPtrs will be notified and 
 * clear their ptrs to us
 */

class TWeakReferenceable
{
public:
  //! project init/exit functions
  static void SInit();
  static void SExit();

  //! inits mNumOfAttachedWeakRefOwners
  TWeakReferenceable();
  
  //! preserves mNumOfAttachedWeakRefOwners (copies nothing)
  TWeakReferenceable(const TWeakReferenceable& Other);
  TWeakReferenceable& operator=(const TWeakReferenceable& Other);
  
  //! notifies all attached TWeakRefOwners that we are about to destruct
  virtual ~TWeakReferenceable();
  
private:
  friend class TWeakRefOwner;

  //! just to avoid searching overhead if no WeakPtr points to us
  int mNumOfAttachedWeakRefOwners;
};

// =================================================================================================

/* 
 * Non templated implementation helper for the TWeakPtr template
 */

class TWeakRefOwner
{
protected:
  friend class TWeakReferenceable;


  //! be virtual
  virtual ~TWeakRefOwner() { }

  
  //@{ ... Interface for TWeakReferenceable

  //! called by TWeakReferenceables destructor
  static void SOnWeakReferencableDying(
    TWeakReferenceable* pWeakReferenceable);
  //@}


  //@{ ... Interface for TWeakPtr

  //! called, as soon as a weak referenceable object that we point to died
  virtual void OnWeakReferenceDied() = 0;

  //! add ownership to an existing TWeakReferenceable
  static void SAddWeakReference(
    TWeakReferenceable* pWeakReferenceable, 
    TWeakRefOwner*      pOwner);
    
  //! remove ownership from an existing TWeakReferenceable
  static void SSubWeakReference(
    TWeakReferenceable* pWeakReferenceable, 
    TWeakRefOwner*      pOwner);

  //! move ownership for an existing TWeakReferenceable
  static void SMoveWeakReference(
    TWeakReferenceable* pWeakReferenceable, 
    TWeakRefOwner*      pOldOwner,
    TWeakRefOwner*      pNewOwner);
  //@}
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

inline TWeakReferenceable::TWeakReferenceable()
  : mNumOfAttachedWeakRefOwners(0)
{
}

inline TWeakReferenceable::TWeakReferenceable(const TWeakReferenceable& Other)
  : mNumOfAttachedWeakRefOwners(0)
{
  MUnused(Other);

  // preserve mNumOfAttachedWeakRefOwners
}

// -------------------------------------------------------------------------------------------------

inline TWeakReferenceable::~TWeakReferenceable()
{
  TWeakRefOwner::SOnWeakReferencableDying(this);
}

// -------------------------------------------------------------------------------------------------

inline TWeakReferenceable& TWeakReferenceable::operator=(
  const TWeakReferenceable& Other)
{
  MUnused(Other);

  // preserve mNumOfAttachedWeakRefOwners
  return *this;
}


#endif   // _WeakReferenceable_h_

