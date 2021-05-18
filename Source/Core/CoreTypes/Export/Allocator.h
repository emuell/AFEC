#pragma once

#ifndef _Allocator_h_
#define _Allocator_h_

// =================================================================================================

#include "CoreTypes/Export/Memory.h"
#include "CoreTypes/Export/Pointer.h"
#include "CoreTypes/Export/ReferenceCountable.h"
#include "CoreTypes/Export/TypeTraits.h"

#include <new>  // global placement news
#include <typeinfo>  // for the allocator descriptions
#include <atomic>  // for the allocator descriptions

// =================================================================================================

template<typename T> class TArrayCopy;

// -------------------------------------------------------------------------------------------------

template<typename T, bool sIsBaseTypeOrPointer> 
class TArrayCopyImpl {};

template<typename T> class TArrayCopyImpl<T, false>
{
private:
  friend class TArrayCopy<T>;

  static inline void SCopy(T* pTo, const T* pFrom, size_t Count)
  {
    for (size_t i = 0; i < Count; ++i)
    {
      pTo[i] = pFrom[i];
    }
  }

  static inline void SMove(T* pTo, const T* pFrom, size_t Count)
  {
    if (pTo <= pFrom)
    {
      for (size_t i = 0; i < Count; ++i)
      {
        pTo[i] = pFrom[i];
      }
    }
    else
    {
      for (int i = (int)Count - 1; i >= 0; --i)
      {
        pTo[i] = pFrom[i];
      }
    }
  }
};

template<typename T> class TArrayCopyImpl<T, true>
{
private:
  friend class TArrayCopy<T>;

  static inline void SCopy(T* pTo, const T* pFrom, size_t Count)
  {
    TMemory::Copy(pTo, pFrom, sizeof(T) * Count);
  }

  static inline void SMove(T* pTo, const T* pFrom, size_t Count)
  {
    TMemory::Move(pTo, pFrom, sizeof(T) * Count);
  }
};

// -------------------------------------------------------------------------------------------------

template<typename T> 
class TArrayCopy
{
public:
  static inline void SCopy(T* pTo, const T* pFrom, size_t Count)
  {
    TArrayCopyImpl<T, TIsBaseType<T>::sValue || 
      TIsPointer<T>::sValue>::SCopy(pTo, pFrom, Count);
  }

  static inline void SMove(T* pTo, const T* pFrom, size_t Count)
  {
    TArrayCopyImpl<T, TIsBaseType<T>::sValue || 
      TIsPointer<T>::sValue>::SMove(pTo, pFrom, Count);
  }
};

// =================================================================================================

template<typename T> class TArrayInitializer;

// -------------------------------------------------------------------------------------------------

template<typename T, bool sIsBaseTypeOrPointer> 
class TArrayInitializerImpl {};

template<typename T> class TArrayInitializerImpl<T, false>
{
private:
  friend class TArrayInitializer<T>;

  static inline void SInitialize(T* pBuffer, size_t Count)
  {
    for (size_t i = 0; i < Count; ++i)
    {
      pBuffer[i] = T();
    }
  }
};

template<typename T> class TArrayInitializerImpl<T, true>
{
private:
  friend class TArrayInitializer<T>;

  static inline void SInitialize(T* pBuffer, size_t Count) { }
};

// -------------------------------------------------------------------------------------------------

template<typename T> 
class TArrayInitializer
{
public:
  static void SInitialize(T* pBuffer, size_t Count)
  {
    TArrayInitializerImpl<T, TIsBaseType<T>::sValue || 
      TIsPointer<T>::sValue>::SInitialize(pBuffer, Count);
  }
};

// =================================================================================================

template<typename T> class TArrayConstructor;

// -------------------------------------------------------------------------------------------------

template<typename T, bool sIsBaseTypeOrPointer> 
class TArrayConstructorImpl {};

template<typename T> class TArrayConstructorImpl<T, true>
{
private:
  friend class TArrayConstructor<T>;

  static inline void SConstruct(T* Ptr, size_t Count) { }
  static inline void SDestruct(T* Ptr, size_t Count) { }
};

template<typename T> class TArrayConstructorImpl<T, false>
{
private:
  friend class TArrayConstructor<T>;

  static inline void SConstruct(T* Ptr, size_t Count)
  {
    for (size_t i = 0; i < Count; ++i)
    {
      new(Ptr + i)T;
    }
  }

  static inline void SDestruct(T* Ptr, size_t Count)
  {
    for (size_t i = 0; i < Count; ++i)
    {
      (Ptr + i)->~T();
    }
  }
};

