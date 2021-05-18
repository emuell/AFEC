#include "CoreTypesPrecompiledHeader.h"

#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <mutex>
#include <typeinfo>

#if defined(MCompiler_GCC)
  // for the type serialization (demangling)
  #include <cxxabi.h>
#endif

#if defined(MWindows)
  #include <malloc.h>
  
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

#if defined(MMac)
  #include <unistd.h> // getpagesize

  #include <mach/vm_map.h>
  #include <mach/mach_init.h>
  
  extern "C" size_t malloc_size(void *ptr);
#endif

#if defined(MLinux)
  #include <unistd.h> // getpagesize
  #include <malloc.h>

  #include <cstring> // memset, memcpy, memmove
#endif

#if defined(MHaveIntelIPP)
  #include "../../3rdParty/IPP/Export/IPPs.h"
#endif

// =================================================================================================

#if defined(MDebug)
  
  //! define to enable leak traces
  #define MEnableLeakTraces

  //! define to enable leak traces for small objects
  //! this is separated from the other MEnableLeakTraces define, because
  //! this slows down the debug build a lot and might not always be useful
  #define MEnableSmallObjectLeakTraces

  //! define to record the number of an allocation. This can be useful to track 
  //! a leak by its number. Slows down things. Enable only when needed...
  // #define MEnableAllocationCounts

  //! define to record stacks for all allocations, when leak traces are enabled
  //! This is slow as hell! So enable this only when really needed...
  // #define MEnableStackTraces

  //! enable in runtime to assert when allocations are done in realtime threads
  bool gAssertRealTimeAssertions = false;

#endif // defined(MDebug)

// =================================================================================================

#if defined(MMac)

// -------------------------------------------------------------------------------------------------

static size_t log2int(size_t x) 
{ 
  return (x < 2) ? 0 : log2int(x>>1) + 1; 
}

static size_t roundUpPowerOf2(size_t x)
{
  static size_t one = 1;
  size_t log2Int = log2int(x);
    
  return ((one << log2Int) == x) ? x : (one << (log2Int + 1));
}

static size_t roundUpToPageBoundary(size_t x)
{
  size_t roundedDown = trunc_page(x);
  return (roundedDown == x) ? x : (roundedDown + vm_page_size);
}

typedef struct _memalign_marker_t 
{
  vm_address_t start; 
  vm_size_t size; 
} 
memalign_marker_t;

// -------------------------------------------------------------------------------------------------

static void* memalign_mac(size_t align, size_t size)
{
  size_t effectiveAlign = align;
  size_t padding = 0;
  size_t amountToAllocate = 0;
  
  if (effectiveAlign < sizeof(void*))
  {
    effectiveAlign = roundUpPowerOf2(sizeof(void *));
  }
  else
  {
    effectiveAlign = roundUpPowerOf2(effectiveAlign);
  }
  
  if (effectiveAlign < sizeof(memalign_marker_t))
  {
    padding = sizeof(memalign_marker_t);
  }
  else
  {
    padding = effectiveAlign;
  }
  
  amountToAllocate = roundUpToPageBoundary(size + padding);
  
  {
    vm_address_t p = 0;
    kern_return_t status = vm_allocate(mach_task_self(), &p, amountToAllocate, 1);
      
    if (status != KERN_SUCCESS)
    {
      return NULL;
    }
    else
    {
      vm_size_t logEffectiveAlign = log2int(effectiveAlign);
      vm_address_t lowestAvaliableAddress = p + sizeof(memalign_marker_t);
      vm_address_t roundedDownAddress = ((lowestAvaliableAddress >> 
        logEffectiveAlign) << logEffectiveAlign);
      
      vm_address_t returnAddress = (roundedDownAddress == lowestAvaliableAddress) ? 
         lowestAvaliableAddress : (roundedDownAddress + effectiveAlign);
      
      vm_address_t firstUnneededPage = 0;
        
      memalign_marker_t* marker = (memalign_marker_t *)returnAddress - 1;
                            
      // lowest address used, then round down to vm_page boundary
      vm_address_t usedPageBase = trunc_page((vm_address_t)marker);
      marker->start = usedPageBase;
      marker->size = returnAddress + size - usedPageBase;
      
      if (usedPageBase > p)
      {
        status = vm_deallocate(mach_task_self(), p, usedPageBase - p);
      
        if (status != KERN_SUCCESS)
        {
          MInvalid("vm_deallocate failed!");
          
          fprintf(stderr, "memalign(%zx, %zx) failed to "
            "deallocate extra header space.\n", align, size);
        }
      }
      
      firstUnneededPage = roundUpToPageBoundary(returnAddress + size);
      
      if (firstUnneededPage < p + amountToAllocate)
      {
        status = vm_deallocate(mach_task_self(), firstUnneededPage, 
          p + amountToAllocate - firstUnneededPage);
      
        if (status != KERN_SUCCESS)
        {
          MInvalid("vm_deallocate failed!");
          
          fprintf(stderr, "memalign(%zx, %zx) failed to "
            "deallocate extra footer space.\n", align, size);
        }
      }
      
      return (void *)returnAddress;
    }
  }
}

// -------------------------------------------------------------------------------------------------

