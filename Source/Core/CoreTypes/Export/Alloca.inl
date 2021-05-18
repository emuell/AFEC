#pragma once

#ifndef _Alloca_inl_
#define _Alloca_inl_

// =================================================================================================

#include "CoreTypes/Export/Allocator.h"
#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/Exception.h"
#include "CoreTypes/Export/InlineMath.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T> 
TAllocaList<T>::TAllocaList(int MaximumSize, int Alignment)
  : mpBuffer(NULL), 
    mSize(0),
    mMaximumSize(MaximumSize),
    mAlignment(Alignment)
{
  MAssert(MaximumSize >= 0, "Invalid size");

  MAssert(TMath::IsPowerOfTwo(Alignment) && 
    Alignment >= 1 && Alignment <= 128, "Invalid alignment"); 
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
TAllocaList<T>::~TAllocaList()
{
  if (mpBuffer)
  {
    TArrayConstructor<T>::SDestruct(mpBuffer, mSize);
    mpBuffer = NULL;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
size_t TAllocaList<T>::RequiredMemorySize()const
{
  return sizeof(T) * mMaximumSize + mAlignment - 1;
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
void* TAllocaList<T>::Memory()const
{
  return mpBuffer;
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
void TAllocaList<T>::SetMemory(void* pAllocaBuffer)
{
  MAssert(mpBuffer == NULL && mSize == 0, 
    "Alloca list was already initialized and should not be reinitialized.");

  MAssert(MImplies(mMaximumSize == 0, pAllocaBuffer == NULL), 
    "Expecting NULL for a zero sized list");

  if (mMaximumSize > 0)
  {
    if (pAllocaBuffer == NULL)
    {
      // must throw or we will corrupt the stack
      throw TReadableException("Fatal Error: Running out of stack memory.");
    }

    T* pAlignedAllocaBuffer = (T*)(((ptrdiff_t)pAllocaBuffer + 
      (mAlignment - 1)) & ~(size_t)(mAlignment - 1));

    mpBuffer = (T*)pAlignedAllocaBuffer;
    TArrayConstructor<T>::SConstruct(mpBuffer, mMaximumSize);
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
inline bool TAllocaList<T>::IsEmpty()const
{
  MAssert(MImplies(mMaximumSize > 0, mpBuffer != NULL), 
    "Trying to use an uninitialized alloca list");
  
  return (mSize == 0);
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
inline int TAllocaList<T>::Size()const
{
  MAssert(MImplies(mMaximumSize > 0, mpBuffer != NULL), 
    "Trying to use an uninitialized alloca list");
  
  return mSize;
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
inline int TAllocaList<T>::MaximumSize()const
{
  return mMaximumSize;
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
void TAllocaList<T>::Empty()
{
  MAssert(mpBuffer != NULL, 
    "Trying to use an uninitialized alloca list");

  // clear all what's in the buffer
  TArrayInitializer<T>::SInitialize(mpBuffer, mSize);
  
  // but keep the buffer memory valid
  mSize = 0; 
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
inline const T& TAllocaList<T>::operator[](int Index)const
{
  MAssert(mpBuffer != NULL, 
    "Trying to use an uninitialized alloca list");
  
  MAssert(Index >= 0 && Index < mSize, "Invalid Index");
  
  return mpBuffer[Index];
}

template<typename T> 
inline T& TAllocaList<T>::operator[](int Index)
{
  MAssert(mpBuffer != NULL, 
    "Trying to use an uninitialized alloca list");
  
  MAssert(Index >= 0 && Index < mSize, "Invalid Index");
  
  return mpBuffer[Index]; 
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
const T& TAllocaList<T>::First()const
{
  return (*this)[0];
}

template<typename T> 
T& TAllocaList<T>::First()
{
  return (*this)[0];
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
const T& TAllocaList<T>::Last()const
{
  return (*this)[Size() - 1];
}

template<typename T> 
T& TAllocaList<T>::Last()
{
  return (*this)[Size() - 1];
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
inline bool TAllocaList<T>::Contains(const T& Entry)const
{
  return (TAllocaList<T>::Find(Entry) != -1);
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
inline int TAllocaList<T>::Find(const T& Entry)const
{
  MAssert(MImplies(mMaximumSize > 0, mpBuffer != NULL), 
    "Trying to use an uninitialized alloca list");
  
  for (int i = 0; i < mSize; ++i)
  {
    if (mpBuffer[i] == Entry)
    {
      return i;
    }
  }

  return -1;
}

template<typename T> 
inline int TAllocaList<T>::Find(
  const T&  Entry, 
  int       StartIndex, 
  bool      Backwards)const
{
  MAssert(MImplies(mMaximumSize > 0, mpBuffer != NULL), 
    "Trying to use an uninitialized alloca list");
  
  if (Backwards)
  {
    for (int i = StartIndex; i >= 0; --i)
    {
      if (mpBuffer[i] == Entry)
      {
        return i;
      }
    }
  }
  else
  {
    for (int i = StartIndex; i < mSize; ++i)
    {
      if (mpBuffer[i] == Entry)
      {
        return i;
      }
    }
  }

  return -1;
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
TAllocaList<T>& TAllocaList<T>::Prepend(const T &Value)
{
  return Insert(Value, 0);
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
TAllocaList<T>& TAllocaList<T>::Append(const T &Value)
{
  return Insert(Value, Size());
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
TAllocaList<T>& TAllocaList<T>::Delete(int Index)
{
  MAssert(mpBuffer != NULL, 
    "Trying to use an uninitialized alloca list");
  
  MAssert(Index >= 0 && Index < mSize, "Invalid Index");

  // shunt
  TArrayCopy<T>::SMove(&mpBuffer[Index], 
    &mpBuffer[Index + 1], mSize - Index - 1);

  // clear freed entry
  TArrayInitializer<T>::SInitialize(&mpBuffer[mSize - 1], 1);

  --mSize;

  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
TAllocaList<T>& TAllocaList<T>::Insert(const T &Value, int Index)
{
  MAssert(mpBuffer != NULL, 
    "Trying to use an uninitialized alloca list");
  
  MAssert(Index >= 0 && Index <= mSize, "Invalid Index");
  
  // check space
  if (mSize + 1 > mMaximumSize)
  {
    MInvalid("Ouch! Running out of preallocated space. "
      "This will corrupt the stack");

    // better throw than corrupting the stack
    throw TReadableException("Fatal error "
      "in a stack based list. Out of memory");
  }

  // make room for the new entry
  for (int i = mSize; i > Index; --i)    
  {
    mpBuffer[i] = mpBuffer[i - 1];   
  }

  // and assign it in the buffer
  mpBuffer[Index] = Value;

  ++mSize;

  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
TAllocaList<T>& TAllocaList<T>::operator+= (const T &Value)
{
  Append(Value);   
  return *this;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T>
bool operator==(
  const TAllocaList<T>& First, 
  const TAllocaList<T>& Second)
{
  if (First.Size() != Second.Size())
  {
    return false;
  }

  for (int i = 0; i < First.Size(); ++i)
  {
    if (!(First[i] == Second[i]))
    {
      return false;
    }
  }

  return true;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
bool operator!=(
  const TAllocaList<T>& First, 
  const TAllocaList<T>& Second)
{
  return !(operator==(First, Second));
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T>
TAllocaArray<T>::TAllocaArray(int Size, int Alignment)
  : mpBuffer(NULL), 
    mSize(Size),
    mAlignment(Alignment)
{
  MAssert(Size >= 0, "Invalid size");

  MAssert(TMath::IsPowerOfTwo(Alignment) && 
    Alignment >= 1 && Alignment <= 128, "Invalid alignment"); 
}

// -------------------------------------------------------------------------------------------------

template<typename T>
TAllocaArray<T>::~TAllocaArray()
{
  if (mpBuffer)
  {
    TArrayConstructor<T>::SDestruct(mpBuffer, mSize);
    mpBuffer = NULL;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
size_t TAllocaArray<T>::RequiredMemorySize()const
{
  return sizeof(T) * mSize + mAlignment - 1;
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
void* TAllocaArray<T>::Memory()const
{
  return mpBuffer;
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
void TAllocaArray<T>::SetMemory(void* pAllocaBuffer)
{
  MAssert(mpBuffer == NULL, 
    "Alloca list was already initialized and should not be reinitialized.");

  MAssert(MImplies(mSize == 0, pAllocaBuffer == NULL), 
    "Expecting NULL for a zero sized array");

  if (mSize > 0)
  {
    if (pAllocaBuffer == NULL)
    {
      // must throw or we will corrupt the stack
      throw TReadableException("Fatal Error: Running out of stack memory.");
    }

    T* pAlignedAllocaBuffer = (T*)(((ptrdiff_t)pAllocaBuffer + 
      (mAlignment - 1)) & ~(size_t)(mAlignment - 1));

    mpBuffer = pAlignedAllocaBuffer;
    TArrayConstructor<T>::SConstruct(mpBuffer, mSize);
  }
}

// -------------------------------------------------------------------------------------------------
 
template<typename T>
MForceInline int TAllocaArray<T>::Size()const
{
  return mSize;
}

// -------------------------------------------------------------------------------------------------

template<typename T>
inline void TAllocaArray<T>::Init(const T& Value)
{
  MAssert(MImplies(mSize > 0, mpBuffer != NULL), 
    "Trying to use an uninitialized alloca array");
  
  for (int i = 0; i < mSize; ++i)
  {
    mpBuffer[i] = Value;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T& TAllocaArray<T>::operator[](int n)
{
  MAssert(MImplies(mSize > 0, mpBuffer != NULL), 
    "Trying to use an uninitialized alloca array");
  
  MAssert(n >= 0 && n < mSize, "Invalid array index");
  
  return mpBuffer[n];
}

template<typename T>
MForceInline const T& TAllocaArray<T>::operator[](int n)const
{
  MAssert(MImplies(mSize > 0, mpBuffer != NULL), 
    "Trying to use an uninitialized alloca array");
  
  MAssert(n >= 0 && n < mSize, "Invalid array index");
  
  return mpBuffer[n];
}

// -------------------------------------------------------------------------------------------------
 
template<typename T>
inline const T* TAllocaArray<T>::FirstRead()const
{
  MAssert(MImplies(mSize > 0, mpBuffer != NULL), 
    "Trying to use an uninitialized alloca array");
  
  return (const T*)mpBuffer;
}

// -------------------------------------------------------------------------------------------------

template<typename T>
inline T* TAllocaArray<T>::FirstWrite()const
{
  MAssert(MImplies(mSize > 0, mpBuffer != NULL), 
    "Trying to use an uninitialized alloca array");
  
  return (T*)mpBuffer;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T>
bool operator==(
  const TAllocaArray<T>& First, 
  const TAllocaArray<T>& Second)
{
  if (First.Size() != Second.Size())
  {
    return false;
  }

  for (int i = 0; i < First.Size(); ++i)
  {
    if (!(First[i] == Second[i]))
    {
      return false;
    }
  }

  return true;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
bool operator!=(
  const TAllocaArray<T>& First, 
  const TAllocaArray<T>& Second)
{
  return !(operator==(First, Second));
}


#endif // _Alloca_inl_

