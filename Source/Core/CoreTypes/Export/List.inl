#pragma once

#ifndef _List_inl_
#define _List_inl_

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/CompilerDefines.h"
#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/InlineMath.h"

#if defined(MCompiler_Has_RValue_References)
  #include <utility> // std::move
#endif

// =================================================================================================

namespace TListHelper {
  //! small helper function to determine how much memory we should
  //! preallocate when changing the size of a List
  int SExpandListSize(size_t SizeOfT, int CurrentSize);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TList<T, TAllocator>::TList()
  : mPhysicalSize(0), 
    mSize(0), 
    mpBuffer(NULL)
{
}

template<typename T, class TAllocator>
TList<T, TAllocator>::TList(const TList& Other)
  : mPhysicalSize(0), 
    mSize(0), 
    mpBuffer(NULL)
{
  InternalCopy(Other.mpBuffer, Other.mSize, Other.mPhysicalSize);
}

#if defined(MCompiler_Has_RValue_References)

template<typename T, class TAllocator>
TList<T, TAllocator>::TList(TList&& Other)
  : mPhysicalSize(Other.mPhysicalSize), 
    mSize(Other.mSize), 
    mpBuffer(Other.mpBuffer)
{
  // moved buffer
  Other.mPhysicalSize = 0;
  Other.mSize = 0; 
  Other.mpBuffer = NULL;
}

#endif

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::operator= (const TList& Other)
{
  if (this != &Other)
  {
    // copy data from other to this
    InternalCopy(Other.mpBuffer, Other.mSize, Other.mPhysicalSize);
  }

  return *this;
}

#if defined(MCompiler_Has_RValue_References)

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::operator= (TList&& Other)
{
  if (this != &Other)
  {
    // move data from other to this
    Empty();

    mpBuffer = Other.mpBuffer;
    mPhysicalSize = Other.mPhysicalSize;
    mSize = Other.mSize;

    Other.mpBuffer = NULL;
    Other.mPhysicalSize = 0;
    Other.mSize = 0;
  }

  return *this;
}

#endif

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TList<T, TAllocator>::~TList()
{
  Empty();
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::IsEmpty()const
{
  return (mSize == 0);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline int TList<T, TAllocator>::Size()const
{
  return mSize;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline int TList<T, TAllocator>::PreallocatedSpace()const
{
  return mPhysicalSize;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TList<T, TAllocator>::PreallocateSpace(int NumEntries)
{
  MAssert(NumEntries >= Size(), "Deleting elements this way is not allowed!");
  InternalExpand(NumEntries);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TList<T, TAllocator>::Empty()
{
  if (mpBuffer)
  {
    TAllocator::SDelete(mpBuffer, mPhysicalSize);
    mpBuffer = NULL;

    mPhysicalSize = 0;
    mSize = 0;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TList<T, TAllocator>::ClearEntries()
{
  if (mpBuffer)
  {
    TArrayInitializer<T>::SInitialize(mpBuffer, mSize);
    mSize = 0;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline T& TList<T, TAllocator>::ObjectAt(int Index)
{
  MAssert(mpBuffer && Index >= 0 && Index < mSize, "Invalid Index");
  return mpBuffer[Index];
}

template<typename T, class TAllocator>
inline const T& TList<T, TAllocator>::ObjectAt(int Index)const
{
  MAssert(mpBuffer && Index >= 0 && Index < mSize, "Invalid Index");
  return mpBuffer[Index];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline const T& TList<T, TAllocator>::operator[](int Index)const
{
  MAssert(mpBuffer && Index >= 0 && Index < mSize, "Invalid Index");
  return mpBuffer[Index];
}

template<typename T, class TAllocator>
MForceInline T& TList<T, TAllocator>::operator[](int Index)
{
  MAssert(mpBuffer && Index >= 0 && Index < mSize, "Invalid Index");
  return mpBuffer[Index];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline const T& TList<T, TAllocator>::First()const
{
  return (*this)[0];
}

template<typename T, class TAllocator>
inline T& TList<T, TAllocator>::First()
{
  return (*this)[0];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline const T& TList<T, TAllocator>::Last()const
{
  return (*this)[Size() - 1];
}

template<typename T, class TAllocator>
inline T& TList<T, TAllocator>::Last()
{
  return (*this)[Size() - 1];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::Init(const T &Value)
{
  for (int i = 0; i < mSize; ++i)
  {
    mpBuffer[i] = Value;
  }

  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TList<T, TAllocator>::Swap(int Position1, int Position2)
{
  MAssert(Position1 >= 0 && Position1 < mSize, "Invalid Index");
  MAssert(Position2 >= 0 && Position2 < mSize, "Invalid Index");

  TMathT<T>::Swap(mpBuffer[Position1], mpBuffer[Position2]);
}

template<typename T, class TAllocator>
void TList<T, TAllocator>::Swap(TIterator& Iter1, TIterator& Iter2)
{
  MAssert(!Iter1.IsEnd() && !Iter2.IsEnd(), "Invalid Iter");

  TMathT<T>::Swap(mpBuffer[Iter1.mIndex], mpBuffer[Iter2.mIndex]);
  TMathT<int>::Swap(Iter1.mIndex, Iter2.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::Reverse()
{
  for (int i = 0; i < mSize / 2; ++i)
  {
    TMathT<T>::Swap(mpBuffer[i], mpBuffer[mSize - i - 1]);
  }

  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
bool TList<T, TAllocator>::Contains(const T& Entry)const
{
  return (Find(Entry) != -1);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
int TList<T, TAllocator>::Find(const T& Entry)const
{
  for (int i = 0; i < mSize; ++i)
  {
    if (mpBuffer[i] == Entry)
    {
      return i;
    }
  }

  return -1;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
typename TList<T, TAllocator>::TConstIterator
  TList<T, TAllocator>::FindIter(const T& Entry)const
{
  const int Index = Find(Entry);

  if (Index == -1)
  {
    return TConstIterator(const_cast<TList<T, TAllocator>*>(this), Size());
  }
  else
  {
    return TConstIterator(const_cast<TList<T, TAllocator>*>(this), Index);
  }
}

template<typename T, class TAllocator>
typename TList<T, TAllocator>::TIterator
  TList<T, TAllocator>::FindIter(const T& Entry)
{
  const int Index = Find(Entry);

  if (Index == -1)
  {
    return TIterator(const_cast<TList<T, TAllocator>*>(this), Size());
  }
  else
  {
    return TIterator(const_cast<TList<T, TAllocator>*>(this), Index);
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
int TList<T, TAllocator>::FindWithBinarySearch(const T& Entry)const
{
  const int LowerBoundIndex = FindLowerBound(Entry);

  return (LowerBoundIndex < Size() && ObjectAt(LowerBoundIndex) == Entry) ?
    LowerBoundIndex : -1;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
typename TList<T, TAllocator>::TConstIterator
  TList<T, TAllocator>::FindIterWithBinarySearch(const T& Entry)const
{
  const TConstIterator LowerBoundIter = FindLowerBoundIter(Entry);

  return (LowerBoundIter.IsEnd() || *LowerBoundIter != Entry) ?
    End() : LowerBoundIter;
}

template<typename T, class TAllocator>
typename TList<T, TAllocator>::TIterator
  TList<T, TAllocator>::FindIterWithBinarySearch(const T& Entry)
{
  const TIterator LowerBoundIter = FindLowerBoundIter(Entry);

  return (LowerBoundIter.IsEnd() || *LowerBoundIter != Entry) ?
    End() : LowerBoundIter;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
int TList<T, TAllocator>::Find(
  const T&  Entry,
  int       StartIndex,
  bool      Backwards)const
{
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

template<typename T, class TAllocator>
int TList<T, TAllocator>::FindNonEqual(
  const T&  Entry,
  int       StartIndex,
  bool      Backwards)const
{
  if (Backwards)
  {
    for (int i = StartIndex; i >= 0; --i)
    {
      if (mpBuffer[i] != Entry)
      {
        return i;
      }
    }
  }
  else
  {
    for (int i = StartIndex; i < mSize; ++i)
    {
      if (mpBuffer[i] != Entry)
      {
        return i;
      }
    }
  }

  return -1;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
int TList<T, TAllocator>::FindLowerBound(const T& Value)const
{
  return FindLowerBoundIter(Value).Index();
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
typename TList<T, TAllocator>::TConstIterator
  TList<T, TAllocator>::FindLowerBoundIter(const T& Entry)const
{
  return std::lower_bound(Begin(), End(), Entry);
}

template<typename T, class TAllocator>
typename TList<T, TAllocator>::TIterator
  TList<T, TAllocator>::FindLowerBoundIter(const T& Entry)
{
  return std::lower_bound(Begin(), End(), Entry);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
bool TList<T, TAllocator>::FindAndDeleteFirst(const T& Entry)
{
  for (int i = 0; i < mSize; ++i)
  {
    if (mpBuffer[i] == Entry)
    {
      Delete(i);
      return true;
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
bool TList<T, TAllocator>::FindAndDeleteLast(const T& Entry)
{
  for (int i = mSize - 1; i >= 0; --i)
  {
    if (mpBuffer[i] == Entry)
    {
      Delete(i);
      return true;
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
bool TList<T, TAllocator>::FindAndDeleteAll(const T& Entry)
{
  bool FoundOne = false;

  for (int i = 0; i < mSize; ++i)
  {
    if (mpBuffer[i] == Entry)
    {
      Delete(i--);
      FoundOne = true;
    }
  }

  return FoundOne;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
int TList<T, TAllocator>::IndexOf(const T& Entry)const
{
  if (mpBuffer)
  {
    if (&Entry >= &mpBuffer[0] && &Entry < &mpBuffer[mSize])
    {
      return (int)(&Entry - &mpBuffer[0]);
    }
  }

  MInvalid("Not in our list!");
  return -1;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline TList<T, TAllocator>& TList<T, TAllocator>::Prepend(const T &Value)
{
  return Insert(Value, 0);
}

template<typename T, class TAllocator>
inline TList<T, TAllocator>& TList<T, TAllocator>::Prepend(
  const TList<T, TAllocator> &List)
{
  return Insert(List, 0);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline TList<T, TAllocator>& TList<T, TAllocator>::Append(const T &Value)
{
  return Insert(Value, Size());
}

template<typename T, class TAllocator>
inline TList<T, TAllocator>& TList<T, TAllocator>::Append(
  const TList<T, TAllocator> &List)
{
  return Insert(List, Size());
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::Delete(TIterator& Iter)
{
  return Delete(Iter.mIndex);
}

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::Delete(int Index)
{
  MAssert(mpBuffer, "Nothing to delete");
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

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::DeleteRange(
  int StartIndex,
  int EndIndex)
{
  MAssert(StartIndex >= 0 && StartIndex < mSize, "Invalid Index");
  MAssert(EndIndex >= 0 && EndIndex < mSize, "Invalid Index");

  MAssert(EndIndex >= StartIndex, "Invalid Index");

  // shunt
  const int MoveRange = mSize - EndIndex - 1;
  TArrayCopy<T>::SMove(&mpBuffer[StartIndex], 
    &mpBuffer[EndIndex + 1], MoveRange);

  // clear freed range
  const int NewEnd = EndIndex + MoveRange;
  TArrayInitializer<T>::SInitialize(&mpBuffer[NewEnd], mSize - NewEnd);

  mSize -= (EndIndex - StartIndex + 1);

  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::Insert(
  const T&    Value,
  TIterator&  Iter)
{
  Insert(Value, Iter.mIndex);
  ++Iter.mIndex;

  return *this;
}

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::Insert(
  const T&  Value,
  int       Index)
{
  MAssert(Index >= 0 && Index <= mSize, "Invalid Index");

  // expand
  if (mSize + 1 > mPhysicalSize)
  {
    InternalExpand(TListHelper::SExpandListSize(sizeof(T), mSize));
  }

  // shunt
  if (Index < mSize)
  {
    TArrayCopy<T>::SMove(&mpBuffer[Index + 1], 
      &mpBuffer[Index], mSize - Index);
  }

  // copy new item (avoid using TArrayCopy for a single item)
  mpBuffer[Index] = Value;

  ++mSize;

  return *this;
}

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::Insert(
  const TList<T, TAllocator>& List,
  int                         Index)
{
  MAssert(Index >= 0 && Index <= mSize, "Invalid Index");

  if (List.mSize)
  {
    // expand
    if (mSize + List.mSize > mPhysicalSize)
    {
      InternalExpand(mSize + List.mSize);
    }

    // shunt
    if (Index < mSize)
    {
      TArrayCopy<T>::SMove(&mpBuffer[Index + List.mSize],
        &mpBuffer[Index], mSize - Index);
    }

    // copy new items
    TArrayCopy<T>::SCopy(&mpBuffer[Index], List.mpBuffer, List.mSize);
  
    mSize += List.mSize;
  }

  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::DeleteFirst()
{
  MAssert(mpBuffer && mSize, "Nothing to delete");
  return Delete(0);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::DeleteLast()
{
  MAssert(mpBuffer && mSize, "Nothing to delete");
  return Delete(mSize - 1);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
T TList<T, TAllocator>::GetAndDeleteLast()
{
  MAssert(Size() > 0, "Nothing to delete");

  T Temp = mpBuffer[Size() - 1];
  Delete(Size() - 1);

  return Temp;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
T TList<T, TAllocator>::GetAndDeleteFirst()
{
  MAssert(Size() > 0, "Nothing to delete");

  T Temp = mpBuffer[0];
  Delete(0);

  return Temp;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TList<T, TAllocator> TList<T, TAllocator>::ItemsInRange(
  int StartIndex,
  int EndIndex)const
{
  MAssert(StartIndex >= 0 && StartIndex < mSize, "Invalid Index");
  MAssert(EndIndex >= 0 && EndIndex < mSize, "Invalid Index");
  MAssert(EndIndex >= StartIndex, "Invalid Index");

  TList<T, TAllocator> Ret;
  Ret.PreallocateSpace(EndIndex - StartIndex + 1);

  for (int i = StartIndex; i <= EndIndex; ++i)
  {
    Ret.Append(mpBuffer[i]);
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline const T* TList<T, TAllocator>::FirstRead()const
{
  return mpBuffer;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::operator+= (const T &Value)
{
  Append(Value);
  return *this;
}

template<typename T, class TAllocator>
TList<T, TAllocator>& TList<T, TAllocator>::operator+= (
  const TList<T, TAllocator>& List)
{
  Append(List);
  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline typename TList<T, TAllocator>::TConstIterator 
  TList<T, TAllocator>::Begin()const
{
  return TConstIterator(const_cast<TList<T, TAllocator>*>(this), 0);
}

template<typename T, class TAllocator>
MForceInline typename TList<T, TAllocator>::TIterator 
  TList<T, TAllocator>::Begin()
{
  return TIterator(this, 0);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
MForceInline typename TList<T, TAllocator>::TConstIterator 
  TList<T, TAllocator>::End()const
{
  return TConstIterator(const_cast<TList<T, TAllocator>*>(this), Size());
}

template<typename T, class TAllocator>
MForceInline typename TList<T, TAllocator>::TIterator 
  TList<T, TAllocator>::End()
{
  return TIterator(this, Size());
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TConstIterator
  TList<T, TAllocator>::IterAt(int Index)const
{
  // yes, <= Size(), as the End is also allowed
  MAssert(Index >= 0 && Index <= Size(), "");
  return TConstIterator(const_cast<TList<T, TAllocator>*>(this), Index);
}

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TIterator
  TList<T, TAllocator>::IterAt(int Index)
{
  // yes, <= Size(), as the End is also allowed
  MAssert(Index >= 0 && Index <= Size(), "");
  return TIterator(this, Index);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline int TList<T, TAllocator>::IndexOf(
  const typename TList<T, TAllocator>::TIterator& Iter)const
{
  return Iter.mIndex;
}

template<typename T, class TAllocator>
inline int TList<T, TAllocator>::IndexOf(
  const typename TList<T, TAllocator>::TConstIterator& Iter)const
{
  return Iter.mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TList<T, TAllocator>::InternalExpand(int NewSize)
{
  MAssert(NewSize >= 0 && NewSize >= mSize, "Invalid NewSize");

  if (NewSize > mPhysicalSize)
  {
    T* pNewBuffer = TAllocator::SNew((size_t)NewSize);

    if (mSize)
    {
      TArrayCopy<T>::SCopy(pNewBuffer, mpBuffer, mSize);
    }

    if (mPhysicalSize)
    {
      TAllocator::SDelete(mpBuffer, (size_t)mPhysicalSize);
    }

    mpBuffer = pNewBuffer;
    mPhysicalSize = NewSize;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
void TList<T, TAllocator>::InternalCopy(
  const T*  pOtherBuffer,
  int       OtherSize,
  int       OtherPhysicalSize)
{
  // reallocate buffer, if needed
  if (OtherSize != mSize)
  {
    if (mPhysicalSize != OtherPhysicalSize)
    {
      // reallocate
      if (mpBuffer)
      {
        TAllocator::SDelete(mpBuffer, mPhysicalSize);

        // in case that out of memory is thrown
        mpBuffer = NULL;
        mPhysicalSize = 0;
        mSize = 0;
      }

      if (OtherSize > 0)
      {
        mpBuffer = TAllocator::SNew(OtherSize);
        mPhysicalSize = OtherSize;
        mSize = OtherSize;
      }
    }
    else
    {
      // set new size only
      MAssert(mPhysicalSize >= OtherSize, "Unexpected buffer size");

      // but clear new empty space first before copying from other
      if (mSize > OtherSize)
      {
        TArrayInitializer<T>::SInitialize(
          &mpBuffer[OtherSize], mSize - OtherSize);
      }

      mSize = OtherSize;
    }
  }

  // copy items
  if (OtherSize > 0)
  {
    TArrayCopy<T>::SCopy(mpBuffer, pOtherBuffer, OtherSize);
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline TList<T, TAllocator>::TConstIterator::TConstIterator()
  : mpList(NULL), mIndex(0)
{
}

template<typename T, class TAllocator>
inline TList<T, TAllocator>::TConstIterator::TConstIterator(
  TList<T, TAllocator>* pList,
  int                   Index)
  : mpList(pList), mIndex(Index)
{
}

template<typename T, class TAllocator>
inline TList<T, TAllocator>::TConstIterator::TConstIterator(
  const TIterator& Iter)
  : mpList(Iter.mpList), mIndex(Iter.mIndex)
{
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline int TList<T, TAllocator>::TConstIterator::Index()const
{
   return mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TConstIterator&
  TList<T, TAllocator>::TConstIterator::operator++()
{
  ++mIndex;
  return *this;
}

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TConstIterator
  TList<T, TAllocator>::TConstIterator::operator++(int)
{
  MAssert(!IsEnd(), "Cant iterate to behind the end!");

  TIterator Temp(*this);
  ++(*this);

  return Temp;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TConstIterator&
  TList<T, TAllocator>::TConstIterator::operator--()
{
  MAssert(!IsHead(), "Cant iterate to before the start!");
  --mIndex;

  return *this;
}

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TConstIterator
  TList<T, TAllocator>::TConstIterator::operator--(int)
{
  TConstIterator Temp(*this);
  --(*this);

  return Temp;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TConstIterator&
  TList<T, TAllocator>::TConstIterator::operator+=(ptrdiff_t Distance)
{
  mIndex = (int)(mIndex + Distance);
  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TConstIterator
  TList<T, TAllocator>::TConstIterator::operator+(ptrdiff_t Distance)const
{
  return TConstIterator(mpList, (int)(mIndex + Distance));
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TConstIterator&
  TList<T, TAllocator>::TConstIterator::operator-=(ptrdiff_t Distance)
{
  mIndex = (int)(mIndex - Distance);
  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TConstIterator
  TList<T, TAllocator>::TConstIterator::operator-(ptrdiff_t Distance)const
{
  return TConstIterator(mpList, (int)(mIndex - Distance));
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline ptrdiff_t TList<T, TAllocator>::TConstIterator::operator+(
  const TConstIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return mIndex + Other.mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline ptrdiff_t TList<T, TAllocator>::TConstIterator::operator-(
  const TConstIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return mIndex - Other.mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TConstIterator::operator==(
  const TConstIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex == Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TConstIterator::operator!=(
  const TConstIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex != Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TConstIterator::operator<(
  const TConstIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex < Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TConstIterator::operator>(
  const TConstIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex > Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TConstIterator::operator<=(
  const TConstIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex <= Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TConstIterator::operator>=(
  const TConstIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex >= Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline const T& TList<T, TAllocator>::TConstIterator::operator[](
  ptrdiff_t Index)const
{
  return (*mpList)[mIndex][Index];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline const T& TList<T, TAllocator>::TConstIterator::operator*()const
{
  return (*mpList)[mIndex];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline const T* TList<T, TAllocator>::TConstIterator::operator->()const
{
  return &(*mpList)[mIndex];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TConstIterator::IsValid()const
{
  MAssert((mpList == NULL) ||
    (mIndex >= 0 && mIndex <= mpList->Size()), "Invalid iterator");

  return (mpList != NULL) && !IsEnd();
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TConstIterator::IsEnd()const
{
  MAssert(mIndex >= 0 && mIndex <= mpList->Size(), "Invalid iterator");
  return (mIndex >= mpList->Size());
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TConstIterator::IsHead()const
{
  MAssert(mIndex >= 0 && mIndex <= mpList->Size(), "Invalid iterator");
  return !mpList->IsEmpty() && (mIndex == 0);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TConstIterator::IsLast()const
{
  MAssert(mIndex >= 0 && mIndex <= mpList->Size(), "Invalid iterator");
  return !mpList->IsEmpty() && (mIndex == mpList->Size() - 1);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline TList<T, TAllocator>::TIterator::TIterator()
  : mpList(NULL), mIndex(0)
{
}

template<typename T, class TAllocator>
inline TList<T, TAllocator>::TIterator::TIterator(
  TList<T, TAllocator>* pList,
  int                   Index)
  : mpList(pList), mIndex(Index)
{
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline int TList<T, TAllocator>::TIterator::Index()const
{
  return mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TIterator&
  TList<T, TAllocator>::TIterator::operator++()
{
  ++mIndex;
  return *this;
}

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TIterator
  TList<T, TAllocator>::TIterator::operator++(int)
{
  MAssert(!IsEnd(), "Cant iterate to behind the end!");

  TIterator Temp(*this);
  ++(*this);

  return Temp;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TIterator&
  TList<T, TAllocator>::TIterator::operator--()
{
  MAssert(!IsHead(), "Cant iterate to before the start!");
  --mIndex;

  return *this;
}

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TIterator
  TList<T, TAllocator>::TIterator::operator--(int)
{
  TIterator Temp(*this);
  --(*this);

  return Temp;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TIterator&
  TList<T, TAllocator>::TIterator::operator+=(ptrdiff_t Distance)
{
  mIndex = (int)(mIndex + Distance);
  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TIterator
  TList<T, TAllocator>::TIterator::operator+(ptrdiff_t Distance)const
{
  return TIterator(mpList, (int)(mIndex + Distance));
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TIterator&
  TList<T, TAllocator>::TIterator::operator-=(ptrdiff_t Distance)
{
  mIndex = (int)(mIndex - Distance);
  return *this;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline typename TList<T, TAllocator>::TIterator
  TList<T, TAllocator>::TIterator::operator-(ptrdiff_t Distance)const
{
  return TIterator(mpList, (int)(mIndex - Distance));
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline ptrdiff_t TList<T, TAllocator>::TIterator::operator+(
  const TIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return mIndex + Other.mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline ptrdiff_t TList<T, TAllocator>::TIterator::operator-(
  const TIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return mIndex - Other.mIndex;
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TIterator::operator==(
  const TIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex == Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TIterator::operator!=(
  const TIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex != Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TIterator::operator<(
  const TIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex < Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TIterator::operator>(
  const TIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex > Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TIterator::operator<=(
  const TIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex <= Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TIterator::operator>=(
  const TIterator& Other)const
{
  MAssert(Other.mpList == mpList, "Expected the same list!");
  return (mIndex >= Other.mIndex);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline T& TList<T, TAllocator>::TIterator::operator[](ptrdiff_t Index)const
{
  return (*mpList)[mIndex][Index];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline T& TList<T, TAllocator>::TIterator::operator*()const
{
  return (*mpList)[mIndex];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline T* TList<T, TAllocator>::TIterator::operator->()const
{
  return &(*mpList)[mIndex];
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TIterator::IsValid()const
{
  MAssert((mpList == NULL) ||
    (mIndex >= 0 && mIndex <= mpList->Size()), "Invalid iterator");

  return (mpList != NULL) && !IsEnd();
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TIterator::IsEnd()const
{
  MAssert(mIndex >= 0 && mIndex <= mpList->Size(), "Invalid iterator");
  return (mIndex >= mpList->Size());
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TIterator::IsHead()const
{
  MAssert(mIndex >= 0 && mIndex <= mpList->Size(), "Invalid iterator");
  return !mpList->IsEmpty() && (mIndex == 0);
}

// -------------------------------------------------------------------------------------------------

template<typename T, class TAllocator>
inline bool TList<T, TAllocator>::TIterator::IsLast()const
{
  MAssert(mIndex >= 0 && mIndex <= mpList->Size(), "Invalid iterator");
  return !mpList->IsEmpty() && (mIndex == mpList->Size() - 1);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T, class TAllocator, typename U, class TOtherAllocator>
bool operator==(
  const TList<T, TAllocator>&       First,
  const TList<U, TOtherAllocator>&  Second)
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

template <typename T, class TAllocator, typename U, class TOtherAllocator>
bool operator!=(
  const TList<T, TAllocator>&       First,
  const TList<U, TOtherAllocator>&  Second)
{
  return !(operator==(First, Second));
}

// -------------------------------------------------------------------------------------------------

template <typename T, class TAllocator, typename U, class TOtherAllocator>
TList<T, TAllocator> operator+(
  const TList<T, TAllocator>&       First,
  const TList<U, TOtherAllocator>&  Second)
{
  TList<T, TAllocator> Temp(First);
  Temp += Second;

  return Temp;
}

template <typename T, class TAllocator, typename U>
TList<T, TAllocator> operator+(
  const TList<T, TAllocator>& List,
  const U&                    Entry)
{
  TList<T, TAllocator> Temp(List);
  Temp += Entry;

  return Temp;
}

#if defined(MCompiler_Has_RValue_References)

  template <typename T, class TAllocator, typename U>
  TList<T, TAllocator> operator+(
    TList<T, TAllocator>&&  List,
    const U&                Entry)
  {
    List += Entry;
    return std::move(List);
  }

  template <typename T, class TAllocator, typename U, class TOtherAllocator>
  TList<T, TAllocator> operator+(
    TList<T, TAllocator>&&            First,
    const TList<U, TOtherAllocator>&  Second)
  {
    First += Second;
    return std::move(First);
  }

#endif // MCompiler_Has_RValue_References


#endif // _List_inl_

