#pragma once

#ifndef _Array_inl_
#define _Array_inl_

// =================================================================================================

#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/InlineMath.h"
#include "CoreTypes/Export/CompilerDefines.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T, size_t sSize>
TStaticArray<T, sSize>::TStaticArray()
{
}

template<typename T, size_t sSize>
TStaticArray<T, sSize>::TStaticArray(const TStaticArray<T, sSize> &Other)
{
  *this = Other;
}

// -------------------------------------------------------------------------------------------------

template<typename T, size_t sSize>
TStaticArray<T, sSize>::TStaticArray(const TCArray& cArray)
{
  *this = cArray;
}

template<typename T, size_t sSize>
TStaticArray<T, sSize>&  TStaticArray<T, sSize>::operator= (
  const TCArray& cArray)
{
  TArrayCopy<T>::SCopy(this->mBuffer, cArray, sSize);
  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, size_t sSize>
TStaticArray<T, sSize>& TStaticArray<T, sSize>::operator= (
  const TStaticArray<T, sSize> &Other)
{
  if (this != &Other)
  {
    TArrayCopy<T>::SCopy(this->mBuffer, Other.mBuffer, sSize);
  }

  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, size_t sSize>
void TStaticArray<T, sSize>::Init(const T& Value)
{
  for (int i = 0; i < (int)sSize; ++i)
  {
    mBuffer[i] = Value;
  }
}

// -------------------------------------------------------------------------------------------------
 
template<typename T, size_t sSize>
MForceInline  int TStaticArray<T, sSize>::Size()const
{
  return (int)sSize;
}

// -------------------------------------------------------------------------------------------------
 
template<typename T, size_t sSize>
MForceInline const T* TStaticArray<T, sSize>::FirstRead()const
{
  return (const T*)mBuffer;
}

// -------------------------------------------------------------------------------------------------

template<typename T, size_t sSize>
MForceInline T* TStaticArray<T, sSize>::FirstWrite()const
{
  return (T*)mBuffer;
}

// -------------------------------------------------------------------------------------------------

template<typename T, size_t sSize>
MForceInline T& TStaticArray<T, sSize>::operator[](int n)
{
  MAssert(n >= 0 && (size_t)n < sSize, "Invalid array index");
  return mBuffer[n];
}

template<typename T, size_t sSize>
MForceInline const T& TStaticArray<T, sSize>::operator[](int n)const
{
  MAssert(n >= 0 && (size_t)n < sSize, "Invalid array index");
  return mBuffer[n];
}

// -------------------------------------------------------------------------------------------------

template<typename T, size_t sSize>
MForceInline const T& TStaticArray<T, sSize>::First()const
{
  return operator[](0);
}

template<typename T, size_t sSize>
MForceInline T& TStaticArray<T, sSize>::First()
{
  return operator[]( 0 );
}

// -------------------------------------------------------------------------------------------------

template<typename T, size_t sSize>
MForceInline const T& TStaticArray<T, sSize>::Last()const
{
  return operator[]( Size() - 1 );
}

template<typename T, size_t sSize>
MForceInline T& TStaticArray<T, sSize>::Last()
{
  return operator[]( Size() - 1 );
}

// -------------------------------------------------------------------------------------------------

template<typename T, size_t sSize>
void TStaticArray<T, sSize>::Swap(int Position1, int Position2)
{
  TMathT<T>::Swap(mBuffer[Position1], mBuffer[Position2]);
}
 
// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T, size_t sSize> 
bool operator== (
  const TStaticArray<T, sSize>& First, 
  const TStaticArray<T, sSize>& Second)
{
  for (int i = 0; i < (int)sSize; ++i)
  {
    if (First[i] != Second[i])
    {
      return false;
    }
  }
  
  return true;
}

// -------------------------------------------------------------------------------------------------

template<typename T, size_t sSize> 
bool operator!= (
  const TStaticArray<T, sSize>& First, 
  const TStaticArray<T, sSize>& Second)
{
  return !operator==(First, Second);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------
  
template<typename T, class TAllocator>
TArray<T, TAllocator>::TArray(int Size)
  : mpBuffer(NULL)
  , mSize(0)
{
  if (Size)
  {
    SetSize(Size);
  }
}
  
template<typename T, class TAllocator>
TArray<T, TAllocator>::TArray(const TArray& Other)
  : mpBuffer(NULL), 
    mSize(0)
{
  if (Other.mSize)
  {
    SetSize(Other.mSize);
    TArrayCopy<T>::SCopy(mpBuffer, Other.mpBuffer, Other.mSize);
  }
}
  
#if defined(MCompiler_Has_RValue_References)

template<typename T, class TAllocator>
TArray<T, TAllocator>::TArray(TArray&& Other)
  : mpBuffer(Other.mpBuffer), 
    mSize(Other.mSize)
{
  // moved buffer
  Other.mpBuffer = NULL;
  Other.mSize = 0;
}

#endif

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TArray<T, TAllocator>::~TArray()
{
  Empty();
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline bool TArray<T, TAllocator>::IsEmpty()const
{
  return (mSize == 0);
}
  
// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline int TArray<T, TAllocator>::Size()const
{
  return (int)mSize;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TArray<T, TAllocator>::SetSize(int Size)
{
  MAssert(Size >= 0, "Invalid Size");

  if (Size != mSize)
  {
    if (mpBuffer)
    {
      TAllocator::SDelete(mpBuffer, mSize);

      mSize = 0;
      mpBuffer = NULL;
    }

    if (Size)
    {
      // in case that out of memory is thrown
      mSize = 0;
      mpBuffer = NULL;

      mpBuffer = TAllocator::SNew(Size);
      mSize = Size;
    }
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TArray<T, TAllocator>::Init(const T& Value)
{
  for (int i = 0; i < mSize; ++i)
  {
    mpBuffer[i] = Value;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TArray<T, TAllocator>::Empty()
{
  if (mpBuffer)
  {
    TAllocator::SDelete(mpBuffer, mSize);
    mpBuffer = NULL;
  }

  mSize = 0;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TArray<T, TAllocator>::Grow(int NewSize)
{
  MAssert(NewSize >= 0, "Invalid grow size");

  if (NewSize > mSize)
  {
    Realloc(NewSize);
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TArray<T, TAllocator>::Swap(int Position1, int Position2)
{
  TMathT<T>::Swap(mpBuffer[Position1], mpBuffer[Position2]);
}

template<typename T, class TAllocator>
void TArray<T, TAllocator>::Swap(TIterator& Iter1, TIterator& Iter2)
{
  MAssert(!Iter1.IsEnd() && !Iter2.IsEnd(), "Invalid Iter");

  TMathT<T>::Swap(mpBuffer[Iter1.mIndex], mpBuffer[Iter2.mIndex]);
  TMathT<int>::Swap(Iter1.mIndex, Iter2.mIndex);
}

// -------------------------------------------------------------------------------------------------
  
template<typename T, class TAllocator>
TArray<T, TAllocator>& TArray<T, TAllocator>::operator= (const TArray& Other)
{
  if (this != &Other)
  {
    // copy array data from other to this
    SetSize(Other.mSize);

    if (Other.mSize)
    {
      TArrayCopy<T>::SCopy(mpBuffer, Other.mpBuffer, Other.mSize);
    }
  }

  return *this;
}

#if defined(MCompiler_Has_RValue_References)

template<typename T, class TAllocator>
TArray<T, TAllocator>& TArray<T, TAllocator>::operator= (TArray&& Other)
{
  if (this != &Other)
  {
    // move array data from other to this
    Empty();

    mpBuffer = Other.mpBuffer;
    mSize = Other.mSize;

    Other.mpBuffer = NULL;
    Other.mSize = 0;
  }

  return *this;
}

#endif

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline T& TArray<T, TAllocator>::operator[](int n)
{
  MAssert(mpBuffer, "Accessing an empty array"); 
  MAssert(n >= 0 && n < mSize, "Invalid array index");  

  return mpBuffer[n];
}

template<typename T, class TAllocator>
MForceInline const T& TArray<T, TAllocator>::operator[](int n)const
{
  MAssert(mpBuffer, "Accessing an empty array"); 
  MAssert(n >= 0 && n < mSize, "Invalid array index");  

  return mpBuffer[n];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline const T& TArray<T, TAllocator>::First()const
{
  return (*this)[0];
}

template<typename T, class TAllocator>
MForceInline T& TArray<T, TAllocator>::First()
{
  return (*this)[0];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline const T& TArray<T, TAllocator>::Last()const
{
  return (*this)[Size() - 1];
}

template<typename T, class TAllocator>
MForceInline T& TArray<T, TAllocator>::Last()
{
  return (*this)[Size() - 1];
}

// -------------------------------------------------------------------------------------------------
 
template<typename T, class TAllocator>
MForceInline const T* TArray<T, TAllocator>::FirstRead()const
{
  return mpBuffer;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline T* TArray<T, TAllocator>::FirstWrite()const
{
  return mpBuffer;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TArray<T, TAllocator>::Realloc(int NewSize)
{
  MAssert(NewSize > mSize && NewSize, "");

  T* pNewBuffer = TAllocator::SNew((size_t)NewSize);

  
  // ...  copy old Values 
  
  if (mSize)
  {
    TArrayCopy<T>::SCopy(pNewBuffer, mpBuffer, mSize);
  }

  
  // ... assign the new Pointer

  if (mpBuffer)
  {
    TAllocator::SDelete(mpBuffer, (size_t)mSize);
  }

  mpBuffer = pNewBuffer;
  mSize = NewSize;   
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline typename TArray<T, TAllocator>::TConstIterator 
  TArray<T, TAllocator>::Begin()const
{
  return TConstIterator(this, 0);
}

template<typename T, class TAllocator>
MForceInline typename TArray<T, TAllocator>::TIterator 
  TArray<T, TAllocator>::Begin()
{
  return TIterator(this, 0);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline typename TArray<T, TAllocator>::TConstIterator 
  TArray<T, TAllocator>::End()const
{
  return TConstIterator(const_cast<TArray<T, TAllocator>*>(this), Size());
}

template<typename T, class TAllocator>
MForceInline typename TArray<T, TAllocator>::TIterator 
  TArray<T, TAllocator>::End()
{
  return TIterator(this, Size());
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline typename TArray<T, TAllocator>::TConstIterator 
  TArray<T, TAllocator>::IterAt(int Index)const
{
  // yes, <= Size(), as the End is also allowed
  MAssert(Index >= 0 && Index <= Size(), "");
  return TConstIterator(const_cast<TArray<T, TAllocator>*>(this), Index);
}

template<typename T, class TAllocator>
MForceInline typename TArray<T, TAllocator>::TIterator 
  TArray<T, TAllocator>::IterAt(int Index)
{
  // yes, <= Size(), as the End is also allowed
  MAssert(Index >= 0 && Index <= Size(), "");
  return TIterator(this, Index);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline TArray<T, TAllocator>::TConstIterator::TConstIterator()
  : mpArray(NULL), mIndex(0) 
{
}

template<typename T, class TAllocator>
inline TArray<T, TAllocator>::TConstIterator::TConstIterator(
  TArray<T, TAllocator>* pArray, 
  int                    Index)
  : mpArray(pArray), mIndex(Index) 
{
}

template<typename T, class TAllocator>
inline TArray<T, TAllocator>::TConstIterator::TConstIterator(
  const TIterator& Iter)
  : mpArray(Iter.mpArray), mIndex(Iter.mIndex)
{
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline int TArray<T, TAllocator>::TConstIterator::Index()const
{
   return mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TConstIterator& 
  TArray<T, TAllocator>::TConstIterator::operator++() 
{
  ++mIndex; 
  return *this;
}

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TConstIterator 
  TArray<T, TAllocator>::TConstIterator::operator++(int) 
{
  MAssert(!IsEnd(), "Cant iterate to behind the end!");
  
  TIterator Temp(*this); 
  ++(*this); 
  
  return Temp;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TConstIterator& 
  TArray<T, TAllocator>::TConstIterator::operator--() 
{
  MAssert(!IsHead(), "Cant iterate to before the start!");
  --mIndex; 
  
  return *this;
}

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TConstIterator 
  TArray<T, TAllocator>::TConstIterator::operator--(int) 
{
  TConstIterator Temp(*this); 
  --(*this); 
  
  return Temp;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TConstIterator& 
  TArray<T, TAllocator>::TConstIterator::operator+=(ptrdiff_t Distance)
{
  mIndex = (int)(mIndex + Distance); 
  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TConstIterator 
  TArray<T, TAllocator>::TConstIterator::operator+(ptrdiff_t Distance)const
{
  return TConstIterator(mpArray, (int)(mIndex + Distance));
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TConstIterator& 
  TArray<T, TAllocator>::TConstIterator::operator-=(ptrdiff_t Distance)
{
  mIndex = (int)(mIndex - Distance); 
  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TConstIterator 
  TArray<T, TAllocator>::TConstIterator::operator-(ptrdiff_t Distance)const
{
  return TConstIterator(mpArray, (int)(mIndex - Distance));
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline ptrdiff_t TArray<T, TAllocator>::TConstIterator::operator+(
  const TConstIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return mIndex + Other.mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline ptrdiff_t TArray<T, TAllocator>::TConstIterator::operator-(
  const TConstIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return mIndex - Other.mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TConstIterator::operator==(
  const TConstIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex == Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TConstIterator::operator!=(
  const TConstIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex != Other.mIndex);
}
  
// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TConstIterator::operator<(
  const TConstIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex < Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TConstIterator::operator>(
  const TConstIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex > Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TConstIterator::operator<=(
  const TConstIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex <= Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TConstIterator::operator>=(
  const TConstIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex >= Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline const T& TArray<T, TAllocator>::TConstIterator::operator[](
  ptrdiff_t Index)const
{
  return (*mpArray)[mIndex][Index];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline const T& TArray<T, TAllocator>::TConstIterator::operator*()const 
{
  return (*mpArray)[mIndex];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline const T* TArray<T, TAllocator>::TConstIterator::operator->()const 
{
  return &(*mpArray)[mIndex];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TConstIterator::IsValid()const
{
  MAssert((mpArray == NULL) || 
    (mIndex >= 0 && mIndex <= mpArray->Size()), "Invalid iterator");
  
  return (mpArray != NULL) && !IsEnd();
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TConstIterator::IsEnd()const 
{
  MAssert(mIndex >= 0 && mIndex <= mpArray->Size(), "Invalid iterator");
  return (mIndex >= mpArray->Size());
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TConstIterator::IsHead()const
{
  MAssert(mIndex >= 0 && mIndex <= mpArray->Size(), "Invalid iterator");
  return !mpArray->IsEmpty() && (mIndex == 0);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TConstIterator::IsLast()const
{
  MAssert(mIndex >= 0 && mIndex <= mpArray->Size(), "Invalid iterator");
  return !mpArray->IsEmpty() && (mIndex == mpArray->Size() - 1);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline TArray<T, TAllocator>::TIterator::TIterator()
  : mpArray(NULL), mIndex(0)
{
}

template<typename T, class TAllocator>
inline TArray<T, TAllocator>::TIterator::TIterator(
  TArray<T, TAllocator>* pArray, 
  int                    Index)
  : mpArray(pArray), mIndex(Index)
{
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline int TArray<T, TAllocator>::TIterator::Index()const
{
   return mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TIterator& 
  TArray<T, TAllocator>::TIterator::operator++() 
{
  ++mIndex; 
  return *this;
}

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TIterator 
  TArray<T, TAllocator>::TIterator::operator++(int) 
{
  MAssert(!IsEnd(), "Cant iterate to behind the end!");
  
  TIterator Temp(*this); 
  ++(*this); 
  
  return Temp;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TIterator& 
  TArray<T, TAllocator>::TIterator::operator--() 
{
  MAssert(!IsHead(), "Cant iterate to before the start!");
  --mIndex; 
  
  return *this;
}

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TIterator 
  TArray<T, TAllocator>::TIterator::operator--(int) 
{
  TIterator Temp(*this); 
  --(*this); 
  
  return Temp;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TIterator& 
  TArray<T, TAllocator>::TIterator::operator+=(ptrdiff_t Distance)
{
  mIndex = (int)(mIndex + Distance); 
  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TIterator 
  TArray<T, TAllocator>::TIterator::operator+(ptrdiff_t Distance)const
{
  return TIterator(mpArray, (int)(mIndex + Distance));
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TIterator& 
  TArray<T, TAllocator>::TIterator::operator-=(ptrdiff_t Distance)
{
  mIndex = (int)(mIndex - Distance); 
  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TArray<T, TAllocator>::TIterator 
  TArray<T, TAllocator>::TIterator::operator-(ptrdiff_t Distance)const
{
  return TIterator(mpArray, (int)(mIndex - Distance));
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline ptrdiff_t TArray<T, TAllocator>::TIterator::operator+(
  const TIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return mIndex + Other.mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline ptrdiff_t TArray<T, TAllocator>::TIterator::operator-(
  const TIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return mIndex - Other.mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TIterator::operator==(
  const TIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex == Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TIterator::operator!=(
  const TIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex != Other.mIndex);
}
  
// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TIterator::operator<(
  const TIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex < Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TIterator::operator>(
  const TIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex > Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TIterator::operator<=(
  const TIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex <= Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TIterator::operator>=(
  const TIterator& Other)const
{
  MAssert(Other.mpArray == mpArray, "Expected the same list!");
  return (mIndex >= Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline T& TArray<T, TAllocator>::TIterator::operator[](ptrdiff_t Index)const
{
  return (*mpArray)[mIndex][Index];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline T& TArray<T, TAllocator>::TIterator::operator*()const 
{
  return (*mpArray)[mIndex];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline T* TArray<T, TAllocator>::TIterator::operator->()const 
{
  return &(*mpArray)[mIndex];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TIterator::IsValid()const
{
  MAssert((mpArray == NULL) || 
    (mIndex >= 0 && mIndex <= mpArray->Size()), "Invalid iterator");
  
  return (mpArray != NULL) && !IsEnd();
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TIterator::IsEnd()const 
{
  MAssert(mIndex >= 0 && mIndex <= mpArray->Size(), "Invalid iterator");
  return (mIndex >= mpArray->Size());
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TIterator::IsHead()const
{
  MAssert(mIndex >= 0 && mIndex <= mpArray->Size(), "Invalid iterator");
  return !mpArray->IsEmpty() && (mIndex == 0);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TArray<T, TAllocator>::TIterator::IsLast()const
{
  MAssert(mIndex >= 0 && mIndex <= mpArray->Size(), "Invalid iterator");
  return !mpArray->IsEmpty() && (mIndex == mpArray->Size() - 1);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------
  
template<typename T, class TAllocator, class TOtherAllocator>
bool operator== (
  const TArray<T, TAllocator>&      First, 
  const TArray<T, TOtherAllocator>& Second)
{
  if (First.Size() != Second.Size())
  {
    return false;
  }
  
  const int Len = First.Size();
  
  for (int i = 0; i < Len; ++i)
  {
    if (First[i] != Second[i])
    {
      return false;
    }
  }
  
  return true;
}

// -------------------------------------------------------------------------------------------------
  
template<typename T, class TAllocator, class TOtherAllocator>
bool operator!= (
  const TArray<T, TAllocator>&      First, 
  const TArray<T, TOtherAllocator>& Second)
{
  return !operator==(First, Second);
}


#endif // _Array_inl_

