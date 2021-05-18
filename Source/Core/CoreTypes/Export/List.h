#pragma once

#ifndef _List_h_
#define _List_h_
    
// =================================================================================================

#include "CoreTypes/Export/Allocator.h"
#include "CoreTypes/Export/CompilerDefines.h"
#include "CoreTypes/Export/StlIterator.h"
#include "CoreTypes/Export/TypeTraits.h"

#include <algorithm> // for TList::Sort

// =================================================================================================

/*!
  * A simple, dynamic vector.
!*/

template<typename T, class TAllocator = TDefaultArrayAllocator<T> > 
class TList
{
public:
  typedef T value_type;

  class TIterator;
  class TConstIterator;

  TList();
  
  //! copy list data from one list to another
  TList(const TList& Other);
  TList& operator= (const TList& Other);

  //! move list data from one list to another
  #if defined(MCompiler_Has_RValue_References)
    TList(TList&& Other);
    TList& operator= (TList&& Other);
  #endif

  //! explicit conversion/copying from lists with different allocators
  template<typename TOtherAllocator>
  explicit TList(const TList<T, TOtherAllocator>& Other)
    : mPhysicalSize(0), mSize(0), mpBuffer(NULL)
  {
    *this = Other;
  }
  template<typename TOtherAllocator>
  TList<T, TAllocator>& operator= (const TList<T, TOtherAllocator>& Other)
  {
    InternalCopy(Other.mpBuffer, Other.mSize, Other.mPhysicalSize);
    return *this;
  }

  //! explicit conversion/copying from lists with convertible types 
  //! and maybe different allocators
  template<typename U, typename TOtherAllocator>
  explicit TList(const TList<U, TOtherAllocator>& Other)
    : mPhysicalSize(0), mSize(0), mpBuffer(NULL)
  {
    *this = Other;
  }
  template<typename U, typename TOtherAllocator>
  TList<T, TAllocator>& operator= (const TList<U, TOtherAllocator>& Other)
  {
    Empty();

    PreallocateSpace(Other.Size());
    for (int i = 0; i < Other.Size(); ++i) 
    { 
      Append(static_cast<T>(Other[i])); 
    }

    return *this;
  }
  
  ~TList();
    
  
  //@{ ... Properties

  //! return true if empty
  bool IsEmpty()const;

  //! returns the actual (not physical) size of the list
  int Size()const;
  //@} 


  //@{ ... Allocation

  //! return the physical size of the list. Should be accessed for debugging
  //! purposes only!
  int PreallocatedSpace()const;

  //! Reallocate the list, using at least @param NumEntries number
  //! of entries. Will not change the number of accessible entries,
  //! and should only be used for performance issues, where adding
  //! new entries costs to much allocation time.
  //! @param NumEntries must be greater than the number of current entries!
  void PreallocateSpace(int NumEntries);

  //! delete all entries of the list, trashing the allocated memory for the list
  void Empty();
  //! delete all entries of the list but keep the allocated memory
  void ClearEntries();
  //@}


  //@{ ... Change Entries in the List

  //! access to the elements
  T& ObjectAt(int n);
  const T& ObjectAt(int n)const;

  T& operator[] (int n);
  const T& operator[] (int n)const;

  const T& First()const;
  T& First();

  const T& Last()const;
  T& Last();

  //! initializes the buffer with the specified value
  TList& Init(const T& Value);

  //! swap the position of two entries
  void Swap(int Position1, int Position2);
  void Swap(TIterator& Iter1, TIterator& Iter2);

  //! NOTE: include <algorithm> in the cpp file when using any sort functions, 
  //! this way we can only use the forward declaration here.

  //! sort all entries (in-place) using operator < (T, T)
  TList& Sort() 
    { std::sort(Begin(), End(), std::less<T>()); return *this; }
  //! sort all entries (in-place) using the specified compare operator
  template <typename TCompareFunc>
  TList& Sort(TCompareFunc CompareFunc) 
  { std::sort(Begin(), End(), CompareFunc); return *this; }

  //! same as sort, but keeping the original order of same elements intact
  TList& StableSort() 
  { std::stable_sort(Begin(), End(), std::less<T>()); return *this; }
  //! same as sort, but keeping the original order of same elements intact
  template <typename TCompareFunc>
  TList& StableSort(TCompareFunc CompareFunc) 
  { std::stable_sort(Begin(), End(), CompareFunc); return *this; }

  //! reverse the order of the entries in the list
  TList& Reverse();
  //@} 


  //@{ ... Find Elements
  
  //! returns true when the given entry is present in the list.
  //! Syntactic sugar for '(Find(X) != -1)'.
  bool Contains(const T& Entry)const;

