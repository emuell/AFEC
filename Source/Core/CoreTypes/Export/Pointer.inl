#pragma once

#ifndef _Pointer_inl_
#define _Pointer_inl_

// =================================================================================================

#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/CompilerDefines.h"
    
// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T>
TWeakPtr<T>::TWeakPtr()
 : mpObject(NULL), 
   mpWeakReferenceable(NULL)
{
}

template<typename T>
TWeakPtr<T>::TWeakPtr(T* pObject)
  : mpObject(pObject), 
    mpWeakReferenceable(const_cast<TWeakReferenceable*>(
      static_cast<const TWeakReferenceable*>(pObject)))
{
  if (mpWeakReferenceable)
  {
    SAddWeakReference(mpWeakReferenceable, this);
  }
}

template<typename T>
TWeakPtr<T>::TWeakPtr(const TWeakPtr& Other)
  : TWeakRefOwner(Other),
    mpObject(Other.mpObject), 
    mpWeakReferenceable(Other.mpWeakReferenceable)
{
  if (mpWeakReferenceable)
  {
    SAddWeakReference(mpWeakReferenceable, this);
  }
}

#if defined(MCompiler_Has_RValue_References)

template<typename T>
TWeakPtr<T>::TWeakPtr(TWeakPtr&& Other)
  : TWeakRefOwner(Other),
    mpObject(Other.mpObject), 
    mpWeakReferenceable(Other.mpWeakReferenceable)
{
  if (mpWeakReferenceable)
  {
    SMoveWeakReference(mpWeakReferenceable, &Other, this);
  }

  Other.mpObject = NULL;
  Other.mpWeakReferenceable = NULL;
}

#endif

// -------------------------------------------------------------------------------------------------

