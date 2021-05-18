#pragma once

#ifndef _Array_h_
#define _Array_h_

// =================================================================================================

#include "CoreTypes/Export/Allocator.h"
#include "CoreTypes/Export/CompilerDefines.h"
#include "CoreTypes/Export/StlIterator.h"
#include "CoreTypes/Export/TypeTraits.h"

#include <algorithm> // for TArray::Sort

// =================================================================================================

/*!
 *  A simple static sized array with boundary checks in debug builds
!*/

template<typename T, size_t sSize> 
class TStaticArray
{
public:
  typedef T TCArray[sSize]; // Our c-array equivalent 

  //! create a default TStaticArray. Elements are not initialized, so base 
  //! types will be uninitialized, complex types will be default constructed.
  TStaticArray();
  
  //! initialize/copy from other TStaticArrays with the same type and size
  TStaticArray(const TStaticArray& Other);
  TStaticArray& operator= (const TStaticArray<T, sSize>& Other);

  //! initialize/copy from a plain c-array with the same type and size
  TStaticArray(const TCArray& cArray);
  TStaticArray& operator= (const TCArray& cArray);

  //! explicit conversion from a TStaticArray with a different convertible 
  //! type but same size
  template <typename U>  
  explicit TStaticArray(const TStaticArray<U, sSize>& Other)
  {
    *this = Other;
  }
  template <typename U> 
  TStaticArray& operator= (const TStaticArray<U, sSize>& Other)
  {
    for (int i = 0; i < (int)sSize; ++i)
    {
      mBuffer[i] = static_cast<T>(Other[i]);
    }
    
    return *this;
  }

  //! returns sSize
  int Size()const;

  //! direct access to the memory
  const T *FirstRead()const;
  T *FirstWrite()const;

  //! access to the Elements
  const T& operator[](int n)const;
  T& operator[](int n);
  
  const T& First()const;
  T& First();
  const T& Last()const;
  T& Last();

  //! Set every entry to the given value
  void Init(const T& Value);
  //! Swap the position of two entries
  void Swap(int Position1, int Position2);

private:
  //! all TStaticArrays are friends
  template <typename U, size_t sOtherSize> 
  friend class TStaticArray;

  TCArray mBuffer;
};

// =================================================================================================

template <typename T, size_t sSize>
struct TIsArray< TStaticArray<T, sSize> > { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};

template <typename T, size_t sSize>
struct TIsBaseTypeArray< TStaticArray<T, sSize> > { 
  static const bool sValue = TIsBaseType<T>::sValue; 
  inline operator bool() const { return sValue; }
};

// =================================================================================================

template<typename T, size_t sSize> 
bool operator== (
  const TStaticArray<T, sSize>& First, 
  const TStaticArray<T, sSize>& Second);

template<typename T, size_t sSize> 
bool operator!= (
  const TStaticArray<T, sSize>& First, 
  const TStaticArray<T, sSize>& Second);

// =================================================================================================

/*!
 *  A simple and fast dynamic Array
!*/

template<typename T, class TAllocator = TDefaultArrayAllocator<T> > 
class TArray
{
public:
  typedef T value_type;

  class TConstIterator;
  class TIterator;

  //! create an empty array, or array with the given initial size
  TArray(int InitialSize = 0);
  
  //! copy array data from other
  TArray(const TArray& Other);
  TArray& operator=(const TArray& Other);

  //! move array data from other to this
  #if defined(MCompiler_Has_RValue_References)
    TArray(TArray&& Other);
    TArray& operator= (TArray&& Other);
  #endif

  //! explicit conversion/copying from arrays with different allocators
  template <typename TOtherAllocator>
  explicit TArray(const TArray<T, TOtherAllocator> &Other)
    : mpBuffer(NULL), mSize(0)
  {
    *this = Other;
  }
  template <typename TOtherAllocator>
  TArray<T, TAllocator>& operator=(const TArray<T, TOtherAllocator>& Other)
  {
    SetSize(Other.mSize);
    TArrayCopy<T>::SCopy(mpBuffer, Other.mpBuffer, Other.mSize);
    return *this;
  }

  //! explicit conversion/copying from arrays with convertible types 
  //! and maybe different allocators
  template <typename U, typename TOtherAllocator>
  explicit TArray(const TArray<U, TOtherAllocator>& Other)
    : mpBuffer(NULL), mSize(0)
  {
    *this = Other;
  }
  template <typename U, typename TOtherAllocator>
  TArray<T, TAllocator>& operator=(const TArray<U, TOtherAllocator>& Other)
  {
    SetSize(Other.mSize);
    for (int i = 0; i < mSize; ++i)
    {
      mpBuffer[i] = static_cast<T>(Other[i]);
    }

    return *this;
  }
  
  ~TArray();
  
  
  //@{ ... Properties
  
  //! returns true when empty
  bool IsEmpty()const;
  //! returns the current size of the array
  int Size()const;
  //@}


  //@{ ... (Re)Size

  //! resize the array, preserves existing entries and inits 
  //! new entries with T(), when T is not a basetype or pointer value
  void SetSize(int Size);
  //! Resizes the array to the new size. When expanding, old values are copied
  //! copied, but new basetype values are !not! initialized
  void Grow(int NewSize);

  //! delete all elements in the Array
  void Empty();
  //@}

  
  //@{ ... Manipulate Content

