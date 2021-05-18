#pragma once

#ifndef _Cast_h_
#define _Cast_h_

// =================================================================================================

#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/CompilerDefines.h"

template <typename T> class TPtr;
template <typename T> class TWeakPtr;
template <typename T> class TOwnerPtr;

// =================================================================================================

// -------------------------------------------------------------------------------------------------

/*!
 * Debug checked static_(down)casts. Allows NULL ptrs, but asserts on failed
 * dynamic_casts in debug builds: The result of static_cast is undefined in 
 * such cases, and will very likely cause a crash afterwards.
!*/

template <class Target, class Source>
MForceInline Target Cast(Source* pObj)
{
  MAssert(MImplies(pObj != NULL,
    dynamic_cast<Target>(pObj) != NULL), "Cast failed!");
  
  return static_cast<Target>(pObj);
}

// -------------------------------------------------------------------------------------------------

//! Help compiler deducing parameters when source is wrapped into a smart ptr: 

template <class Target, class Source> 
MForceInline Target Cast(const TPtr<Source>& Ptr)
{
  return Cast<Target, Source>(Ptr.Object());
}

template <class Target, class Source>
MForceInline Target Cast(const TWeakPtr<Source>& Ptr)
{
  return Cast<Target, Source>(Ptr.Object());
}

template <class Target, class Source>
MForceInline Target Cast(const TOwnerPtr<Source>& Ptr)
{
  return Cast<Target, Source>(Ptr.Object());
}

// -------------------------------------------------------------------------------------------------

/*!
 * Even milder than static_cast. Usage is identical to static_cast<>
!*/

template <class OutputClass, class InputClass>
MForceInline OutputClass implicit_cast(InputClass Object)
{
  return Object;
}

// -------------------------------------------------------------------------------------------------

/*!
 * Truly evil cast! It completely subverts C++'s type system, allowing you 
 * to cast from any class to any other class. 
!*/

template <class OutputClass, class InputClass>
MForceInline OutputClass horrible_cast(const InputClass Object)
{
  union 
  {
    OutputClass out;
    InputClass in;
  } u;
  
  // Cause a compile-time error if in, out and u are not the same size.
  // If the compile fails here, it means the compiler has peculiar
  // unions which would prevent the cast from working.
  MStaticAssert(sizeof(InputClass) == sizeof(u) && 
    sizeof(InputClass) == sizeof(OutputClass)); 
  
  u.in = Object;
  return u.out;
}

// -------------------------------------------------------------------------------------------------

/*!
 * Workaround for GCC's strict aliasing warning.
 * Tells GCC that a reinterpreted cast !will! alias.
!*/

template<typename TOutputType, typename TInputType>
#if defined(MCompiler_GCC)
  MForceInline TOutputType __attribute__((__may_alias__)) aliased_cast(TInputType Object)
#else
  MForceInline TOutputType aliased_cast(TInputType Object)
#endif
{
  return reinterpret_cast<TOutputType>(Object);
}


#endif // _Cast_h_