  //! find first Entry 
  //! @return Index if found else -1
  int Find(const T& Entry)const;
  //! find first Entry 
  //! @returns Iterator
  TConstIterator FindIter(const T& Entry)const;
  TIterator FindIter(const T& Entry);

  //! find first Entry using binary search
  //! @return Index if found else -1
  int FindWithBinarySearch(const T& Entry)const;
  //! find first Entry  using binary search
  //! @returns Iterator
  TConstIterator FindIterWithBinarySearch(const T& Entry)const;
  TIterator FindIterWithBinarySearch(const T& Entry);

  //! extended Find: return the first occurrence of Entry, starting to search
  //! at index StartIndex forwards or backwards
  int Find(const T& Entry, int StartIndex, bool Backwards)const;
  //! same as the find above but returns the first entry that matches 
  //! the != operator with Type T
  int FindNonEqual(const T& Entry, int StartIndex, bool Backwards)const;

  //! binary search to the list index where \param Entry should be inserted, 
  //! while keeping the list sorted (using operator < (T, T))
  //! @return Index >= [0, Size]
  int FindLowerBound(const T& Entry)const;
  //! @return valid insert iterator
  TConstIterator FindLowerBoundIter(const T& Entry)const;
  TIterator FindLowerBoundIter(const T& Entry);


  //! find and delete the first Entry 
  //! @return true if found and deleted
  bool FindAndDeleteFirst(const T& Entry);
  //! find and delete the last Entry 
  //! @return true if found and deleted
  bool FindAndDeleteLast(const T& Entry);
  
  //! find and delete all Entries that match @pram Entry
  //! @return true if at least one was found and deleted
  bool FindAndDeleteAll(const T& Entry);
  
  //! return the index of the entry in this list, or -1 if its not part of the 
  //! list. This will not do a compare of \param entry as 'Find' does and assert
  //! if the entry is not in our list.
  int IndexOf(const T& Entry)const;
  int IndexOf(const TIterator& Iter)const;
  int IndexOf(const TConstIterator& Iter)const;
  //@} 


  //@{ ... Add/Delete Elements to/out of the List

  //! prepend a entry
  TList& Prepend(const T& Value);
  //! prepend a list
  TList& Prepend(const TList& List);
  //! appends an entry
  TList& Append(const T& Value);
  //! appends a list
  TList& Append(const TList& List);
  
  //! appends a entry
  TList& operator+= (const T& Value);
  //! appends a List
  TList& operator+= (const TList& List);
  
  //! insert a Value somewhere in the List
  TList& Insert(const T& Value, int Index);
  //! insert a whole List somewhere in the List, using an iterator
  TList& Insert(const T& Value, TIterator& Iter);

  //! insert a whole List somewhere in the List
  TList& Insert(const TList& List, int Index);
  
  //! delete a Value somewhere in the List
  TList& Delete(int Index);
  //! delete a Value somewhere in the List, using an iterator
  TList& Delete(TIterator& Iter);

  //! delete a range of Values somewhere in the List
  TList& DeleteRange(int StartIndex, int EndIndex);

  //! remove the first Value of the List 
  TList& DeleteFirst();
  //! remove the last Value of the List 
  TList& DeleteLast();

  //! get the last Value of the List and remove it
  T GetAndDeleteLast();
  //! get the first Value of the List and remove it
  T GetAndDeleteFirst();

  //! return a copy of the specified Range in the List
  TList ItemsInRange(int StartIndex, int EndIndex)const;

  // const access to the buffer (might be NULL!)
  const T* FirstRead()const;
  //@} 

  
  //@{ ... Access through Iterators
  
  TConstIterator Begin()const;
  TIterator Begin();
  
  TConstIterator End()const;
  TIterator End();

  TConstIterator IterAt(int Index)const;
  TIterator IterAt(int Index);
  //@} 

  // ===============================================================================================
  
  /*!
   * std::const_iterator compatible random access iterator for a TList
  !*/

  class TConstIterator : public TStlRandomAccessIterator<T>
  {
  public:
    TConstIterator();
    TConstIterator(TList* pList, int Index);
    TConstIterator(const TIterator& Iter);

    //! return our internal list index
    int Index()const;

    //! let the iterator point to the next element in the list
    TConstIterator& operator++();
    TConstIterator operator++(int);

    //! let the iterator point to the previous element in the list
    TConstIterator& operator--();
    TConstIterator operator--(int);

    //! let the iterator point to a next element in the list
    TConstIterator& operator+=(ptrdiff_t Distance);
    TConstIterator operator+(ptrdiff_t Distance)const;
    
    //! let the iterator point to a previous element in the list
    TConstIterator& operator-=(ptrdiff_t Distance);
    TConstIterator operator-(ptrdiff_t Distance)const;