static void memalign_free_mac(void *p)
{
  memalign_marker_t* marker = (memalign_marker_t *)p - 1;
  
  kern_return_t status = 
    ::vm_deallocate(mach_task_self(), marker->start, marker->size);

  if (status != KERN_SUCCESS )
  {
    MInvalid("vm_deallocate failed")
    ::fprintf(stderr, "free_memalign(%p) failed!\n", p);
  }
}


#endif // defined(MMac)

// =================================================================================================

template <size_t sBufferSize>
class TEmergencyAllocator
{
public:
  TEmergencyAllocator();
  ~TEmergencyAllocator();

  bool WasAllocated(void* pMem)const;

  void* Alloc(size_t Size);
  void* Realloc(void* pMem, size_t NewSize);

  void Free(void* pMem);

private:
  char mBuffer[sBufferSize];
  std::mutex mBufferLock;

  char* mpExraHeapBuffer;
  
  static bool sEmergencyAllocUsed;
};

// =================================================================================================

//! TMemorys emergency allocator, when malloc fails
static TEmergencyAllocator<1024 * 1024 * 4> sEmergencyAllocator; // 4 + 4 MB

//! Size in bytes where will not ask the emergency allocator for memory
//! but directly throw a TOutOfMemoryException
#define MEmergencyAllocThreshold 1024*1024 // 1 MB

// =================================================================================================

#if defined(MEnableLeakTraces)

namespace TLeakCheck
{
  // ===============================================================================================

  class TLeakTrace
  {
  public:
    TLeakTrace(
      const char* pAllocatorDesc = NULL, 
      int         AllocationCount = 0, 
      size_t      ByteSize = 0)
      : mpDescription(pAllocatorDesc),
        mAllocationCount(AllocationCount), 
        mByteSize(ByteSize) 
    {
      #if defined(MEnableStackTraces)
        TStackWalk::GetStack(mFramePointers, 10, 4);
      #endif
    }

    const char* mpDescription;
    
    int mAllocationCount;
    size_t mByteSize;
    
    #if defined(MEnableStackTraces) 
      std::vector<uintptr_t> mFramePointers;
    #endif  
  };

  // ===============================================================================================

  typedef std::map<void*, TLeakTrace> TInstanceMap;
  
  #if defined(MEnableAllocationCounts) 
    typedef std::map<const char*, int> TAllocationCountMap;
    typedef std::map<const char*, int> TBreakAllocationMap;
  #endif
  
  typedef std::vector<TLeakTrace> TInstancesVector;
  typedef std::pair<std::string, TInstancesVector> TInstancesVectorPair;         
  typedef std::vector<TInstancesVectorPair> TSortedInstancesVector;
  
  TInstanceMap sInstances;

  #if defined(MEnableAllocationCounts)
    TAllocationCountMap sAllocatingCount;
    TBreakAllocationMap sBreakOnAllocationCount;
  #endif

  std::mutex sLeakTraceLock;

  // -----------------------------------------------------------------------------------------------

  const char* ClassName(const char* pAllocator, void* Ptr)
  {
    // Already got a mangled name from typeid()?
    #if defined(MCompiler_GCC)
      int Status = 0;
      char* pDemangledName = abi::__cxa_demangle(pAllocator, 0, 0, &Status);
      if (Status == 0 && pDemangledName != NULL)
      {
        ::free(pDemangledName);
        return pAllocator; // is a mangled name already
      }
    #endif
    
    // Else try typeid() to resolve the runtime type
    const std::string AllocatorStr = pAllocator;
    
    if (AllocatorStr.find("class ") != 0 && // already serialized
        AllocatorStr.find("struct ") != 0 && // already serialized
        AllocatorStr.find("#") != 0) // raw buffer allocs
    {
      return typeid(*reinterpret_cast<TReferenceCountable*>(Ptr)).name();
    }
    
    return pAllocator;
  }

  // -----------------------------------------------------------------------------------------------

  const char* ByteCountString(size_t ByteCount)
  {
    static char sByteCountString[256];
    
    if (ByteCount >= 1024*1024)
    {
      ::snprintf(sByteCountString, sizeof(sByteCountString), 
        "%.2f MB", ((float)ByteCount / float(1024*1024)));
    }
    else if (ByteCount >= 1024)
    {
      ::snprintf(sByteCountString, sizeof(sByteCountString), 
        "%.2f KB", ((float)ByteCount / float(1024)));
    }
    else
    {
      ::snprintf(sByteCountString, sizeof(sByteCountString), 
        "%d Bytes", (int)ByteCount);
    }
    
    return sByteCountString;
  }  
  
  // -----------------------------------------------------------------------------------------------

  inline bool WasAllocated(void* Ptr)
  {
    return sInstances.find(Ptr) != sInstances.end();
  }

  // -----------------------------------------------------------------------------------------------

  inline size_t RealMallocedSize(void* ptr, size_t size)
  {
    #if defined(MWindows)
      // real size + size of the block header debug info
      return size + 36; // sizeof(_CrtMemBlockHeader);
    #else
      return size;
    #endif
  }

  // -----------------------------------------------------------------------------------------------

  struct TAllocationBreakInit 
  {
    TAllocationBreakInit() 
    {
      // For example:
      //sBreakOnAllocationCount["class TRect"] = 28;
    }
  } mDummy;
  

