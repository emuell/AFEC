#pragma once

#ifndef _StlAllocator_h_
#define _StlAllocator_h_

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/Memory.h"

#include <new> // global placement new
#include <utility> // std::pair
#include <typeinfo> // allocator descriptions

// avoid including Memory.h
namespace TMemory {  
  void* Alloc(const char* pDescription, size_t SizeInBytes);
  void Free(void* Ptr);

  void* AlignedAlloc(const char* pDescription, size_t SizeInBytes, size_t Alignment);
  void AlignedFree(void* Ptr);
}

// =================================================================================================

/*!
 * std::allocator which uses TMemory::Alloc/Free or 
 * TMemory::AllocSmallObject/FreeSmallObject
!*/

template <class T, bool sUseSmallObjectAllocator = false> 
class TStlAllocator
{
public:
  // Standard definitions, borrowed from the default STL allocator
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef T         value_type;

  template<class T2>
  struct rebind
  {  // convert an allocator<T> to an allocator <T2>
    typedef TStlAllocator<T2, sUseSmallObjectAllocator> other;
  };

  pointer address(reference _Val) const
  {  // return address of mutable _Val
    return (&_Val);
  }

  const_pointer address(const_reference _Val) const
  {  // return address of non mutable _Val
    return (&_Val);
  }

  TStlAllocator()
  {  // construct default allocator (do nothing)
  }

  TStlAllocator(const TStlAllocator<T>&) 
  {  // construct default allocator (do nothing)
  }

  template<class T2, bool sUseSmallObjectAllocator2> 
  TStlAllocator(const TStlAllocator<T2, sUseSmallObjectAllocator2>&)
  {  // construct default allocator (do nothing)
  }

  pointer allocate(size_type n, const void* hint = 0)
  {
    MUnused(hint);
    
    static const char* const spAllocatorName = typeid(this).name();

    if (sUseSmallObjectAllocator)
    {
      return reinterpret_cast<pointer>(
        TMemory::AllocSmallObject(spAllocatorName, n * sizeof(T)));
    }
    else
    {
      return reinterpret_cast<pointer>(
        TMemory::Alloc(spAllocatorName, n * sizeof(T)));
    }
  }

  void deallocate(void* p, size_type n)
  {
    if (p != NULL && n > 0)
    {
      if (sUseSmallObjectAllocator)
      {
        TMemory::FreeSmallObject(p);
      }
      else
      {
        TMemory::Free(p);
      }
    }
  }

  void construct(pointer p,const T& val)
  {
    new ((void *)p) T(val);
  }

  void destroy(pointer p)
  {
    p->T::~T();
  }

  size_type max_size()const
  {
    size_type n = (size_type)(-1) / sizeof(T);
    return (0 < n ? n : 1);
  }

  bool operator==(const TStlAllocator<T>& Other) const
  {
    return true;
  }

  bool operator !=(const TStlAllocator<T>& Other) const
  {
    return false;
  }

private:
  //! explicit copying is not allowed
  TStlAllocator<T>& operator=(const TStlAllocator<T>&);

  template<class T2> 
  TStlAllocator<T>& operator=(const TStlAllocator<T2>&);
};

// =================================================================================================

/*!
 * std::map TStlAllocator syntactic sugar
!*/

template <class TKey, class TValue, bool sUseSmallObjectAllocator = false> 
class TStlMapAllocator : public 
  TStlAllocator<std::pair<TKey const, TValue>, sUseSmallObjectAllocator> 
{ 
public:
  TStlMapAllocator() { }

  template<class T2, bool sUseSmallObjectAllocator2>
  TStlMapAllocator(const TStlAllocator<T2, sUseSmallObjectAllocator2>&)
  {  // construct from a related allocator (do nothing)
  }

  template<class T2, bool sUseSmallObjectAllocator2>
  TStlMapAllocator& operator=(
    const TStlAllocator<T2, sUseSmallObjectAllocator2>&)
  {  // assign from a related allocator (do nothing)
    return (*this);
  }

private:
  //! explicit copying is not allowed
  TStlMapAllocator& operator=(const TStlMapAllocator&);

  template<class TKey2, class TValue2, bool sUseSmallObjectAllocator2> 
  TStlMapAllocator& operator=(
    const TStlMapAllocator<TKey2, TValue2, sUseSmallObjectAllocator2>&);
};


#endif // _StlAllocator_h_