template<typename T>
TWeakPtr<T>::~TWeakPtr()
{
  if (mpWeakReferenceable)
  {
    SSubWeakReference(mpWeakReferenceable, this);
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T* TWeakPtr<T>::Object()const
{ 
  return mpObject; 
} 

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T* TWeakPtr<T>::operator->()const
{ 
  MAssert(mpObject, "Trying to use an object that was not initialized");
  return mpObject;
} 

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline TWeakPtr<T>::operator T*()const           
{
  return mpObject; 
}  

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T& TWeakPtr<T>::operator*()const
{ 
  MAssert(mpObject, "Trying to use an object that was not initialized");
  return *mpObject;
}

// -------------------------------------------------------------------------------------------------

template<typename T>
TWeakPtr<T>& TWeakPtr<T>::operator=(T* pObject)
{
  if (mpObject != pObject)
  {
    if (mpWeakReferenceable)
    {
      SSubWeakReference(mpWeakReferenceable, this);
    }
    
    mpObject = pObject;
    mpWeakReferenceable = pObject;

    if (mpWeakReferenceable)
    {
      SAddWeakReference(mpWeakReferenceable, this);
    }
  }  

  return *this;
}

template<typename T>
TWeakPtr<T>& TWeakPtr<T>::operator=(const TWeakPtr& Other)
{
  if (mpObject != Other.mpObject)
  {
    if (mpWeakReferenceable)
    {
      SSubWeakReference(mpWeakReferenceable, this);
    }

    // avoid implicit casts here to allow dealing with forward declared types only
    mpObject = Other.mpObject;
    mpWeakReferenceable = Other.mpWeakReferenceable;

    if (mpWeakReferenceable)
    {
      SAddWeakReference(mpWeakReferenceable, this);
    }
  }

  return *this;
}


#if defined(MCompiler_Has_RValue_References)

template<typename T>
TWeakPtr<T>& TWeakPtr<T>::operator=(TWeakPtr&& Other)
{
  if (mpObject != Other.mpObject)
  {
    if (mpWeakReferenceable)
    {
      SSubWeakReference(mpWeakReferenceable, this);
    }

    mpObject = Other.mpObject;
    mpWeakReferenceable = Other.mpWeakReferenceable;

    if (mpWeakReferenceable)
    {
      SMoveWeakReference(mpWeakReferenceable, &Other, this);
    }
    
    Other.mpObject = NULL;
    Other.mpWeakReferenceable = NULL;
  }

  return *this;
}

#endif

// -------------------------------------------------------------------------------------------------

template<typename T>
void TWeakPtr<T>::OnWeakReferenceDied()
{
  MAssert(mpObject != NULL, "Should have released the weakref before !");
  
  mpObject = NULL;
  mpWeakReferenceable = NULL;
}
  
// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T>
TDebugWeakPtr<T>::TDebugWeakPtr()
 : mpObject(NULL)
  #if defined(MDebug)
  , mpWeakReferenceable(NULL)
  #endif
{
}

template<typename T>
TDebugWeakPtr<T>::TDebugWeakPtr(T* pObject)
  : mpObject(pObject)
  #if defined(MDebug)
  , mpWeakReferenceable(const_cast<TWeakReferenceable*>(
      static_cast<const TWeakReferenceable*>(pObject)))
  #endif
{
  #if defined(MDebug)
    if (mpWeakReferenceable)
    {
      SAddWeakReference(mpWeakReferenceable, this);
    }
  #endif  
}

template<typename T>
TDebugWeakPtr<T>::TDebugWeakPtr(const TDebugWeakPtr& Other)
  : mpObject(Other.mpObject)
  #if defined(MDebug)
  , mpWeakReferenceable(Other.mpWeakReferenceable)
  #endif
{
  #if defined(MDebug)
    if (mpWeakReferenceable)
    {
      SAddWeakReference(mpWeakReferenceable, this);
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

template<typename T>
TDebugWeakPtr<T>::~TDebugWeakPtr()
{
  #if defined(MDebug)
    if (mpWeakReferenceable)
    {
      SSubWeakReference(mpWeakReferenceable, this);
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T* TDebugWeakPtr<T>::Object()const
{ 
  return mpObject; 
} 

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T* TDebugWeakPtr<T>::operator->()const
{ 
  MAssert(mpObject, "Trying to use an object that was not initialized");
  return mpObject;
} 

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline TDebugWeakPtr<T>::operator T*()const           
{
  return mpObject; 
}  

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T& TDebugWeakPtr<T>::operator*()const
{ 
  MAssert(mpObject, "Trying to use an object that was not initialized");
  return *mpObject;
}

// -------------------------------------------------------------------------------------------------

template<typename T>
TDebugWeakPtr<T>& TDebugWeakPtr<T>::operator=(T* pObject)
{
  if (mpObject != pObject)
  {
    #if defined(MDebug)
      if (mpWeakReferenceable)
      {
        SSubWeakReference(mpWeakReferenceable, this);
      }
      
      mpObject = pObject;
      mpWeakReferenceable = pObject;

      if (mpWeakReferenceable)
      {
        SAddWeakReference(mpWeakReferenceable, this);
      }
    #else
      mpObject = pObject;
    #endif
  }  

  return *this;
}

template<typename T>
TDebugWeakPtr<T>& TDebugWeakPtr<T>::operator=(const TDebugWeakPtr& Other)
{
  if (mpObject != Other.mpObject)
  {
    #if defined(MDebug)
      if (mpWeakReferenceable)
      {
        SSubWeakReference(mpWeakReferenceable, this);
      }

      // avoid implicit casts here to allow dealing with forward declared types only
      mpObject = Other.mpObject;
      mpWeakReferenceable = Other.mpWeakReferenceable;

      if (mpWeakReferenceable)
      {
        SAddWeakReference(mpWeakReferenceable, this);
      }
    #else
      mpObject = Other.mpObject;
    #endif
  }

  return *this;
}

// -------------------------------------------------------------------------------------------------

#if defined(MDebug)

template<typename T>
void TDebugWeakPtr<T>::OnWeakReferenceDied()
{
  MAssert(mpObject != NULL, "Should have released the weakref before !");
  
  //! IMPORTANT: You may get crashes in release builds when using the 
  //! pointer from now on (release builds won't set the ptr to NULL 
  //! like we do now here now!
  MInvalid("TDebugWeakPtr object unexpectedly died.");
  
  mpObject = NULL;
  mpWeakReferenceable = NULL;
}

#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T>
TPtr<T>::TPtr()
  : mpObject(NULL), 
    mpReferenceCounteable(NULL)
{
}

template<typename T>
TPtr<T>::TPtr(T* pObject)
  : mpObject(pObject), 
    mpReferenceCounteable(const_cast<TReferenceCountable*>(
      static_cast<const TReferenceCountable*>(pObject)))
{
  if (mpReferenceCounteable)
  {
    mpReferenceCounteable->IncRef();
  }
}

template<typename T>
TPtr<T>::TPtr(const TPtr& Other)
  : mpObject(Other.mpObject), 
    mpReferenceCounteable(Other.mpReferenceCounteable)
{ 
  if (mpReferenceCounteable)
  {
    mpReferenceCounteable->IncRef();
  }
}  

#if defined(MCompiler_Has_RValue_References)

template<typename T>
TPtr<T>::TPtr(TPtr&& Other)
  : mpObject(Other.mpObject), 
    mpReferenceCounteable(Other.mpReferenceCounteable)
{ 
  // moved from other
  Other.mpObject = NULL;
  Other.mpReferenceCounteable = NULL;
}  

#endif

// -------------------------------------------------------------------------------------------------

template<typename T>
TPtr<T>::~TPtr()
{ 
  if (mpReferenceCounteable)
  {
    mpReferenceCounteable->DecRef();
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T>
void TPtr<T>::Delete()
{
  MAssert(MImplies(mpReferenceCounteable, mpReferenceCounteable->RefCount() != -1), 
    "Trying to use a dead object!");

  if (mpReferenceCounteable)
  {
    mpReferenceCounteable->DecRef();
    
    mpReferenceCounteable = NULL;
    mpObject = NULL;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T* TPtr<T>::Object()const
{ 
  MAssert(MImplies(mpReferenceCounteable, mpReferenceCounteable->RefCount() != -1), 
    "Trying to use a dead object!");

  return mpObject; 
} 

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T* TPtr<T>::operator->()const
{ 
  MAssert(mpObject, "Trying to use an object that was not initialized");

  MAssert(MImplies(mpReferenceCounteable, mpReferenceCounteable->RefCount() != -1), 
    "Trying to use a dead object!");

  return mpObject;
} 

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline TPtr<T>::operator T*()const           
{
  MAssert(MImplies(mpReferenceCounteable, mpReferenceCounteable->RefCount() != -1), 
    "Trying to use a dead object!");

  return mpObject; 
}  

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T& TPtr<T>::operator*()const
{ 
  MAssert(mpObject, "Trying to use an object that was not initialized");

  MAssert(MImplies(mpReferenceCounteable, mpReferenceCounteable->RefCount() != -1), 
    "Trying to use a dead object!");

  return *mpObject;
}

// -------------------------------------------------------------------------------------------------

template<typename T>
TPtr<T>& TPtr<T>::operator=(T* pObject)
{
  if (mpObject != pObject)
  {
    if (pObject)
    {
      pObject->IncRef();
    }

    TReferenceCountable* pOldReferenceCountable = mpReferenceCounteable;

    mpReferenceCounteable = pObject;
    mpObject = pObject;
    
    if (pOldReferenceCountable)
    {
      pOldReferenceCountable->DecRef();
    }
  }

  return *this;
}

template<typename T>
TPtr<T>& TPtr<T>::operator=(const TPtr& Other) 
{
  if (mpObject != Other.mpObject)
  {
    if (Other.mpReferenceCounteable)
    {
      Other.mpReferenceCounteable->IncRef();
    }

    TReferenceCountable* pOldReferenceCountable = mpReferenceCounteable;

    // avoid implicit casts here to allow dealing with forward declared types only
    mpReferenceCounteable = Other.mpReferenceCounteable;
    mpObject = Other.mpObject;

    if (pOldReferenceCountable)
    {
      pOldReferenceCountable->DecRef();
    }
  }

  return *this;
}

#if defined(MCompiler_Has_RValue_References)

template<typename T>
TPtr<T>& TPtr<T>::operator=(TPtr&& Other) 
{
  if (mpObject != Other.mpObject)
  {
    TReferenceCountable* pOldReferenceCountable = mpReferenceCounteable;

    // move object from other to this
    mpObject = Other.mpObject;
    mpReferenceCounteable = Other.mpReferenceCounteable;

    Other.mpObject = NULL;
    Other.mpReferenceCounteable = NULL;

    // and release our old ref
    if (pOldReferenceCountable)
    {
      pOldReferenceCountable->DecRef();
    }
  }

  return *this;
}

#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T>
TOwnerPtr<T>::TOwnerPtr()
  : mpObject(NULL)
{
}

template<typename T>
TOwnerPtr<T>::TOwnerPtr(T* pObject)
  : mpObject(pObject)
{
  // take ownership of pObject
}

template<typename T>
TOwnerPtr<T>::TOwnerPtr(const TOwnerPtr<T>& Other)
  : mpObject(const_cast<TOwnerPtr<T>&>(Other).ReleaseOwnership())
{
  // overtake ownership from other
}

// -------------------------------------------------------------------------------------------------

template<typename T>
TOwnerPtr<T>::~TOwnerPtr()
{
  Delete();
}

// -------------------------------------------------------------------------------------------------

template<typename T>
TOwnerPtr<T>& TOwnerPtr<T>::operator=(const TOwnerPtr<T>& Other)
{
  if (this != &Other)
  {
    // Release our old object (if any)
    Delete();
    // Overtake ownership from other
    mpObject = const_cast<TOwnerPtr<T>&>(Other).ReleaseOwnership();
  }

  return *this;
}
  
// -------------------------------------------------------------------------------------------------

template<typename T>
T* TOwnerPtr<T>::ReleaseOwnership()
{
  T* pObject = mpObject;
  mpObject = NULL;

  return pObject;
}

// -------------------------------------------------------------------------------------------------

template<typename T>
void TOwnerPtr<T>::Delete()
{
  if (mpObject)
  {
    delete mpObject;
    mpObject = NULL;
  }
}
  
// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T* TOwnerPtr<T>::Object()const
{ 
  return mpObject; 
} 

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T* TOwnerPtr<T>::operator->()const
{ 
  MAssert(mpObject, "Trying to use an object that was not initialized");
  return mpObject;
} 

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline TOwnerPtr<T>::operator T*()const
{
  return mpObject;
}

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T& TOwnerPtr<T>::operator*()const
{ 
  MAssert(mpObject, "Trying to use an object that was not initialized");
  return *mpObject;
}
  

#endif  