  void OnAlloc(const char* pAllocator, void* Ptr, size_t Size)
  {
    int AllocationCount = -1;
    
    #if defined(MEnableAllocationCounts)
      TAllocationCountMap::iterator AllocCountIter(
        sAllocatingCount.lower_bound(pAllocator));

      if (AllocCountIter != sAllocatingCount.end() && 
          pAllocator == AllocCountIter->first)
      {
        AllocationCount = ++AllocCountIter->second;
      }
      else
      {
        sAllocatingCount.insert(
          AllocCountIter, std::make_pair(pAllocator, 0));
      }
    
      // set sBreakOnAllocationCount to an object you want to trace, 
      // then this assertion will be fired. See TAllocationBreakInit below...
      if (sBreakOnAllocationCount.size())
      {
        MAssert(sBreakOnAllocationCount.find(pAllocator) == sBreakOnAllocationCount.end() || 
          sBreakOnAllocationCount[pAllocator] != AllocationCount, "Allocation Break");
      }
    #endif

    // This is slow. Enable manually, if needed
    // MAssert(sInstances.find(Ptr) == sInstances.end(), "");
    
    sInstances.insert(std::make_pair(Ptr, 
      TLeakTrace(pAllocator, AllocationCount, RealMallocedSize(Ptr, Size))));
  }

  // -----------------------------------------------------------------------------------------------

  void OnAllocAligned(const char* pAllocator, void* Ptr, size_t Size)
  {
    int AllocationCount = -1;
    
    #if defined(MEnableAllocationCounts)
      TAllocationCountMap::iterator AllocCountIter(
        sAllocatingCount.lower_bound(pAllocator));

      if (AllocCountIter != sAllocatingCount.end() && 
          pAllocator == AllocCountIter->first)
      {
        AllocationCount = ++AllocCountIter->second;
      }
      else
      {
        sAllocatingCount.insert(
          AllocCountIter, std::make_pair(pAllocator, 0));
      }
    
      // set sBreakOnAllocationCount to an object you want to trace, 
      // then this assertion will be fired. See TAllocationBreakInit below...
      if (sBreakOnAllocationCount.size())
      {
        MAssert(sBreakOnAllocationCount.find(pAllocator) == sBreakOnAllocationCount.end() || 
          sBreakOnAllocationCount[pAllocator] != AllocationCount, "Allocation Break");
      }
    #endif
    
    // This is slow. Enable manually, if needed
    // MAssert(sInstances.find(Ptr) == sInstances.end(), "");
    
    sInstances.insert(std::make_pair(Ptr, 
      TLeakTrace(pAllocator, AllocationCount, Size)));
  }

  // -----------------------------------------------------------------------------------------------

  void OnRealloc(void* OldPtr, void* NewPtr, size_t Size)
  {
    TInstanceMap::iterator Iter = sInstances.find(OldPtr);
    MAssert(Iter != sInstances.end(), "");

    Iter->second.mByteSize = Size;
    
    if (NewPtr != OldPtr)
    {
      sInstances.insert(std::make_pair(NewPtr, Iter->second));
      sInstances.erase(Iter);
    }
  }
  
  // -----------------------------------------------------------------------------------------------

  void OnDelete(void* Ptr)
  {
    TInstanceMap::iterator Iter = sInstances.find(Ptr);
    
    MAssert(Iter != sInstances.end(), "Deleting an object with "
      "TMemory which was not allocated by TMemory");

    if (Iter != sInstances.end())
    {
      sInstances.erase(Iter);
    }
  }

  // -----------------------------------------------------------------------------------------------

  struct TSortByByteSize {
    bool operator()(
      const TInstancesVectorPair& First, 
      const TInstancesVectorPair& Second)const
    {
      size_t FirstByteCount = 0;
                                    
      for (TInstancesVector::const_iterator SubIter = First.second.begin(); 
           SubIter != First.second.end(); ++SubIter)
      {
        FirstByteCount += SubIter->mByteSize;
      }
      
      size_t SecondByteCount = 0;
                                    
      for (TInstancesVector::const_iterator SubIter = Second.second.begin(); 
           SubIter != Second.second.end(); ++SubIter)
      {
        SecondByteCount += SubIter->mByteSize;
      }

      return SecondByteCount > FirstByteCount;
    }
  };
  
  // -----------------------------------------------------------------------------------------------