    //! return the distance_type between two iterators
    ptrdiff_t operator+(const TConstIterator& Other)const;
    ptrdiff_t operator-(const TConstIterator& Other)const;
    
    //! check if the iterators point to the same value
    bool operator==(const TConstIterator& Other)const;
    bool operator!=(const TConstIterator& Other)const;

    //! check iterator positions
    bool operator<(const TConstIterator& Other)const;
    bool operator>(const TConstIterator& Other)const;
    bool operator<=(const TConstIterator& Other)const;
    bool operator>=(const TConstIterator& Other)const;

    //! random access to an item in the list
    const T& operator[](ptrdiff_t Index)const;

    //! access to the lists value
    const T& operator*()const;
    const T* operator->()const;

    //! returns true if not past the end of the list
    bool IsValid()const;
    //! returns true if past the end of the list
    bool IsEnd()const;
    //! returns true if on the fist object
    bool IsHead()const;
    //! returns true if on the last object
    bool IsLast()const;
    
  private:
    friend class TIterator;
    friend class TList;
    
    TList* mpList;
    int mIndex;
  };
  
  // ===============================================================================================

  /*!
   * std::iterator compatible random access iterator for a TList
  !*/

  class TIterator : public TStlRandomAccessIterator<T>
  {
  public:
    TIterator();
    TIterator(TList* pList, int Index);

    //! return our internal list index
    int Index()const;

    //! let the iterator point to the next element in the list
    TIterator& operator++();
    TIterator operator++(int);

    //! let the iterator point to the previous element in the list
    TIterator& operator--();
    TIterator operator--(int);

    //! let the iterator point to a next element in the list
    TIterator& operator+=(ptrdiff_t Distance);
    TIterator operator+(ptrdiff_t Distance)const;
    
    //! let the iterator point to a previous element in the list
    TIterator& operator-=(ptrdiff_t Distance);
    TIterator operator-(ptrdiff_t Distance)const;

    //! return the distance_type between two iterators
    ptrdiff_t operator+(const TIterator& Other)const;
    ptrdiff_t operator-(const TIterator& Other)const;
    
    //! check if the iterators point to the same value
    bool operator==(const TIterator& Other)const;
    bool operator!=(const TIterator& Other)const;

    //! check iterator positions
    bool operator<(const TIterator& Other)const;
    bool operator>(const TIterator& Other)const;
    bool operator<=(const TIterator& Other)const;
    bool operator>=(const TIterator& Other)const;

    //! random access to an item in the list
    T& operator[](ptrdiff_t Index)const;

    //! access to the lists value
    T& operator*()const;
    T* operator->()const;

    //! returns true if not past the end of the list
    bool IsValid()const;
    //! returns true if past the end of the list
    bool IsEnd()const;
    //! returns true if on the fist object
    bool IsHead()const;
    //! returns true if on the last object
    bool IsLast()const;
    
  private:
    friend class TConstIterator;
    friend class TList;
    
    //! conversion from a const iter is not allowed
    TIterator(const TConstIterator& ConstIter);

    TList* mpList;
    int mIndex;
  };
    
private:
  //! all TLists are friends
  template <typename U, class TOtherAllocator> friend class TList;

  //! expand the list and copy the old values
  //! Changes only the physical size!
  void InternalExpand(int NewSize);
  
  //! internal copy helper to copy a list from another one
  void InternalCopy(
    const T*  pOtherBuffer, 
    int       OtherSize, 
    int       OtherPhysicalSize);

  int mPhysicalSize;
  int mSize;

  T* mpBuffer;
};

// =================================================================================================

// compare lists, allow comparing lists with different, but convertible types

template <typename T, class TAllocator, typename U, class TOtherAllocator>
bool operator==(
  const TList<T, TAllocator>&       First, 
  const TList<U, TOtherAllocator>&  Second);

template <typename T, class TAllocator, typename U, class TOtherAllocator>
bool operator!=(
  const TList<T, TAllocator>&       First, 
  const TList<U, TOtherAllocator>&  Second);


// add items or lists to lists, again allow different, but convertible types

template <typename T, class TAllocator, typename U>
TList<T, TAllocator> operator+(
  const TList<T, TAllocator>& List, 
  const U&                    Entry);

template <typename T, class TAllocator, typename U, class TOtherAllocator>
TList<T, TAllocator> operator+(
  const TList<T, TAllocator>&       First, 
  const TList<U, TOtherAllocator>&  Second);

