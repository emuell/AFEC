#pragma once

#ifndef _Alloca_h_
#define _Alloca_h_
    
// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/TypeTraits.h"

#if defined(MCompiler_VisualCPP)
  #include <malloc.h>
#elif defined(MCompiler_GCC)
  #include <alloca.h>
#else
  #error "unknown compiler"
#endif

// =================================================================================================

//! small helper macro that initializes TAllocaList's. "alloca" can 
//! not be called as argument in TAllocaList#s constructor, because that 
//! would lead to undefined "free" behavior in the stack on some systems. 
//! 
//! Example usage:
//!   TAllocaList<int> MyAllocaList(QueryMaxCountDynamicallySomeHow());
//!   MInitAllocaList(MyLongList);
//!   ...do something with MyAllocaList...


#define MInitAllocaList(List) \
  void* MJoinMakros(pAllocaListMemory_, __LINE__) = (List.MaximumSize() > 0) ? \
    alloca(List.RequiredMemorySize()) : NULL; \
  List.SetMemory(MJoinMakros(pAllocaListMemory_, __LINE__));

#define MInitAllocaArray(Array) \
  void* MJoinMakros(pAllocaArrayMemory_, __LINE__) = (Array.Size() > 0) ? \
    alloca(Array.RequiredMemorySize()) : NULL; \
  Array.SetMemory(MJoinMakros(pAllocaArrayMemory_, __LINE__));

// =================================================================================================

/*! 
 * A simple list wrapper for memory that gets allocated on the stack alloca. 
 * NB: "alloca" will by default on most platforms return 16 byte aligned memory. 
 * By using the alignment parameter in the constructor this can be enforced.
 *
 * Needs to be constructed with a max size, but the maximum size can be 
 * specified during runtime, unlike for static arrays or lists. 
 *
 * Take care of only using predictable small amounts of data with TAllocaList 
 * cause you else are easily running out of stack memory, which is just a MB 
 * by default on most systems.
 *
 * A TAllocaList is empty by default and has no memory assigned. Use the 
 * MInitAllocaList macro to initialize the list before doing !anything!
 * with the list.
!*/

template<typename T> 
class TAllocaList
{
  //! please use only small base types with TAllocaList or you 
  //! will easily run out of stack memory.
  //! Note: 4*ptrdiff_t is just a rough guess of what's maybe OK...
  MStaticAssert(sizeof(T) <= 4*sizeof(ptrdiff_t));

public:
  typedef T value_type;
  
  //! important: use MInitAllocaList to initialize the list after construction!
  //! Alignment should be a power of two and <= 128.
  TAllocaList(int MaximumSize, int Alignment = 1);
  ~TAllocaList();


  //@{ ... Initialization

  //! required size in bytes for \function SetMemory.
  size_t RequiredMemorySize()const;

  void* Memory()const;
  //! implementation detail. Do not call this directly. 
  //! * USE "MInitAllocaList" TO INITIALIZE AFTER CONSTURCTION *
  void SetMemory(void* pAllocaBuffer);
  //@} 


  //@{ ... Properties

  //! return true if empty
  bool IsEmpty()const;

  //! returns the current (not physical, maximum) size of the list
  int Size()const;
  //! return the maximum number of entries this list can hold
  int MaximumSize()const;
  //@} 


  //@{ ... Entry Access

  T& operator[] (int n);
  const T& operator[] (int n)const;

  const T& First()const;
  T& First();

  const T& Last()const;
  T& Last();
  //@} 


  //@{ ... Find Elements
  
  //! @return true if found else false
  bool Contains(const T& Entry)const;

  //! find first Entry 
  //! @return Index if found else -1
  int Find(const T& Entry)const;

  //! extended Find: return the first occurrence of Entry, starting to 
  //! search at index StartIndex, forwards or backwards
  int Find(const T& Entry, int StartIndex, bool Backwards)const;
  //@} 


  //@{ ... Add/Delete Elements to/out of the List

  //! prepend an entry
  TAllocaList& Prepend(const T& Value);
  //! append an entry
  TAllocaList& Append(const T& Value);
  