  void DumpStatistics()
  {
    if (sInstances.size())
    {
      size_t TotalBytes = 0;

      for (TInstanceMap::const_iterator Iter = sInstances.begin(); 
           Iter != sInstances.end(); ++Iter)
      {
        TotalBytes += (*Iter).second.mByteSize;
      }

      gTraceVar("**********************************************************");
      gTraceVar("Memory statistics (%d Blocks, %s total):", 
        (int)sInstances.size(), ByteCountString(TotalBytes));
      gTraceVar("**********************************************************");

      gTraceVar(" ");
             
      // sort by name
      typedef std::map<std::string, TInstancesVector> TSortedInstancesMap;
      TSortedInstancesMap InstancesCopy;

      for (TInstanceMap::const_iterator Iter = sInstances.begin(); 
           Iter != sInstances.end(); ++Iter)
      {
        const std::string Name = ClassName((*Iter).second.mpDescription, (*Iter).first);
        InstancesCopy[Name].push_back(Iter->second);
      }
               
      // sort by bytes
      TSortedInstancesVector SortedInstancesCopy;

      for (TSortedInstancesMap::const_iterator Iter = InstancesCopy.begin(); 
           Iter != InstancesCopy.end(); ++Iter)
      {
        const std::string Name = (*Iter).first;
        SortedInstancesCopy.push_back(std::make_pair(Name, Iter->second));
      }
        
      std::sort(SortedInstancesCopy.begin(), SortedInstancesCopy.end(), TSortByByteSize());
                                                          
      // dump, sorted by bytes
      for (TSortedInstancesVector::const_iterator Iter = SortedInstancesCopy.begin(); 
           Iter != SortedInstancesCopy.end(); ++Iter)
      {
        size_t ByteCount = 0;
                                      
        for (TInstancesVector::const_iterator SubIter = Iter->second.begin(); 
             SubIter != Iter->second.end(); ++SubIter)
        {
          ByteCount += SubIter->mByteSize;
        }

        gTraceVar("Type: %s is instantiated %d times, allocating %s", 
          Iter->first.c_str(), (int)Iter->second.size(), ByteCountString(ByteCount));
      }

      gTraceVar(" ");
    }

    // dump info CRT memory info on windows as well:
    #if defined(MWindows)
      _CrtMemState MemState;
      _CrtMemCheckpoint(&MemState);

      _CrtMemDumpStatistics(&MemState);
    #endif
  }

  // -----------------------------------------------------------------------------------------------

  void DumpLeaks()
  {
    if (sInstances.size())
    {
      if (TSystem::ProcessKilled())
      {
        gTraceVar("Process was aborted/killed. Ignoring leaks...");
      }
      else
      {
        MInvalid("Found some leaked memory. See the following trace for details!");

        size_t TotalBytes = 0;

        for (TInstanceMap::const_iterator Iter = sInstances.begin(); 
            Iter != sInstances.end(); ++Iter)
        {
          TotalBytes += (*Iter).second.mByteSize;
        }

        gTraceVar("**********************************************************");
        gTraceVar("Found %d leaked Objects (%d Bytes total):", 
          (int)sInstances.size(), (int)TotalBytes);
        gTraceVar("**********************************************************");

        int Count = 0;

        for (TInstanceMap::const_iterator Iter = sInstances.begin(); 
            Iter != sInstances.end(); ++Iter)
        {
          gTraceVar("%d: \t Ptr: 0x%p \t AllocCount: %d \t Bytes: %d \t Type: %s (%s)",
            (int)Count++, (*Iter).first, (int)(*Iter).second.mAllocationCount, 
            (int)(*Iter).second.mByteSize, ClassName((*Iter).second.mpDescription, (*Iter).first),
            (*Iter).second.mpDescription);

          #if defined(MEnableStackTraces) 
            std::vector<std::string> Stack;
            TStackWalk::DumpStack((*Iter).second.mFramePointers, Stack);
            
            std::vector<std::string>::const_iterator StackIter;
            for (StackIter = Stack.begin(); StackIter != Stack.end(); ++StackIter)
            {
              gTraceVar("    %s", StackIter->c_str());
            }
          #endif  
        }
      }  
    }
    else
    {
      gTraceVar("No leaked objects found.");
    }
  }
}

#endif // #if defined(MEnableLeakTraces)

// =================================================================================================

template <size_t sBufferSize>
bool TEmergencyAllocator<sBufferSize>::sEmergencyAllocUsed = false;

// -------------------------------------------------------------------------------------------------

template <size_t sBufferSize>
TEmergencyAllocator<sBufferSize>::TEmergencyAllocator()
{
  // start with one big block which covers the whole available mem
  TMemory::SetByte(mBuffer, 0xFC, sBufferSize);
  *aliased_cast<int*>(mBuffer) = sBufferSize;
  
  // and alloc our extra heap buffer, which gets freed,
  // as soon as we kick in as second emergency stage...
  mpExraHeapBuffer = (char*)::malloc(sBufferSize);
}

// -------------------------------------------------------------------------------------------------

template <size_t sBufferSize>
TEmergencyAllocator<sBufferSize>::~TEmergencyAllocator()
{
  if (mpExraHeapBuffer)
  {
    ::free(mpExraHeapBuffer);
    mpExraHeapBuffer = NULL;
  }
}

// -------------------------------------------------------------------------------------------------

template <size_t sBufferSize>
inline bool TEmergencyAllocator<sBufferSize>::WasAllocated(void* pMem)const
{
  return (pMem >= (void*)mBuffer && pMem < (void*)(mBuffer + sBufferSize));
}

// -------------------------------------------------------------------------------------------------

