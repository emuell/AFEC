#pragma once

#ifndef _BaseTypes_h_
#define _BaseTypes_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

// NULL, size_t, ptrdiff_t, uintptr_t and intptr_t and wchar_t
#if defined(MCompiler_VisualCPP)
  // NB: no cstddef include available in Visual CPP
  #include <stddef.h>

#elif defined(MCompiler_GCC)
  // NB: no cstddef, cstdint includes available in older GCC versions
  #include <stddef.h> 
  #include <stdint.h>

#else
  error "Unknown compiler"
#endif

// =================================================================================================

//! Typedefs with defined sizes and signs, valid on any architecture 
//! we are compiling for. Asserted in Debug.cpp

typedef char TInt8;
typedef unsigned char TUInt8;

typedef short TInt16;
typedef unsigned short TUInt16;

typedef char T24[3];
typedef unsigned char TU24[3];

typedef int TInt32;
typedef unsigned int TUInt32;

typedef long long TInt64;
typedef unsigned long long TUInt64;

// -------------------------------------------------------------------------------------------------

//! Dummy, do nothing class, which can be used as a "nil" class
class TNullClass { }; 

// -------------------------------------------------------------------------------------------------

//! Get min/max of two integer values at compile type

template<int sValue1, int sValue2> 
struct TMin {
  enum { sValue = (sValue1 < sValue2) ? sValue1 : sValue2 };
  inline operator bool()const { return sValue; }
};

template<int sValue1, int sValue2> 
struct TMax {
  enum { sValue = (sValue1 > sValue2) ? sValue1 : sValue2 };
  inline operator bool()const { return sValue; }
};

// -------------------------------------------------------------------------------------------------

//! Check if a number is a pow2 number at compile time

template<int N>
struct TIsPowerOfTwo {
  static const bool sValue = N && !(N & (N - 1));
  inline operator bool() const { return sValue; }
};

// -------------------------------------------------------------------------------------------------

//! Get next pow2 number at compile time

template<int N>
struct TNextPowerOfTwo {
  
  template<int K, int S = 0>
  struct TCalculate {
    static const int sValue = TCalculate<K/2, S + 1>::sValue;
  };

  template<int S>
  struct TCalculate<0, S> {
    static const int sValue = S;
  };
 
  static const int sValue = (1 << TCalculate<N - 1>::sValue);
  inline operator int() const { return sValue; }
};

// -------------------------------------------------------------------------------------------------

//! alternative to the compilers offset_of
#define MOffsetOf(s, m) (size_t)(ptrdiff_t)&(((s*)0)->m)

// -------------------------------------------------------------------------------------------------

//! count number of elements in a C array - typesafe
#define MCountOf(x) (sizeof(x) / sizeof((x)[0]) + \
  0 * sizeof(reinterpret_cast<const ::BAD_COUNTOF_ARG*>(x)) + \
  0 * sizeof(::BAD_COUNTOF_ARG::CheckType((x), &(x))))     

// implementation details for MCountOf
class BAD_COUNTOF_ARG
{
public:
   class IS_POINTER; // intentionally incomplete
   class IS_ARRAY { };
   template <typename T>
   static IS_POINTER CheckType(const T*, const T* const*);
   static IS_ARRAY CheckType(const void*, const void*);
};

// -------------------------------------------------------------------------------------------------

//! useful for never ending for loops: for(MEver)
#define MEver ;;

// -------------------------------------------------------------------------------------------------

//! suppress compiler warning, when x is not used
#define MUnused(X) ((void)X)

// -------------------------------------------------------------------------------------------------

//! A -> B
#define MImplies(A, B) (!(A) || (B))

// -------------------------------------------------------------------------------------------------

//! Join the two arguments together, even when one of the arguments is itself a macro
#define MJoinMakros(A, B) MDoJoinMakros1(A, B)
#define MDoJoinMakros1(A, B) MDoJoinMakros2(A, B)
#define MDoJoinMakros2(A, B) A##B

// -------------------------------------------------------------------------------------------------

#if defined(MCompiler_Has_StaticAssert)
  #define MStaticAssert(Expression) \
    static_assert(Expression, "static assertion failure: '"#Expression"'")

#else
  //! compile time assertion workaround. Shamelessly stolen from boost.
  template<int T>  struct TStaticAssertionTest { };
  template<bool T> struct TStaticAssertionFailure; 
  template<> struct TStaticAssertionFailure<true> { };

  #define MStaticAssert(Expression) \
    typedef TStaticAssertionTest<sizeof(TStaticAssertionFailure<(bool)(Expression)>)> \
    MJoinMakros(TStaticAssertTypedef, __LINE__)

#endif


#endif // _BaseTypes_h_