  //! append an entry
  TAllocaList& operator+= (const T& Value);
  
  //! insert a new value somewhere in the List
  TAllocaList& Insert(const T& Value, int Index);
  //! delete a value from the List
  TAllocaList& Delete(int Index);

  //! delete all entries of the list, but keeps the buffer allocated
  void Empty();
  //@} 

private:
  //! copying makes no sense for stack allocated lists, and thus is not allowed
  TAllocaList(const TAllocaList& Other);
  TAllocaList& operator= (const TAllocaList& Other);

  const int mMaximumSize;
  const int mAlignment;

  T* mpBuffer;
  int mSize;
};

// =================================================================================================

template <typename T>
bool operator==(
  const TAllocaList<T>& First, 
  const TAllocaList<T>& Second);

template <typename T>
bool operator!=(
  const TAllocaList<T>& First, 
  const TAllocaList<T>& Second);

// =================================================================================================

template <typename T> 
struct TIsList< TAllocaList<T> > { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};

template <typename T> 
struct TIsBaseTypeList< TAllocaList<T> > { 
  static const bool sValue = TIsBaseType<T>::sValue; 
  inline operator bool() const { return sValue; }
};

// =================================================================================================

/*!
 * A simple array wrapper for memory that gets allocated on the stack with alloca. 
 * NB: "alloca" will by default on most platforms return 16 byte aligned memory. 
 * By using the alignment parameter in the constructor this can be enforced.
 *
 * Take care of only using predictable small amounts of data with TAllocaArray
 * cause you else are easily running out of stack memory, which is just a MB 
 * by default on most systems.
 *
 * A TAllocaArray is uninitialized by default, has no memory assigned. Use the 
 * MInitAllocaArray macro to initialize the array before doing !anything!
 * with the array.
!*/

template<typename T>
class TAllocaArray
{
  //! please use only small base types with TAllocaArray or you 
  //! will easily run out of stack memory.
  //! Note: 4*ptrdiff_t is just a rough guess of what's maybe OK...
  MStaticAssert(sizeof(T) <= 4*sizeof(ptrdiff_t));

public:
  typedef T value_type;
  static const size_t value_size = sizeof(T);
  
  //! Important: MInitAllocaArray to initialize after construction!
  //! Alignment should be a power of two and <= 128.
  TAllocaArray(int Size, int Alignment = 1);
  ~TAllocaArray();


  //@{ ... Initialization

  //! Required size in bytes for \function SetMemory.
  size_t RequiredMemorySize()const;

  void* Memory()const;
  //! implementation detail. Do not call this directly. 
  //! * USE "MInitAllocaArray" TO INITIALIZE AFTER CONSTURCTION *
  void SetMemory(void* pAllocaBuffer);
  //@} 


  //@{ ... Properties

  //! the size of the array
  int Size()const;
  //@} 


  //@{ ... Entry Access

  //! Set every entry to the given Value
  void Init(const T& Value);

  //! access to the Elements
  T& operator[](int n);
  const T& operator[](int n)const;

  //! direct access to the memory
  const T* FirstRead()const;
  T* FirstWrite()const;
  //@} 
  
protected:
  //! copying makes no sense for stack allocated arrays, and thus is not allowed
  TAllocaArray(const TAllocaArray& Other);
  TAllocaArray& operator= (const TAllocaArray& Other);

  const int mSize;
  const int mAlignment;

  T* mpBuffer;
};

// =================================================================================================

template <typename T>
bool operator==(
  const TAllocaArray<T>& First, 
  const TAllocaArray<T>& Second);

template <typename T>
bool operator!=(
  const TAllocaArray<T>& First, 
  const TAllocaArray<T>& Second);

// =================================================================================================

template <typename T>
struct TIsArray< TAllocaArray<T> > { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};

template <typename T>
struct TIsBaseTypeArray< TAllocaArray<T> > { 
  static const bool sValue = TIsBaseType<T>::sValue; 
  inline operator bool() const { return sValue; }
};

// =================================================================================================

#include "CoreTypes/Export/Alloca.inl"


#endif // _Alloca_h_

