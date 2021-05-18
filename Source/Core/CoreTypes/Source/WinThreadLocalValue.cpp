#include "CoreTypesPrecompiledHeader.h"

#include <windows.h>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TThreadLocalValueSlot::TThreadLocalValueSlot()
  : mKey((intptr_t)::TlsAlloc())
{
  if ((DWORD)mKey == TLS_OUT_OF_INDEXES)
  {
    throw TReadableException(MText("Failed to allocate a thread local storage slot!"));
  }
}

// -------------------------------------------------------------------------------------------------

TThreadLocalValueSlot::~TThreadLocalValueSlot()
{
  if ((DWORD)mKey != TLS_OUT_OF_INDEXES)
  {
    ::TlsFree(mKey);
  }
}

// -------------------------------------------------------------------------------------------------

void* TThreadLocalValueSlot::Value()const
{
  return ::TlsGetValue((DWORD)mKey);
}

// -------------------------------------------------------------------------------------------------

void TThreadLocalValueSlot::SetValue(void* pValue)
{
  ::TlsSetValue((DWORD)mKey, pValue);
}
                      
