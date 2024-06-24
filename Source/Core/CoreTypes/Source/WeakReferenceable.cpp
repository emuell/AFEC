#include "CoreTypesPrecompiledHeader.h"

#include <map>
#include <set>
#include <mutex>

// =================================================================================================

typedef std::set<
  TWeakRefOwner*, 
  std::less<TWeakRefOwner*>
> TWeakRefOwnerSet;

typedef std::map<
  TWeakReferenceable*, 
  TWeakRefOwnerSet, 
  std::less<TWeakReferenceable*>
> TWeakRefPool;


static std::mutex sWeakRefPoolLock;
static TWeakRefPool* spWeakRefPool = NULL;

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TWeakReferenceable::SInit()
{
  spWeakRefPool = new TWeakRefPool();
}

// -------------------------------------------------------------------------------------------------

void TWeakReferenceable::SExit()
{
  if (spWeakRefPool)
  {
    MAssert(spWeakRefPool->empty(), "Leaked weakref!");

    delete spWeakRefPool;
    spWeakRefPool = NULL;
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TWeakRefOwner::SOnWeakReferencableDying(
  TWeakReferenceable* pWeakReferenceable)
{
  MAssert(pWeakReferenceable, "Need a valid object");
  
  if (! spWeakRefPool)
  {
    MInvalid("Can not be called from static (de)initializers: "
      "must be called within 'main'.");

    return; // security hack for aborted processes
  }

  // lock to allow calls from multiple threads
  const std::lock_guard<std::mutex> Lock(sWeakRefPoolLock);

  if (pWeakReferenceable->mNumOfAttachedWeakRefOwners > 0)
  {
    TWeakRefPool::iterator PoolIter = spWeakRefPool->find(pWeakReferenceable);
    MAssert(PoolIter != spWeakRefPool->end(), "Unknown weak referenceable");
    
    // call "OnWeakReferenceDied" for all owners
    TWeakRefOwnerSet& OwnerSet = PoolIter->second;
    
    #if defined(MDebug)
      const size_t __PrevOwnerSetSize = OwnerSet.size();
    #endif
    
    for (TWeakRefOwnerSet::const_iterator Iter = OwnerSet.begin(); 
           Iter != OwnerSet.end(); ++Iter)
    {
      (*Iter)->OnWeakReferenceDied();
      --pWeakReferenceable->mNumOfAttachedWeakRefOwners;

      MAssert(OwnerSet.size() == __PrevOwnerSetSize, 
        "OnWeakReferenceDied() should never add new or remove existing "
        "weakrefs! Can't get them cleaned up properly then.")
    }

    MAssert(pWeakReferenceable->mNumOfAttachedWeakRefOwners == 0, 
      "Owner list and weak count not in sync");

    // and remove the weakref from the pool
    spWeakRefPool->erase(PoolIter);
  } 
  else
  {
    MAssert(spWeakRefPool->find(pWeakReferenceable) == spWeakRefPool->end(), 
      "Owner list and weak count not in sync");
  }
}

// -------------------------------------------------------------------------------------------------

void TWeakRefOwner::SAddWeakReference(
  TWeakReferenceable* pWeakReferenceable, 
  TWeakRefOwner*      pOwner)
{
  MAssert(pOwner, "Must specify a valid owner");
  MAssert(pWeakReferenceable, "Must specify a valid object");

  if (! spWeakRefPool)
  {
    MInvalid("Can not be called from static (de)initializers: "
      "must be called within 'main'.");

    return; // security hack for aborted processes
  }

  // lock to allow calls from multiple threads
  const std::lock_guard<std::mutex> Lock(sWeakRefPoolLock);

  TWeakRefPool::iterator PoolIter = spWeakRefPool->find(pWeakReferenceable);
  if (PoolIter == spWeakRefPool->end())
  {
    PoolIter = spWeakRefPool->insert(
      std::make_pair(pWeakReferenceable, TWeakRefOwnerSet())).first;
  
    MAssert(pWeakReferenceable->mNumOfAttachedWeakRefOwners == 0, 
      "Expected to have no owners with no weak map entry.");
  }
  
  TWeakRefOwnerSet& OwnerSet = PoolIter->second;

  #if defined(MDebug)
    MAssert(OwnerSet.insert(pOwner).second, "Owner was already added");
  #else
    OwnerSet.insert(pOwner);
  #endif    
  
  ++pWeakReferenceable->mNumOfAttachedWeakRefOwners;
  
  MAssert(pWeakReferenceable->mNumOfAttachedWeakRefOwners == 
    (int)OwnerSet.size(), "Owner list and weak count not in sync");
}

// -------------------------------------------------------------------------------------------------

void TWeakRefOwner::SSubWeakReference(
  TWeakReferenceable* pWeakReferenceable, 
  TWeakRefOwner*      pOwner)
{
  MAssert(pOwner, "Must specify a valid owner");
  MAssert(pWeakReferenceable, "Must specify a valid object");

  if (! spWeakRefPool)
  {
    MInvalid("Can not be called from static (de)initializers: "
      "must be called within 'main'.");

    return; // security hack for aborted processes
  }

  // lock to allow calls from multiple threads
  const std::lock_guard<std::mutex> Lock(sWeakRefPoolLock);

  TWeakRefPool::iterator PoolIter = spWeakRefPool->find(pWeakReferenceable);
  MAssert(PoolIter != spWeakRefPool->end(), "Unknown weak referenceable");
  
  TWeakRefOwnerSet& OwnerSet = PoolIter->second;
  
  #if defined(MDebug)
    MAssert(OwnerSet.erase(pOwner), "Owner was not added");
  #else
    OwnerSet.erase(pOwner);
  #endif

  if (OwnerSet.empty()) 
  {
    // delete the pool entry when the owner set is empty
    spWeakRefPool->erase(PoolIter);
  }

  --pWeakReferenceable->mNumOfAttachedWeakRefOwners;

  MAssert(pWeakReferenceable->mNumOfAttachedWeakRefOwners >= 0, 
    "Owner list and weak count not in sync");
}

// -------------------------------------------------------------------------------------------------

void TWeakRefOwner::SMoveWeakReference(
  TWeakReferenceable* pWeakReferenceable, 
  TWeakRefOwner*      pOldOwner,
  TWeakRefOwner*      pNewOwner)
{
  MAssert(pOldOwner && pNewOwner && pOldOwner != pNewOwner, 
    "Must specify valid new owners");
  
  MAssert(pWeakReferenceable, "Must specify a valid object");

  if (! spWeakRefPool)
  {
    MInvalid("Can not be called from static (de)initializers: "
      "must be called within 'main'.");

    return; // security hack for aborted processes
  }

  // lock to allow calls from multiple threads
  const std::lock_guard<std::mutex> Lock(sWeakRefPoolLock);

  TWeakRefPool::iterator PoolIter = spWeakRefPool->find(pWeakReferenceable);
  MAssert(PoolIter != spWeakRefPool->end(), "Unknown weak referenceable");

  TWeakRefOwnerSet& OwnerSet = PoolIter->second;
  
  #if defined(MDebug)
    MAssert(OwnerSet.erase(pOldOwner), "pOldOwner was not added");
    MAssert(OwnerSet.insert(pNewOwner).second, "pNewOwner was already added");
  #else
    OwnerSet.erase(pOldOwner);
    OwnerSet.insert(pNewOwner);
  #endif    
}

