#include "CoreTypesPrecompiledHeader.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

int TListHelper::SExpandListSize(size_t SizeOfT, int CurrentSize)
{
  // do not preallocate anything for big object lists
  if (SizeOfT >= 100)
  {
    return CurrentSize + 1;
  }
  
  // else, try to find a good compromise between speed and memory waste
  if (CurrentSize >= 10000)
  {
    return (int)((float)CurrentSize*1.05f);
  }
  else if (CurrentSize >= 1000)
  {
    return (int)((float)CurrentSize*1.15f);
  }
  else if (CurrentSize >= 100)
  {
    return (int)((float)CurrentSize*1.25f);
  }
  else if (CurrentSize >= 10)
  {
    return (int)((float)CurrentSize*1.5f);
  }
  else if (CurrentSize >= 1)
  {
    return CurrentSize + 2;
  }
  else // (CurrentSize == 0)
  {
    return CurrentSize + 1;
  }
}

