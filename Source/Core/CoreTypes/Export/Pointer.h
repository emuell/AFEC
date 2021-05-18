#pragma once

#ifndef _Pointer_h_
#define _Pointer_h_

// =================================================================================================

#include "CoreTypes/Export/WeakReferenceable.h"
#include "CoreTypes/Export/ReferenceCountable.h"

template<class T> class TWeakPtr;
template<class T> class TDebugWeakPtr;

template<class T> class TOwnerPtr;
template<class T> class TPtr;

// =================================================================================================

/* 
 * A non reference counting raw "Pointer" wrapper.
 * Pointer to the object is guaranteed to be always valid: as soon as 
 * an object we are pointing to dies, TWeakPtr will update its pointer 
 * to NULL.
 */

template<class T> 
class TWeakPtr : public TWeakRefOwner
{
public:
  typedef T element_type; // needed for boost

  //! Initializes our ObjectPtr to NULL
  TWeakPtr();

  //! Adds a WeakReference if pObject != NULL
  TWeakPtr(T* pObject);
  TWeakPtr(const TWeakPtr& Other);
  
  //! Move WeakReference from other to us
  #if defined(MCompiler_Has_RValue_References)
    TWeakPtr(TWeakPtr&& Other);
  #endif

  //! Allow conversion to compatible TWeakPtr
  template<class U> 
  TWeakPtr(const TWeakPtr<U>& Other)
    : TWeakRefOwner(Other),
      mpObject(Other.mpObject), 
      mpWeakReferenceable(Other.mpWeakReferenceable)
  {
    if (mpWeakReferenceable)
    {
      SAddWeakReference(mpWeakReferenceable, this);
    }
  }

  //! Releases the Weakref
  ~TWeakPtr();
    
  //! Assignment Operators :
  //! Adds a WeakReference if pObject != NULL
  TWeakPtr<T>& operator=(T *pObject);
  TWeakPtr<T>& operator=(const TWeakPtr& Other);
  
  //! Move operator: Move weakref to us if pObject != NULL
  #if defined(MCompiler_Has_RValue_References)
    TWeakPtr<T>& operator=(TWeakPtr&& Other);
  #endif

  //! Allow conversion to compatible TWeakPtr
  template<class U> 
  TWeakPtr<T>& operator=(const TWeakPtr<U>& Other)
  {
    operator=(Other.mpObject);
    return *this;
  }
  
  //! Direct access to the Pointer (may be NULL)
  T* Object()const;
  operator T*()const;
  
  //! Access to the object (asserts NULL before crashing)
  T& operator*()const;
  T* operator->()const;

protected:
  //! Called as soon as a weak referenceable object we point to died.
  //! When overridden, call this impl, or the hold raw ptr will be invalid.
  virtual void OnWeakReferenceDied();

private:
  template<class U> friend class TWeakPtr;
  
  //! Store both pointers to avoid that we have to cast from either
  //! TWeakReferenceable to T or vice versa. 
  //! Raw pointers may be different for multiple inherited classes of T, 
  //! and C-Casting could result in a bad cast. Static or dynamic 
  //! casts would require that we have to include headers, that we can't 
  //! forward declare T like you can do with raw pointers.
  T* mpObject;
  TWeakReferenceable* mpWeakReferenceable;
};

// =================================================================================================

/* 
 * TDebugWeakPtr is a non reference counting Wrapper for a "Pointer".
 *
 * In debug versions, it behaves exactly as TWeakPtr, but will fire
 * an assertion as soon as the object dies before the TDebugWeakPtr object
 * dies. In release builds it will be an unchecked C-Pointer for maximum 
 * performance, with the only difference that its raw pointer is initialized
 * to NULL on construction. 
 */

template<class T> 
#if defined(MDebug)
  class TDebugWeakPtr : public TWeakRefOwner
#else
  class TDebugWeakPtr
#endif  
{
public:
  //! Initializes our raw Ptr to NULL
  TDebugWeakPtr();

  //! Adds a WeakReferences if pObject != NULL
  TDebugWeakPtr(T* pObject);
  TDebugWeakPtr(const TDebugWeakPtr<T>& Other);

  //! Allow conversion to compatible TDebugWeakPtr
  template<typename U>
  TDebugWeakPtr(const TDebugWeakPtr<U>& Other)
    : mpObject(NULL)
    #if defined(MDebug)
    , mpWeakReferenceable(NULL)
    #endif
  {
    #if defined(MDebug)
      if (mpWeakReferenceable)
      {
        SAddWeakReference(mpWeakReferenceable, this);
      }
    #endif
  }

  //! Releases the Weakref when pObject != NULL
  ~TDebugWeakPtr();
    
  //! Assignment Operators :
  //! Assigns the raw Ptr and adds a weak reference when new pObject != NULL
  TDebugWeakPtr<T>& operator=(T *pObject);
  TDebugWeakPtr<T>& operator=(const TDebugWeakPtr<T>& Other);
  
  //! Allow conversion to compatible TDebugWeakPtr
  template<typename U>
  TDebugWeakPtr<T>& operator=(const TDebugWeakPtr<U>& Other)
  {
    operator=(Other.Object());
    return *this;
  }
  
  //! Direct access to the Pointer (may be NULL)
  T* Object()const;
  operator T*()const;
  
  //! Access to the object (asserts NULL before crashing)
  T& operator*()const;
  T* operator->()const;

private:
  template<class U> friend class TDebugWeakPtr;

  #if defined(MDebug)
    //! Called as soon as a weak referenceable object we point to died
    virtual void OnWeakReferenceDied();
  #endif  

  // see comment in TWeakPtr, why two pointers
  T* mpObject;
  #if defined(MDebug)
    // no need to memorize this in release builds
    TWeakReferenceable* mpWeakReferenceable;
  #endif
};

