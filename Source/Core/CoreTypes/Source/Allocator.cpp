#include "CoreTypesPrecompiledHeader.h"

#include <cstring>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TBlockAllocator::TBlockAllocator(
  const char* pAllocatorDescription,
  size_t      ObjectSize, 
  size_t      NumObjectsPerBlock)
  : mpAllocatorDescription(pAllocatorDescription),
    mObjectSize(ObjectSize),
    mNumberOfObjectsPerBlock(NumObjectsPerBlock)
{
  const size_t BlockSize = sizeof(TBlockHeader) + mNumberOfObjectsPerBlock * 
    (sizeof(TElementHeader) + mObjectSize); 

  mpCurrentBlock = (TBlockHeader*)TMemory::Alloc(
    mpAllocatorDescription, BlockSize);
  
  mpCurrentBlock->mpNextBlock = NULL;
  mpCurrentBlock->mpPrevBlock = NULL;
  mpCurrentBlock->mNumberOfAllocatedObjects = 0;

  mNumberOfObjectsInCurrentBlock = 0;
}

// -------------------------------------------------------------------------------------------------

TBlockAllocator::~TBlockAllocator()
{
  MAssert(mpCurrentBlock->mNumberOfAllocatedObjects == 0 && 
      mpCurrentBlock->mpPrevBlock == NULL && mpCurrentBlock->mpNextBlock == NULL, 
    "Leaked some blocks/objects!");
  
  TMemory::Free(mpCurrentBlock);
  mpCurrentBlock = NULL;
}

// -------------------------------------------------------------------------------------------------
  
void* TBlockAllocator::Alloc()
{
  #if defined(MDebug)
    // should never be called from multiple threads at once
    MAssert(!m__Allocating, "Not thread safe. Lock access to Alloc/Free!");
    const TIncDecValue<std::atomic<int>> Allocating(m__Allocating);
  #endif

  // NB: !mNumberOfObjectsInCurrentBlock! and NOT pBlock->mNumberOfAllocatedObjects.
  // mNumberOfObjectsInCurrentBlock never gets decreased, only reset. When using 
  // mNumberOfAllocatedObjects, we may overwrite an existing object's memory slot:

  // allocated a new block when the current block has no more free space
  if (mNumberOfObjectsInCurrentBlock == mNumberOfObjectsPerBlock)
  {
    const size_t BlockSize = sizeof(TBlockHeader) + mNumberOfObjectsPerBlock * 
      (sizeof(TElementHeader) + mObjectSize); 

    TBlockHeader* pNewBlock = (TBlockHeader*)TMemory::Alloc(
      mpAllocatorDescription, BlockSize);
    
    pNewBlock->mNumberOfAllocatedObjects = 0;

    mpCurrentBlock->mpNextBlock = pNewBlock;
    pNewBlock->mpPrevBlock = mpCurrentBlock;
    pNewBlock->mpNextBlock = NULL;

    mpCurrentBlock = pNewBlock;
    mNumberOfObjectsInCurrentBlock = 0;
  }
  
  // get memory for object from the current block
  MAssert(mpCurrentBlock->mNumberOfAllocatedObjects < 
    mNumberOfObjectsPerBlock, "");

  // Use mNumberOfObjectsInCurrentBlock here. See big comment above.
  void* ElementPtr = (char*)mpCurrentBlock + sizeof(TBlockHeader) + 
    (mObjectSize + sizeof(TElementHeader)) * mNumberOfObjectsInCurrentBlock;

  TElementHeader* pElementHeader = (TElementHeader*)ElementPtr;
  pElementHeader->mpBlock = mpCurrentBlock;
  
  ++mpCurrentBlock->mNumberOfAllocatedObjects;
  ++mNumberOfObjectsInCurrentBlock;

  return (char*)pElementHeader + sizeof(TElementHeader);
}

// -------------------------------------------------------------------------------------------------