// -------------------------------------------------------------------------------------------------

template<typename T> 
class TArrayConstructor
{
public:
  static void SConstruct(T* Ptr, size_t Count)
  {
    TArrayConstructorImpl<T, TIsBaseType<T>::sValue || 
      TIsPointer<T>::sValue>::SConstruct(Ptr, Count);
  }

  static void SDestruct(T* Ptr, size_t Count)
  {
    TArrayConstructorImpl<T, TIsBaseType<T>::sValue || 
      TIsPointer<T>::sValue>::SDestruct(Ptr, Count);
  }
};

// =================================================================================================

template<typename T, size_t sAlignment = 16> 
class TAlignedAllocator
{
public:
  static T* SNew(size_t Count)
  { 
    static const char* const spAllocatorName = 
      typeid(TAlignedAllocator<T>).name();
    
    return reinterpret_cast<T*>(
      TMemory::AlignedAlloc(spAllocatorName, Count * sizeof(T), sAlignment));
  }
  
  static void SDelete(T *Ptr, size_t Count)
  {
    TMemory::AlignedFree((void*)Ptr);
  }
};

// =================================================================================================

template<typename T> 
class TDefaultArrayAllocator
{
public:
  static T* SNew(size_t Count) 
  {
    static const char* const spAllocatorName = 
      typeid(TDefaultArrayAllocator<T>).name();

    T* Ptr = reinterpret_cast<T*>(
      TMemory::Alloc(spAllocatorName, sizeof(T) * Count)); 

    TArrayConstructor<T>::SConstruct(Ptr, Count);

    return Ptr;
  }
  
  static void SDelete(T *Ptr, size_t Count) 
  {
    TArrayConstructor<T>::SDestruct(Ptr, Count);
    TMemory::Free((void*)Ptr);
  }
};

// =================================================================================================

/*!
 *  Fast and simple fixed size block allocator, which allocates/frees blocks
 *  in kNumElementsPerBlock * sizeof(TElementType) blocks.
 *  
 *  Important: This allocator is not thread safe! Lock, unlock access to 
 *  it by yourself if it needs to be thread safe.
!*/

class TBlockAllocator : public TReferenceCountable
{
public:
  TBlockAllocator(
    const char* pAllocatorDescription, 
    size_t      ObjectSize, 
    size_t      NumObjectsPerBlock);

  ~TBlockAllocator();
  
  void* Alloc();
  void Free(void* pMem);

private:
  // copying is not allowed
  TBlockAllocator(const TBlockAllocator& Other);
  TBlockAllocator& operator= (const TBlockAllocator& Other);

  struct TBlockHeader
  {
    TBlockHeader* mpPrevBlock;
    TBlockHeader* mpNextBlock;
    size_t mNumberOfAllocatedObjects;
    // char mpBlockMemory[mNumberOfObjectsPerBlock * 
    //    (sizeof(TElementHeader) + mObjectSize)]
  };
  
  struct TElementHeader
  {
    TBlockHeader* mpBlock;
    // char mpObjectMemory[mObjectSize]
  };
  
  const char* const mpAllocatorDescription;

  const size_t mObjectSize;
  const size_t mNumberOfObjectsPerBlock;
  
  TBlockHeader* mpCurrentBlock;
  size_t mNumberOfObjectsInCurrentBlock;

  #if defined(MDebug)
    std::atomic<int> m__Allocating;
  #endif
};

// =================================================================================================

/*!
 *  Fixed size Block allocator, which will release the requested memory only 
 *  on destruction.
 *
 *  Only objects of one type and therefore size can be allocated. As the memory is 
 *  not released immediately, allocations / frees are fast, but also temporary require 
 *  more memory than necessary.
 *
 *  Important: This allocator is not thread safe! Lock, unlock access to 
 *  it by yourself if it needs to be thread safe.
!*/

class TLocalBlockAllocator : public TReferenceCountable
{
public:
  TLocalBlockAllocator(
    const char* pAllocatorDescription,
    size_t      ObjectSize, 
    size_t      NumObjectsPerBlock);

  ~TLocalBlockAllocator();
  
  void* Alloc();
  void Free(void* pMem);

  //! Explicitly free all allocated blocks. This can only be called when all 
  //! allocated elements where freed!
  void PurgeAll();
  
private:
  // copying is not allowed
  TLocalBlockAllocator(const TLocalBlockAllocator& Other);
  TLocalBlockAllocator& operator= (const TLocalBlockAllocator& Other);
  
