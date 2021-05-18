#pragma once

#ifndef _Memory_h_
#define _Memory_h_
                                                           
// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"

// =================================================================================================

namespace TMemory
{
  //! call init before any other function to set up the 
  //! correct optimizations for the current processor
  void Init();
  void Exit();


  //@{ ... Fast, vectorized Move/Copy/Set functions
  
  void Move(void* pDestBuffer, const void* pSourceBuffer, size_t ByteCount);

  void Copy(void* pDestBuffer, const void* pSourceBuffer, size_t ByteCount);
  void CopyWord(void* pDestBuffer, const void* pSourceBuffer, size_t WordCount);
  void CopyDWord(void* pDestBuffer, const void* pSourceBuffer, size_t DWordCount);
  
  void SetByte(void* pDestBuffer, TUInt8 FillByte, size_t ByteCount);
  void SetWord(void* pDestBuffer, TUInt16 FillWord, size_t WordCount);
  void SetDWord(void* pDestBuffer, TUInt32 FillDWord, size_t DWordCount);

  //! SetByte 0 syntactic sugar
  void Zero(void* pDestBuffer, size_t ByteCount);
  //@}
  
  
  //@{ ... Allocate / Free Memory 

  //! return the size of a memory page
  size_t PageSize();
  
  //! "normal" allocation, with leak traces in debug builds
  //! pDescription should be a const c string, which is used for the 
  //! leak traces only (giving you a hint, what kind of object has leaked)
  //! @throw TOutOfMemoryException.
  void* Alloc(const char* pDescription, size_t SizeInBytes);
  void* Calloc(const char* pDescription, size_t Count, size_t SizeInBytes);
  void* Realloc(void* Ptr, size_t NewSizeInBytes);
  void Free(void* Ptr);

  //! aligned pointer allocations, with leak traces in debug builds
  //! @throw TOutOfMemoryException.
  void* AlignedAlloc(const char* pDescription, size_t SizeInBytes, size_t Alignment);
  void AlignedFree(void* Ptr);

  //! page aligned pointer allocations, with leak traces in debug builds
  //! @throw TOutOfMemoryException.
  void* PageAlignedAlloc(const char* pDescription, size_t SizeInBytes);
  void PageAlignedFree(void* Ptr);

  //! faster when frequently called for small allocations.
  //! with leak traces in debug builds, when MSmallObjectLeakChecking 
  //! is defined (see above)
  void* AllocSmallObject(const char* pDescription, size_t SizeInBytes);
  void FreeSmallObject(void* Ptr);
  //@}

  
  //@{ ... Leak checking
  
  //! dump information about all leaks that we have (called by std::atexit())
  //! only available when leak tracing is enabled
  void DumpLeaks();
  //! dump information about the currently allocated blocks (useful to get an 
  //! idea about which objects/classes require how much memory)
  //! only available when leak tracing is enabled
  void DumpStatistics();

  //! returns true if allocated by any of the allocation functions below
  //! (except the small object allocations)
  //! only available when leak tracing is enabled
  bool WasAllocated(void* Ptr);
  //@}
  

  //@{ ... ObjC AutoReleasePool (Mac only)

  #if defined(MMac)
    //! Creates/destroys a local auto relase pool. Does not do anythign with 
    //! objects allocated via TMemory, but simply creates/destroy an instance
    //! of NSAutoreleasePool
    class TObjCAutoReleasePool
    {
    public:
      TObjCAutoReleasePool();
      ~TObjCAutoReleasePool();

    private:
      void* mpPool;
    };
  #endif
}

// =================================================================================================

/*!
 * Macros to easily use TMemory::Alloc/Free or 
 * TMemory::AllocSmallObject/FreeSMallObject in classes
!*/

#define MDeclareDebugNewAndDelete \
  void* operator new(size_t size);  \
  void operator delete(void* pObject); \
  /* placement new/delete */ \
  inline void* operator new(size_t size, void* pMemory) \
    { MUnused(size); return pMemory; } \
  inline void operator delete(void* pMemory, void*) { }


#define MImplementDebugNewAndDelete(Class)  \
  void* Class::operator new(size_t size)  \
    { return TMemory::Alloc(#Class, size); }  \
  void Class::operator delete(void* pObject)  \
    { TMemory::Free(pObject); }

#define MImplementDebugNewAndDeleteTemplate(Class)  \
  template<typename T>  \
  void* Class<T>::operator new(size_t size) \
    { return TMemory::Alloc(#Class, size); }  \
  template<typename T>  \
  void Class<T>::operator delete(void* pObject) \
    { TMemory::Free(pObject); }

#define MImplementSmallObjDebugNewAndDelete(Class)  \
  void* Class::operator new(size_t size)  \
    { return TMemory::AllocSmallObject(#Class, size); } \
  void Class::operator delete(void* pObject)  \
    { TMemory::FreeSmallObject(pObject); }

#define MImplementSmallObjDebugNewAndDeleteTemplate(Class)  \
  template<typename T>  \
  void* Class<T>::operator new(size_t size) \
    { return TMemory::AllocSmallObject(#Class, size); }  \
  template<typename T>  \
  void Class<T>::operator delete(void* pObject) \
    { TMemory::FreeSmallObject(pObject); }

  
#endif // _Memory_h_