void TBlockAllocator::Free(void* pMem)
{
  #if defined(MDebug)
    // should never be called from multiple threads at once
    MAssert(!m__Allocating, "Not thread safe. Lock access to Alloc/Free!");
    const TIncDecValue<std::atomic<int>> Allocating(m__Allocating);
  #endif

  #if defined(MDebug)
    TMemory::SetByte(pMem, 0xCC, mObjectSize);
  #endif
         
  TElementHeader* pElementHeader = (TElementHeader*)
    ((char*)pMem - sizeof(TElementHeader));
  
  TBlockHeader* pBlock = pElementHeader->mpBlock;

  MAssert(pBlock->mNumberOfAllocatedObjects > 0 && 
    pBlock->mNumberOfAllocatedObjects <= mNumberOfObjectsPerBlock, "");
                        
  if (--pBlock->mNumberOfAllocatedObjects == 0)
  {
    if (pBlock == mpCurrentBlock)
    {
      // do not free mpCurrentBlock, but keep it ready to use
      mNumberOfObjectsInCurrentBlock = 0;
    }
    else
    {
      // unlink and free block when it's not the current one
      if (pBlock->mpNextBlock)
      {
        pBlock->mpNextBlock->mpPrevBlock = pBlock->mpPrevBlock;
      }
      
      if (pBlock->mpPrevBlock)
      {
        pBlock->mpPrevBlock->mpNextBlock = pBlock->mpNextBlock;
      }
      
      TMemory::Free(pBlock);
    }
  }  
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TLocalBlockAllocator::TLocalBlockAllocator(
  const char* pAllocatorDescription,
  size_t      ObjectSize, 
  size_t      NumObjectsPerBlock)
  : mpAllocatorDescription(pAllocatorDescription),
    mObjectSize(ObjectSize), 
    mNumberOfObjectsPerBlock(NumObjectsPerBlock)
{
  #if defined(MDebug)
    m__TotalNumberOfObjects = 0;
  #endif

  mNumberOfObjectsInCurrentBlock = 0;
  mpCurrentBlock = NULL;
}

// -------------------------------------------------------------------------------------------------

TLocalBlockAllocator::~TLocalBlockAllocator()
{
  PurgeAll();
}

// -------------------------------------------------------------------------------------------------
  
void* TLocalBlockAllocator::Alloc()
{
  #if defined(MDebug)
    // should never be called from multiple threads at once
    MAssert(!m__Allocating, "Not thread safe. Lock access to Alloc/Free!");
    const TIncDecValue<std::atomic<int>> Allocating(m__Allocating);
  #endif

  if (mpCurrentBlock == NULL || 
      mNumberOfObjectsInCurrentBlock == mNumberOfObjectsPerBlock)
  {
    const size_t BlockSize = sizeof(TBlockHeader) + 
      mNumberOfObjectsPerBlock * mObjectSize;
    
    TBlockHeader* pNewBlock = (TBlockHeader*)TMemory::PageAlignedAlloc(
      "#LocalBlockAllocator", BlockSize);
    
    pNewBlock->mpPrevBlock = mpCurrentBlock;
    mpCurrentBlock = pNewBlock;
    
    mNumberOfObjectsInCurrentBlock = 0;
  }
  
  #if defined(MDebug)
    ++m__TotalNumberOfObjects;
  #endif
  
  return (char*)mpCurrentBlock + sizeof(TBlockHeader) + 
    mObjectSize * mNumberOfObjectsInCurrentBlock++;
}

// -------------------------------------------------------------------------------------------------

void TLocalBlockAllocator::Free(void* pMem)
{
  #if defined(MDebug)
    --m__TotalNumberOfObjects;
  #endif

  #if defined(MDebug)
    TMemory::SetByte(pMem, 0xCC, mObjectSize);
  #endif

  // keep memory allocated until this object gets destructed...
}

// -------------------------------------------------------------------------------------------------
    
void TLocalBlockAllocator::PurgeAll()
{
  #if defined(MDebug)
    // should never be called from multiple threads at once
    MAssert(!m__Allocating, "Not thread safe. Lock access to Alloc/Free!");
    const TIncDecValue<std::atomic<int>> Allocating(m__Allocating);
  #endif

  MAssert(m__TotalNumberOfObjects == 0, "Not all Objects where freed!");
  
  TBlockHeader* pCur = mpCurrentBlock;
  
  while (pCur)
  {    
    TBlockHeader* pPrev = pCur->mpPrevBlock;    
    TMemory::PageAlignedFree(pCur);
    pCur = pPrev;
  }

  mpCurrentBlock = NULL;
}

// =================================================================================================

//lint -save -e1790, -e1509 (base class destructor not virtual)

class TVoidPointerList : public TList<void*> { };

//lint -restore

// -------------------------------------------------------------------------------------------------

TLocalAllocator::TLocalAllocator()
  : mBlockAllocationSizeThreshold(TMemory::PageSize() * 4 - sizeof(TChunkHeader)),
    mpCurrBlockChunkHeader(NULL), 
    mpBlocks(new TVoidPointerList()),
    mpHugeBuffers(new TVoidPointerList())
{  
  // mBlockAllocationSizeThreshold: allocated blocks page aligned in size of 4 pages
  // to avoid messing up the systems allocator (we temporarily allocate a lot of mem)

  #if defined(MDebug)
    m__CurrentlyAllocatedBytes = 0;
  #endif
}

// -------------------------------------------------------------------------------------------------

TLocalAllocator::~TLocalAllocator()
{
  MAssert(mpHugeBuffers->IsEmpty() && m__CurrentlyAllocatedBytes == 0, 
    "Leaked huge or block buffer");
  
  for (int i = 0; i < mpBlocks->Size(); ++i)
  {
    TMemory::PageAlignedFree((*mpBlocks)[i]);
  }
    
  delete mpBlocks;
  mpBlocks = NULL;

  delete mpHugeBuffers;
  mpHugeBuffers = NULL;

  // not allocated
  mpCurrBlockChunkHeader = NULL;
}

// -------------------------------------------------------------------------------------------------

void* TLocalAllocator::Alloc(size_t RequestedSize)
{
  #if defined(MDebug)
    // should never be called from multiple threads at once
    MAssert(!m__Allocating, "Not thread safe. Lock access to Alloc/Free!");
    const TIncDecValue<std::atomic<int>> Allocating(m__Allocating);
  #endif

  if (RequestedSize > mBlockAllocationSizeThreshold)
  {
    void* pHugeBuffer = TMemory::Alloc("#LocalBlockAllocator_Huge", 
      RequestedSize + sizeof(TChunkHeader));
    
    TChunkHeader* pHeader = (TChunkHeader*)pHugeBuffer; 
    pHeader->mMagic = (char)kHugeBufferMagic;
    pHeader->mSize = RequestedSize;
    pHeader->mpPrev = NULL;
    
    mpHugeBuffers->Append(pHugeBuffer);
  
    #if defined(MDebug)
      m__CurrentlyAllocatedBytes += RequestedSize;
    #endif

    return (char*)pHugeBuffer + sizeof(TChunkHeader);
  }
  else
  {
    TChunkHeader* pNewChunk;
    
    if (!mpBlocks->Size() || (int)RequestedSize > FreeMemoryInCurrentBlock())
    {
      void* pNewBlock = TMemory::PageAlignedAlloc("#LocalBlockAllocator_Block", 
        mBlockAllocationSizeThreshold + sizeof(TChunkHeader));
        
      mpBlocks->Append(pNewBlock);
      
      pNewChunk = (TChunkHeader*)mpBlocks->Last();
      
      pNewChunk->mpPrev = NULL;
      mpCurrBlockChunkHeader = NULL;
    }
    else
    {
      pNewChunk = (TChunkHeader*)((ptrdiff_t)mpCurrBlockChunkHeader + 
        sizeof(TChunkHeader) + mpCurrBlockChunkHeader->mSize);
        
      pNewChunk->mpPrev = mpCurrBlockChunkHeader;
    }
    
    pNewChunk->mSize = RequestedSize;
    pNewChunk->mMagic = (char)kBlockBufferMagic;
        
    mpCurrBlockChunkHeader = pNewChunk;

    #if defined(MDebug)
      m__CurrentlyAllocatedBytes += RequestedSize;
    #endif

    return ((char*)pNewChunk) + sizeof(TChunkHeader);
  }
}

// -------------------------------------------------------------------------------------------------

void* TLocalAllocator::Realloc(void* pMem, size_t NewSize)
{
  TChunkHeader* pHeader = (TChunkHeader*)
    ((char*)pMem - sizeof(TChunkHeader));
  
  if (pHeader->mMagic == kHugeBufferMagic)
  {
    #if defined(MDebug)
      // should never be called from multiple threads at once
      MAssert(!m__Allocating, "Not thread safe. Lock access to Alloc/Free!");
      const TIncDecValue<std::atomic<int>> Allocating(m__Allocating);
    #endif

    #if defined(MDebug)
      const size_t __OldSize = pHeader->mSize;
    #endif

    void* pNew = TMemory::Realloc((void*)pHeader, NewSize + sizeof(TChunkHeader));
    
    #if defined(MDebug)
      m__CurrentlyAllocatedBytes += NewSize;
      m__CurrentlyAllocatedBytes -= __OldSize;
    #endif
    
    TChunkHeader* pNewHeader = (TChunkHeader*)pNew;
    pNewHeader->mSize = NewSize;
  
    if (pNew != pHeader)
    {
      MAssert(mpHugeBuffers->Find(pHeader) != -1, "Not our Buffer!");
      (*mpHugeBuffers)[mpHugeBuffers->Find(pHeader)] = pNew;
    }
    
    return (char*)pNew + sizeof(TChunkHeader);
  }
  else
  {
    // should never be called from multiple threads at once
    MAssert(!m__Allocating, "Not thread safe. Lock access to Alloc/Free!");

    const size_t OldSize = pHeader->mSize;
    
    void* pNew = Alloc(NewSize);
    TMemory::Copy(pNew, pMem, MMin(OldSize, NewSize));
    Free(pMem);
    
    return pNew;
  }
}

// -------------------------------------------------------------------------------------------------

void TLocalAllocator::Free(void* pMem)
{
  #if defined(MDebug)
    // should never be called from multiple threads at once
    MAssert(!m__Allocating, "Not thread safe. Lock access to Alloc/Free!");
    const TIncDecValue<std::atomic<int>> Allocating(m__Allocating);
  #endif

  TChunkHeader* pHeader = (TChunkHeader*)
    ((char*)pMem - sizeof(TChunkHeader));
  
  #if defined(MDebug)
    m__CurrentlyAllocatedBytes -= pHeader->mSize;
  #endif
  
  if (pHeader->mMagic == kHugeBufferMagic)
  {
    TMemory::Free(pHeader);
    
    MAssert(mpHugeBuffers->Find(pHeader) != -1, "Not our Buffer!");
    mpHugeBuffers->Delete(mpHugeBuffers->Find(pHeader));
  }
  else
  {
    // only free if its the last allocated block chunk
    if (pHeader == mpCurrBlockChunkHeader)
    {
      mpCurrBlockChunkHeader = mpCurrBlockChunkHeader->mpPrev;
      
      if (mpCurrBlockChunkHeader == NULL)
      {
        mpCurrBlockChunkHeader = (TChunkHeader*)mpBlocks->Last();
      }
    }
    
    #if defined(MDebug)
      TMemory::SetByte(pMem, 0xCC, pHeader->mSize);
    #endif
  }
}

// -------------------------------------------------------------------------------------------------
  
int TLocalAllocator::FreeMemoryInCurrentBlock()const
{
  if (mpCurrBlockChunkHeader == NULL)
  {
    return (int)mBlockAllocationSizeThreshold;
  }
  else
  {
    const ptrdiff_t LastBlockEnd = 
      ((ptrdiff_t)mpCurrBlockChunkHeader - (ptrdiff_t)mpBlocks->Last()) + 
      sizeof(TChunkHeader) + mpCurrBlockChunkHeader->mSize;
      
    const ptrdiff_t AvailableRaw = ((ptrdiff_t)mBlockAllocationSizeThreshold + 
      sizeof(TChunkHeader)) - LastBlockEnd;
    
    const ptrdiff_t AvailableUseable = AvailableRaw - sizeof(TChunkHeader);
    
    MAssert(AvailableRaw >= 0 && 
      AvailableUseable <= (int)mBlockAllocationSizeThreshold, "");
    
    // Note: can be < 0 here
    return (int)AvailableUseable;
  }
}
     