  struct TBlockHeader
  {
    TBlockHeader* mpPrevBlock;
    // char mpBlockMemory[mNumberOfObjectsPerBlock * mObjectSize]
  };
    
  const size_t mObjectSize;
  const size_t mNumberOfObjectsPerBlock;

  const char* const mpAllocatorDescription;

  #if defined(MDebug)
    size_t m__TotalNumberOfObjects;
    std::atomic<int> m__Allocating;
  #endif
  
  TBlockHeader* mpCurrentBlock;
  size_t mNumberOfObjectsInCurrentBlock;
};

// =================================================================================================

/*!
 *  Hybrid of a "normal" allocator and a block allocator for blocks of 
 *  any size below \param BlockAllocationSizeThreshold.
 *
 *  Only objects which are bigger than the threshold (which is the page size) will 
 *  be deleted immediately.
 *  All other objects will be freed as soon as the Allocator dies. This makes
 *  allocating/freeing small object very fast and allows to use it for mixed objects
 *  of any size.
 * 
 *  Important: You must make sure that all objects are freed before the Allocator
 *  dies. This is asserted and will result into undefined behavior if not! The Allocator
 *  frees all memory that was allocated while construction when dying.
 *
 *  Even more Important: This allocator is not thread safe! Lock, unlock access to 
 *  it by yourself if it needs to be thread safe.
!*/

class TLocalAllocator : public TReferenceCountable
{
public:
  TLocalAllocator();
  ~TLocalAllocator();
  
  void* Alloc(size_t RequestedSize);
  void* Realloc(void* pMem, size_t size);
  
  void Free(void* pMem);
  
private:
  // copying is not allowed
  TLocalAllocator(const TLocalAllocator& Other);
  TLocalAllocator& operator= (const TLocalAllocator& Other);
  
  enum 
  { 
    kHugeBufferMagic  = 'H',
    kBlockBufferMagic = 'B'
  };
  
  struct TChunkHeader
  {
    char mMagic;
    size_t mSize;
    TChunkHeader* mpPrev;
    // char mpChunkMemory[];
  };

  int FreeMemoryInCurrentBlock()const;
  
  #if defined(MDebug)
    size_t m__CurrentlyAllocatedBytes;
    std::atomic<int> m__Allocating;
  #endif

  TChunkHeader* mpCurrBlockChunkHeader;
 
  // avoid including List.h here
  class TVoidPointerList* mpBlocks;
  class TVoidPointerList* mpHugeBuffers;
  
  const size_t mBlockAllocationSizeThreshold;
};

// =================================================================================================

/*!
 *  Fixed size Block allocator for objects. 
 *
 *  Object allocator wrapper for the TBlockAllocator, which will call 
 *  con/destructors for the given object type.
!*/

template <class T>
class TBlockObjectAllocator : public TBlockAllocator
{
public:
  TBlockObjectAllocator(size_t NumElementsPerBlock)
    : TBlockAllocator(
        typeid(TBlockObjectAllocator<T>).name(), 
        sizeof(T), 
        NumElementsPerBlock) { }
  
  inline T* Construct() 
  { 
    return new (this->Alloc()) T(); 
  }
  
  inline T* Construct(const T& CopyFrom) 
  { 
    return new (this->Alloc()) T(CopyFrom); 
  }
  
  inline void Destruct(T* pObject) 
  { 
    pObject->~T(); 
    this->Free(pObject); 
  }
};

// =================================================================================================

/*!
 *  Fixed size Block allocator for objects. 
 *
 *  Object allocator wrapper for the TLocalBlockAllocator, which will call 
 *  con/destructors for the given object type.
!*/

template <class T>
class TLocalBlockObjectAllocator : public TLocalBlockAllocator
{
public:
  TLocalBlockObjectAllocator(size_t NumElementsPerBlock)
    : TLocalBlockAllocator(
        typeid(TLocalBlockObjectAllocator<T>).name(), 
        sizeof(T), 
        NumElementsPerBlock) { }
  
  inline T* Construct() 
  { 
    return new (this->Alloc()) T(); 
  }
  
  inline T* Construct(const T& CopyFrom) 
  { 
    return new (this->Alloc()) T(CopyFrom); 
  }
  
  inline void Destruct(T* pObject) 
  { 
    pObject->~T(); 
    this->Free(pObject); 
  }
};


#endif