template <size_t sBufferSize>
void* TEmergencyAllocator<sBufferSize>::Alloc(size_t SizeInBytes)
{
  MAssert(SizeInBytes <= sBufferSize - sizeof(int), 
    "Fatal error. Can not handle such big allocations!");

  // can not handle 0 sizes below...
  if (!SizeInBytes)
  {
    return NULL;
  }

  const std::lock_guard<std::mutex> Lock(mBufferLock);

  // show a memory is low warning when used the first time...
  if (!sEmergencyAllocUsed)
  {
    sEmergencyAllocUsed = true;
    
    // and free up some memory for other non TMemory clients
    // as second security stage
    if (mpExraHeapBuffer)
    {
      ::free(mpExraHeapBuffer);
      mpExraHeapBuffer = NULL;
    }
    
    TSystem::ShowMessage(MText("Memory is getting low! "
      "Please free up some memory, save the current document "
      "under a new name and !restart! $MProductString.\n\n"
      "This message is only shown once..."), TSystem::kWarning);
  }

  
  // . try to find a free memory section in out buffer

  char* pBuffer = aliased_cast<char*>(mBuffer);

  for (int CurrentBlockPosition = 0; (size_t)CurrentBlockPosition < sBufferSize; )
  {
    int CurrentBlockSize = *(int*)(pBuffer + CurrentBlockPosition);
    MAssert(CurrentBlockSize != 0, "Damaged Buffer!");

    // > 0 free, < 0 used
    const bool BlockIsFree = (CurrentBlockSize > 0);

    if (BlockIsFree)
    {

      // . merge all following free blocks (Free wont do this)

      int NextBlockHeaderPosition = 
        (int)(CurrentBlockPosition + CurrentBlockSize + sizeof(int));

      while ((size_t)NextBlockHeaderPosition < sBufferSize && 
        *(int*)(pBuffer + NextBlockHeaderPosition) > 0)
      {
        const int NextBlockSize = *(int*)(pBuffer + NextBlockHeaderPosition);
        MAssert(NextBlockSize != 0, "Damaged Buffer!");
        
        *(int*)(pBuffer + CurrentBlockPosition) += (int)(NextBlockSize + sizeof(int));
        CurrentBlockSize += (int)(NextBlockSize + sizeof(int));

        NextBlockHeaderPosition += (int)(NextBlockSize + sizeof(int));
      }


      // . got enough free space in the current block?

      if ((size_t)CurrentBlockSize >= SizeInBytes + sizeof(int))
      {
        // then mark it as used
        const int CurrentBlocksTotalSize = *(int*)(pBuffer + CurrentBlockPosition);
        *(int*)(pBuffer + CurrentBlockPosition) = -(int)SizeInBytes;

        // split our size from the current block
        if (CurrentBlockPosition + sizeof(int) + SizeInBytes < sBufferSize)
        {
          if ((size_t)CurrentBlocksTotalSize > sizeof(int) + SizeInBytes)
          {
            *(int*)(pBuffer + CurrentBlockPosition + sizeof(int) + SizeInBytes) =
              (int)(CurrentBlocksTotalSize - sizeof(int) - SizeInBytes);
          }
          else
          {
            // add the nexts header to our size...
            MAssert((size_t)CurrentBlocksTotalSize == sizeof(int) + SizeInBytes, 
              "Unexpected size");
            
            *(int*)(pBuffer + CurrentBlockPosition) -= (int)(sizeof(int));
          }
        }

        // and return the mem behind the header
        return (void*)(pBuffer + CurrentBlockPosition + sizeof(int));
      }
    }

    CurrentBlockPosition += (int)(::labs(CurrentBlockSize) + sizeof(int));
  }


  // . out of emergency buffer space...

  return NULL; 
}

// -------------------------------------------------------------------------------------------------

template <size_t sBufferSize>
void* TEmergencyAllocator<sBufferSize>::Realloc(void* pMem, size_t NewSize)
{
  const size_t OldSize = ::labs(((int*)pMem)[-1]);
  
  void* pNew = Alloc(NewSize);

  if (pNew)
  {
    TMemory::Copy(pNew, pMem, MMin(OldSize, NewSize));
    Free(pMem);
  }

  return pNew;
}

// -------------------------------------------------------------------------------------------------