#if defined(MCompiler_Has_RValue_References)

  template <typename T, class TAllocator, typename U>
  TList<T, TAllocator> operator+(
    TList<T, TAllocator>&&  First, 
    const U&                Entry);

  template <typename T, class TAllocator, typename U, class TOtherAllocator>
    TList<T, TAllocator> operator+(
    TList<T, TAllocator>&&            First, 
    const TList<U, TOtherAllocator>&  Second);

#endif

// =================================================================================================

// TIsList specialization, implementation for TypeTraits.h

template <typename T, class TAllocator>
struct TIsList< TList<T, TAllocator> > { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};

template <typename T, class TAllocator>
struct TIsBaseTypeList< TList<T, TAllocator> > { 
  static const bool sValue = TIsBaseType<T>::sValue; 
  inline operator bool() const { return sValue; }
};

// =================================================================================================

// Helper functions to easily create a list from a list of arguments

template <typename T>
inline TList<T> MakeList(const T& _1)
{
  TList<T> Ret;
  Ret.PreallocateSpace(1);

  Ret.Append(_1);

  return Ret;
}

template <typename T>
inline TList<T> MakeList(const T& _1, const T& _2)
{
  TList<T> Ret;
  Ret.PreallocateSpace(2);

  Ret.Append(_1); Ret.Append(_2);

  return Ret;
}

template <typename T>
inline TList<T> MakeList(const T& _1, const T& _2, const T& _3)
{
  TList<T> Ret; 
  Ret.PreallocateSpace(3);

  Ret.Append(_1); Ret.Append(_2); Ret.Append(_3);

  return Ret;
}

template <typename T>
inline TList<T> MakeList(const T& _1, const T& _2, const T& _3, const T& _4)
{
  TList<T> Ret; 
  Ret.PreallocateSpace(4);

  Ret.Append(_1); Ret.Append(_2); Ret.Append(_3); Ret.Append(_4);

  return Ret;
}

template <typename T>
inline TList<T> MakeList(
  const T& _1, const T& _2, const T& _3, const T& _4, const T& _5)
{
  TList<T> Ret; 
  Ret.PreallocateSpace(5);

  Ret.Append(_1); Ret.Append(_2); Ret.Append(_3); Ret.Append(_4); Ret.Append(_5);

  return Ret;
}

template <typename T>
inline TList<T> MakeList(
  const T& _1, const T& _2, const T& _3, const T& _4, const T& _5, 
  const T& _6)
{
  TList<T> Ret; 
  Ret.PreallocateSpace(6);

  Ret.Append(_1); Ret.Append(_2); Ret.Append(_3); Ret.Append(_4); Ret.Append(_5); 
  Ret.Append(_6);

  return Ret;
}

template <typename T>
inline TList<T> MakeList(
  const T& _1, const T& _2, const T& _3, const T& _4, const T& _5, 
  const T& _6, const T& _7)
{
  TList<T> Ret; 
  Ret.PreallocateSpace(7);

  Ret.Append(_1); Ret.Append(_2); Ret.Append(_3); Ret.Append(_4); Ret.Append(_5); 
  Ret.Append(_6); Ret.Append(_7);

  return Ret;
}

template <typename T>
inline TList<T> MakeList(
  const T& _1, const T& _2, const T& _3, const T& _4, const T& _5, 
  const T& _6, const T& _7, const T& _8)
{
  TList<T> Ret; 
  Ret.PreallocateSpace(8);

  Ret.Append(_1); Ret.Append(_2); Ret.Append(_3); Ret.Append(_4); Ret.Append(_5); 
  Ret.Append(_6); Ret.Append(_7); Ret.Append(_8);

  return Ret;
}

template <typename T>
inline TList<T> MakeList(
  const T& _1, const T& _2, const T& _3, const T& _4, const T& _5, 
  const T& _6, const T& _7, const T& _8, const T& _9)
{
  TList<T> Ret; 
  Ret.PreallocateSpace(9);

  Ret.Append(_1); Ret.Append(_2); Ret.Append(_3); Ret.Append(_4); Ret.Append(_5); 
  Ret.Append(_6); Ret.Append(_7); Ret.Append(_8); Ret.Append(_9);

  return Ret;
}

template <typename T>
inline TList<T> MakeList(
  const T& _1, const T& _2, const T& _3, const T& _4, const T& _5, 
  const T& _6, const T& _7, const T& _8, const T& _9, const T& _10)
{
  TList<T> Ret; 
  Ret.PreallocateSpace(10);

  Ret.Append(_1); Ret.Append(_2); Ret.Append(_3); Ret.Append(_4); Ret.Append(_5); 
  Ret.Append(_6); Ret.Append(_7); Ret.Append(_8); Ret.Append(_9); Ret.Append(_10);

  return Ret;
}

// =================================================================================================

#include "CoreTypes/Export/List.inl"


#endif // _List_h_

