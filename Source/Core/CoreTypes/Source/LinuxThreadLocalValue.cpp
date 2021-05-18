#include "CoreTypesPrecompiledHeader.h"

#include <pthread.h>

#include <climits> // PTHREAD_KEYS_MAX

// =================================================================================================

static const long kInvalidPThreadKey = PTHREAD_KEYS_MAX + 1;

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TThreadLocalValueSlot::TThreadLocalValueSlot()
  : mKey(kInvalidPThreadKey)
{
  MStaticAssert(sizeof(pthread_key_t) == sizeof(mKey));
  
  if (::pthread_key_create((pthread_key_t*)&mKey, NULL) != 0)
  {
    throw TReadableException(MText("Fatal Error: Failed to allocate "
      "a thread local storage slot!"));
  }
}

// -------------------------------------------------------------------------------------------------

TThreadLocalValueSlot::~TThreadLocalValueSlot()
{
  if (mKey != kInvalidPThreadKey)
  {
    if (::pthread_key_delete((pthread_key_t)mKey) != 0)
    {
      ::perror("pthread_key_delete failed");
      MInvalid("pthread_key_delete failed");
    }
  }
}

// -------------------------------------------------------------------------------------------------

void* TThreadLocalValueSlot::Value()const
{
  return ::pthread_getspecific((pthread_key_t)mKey);
}

// -------------------------------------------------------------------------------------------------

void TThreadLocalValueSlot::SetValue(void* pValue)
{
  MAssert(mKey != kInvalidPThreadKey, "");

  if (::pthread_setspecific((pthread_key_t)mKey, pValue) != 0)
  {
    ::perror("pthread_setspecific failed");
    MInvalid("pthread_setspecific failed");
  }
}