template <size_t sBufferSize>
void TEmergencyAllocator<sBufferSize>::Free(void* pMem)
{
  if (pMem != NULL)
  {
    const std::lock_guard<std::mutex> Lock(mBufferLock);

    MAssert(WasAllocated(pMem), "Not ours!");
    MAssert(((int*)pMem)[-1] < 0, "Invalid adress");

    #if defined(MDebug)
      const int BlockSize = -*(int*)((char*)pMem - sizeof(int));
      TMemory::SetByte(pMem, 0xFC, BlockSize);
    #endif

    // simply mark as free, alloc will merge the blocks if needed
    *(int*)((char*)pMem - sizeof(int)) *= -1;
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TMemory::Move(void* pDestBuffer, const void* pSourceBuffer, size_t ByteCount)
{
  // NB: no memmove alike function in IPP
  ::memmove(pDestBuffer, pSourceBuffer, ByteCount);
}

// -------------------------------------------------------------------------------------------------

void TMemory::Copy(void* pDestBuffer, const void* pSourceBuffer, size_t ByteCount)
{
  MAssert((TUInt8*)pDestBuffer < (TUInt8*)pSourceBuffer  ||
      (TUInt8*)pDestBuffer >= (TUInt8*)pSourceBuffer + ByteCount,
    "Overlapped src/dest buffer. Use TMemory::Move instead!");

  #if defined(MHaveIntelIPP)
    if (ByteCount > 0) // be compatible with memcpy. ipps call will else assert
    {
      MCheckIPPStatus(
        ::ippsCopy_8u((const Ipp8u*)pSourceBuffer, (Ipp8u*)pDestBuffer, (int)ByteCount));
    }
  #else
    ::memcpy(pDestBuffer, pSourceBuffer, ByteCount);
  #endif
}

// -------------------------------------------------------------------------------------------------

void TMemory::CopyWord(void* pDestBuffer, const void* pSourceBuffer, size_t WordCount)
{
  MAssert((TUInt16*)pDestBuffer < (TUInt16*)pSourceBuffer  ||
      (TUInt16*)pDestBuffer >= (TUInt16*)pSourceBuffer + WordCount,
    "Overlapped src/dest buffer. Use TMemory::Move instead!");

  #if defined(MHaveIntelIPP)
    if (WordCount > 0) // be compatible with memcpy. ipps call will else assert
    {
      MCheckIPPStatus(
        ::ippsCopy_16s((const Ipp16s*)pSourceBuffer, (Ipp16s*)pDestBuffer, (int)WordCount));
    }
  #else
    ::memcpy(pDestBuffer, pSourceBuffer, WordCount << 1);
  #endif
}

// -------------------------------------------------------------------------------------------------

void TMemory::CopyDWord(void* pDestBuffer, const void* pSourceBuffer, size_t DWordCount)
{
  MAssert((TUInt32*)pDestBuffer < (TUInt32*)pSourceBuffer  ||
      (TUInt32*)pDestBuffer >= (TUInt32*)pSourceBuffer + DWordCount,
    "Overlapped src/dest buffer. Use TMemory::Move instead!");

  #if defined(MHaveIntelIPP)
    if (DWordCount > 0) // be compatible with memcpy. ipps call will else assert
    {
      MCheckIPPStatus(
        ::ippsCopy_32s((const Ipp32s*)pSourceBuffer, (Ipp32s*)pDestBuffer, (int)DWordCount));
    }
  #else
    ::memcpy(pDestBuffer, pSourceBuffer, DWordCount << 2);
  #endif
}

// -------------------------------------------------------------------------------------------------

void TMemory::SetByte(void* pDestBuffer, TUInt8 FillByte, size_t ByteCount)
{
  #if defined(MHaveIntelIPP)
    if (ByteCount > 0) // be compatible with memset. ipps call will else assert
    {
      if (FillByte == 0)
      {
        MCheckIPPStatus(
          ::ippsZero_8u((Ipp8u*)pDestBuffer, (int)ByteCount));
      }
      else
      {
        MCheckIPPStatus(
          ::ippsSet_8u((Ipp8u)FillByte, (Ipp8u*)pDestBuffer, (int)ByteCount));
      }
    }
  #else
    ::memset(pDestBuffer, FillByte, ByteCount);
  #endif
}

// -------------------------------------------------------------------------------------------------

void TMemory::SetWord(void* pDestBuffer, TUInt16 FillWord, size_t WordCount)
{
  #if defined(MHaveIntelIPP)
    if (WordCount > 0) // be compatible with memset. ipps call will else assert
    {
      if (FillWord == 0)
      {
        MCheckIPPStatus(
          ::ippsZero_16s((Ipp16s*)pDestBuffer, (int)WordCount));
      }
      else
      {
        MCheckIPPStatus(
          ::ippsSet_16s((Ipp16s)FillWord, (Ipp16s*)pDestBuffer, (int)WordCount));
      }
    }
  #else
    TUInt16* pWordBuffer = (TUInt16*)pDestBuffer;
    for (size_t i = 0; i < WordCount; ++i)
    {
      pWordBuffer[i] = FillWord;
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

void TMemory::SetDWord(void* pDestBuffer, TUInt32 FillDWord, size_t DWordCount)
{
  #if defined(MHaveIntelIPP)
    if (DWordCount > 0) // be compatible with memset. ipps call will else assert
    {
      if (FillDWord == 0)
      {
        MCheckIPPStatus(
          ::ippsZero_32s((Ipp32s*)pDestBuffer, (int)DWordCount));
      }
      else
      {
        MCheckIPPStatus(
          ::ippsSet_32s((Ipp32s)FillDWord, (Ipp32s*)pDestBuffer, (int)DWordCount));
      }
    }
  #else
    TUInt32* pDWordBuffer = (TUInt32*)pDestBuffer;
    for (size_t i = 0; i < DWordCount; ++i)
    {
      pDWordBuffer[i] = FillDWord;
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

void TMemory::Zero(void* pDestBuffer, size_t ByteCount)
{
  #if defined(MHaveIntelIPP)
    if (ByteCount > 0) // be compatible with memset. ipps call will else assert
    {
      MCheckIPPStatus(
        ::ippsZero_8u((Ipp8u*)pDestBuffer, (int)ByteCount));
    }
  #else
    ::memset(pDestBuffer, 0, ByteCount);
  #endif
}

// -------------------------------------------------------------------------------------------------

void TMemory::DumpLeaks()
{
  #if defined(MEnableLeakTraces)
    TLeakCheck::DumpLeaks();
  #endif
}

// -------------------------------------------------------------------------------------------------

bool TMemory::WasAllocated(void* Ptr)
{
  #if defined(MEnableLeakTraces)
    const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock);
    return TLeakCheck::WasAllocated(Ptr);
  #else
    return true;
  #endif
}

// -------------------------------------------------------------------------------------------------

void TMemory::DumpStatistics()
{
  #if defined(MEnableLeakTraces)
    const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock);
    TLeakCheck::DumpStatistics();
  #endif
}

// -------------------------------------------------------------------------------------------------

size_t TMemory::PageSize()
{
  #if defined(MWindows)
    static int sPagesSize = 0;
    
    if (!sPagesSize) 
    {
      SYSTEM_INFO system_info;
      ::GetSystemInfo(&system_info);
      sPagesSize = system_info.dwPageSize;
    }

    return sPagesSize;

  #else
    static int sPagesSize = ::getpagesize();
    return sPagesSize;
  
  #endif
}
  
// -------------------------------------------------------------------------------------------------

void* TMemory::Alloc(const char* pDescription, size_t SizeInBytes)
{
  MAssert(TSystem::InMain(), "Not yet (or no longer) initialized.");

  if (SizeInBytes)
  {
    #if defined(MEnableLeakTraces)
    const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock); 
    #endif
    
    void* Ptr = ::malloc(SizeInBytes);

    if (!Ptr)
    {
      if (SizeInBytes < MEmergencyAllocThreshold)
      {
        Ptr = sEmergencyAllocator.Alloc(SizeInBytes);
        
        if (Ptr == NULL)
        {
          throw TOutOfMemoryException();
        }
      }
      else
      {
        throw TOutOfMemoryException();
      }
    }

    #if defined(MEnableLeakTraces)
      TLeakCheck::OnAlloc(pDescription, Ptr, SizeInBytes);
    #endif

    return Ptr;
  }
  else
  {
    return NULL;
  }
}

// -------------------------------------------------------------------------------------------------

void* TMemory::Calloc(const char* pDescription, size_t Count, size_t SizeInBytes)
{
  MAssert(TSystem::InMain(), "Not yet (or no longer) initialized.");
  
  if (SizeInBytes)
  {
    #if defined(MEnableLeakTraces)
      const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock);
    #endif
    
    void* Ptr = ::calloc(Count, SizeInBytes);
    
    // no emergency alloc...
    if (!Ptr)
    {
      throw TOutOfMemoryException();
    }

    #if defined(MEnableLeakTraces)
      TLeakCheck::OnAlloc(pDescription, Ptr, Count * SizeInBytes);
    #endif

    return Ptr;
  }
  else
  {
    return NULL;
  }
}

// -------------------------------------------------------------------------------------------------

void* TMemory::Realloc(void* pMem, size_t NewSizeInBytes)
{
  MAssert(TSystem::InMain(), "Not yet (or no longer) initialized.");
  MAssert(NewSizeInBytes && pMem, "Invalid realloc request");

  #if defined(MEnableLeakTraces)
    const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock);
  #endif
  
  void* Ptr;
  
  if (sEmergencyAllocator.WasAllocated(pMem))
  {
    Ptr = sEmergencyAllocator.Realloc(pMem, NewSizeInBytes);
  }
  else
  {
    Ptr = ::realloc(pMem, NewSizeInBytes);
  }
  
  #if defined(MEnableLeakTraces)
    TLeakCheck::OnRealloc(pMem, Ptr, NewSizeInBytes);
  #endif
  
  if (!Ptr && NewSizeInBytes)
  {
    throw TOutOfMemoryException();
  }

  return Ptr;
}

// -------------------------------------------------------------------------------------------------

void TMemory::Free(void* Ptr)
{
  MAssert(TSystem::InMain(), "Not yet (or no longer) initialized.");

  // just leak when called from the static deinitializers. our static
  // objects (sLeakTraceLock and sEmergencyAllocator) may already be dead.
  if (Ptr != NULL && TSystem::InMain())
  {
    #if defined(MEnableLeakTraces)
      const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock);
    #endif
      
    if (sEmergencyAllocator.WasAllocated(Ptr))
    {
      sEmergencyAllocator.Free(Ptr);
    }
    else
    {
      ::free((char*)Ptr);
    }

    #if defined(MEnableLeakTraces)
      TLeakCheck::OnDelete(Ptr);
    #endif
  }
}