  //! Set every entry to the given Value
  void Init(const T& Value);
  
  //! Swap the position of two entry's
  void Swap(int Position1, int Position2);
  void Swap(TIterator& Position1, TIterator& Position2);

  //! NOTE: include <algorithm> in the cpp file when using any sort functions, 
  //! this way we can only use the forward declaration here.

  //! sort all entries (in-place) using operator < (T, T)
  TArray& Sort() 
  { std::sort(Begin(), End()); return *this; }
  //! sort all entries (in-place) using the specified compare operator
  template <typename TCompareFunc>
  TArray& Sort(TCompareFunc CompareFunc) 
  { std::sort(Begin(), End(), CompareFunc); return *this; }

  //! same as sort, but keeping the original order of same elements intact
  TArray& StableSort() 
  { std::stable_sort(Begin(), End()); return *this; }
  //! same as sort, but keeping the original order of same elements intact
  template <typename TCompareFunc>
  TArray& StableSort(TCompareFunc CompareFunc) 
  { std::stable_sort(Begin(), End(), CompareFunc); return *this; }
  //@}


  //@{ ... Element Access

  //! access to the elements
  T& operator[](int n);
  const T& operator[](int n)const;
  
  const T& First()const;
  T& First();
  const T& Last()const;
  T& Last();

  //! direct access to the raw buffer
  const T *FirstRead()const;
  T *FirstWrite()const;
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
   * std::const_iterator compatible random access iterator for a TArray
  !*/

  class TConstIterator : public TStlRandomAccessIterator<T>
  {
  public:
    TConstIterator();
    TConstIterator(TArray* pArray, int Index);
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
    friend class TArray;
    
    TArray* mpArray;
    int mIndex;
  };
  
  // ===============================================================================================

  /*!
   * std::iterator compatible random access iterator for a TArray
  !*/

  class TIterator : public TStlRandomAccessIterator<T>
  {
  public:
    TIterator();
    TIterator(TArray* pArray, int Index);

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
    friend class TArray;
    
    //! conversion from a const iterator is not allowed
    TIterator(const TConstIterator& ConstIter);

    TArray* mpArray;
    int mIndex;
  };
  
private:
  //! all TArrays are friends
  template <typename U, class TOtherAllocator> friend class TArray;

  void Realloc(int NewSize);
  
  T* mpBuffer;
  int mSize;
};

// =================================================================================================

template <typename T, class TAllocator>
struct TIsArray< TArray<T, TAllocator> > { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};

template <typename T, class TAllocator> 
struct TIsBaseTypeArray< TArray<T, TAllocator> > { 
  static const bool sValue = TIsBaseType<T>::sValue; 
  inline operator bool() const { return sValue; }
};

// =================================================================================================

/*!
 * A TArray<T> which uses an aligned allocator to align buffers to 32 bytes.
 * NB: 16 would be enough for SSE, but 32 allows aligned 256-bit access and 
 * thus may speed up memory access in some situations even more.
!*/

typedef TArray<float, TAlignedAllocator<float, 32> > TAlignedFloatArray;
typedef TArray<double, TAlignedAllocator<double, 32> > TAlignedDoubleArray;

// =================================================================================================

template<typename T, class TAllocator, class TOtherAllocator>
bool operator== (
  const TArray<T, TAllocator>&      First, 
  const TArray<T, TOtherAllocator>& Second);

template<typename T, class TAllocator, class TOtherAllocator>
bool operator!= (
  const TArray<T, TAllocator>&      First, 
  const TArray<T, TOtherAllocator>& Second);

// =================================================================================================

template <typename T>
inline TArray<T> MakeArray(const T& _1)
{
  TArray<T> Ret(1);
  Ret[0] = _1;

  return Ret;
}

template <typename T>
inline TArray<T> MakeArray(const T& _1, const T& _2)
{
  TArray<T> Ret(2);
  Ret[0] = _1; Ret[1] = _2;

  return Ret;
}

template <typename T>
inline TArray<T> MakeArray(const T& _1, const T& _2, const T& _3)
{
  TArray<T> Ret(3);
  Ret[0] = _1; Ret[1] = _2; Ret[2] = _3;

  return Ret;
}

template <typename T>
inline TArray<T> MakeArray(const T& _1, const T& _2, const T& _3, const T& _4)
{
  TArray<T> Ret(4);
  Ret[0] = _1; Ret[1] = _2; Ret[2] = _3; Ret[3] = _4;

  return Ret;
}

template <typename T>
inline TArray<T> MakeArray(const T& _1, const T& _2, const T& _3, const T& _4, 
  const T& _5)
{
  TArray<T> Ret(5);
  Ret[0] = _1; Ret[1] = _2; Ret[2] = _3; Ret[3] = _4; Ret[4] = _5;

  return Ret;
}

template <typename T>
inline TArray<T> MakeArray(const T& _1, const T& _2, const T& _3, const T& _4, 
  const T& _5, const T& _6)
{
  TArray<T> Ret(6);
  Ret[0] = _1; Ret[1] = _2; Ret[2] = _3; Ret[3] = _4; Ret[4] = _5; Ret[5] = _6;

  return Ret;
}

// =================================================================================================

#include "CoreTypes/Export/Array.inl"


#endif // _Array_h_

