#pragma once

#ifndef _ThreadLocalValue_h_
#define _ThreadLocalValue_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"
#include "CoreTypes/Export/List.h"

#include <mutex>

// =================================================================================================

/*!
 * Providing access to a thread local value slot (a void pointer).
 * Calling Value() will automatically create a new slot for the calling thread
 * if accessed the first time. The slot is then initialized with 0. 
 * If you store objects into the slot (us it as a Ptr) you are responsible for 
 * deleting all the objects. See also class TThreadLocalValue...
!*/

class TThreadLocalValueSlot
{
public:
  TThreadLocalValueSlot();
  ~TThreadLocalValueSlot();
  
  //! Get/set the calling threads local value
  //! throws TReadableException when allocation failed (Thread Local 
  //! Objects slots are limited per thread!)
  void* Value()const;
  void SetValue(void* pNewValue);

private:
  #if defined(MCompiler_GCC)
    #if defined(MMac)
      typedef volatile long __attribute__((__may_alias__)) int_a;
    #else
      typedef volatile int __attribute__((__may_alias__)) int_a;
    #endif
  #else
    typedef int int_a;
  #endif
  
  mutable int_a mKey;
};

// =================================================================================================

/*!
 *  Simple Interface for thread local storage of any type T.
 *  Access to the object will automatically create a new object when needed 
 *  the first time. All created objects are deleted, as soon as the TThreadLocalValue 
 *  object dies (calling operator delete)
!*/

template <typename T>
class TThreadLocalValue
{
public:
  typedef T*(*TPCreateObjectFunc)();
  typedef void(*TPDeleteObjectFunc)(T* pObject);

  //! Pass a optional create function which creates a new T object when needed 
  //! with. This allows complex constructors to be called for T.
  //! Note that the default SCreateDefault functions will not ref count: They 
  //! simply create via 'operator new' and delete via 'operator delete'.
  TThreadLocalValue(
    TPCreateObjectFunc pCreateObjFunc = TThreadLocalValue::SCreateNewObject,
    TPDeleteObjectFunc pDeleteObjFunc = TThreadLocalValue::SDeleteObject);
  
  ~TThreadLocalValue();

  //! Access to the value. A new object for the given thread is automatically 
  //! created when accessed the first time...
  T* Object()const;

  //! get set the dereferenced object value
  const T& Value()const;
  void SetValue(const T& Value);
  
  //! access to 'Object'
  T* operator->()const;
  operator T*()const;
  T& operator*()const;
  
private:
  static T* SCreateNewObject() { return new T(); }
  static void SDeleteObject(T* pObject) { delete pObject; }
  
  mutable std::mutex mCreatedObjectsLock;
  mutable TList<T*> mCreatedObjects;

  mutable TThreadLocalValueSlot mThreadSlot;

  TPCreateObjectFunc mpCreateObjFunc;
  TPDeleteObjectFunc mpDeleteObjFunc;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T>
TThreadLocalValue<T>::TThreadLocalValue(
  TPCreateObjectFunc  pCreateObjFunc,
  TPDeleteObjectFunc  pDeleteObjFunc) 
  : mpCreateObjFunc(pCreateObjFunc),
    mpDeleteObjFunc(pDeleteObjFunc)
{ 
}

// -------------------------------------------------------------------------------------------------

template <typename T>
TThreadLocalValue<T>::~TThreadLocalValue() 
{
  // release all object from all threads
  for (int i = 0; i < mCreatedObjects.Size(); ++i)
  {
    mpDeleteObjFunc(mCreatedObjects[i]);
  }
}

// -------------------------------------------------------------------------------------------------

template <typename T>
T* TThreadLocalValue<T>::Object()const
{
  T* pValue = (T*)mThreadSlot.Value();
  
  if (pValue == NULL)
  {
    pValue = mpCreateObjFunc();
    MAssert(pValue, "Should never fail! Throw TOutOfMemoryException instead.");

    // access to mCreatedObjects must be serialized
    {
      const std::lock_guard<std::mutex> Lock(mCreatedObjectsLock);
      mCreatedObjects.Append(pValue);
    }

    mThreadSlot.SetValue(pValue);
  }
  
  return pValue; 
}

// -------------------------------------------------------------------------------------------------
    
template <typename T>
const T& TThreadLocalValue<T>::Value()const
{
  return *Object();
}

// -------------------------------------------------------------------------------------------------
    
template <typename T>
void TThreadLocalValue<T>::SetValue(const T& Value)
{
  *Object() = Value;
}

// -------------------------------------------------------------------------------------------------
    
template <typename T>
T* TThreadLocalValue<T>::operator->()const 
{
  return Object(); 
}

// -------------------------------------------------------------------------------------------------

template <typename T>
TThreadLocalValue<T>::operator T*()const 
{
  return Object(); 
}

// -------------------------------------------------------------------------------------------------

template <typename T>
T& TThreadLocalValue<T>::operator*()const
{
  return *Object(); 
}


#endif // _ThreadLocalValue_h_