// -------------------------------------------------------------------------------------------------

void* TMemory::PageAlignedAlloc(const char* pDescription, size_t SizeInBytes)
{
  MAssert(TSystem::InMain(), "Not yet (or no longer) initialized.");

  if (SizeInBytes)
  {
    #if defined(MEnableLeakTraces)
      const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock);
    #endif

    #if defined(MWindows)
      void* Ptr = ::malloc(SizeInBytes);
    #else
      void* Ptr = ::valloc(SizeInBytes);
    #endif

    if (!Ptr)
    {
      throw TOutOfMemoryException();
    }

    #if defined(MEnableLeakTraces)
      TLeakCheck::OnAlloc(pDescription, Ptr, SizeInBytes);
    #endif

    return Ptr;
  }
  else
  {
    return NULL;
  }
}

// -------------------------------------------------------------------------------------------------

void TMemory::PageAlignedFree(void* Ptr)
{
  MAssert(TSystem::InMain(), "Not yet (or no longer) initialized.");

  // just leak when called from the static deinitializers. our static
  // objects (sLeakTraceLock and sEmergencyAllocator) may already be dead.
  if (Ptr != NULL && TSystem::InMain())
  {
    #if defined(MEnableLeakTraces)
      const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock);
    #endif

    if (sEmergencyAllocator.WasAllocated(Ptr))
    {
      sEmergencyAllocator.Free(Ptr);
    }
    else
    {
      ::free((char*)Ptr);
    }

    #if defined(MEnableLeakTraces)
      TLeakCheck::OnDelete(Ptr);
    #endif
  }
}