// =================================================================================================

/* 
 * Wrapper for a raw Ptr which also handles reference counting for the 
 * assigned objects. The reference count is maintained in the object itself 
 * (TReferenceCountable) and not in TPtr, so raw ptr and TPtr (other other 
 * smart pointers) usage can be mixed. 
 * 
 * The hold object will be increfed or decrefed as soon as we get a new ptr
 * assigned. To release a reference count of an object, assign NULL or a new 
 * ptr to the TPtr. Objects wich no longer are referenced (have a ref count 
 * of 0) will immediately be deleted.
 *
 * Cyclic dependencies are NOT handled by TPtr and friends, so make sure you
 * are using a TWeakPtr or raw pointer to point back to an object which already
 * reference counts you.
 */

template<class T> 
class TPtr
{
public:
  typedef T element_type; // needed for boost

  //! Initializes to NULL
  TPtr();  

  //! Increases the refcount
  TPtr(T* pObject);
  TPtr(const TPtr& Other);
  
  //! Does not change the refcount
  #if defined(MCompiler_Has_RValue_References)
    TPtr(TPtr&& Other);
  #endif
  
  //! Allow conversion to compatible TPtrs
  template<typename U>
  TPtr(const TPtr<U>& Other)
    : mpObject(Other.mpObject), 
      mpReferenceCounteable(Other.mpReferenceCounteable)
  { 
    if (mpReferenceCounteable)
    {
      mpReferenceCounteable->IncRef();
    }
  }  

  //! Decreases the refcount
  ~TPtr();
  
  //! Decreases existing refs and sets the internal pointer to NULL
  void Delete();
  
  //! Assignment Operators:
  //! Increases refcount of pOther and decreases, releases previous 
  //! owned object's refcount
  TPtr<T>& operator=(T *pObject);
  TPtr<T>& operator=(const TPtr& Other);
  
  //! Decreases existing ref (if any) and moves other's pointer to us.
  #if defined(MCompiler_Has_RValue_References)
    TPtr& operator= (TPtr&& Other);
  #endif

  //! Allow conversion to compatible TPtr
  template<typename U>
  TPtr<T>& operator=(const TPtr<U>& Other) 
  {
    operator=(Other.Object());
    return *this;
  }
        
  //! Direct access to the pointer (may be NULL)
  T* Object()const;
  operator T*()const;
  
  //! Access to the object (asserts NULL)
  T& operator*()const;
  T* operator->()const;
  
private:
  template<class U> friend class TPtr;

  //! private and not implemented. 
  //! Referencing an OwnerPtr will always cause trouble!
  TPtr& operator=(const TOwnerPtr<T>& Other);
  template<class U> TPtr& operator=(const TOwnerPtr<U>& Other);
  
  //! see comment in TWeakPtrs private section
  T* mpObject;
  TReferenceCountable* mpReferenceCounteable;
};  

// =================================================================================================

/* 
 * A non reference counting pointer with strong ownership. As soon as the 
 * TOwnerPtr instance dies, it deletes its assigned object, unless ownership 
 * was explicitly released before.
 *
 * Also useful if you don't need (or can't use) reference counts to maintain 
 * an objects lifetime via TPtrs.
 */

template<class T> 
class TOwnerPtr
{
public:
  typedef T element_type; // needed for boost

  //! Don't owns an object by default, initializes to NULL
  TOwnerPtr();
  //! Take ownership for a new unowned object.
  TOwnerPtr(T* pObject);
  //! Overtake the ownership: 'Other' will loose the object ptr AND ownership.
  //! Const to allow list operation to copy. Will modify Other!
  TOwnerPtr(const TOwnerPtr<T>& Other);

  //! Delete the owned object, if any
  ~TOwnerPtr();
  
  //! Overtake the ownership: 'Other' will loose the object ptr AND ownership.
  //! Const to allow list operation to copy. Will modify Other!
  TOwnerPtr& operator=(const TOwnerPtr<T>& Other);
  
  //! release ownership without deleting the object. returns the raw 
  //! pointer of the now unowned object or NULL.
  T* ReleaseOwnership();

  //! delete the owned object (if any) explicitly before destructing
  void Delete();
  
  //! Direct access to the pointer (may be NULL)
  T* Object()const;
  operator T*()const;
  
  //! Access to the object (asserts NULL)
  T& operator*()const;
  T* operator->()const;

private:
  template<class U> friend class TOwnerPtr;

  //! Assignment from other ptrs is not implemented and private:
  //! For raw pointers, because we then don't know if they are already 
  //! owned by another TOwnerPtr or not. For smart pointers, because the
  //! other ptrs obviously take care about the objects lifetime...
  //! Explicitly wrap the object into a temp TOwnerPtr<T> to allow assigning 
  //! new raw ptrs: TOwnerPtr<T> Ptr; Ptr = TOwnerPtr<T>(new T());

  TOwnerPtr<T>& operator=(T* pObject);
  TOwnerPtr<T>& operator=(const TPtr<T>& Other);
  TOwnerPtr<T>& operator=(const TWeakPtr<T>& Other);
  TOwnerPtr<T>& operator=(const TDebugWeakPtr<T>& Other);
  
  template<class U> TOwnerPtr<T>& operator=(U* pObject);
  template<class U> TOwnerPtr<T>& operator=(const TPtr<U>& Other);
  template<class U> TOwnerPtr<T>& operator=(const TWeakPtr<U>& Other);
  template<class U> TOwnerPtr<T>& operator=(const TDebugWeakPtr<U>& Other);
  
  T* mpObject;
};  

// =================================================================================================

#include "CoreTypes/Export/Pointer.inl"


#endif   // _Pointer_h_