// -------------------------------------------------------------------------------------------------

void* TMemory::AllocSmallObject(const char* pIdentifier, size_t SizeInBytes)
{
  MAssert(TSystem::InMain(), "Not yet (or no longer) initialized.");
  
  if (SizeInBytes)
  {
    #if defined(MEnableLeakTraces) && defined(MEnableSmallObjectLeakTraces)
      const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock);
    #endif
    
    void* Ptr = ::malloc(SizeInBytes);

    if (!Ptr)
    {
      if (SizeInBytes < MEmergencyAllocThreshold)
      {
        Ptr = sEmergencyAllocator.Alloc(SizeInBytes);
        
        if (Ptr == NULL)
        {
          throw TOutOfMemoryException();
        }
      }
      else
      {
        throw TOutOfMemoryException();
      }
    }

    #if defined(MEnableLeakTraces) && defined(MEnableSmallObjectLeakTraces)
      TLeakCheck::OnAlloc(pIdentifier, Ptr, SizeInBytes);
    #endif

    return Ptr;
  }
  else
  {
    return NULL;
  }
}

// -------------------------------------------------------------------------------------------------

void TMemory::FreeSmallObject(void* Ptr)
{
  MAssert(TSystem::InMain(), "Not yet (or no longer) initialized.");

  // just leak when called from the static deinitializers. our static
  // objects (sLeakTraceLock and sEmergencyAllocator) may already be dead.
  if (Ptr != NULL && TSystem::InMain())
  {
    #if defined(MEnableLeakTraces) && defined(MEnableSmallObjectLeakTraces)
      const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock);
    #endif

    if (sEmergencyAllocator.WasAllocated(Ptr))
    {
      sEmergencyAllocator.Free(Ptr);
    }
    else
    {
      ::free((char*)Ptr);
    }

    #if defined(MEnableLeakTraces) && defined(MEnableSmallObjectLeakTraces)
      TLeakCheck::OnDelete(Ptr);
    #endif
  }
}

// -------------------------------------------------------------------------------------------------

void* TMemory::AlignedAlloc(const char* pDescription, size_t SizeInBytes, size_t Alignment)
{
  MAssert(TSystem::InMain(), "Not yet (or no longer) initialized.");
 
  if (SizeInBytes)
  {
    #if defined(MEnableLeakTraces)
      const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock);
    #endif
    
    #if defined(MWindows)
      char* ptr = (char*)::_aligned_malloc(SizeInBytes, Alignment);
    #elif defined(MMac)
      char* ptr = (char*)::memalign_mac(Alignment, SizeInBytes);
    #elif defined(MLinux)
      char* ptr = (char*)::memalign(Alignment, SizeInBytes);
    #else
      #error "Unknown platform"
    #endif

    if (!ptr)
    {
      if (SizeInBytes < MEmergencyAllocThreshold)
      {
        const size_t FinalSize = SizeInBytes + Alignment + sizeof(int);
        ptr = (char*)sEmergencyAllocator.Alloc(FinalSize);
        
        if (ptr == NULL)
        {
          throw TOutOfMemoryException();
        }

        const ptrdiff_t align_mask = (ptrdiff_t)(Alignment - 1);
        char* ptr2 = ptr + sizeof(int);
        char* aligned_ptr = ptr2 + (Alignment - ((ptrdiff_t)ptr2 & align_mask));

        ptr2 = aligned_ptr - sizeof(int);
        *((int*)ptr2) = (int)(aligned_ptr - ptr);

        ptr = aligned_ptr;
      }
      else
      {
        throw TOutOfMemoryException();
      }
    }

    #if defined(MEnableLeakTraces)
      TLeakCheck::OnAllocAligned(pDescription, ptr, SizeInBytes);
    #endif

    return ptr;
  }
  else
  {
    return NULL;
  }
}

// -------------------------------------------------------------------------------------------------

void TMemory::AlignedFree(void* Ptr)
{
  MAssert(TSystem::InMain(), "Not yet (or no longer) initialized.");

  // just leak when called from the static deinitializers. our static
  // objects (sLeakTraceLock and sEmergencyAllocator) may already be dead.
  if (Ptr != NULL && TSystem::InMain())
  {
    #if defined(MEnableLeakTraces)
      const std::lock_guard<std::mutex> Lock(TLeakCheck::sLeakTraceLock);
    #endif
    
    if (sEmergencyAllocator.WasAllocated(Ptr))
    {
      int* ptr2 = (int*)Ptr - 1;
      const int ShiftAmount = *ptr2;
      char* UnalignedPtr = (char*)Ptr - ShiftAmount;

      sEmergencyAllocator.Free(UnalignedPtr);
    }
    else
    {
      #if defined(MWindows)
        ::_aligned_free((char*)Ptr);
      #elif defined(MMac)
        ::memalign_free_mac((char*)Ptr);
      #elif defined(MLinux)
        ::free((char*)Ptr);
      #else
        #error "Unknown platform"
      #endif
    }

    #if defined(MEnableLeakTraces)
      TLeakCheck::OnDelete(Ptr);
    #endif
  }
}

