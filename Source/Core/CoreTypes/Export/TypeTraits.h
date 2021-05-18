#pragma once

#ifndef _TypeTraits_h_
#define _TypeTraits_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

// =================================================================================================

/*!
 * Collection of helper struct to gather type traits at compile time.
 *
 * All structs also define a operator bool() so they can be used in runtime
 * checks too, silencing "condition always evaluates to true/false" warnings.
!*/

// =================================================================================================

//! TIsSameType 

template <typename T1, typename T2> struct TIsSameType { 
  static const bool sValue = false; 
  operator bool()const { return sValue; }
};

template <typename T> struct TIsSameType<T, T> { 
  static const bool sValue = true; 
  operator bool()const { return sValue; }
};

// =================================================================================================

//! TIsBaseClassOf 

#if defined(MCompiler_Has_TypeTraitsIntrinsics)

  //! Can use the compiler's type trait intrinsics
  template <typename BaseClass, typename Class>
  struct TIsBaseClassOf {
    static const bool sValue = __is_base_of(BaseClass, Class);
    inline operator bool() const { return sValue; }
  };

#else

  //! TIsBaseClassOf test which will work on most compilers which do not support
  //! type trait intrinsics. Impl shamelessly stolen from boost type traits.
  template <typename BaseClass, typename Class>
  struct TIsBaseClassOf
  {
  private:
    struct THost {
      operator BaseClass*() const;
      operator Class*();
    };

    typedef char (&TYes)[1];
    typedef char (&TNo)[2];

    template <typename T>
    static TYes SCheck(Class*, T);
    static TNo SCheck(BaseClass*, int);

  public:
    static const bool sValue =
      sizeof(SCheck(THost(), int())) == sizeof(TYes);
   
    inline operator bool() const { return sValue; }
  };

#endif

// =================================================================================================

//! TIsBaseType

template <typename T> struct TIsBaseType { 
  static const bool sValue = false; 
  inline operator bool() const { return sValue; }
};

template <> struct TIsBaseType<bool> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<char> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<unsigned char> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<short> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<unsigned short> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<int> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<unsigned int> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<long> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<unsigned long> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<long long> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<unsigned long long> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<float> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<double> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template <> struct TIsBaseType<long double> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};

// =================================================================================================

//! TIsFloatType

template<typename T> struct TIsFloatType { 
  static const bool sValue = false; 
  inline operator bool() const { return sValue; }
};

template<> struct TIsFloatType<float> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template<> struct TIsFloatType<double> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};
template<> struct TIsFloatType<long double> { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};

// =================================================================================================

//! TIsPointer

template <class T> struct TIsPointer { 
  static const bool sValue = false; 
  inline operator bool() const { return sValue; }
};

template <class T> struct TIsPointer<T*> {
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};

// =================================================================================================

//! TIsReference

template <class T> struct TIsReference { 
  static const bool sValue = false; 
  inline operator bool() const { return sValue; }
};

template <class T> struct TIsReference<T&> {
  static const bool sValue = true; 
  inline operator bool() const { return sValue; } 
};

// =================================================================================================

//! TIsArray

template <typename T> struct TIsArray { 
  static const bool sValue = false; 
  inline operator bool() const { return sValue; } 
};

template <typename T> struct TIsBaseTypeArray { 
  static const bool sValue = false; 
  inline operator bool() const { return sValue; } 
};

// specialized in Array.h

// =================================================================================================

//! TIsList

template <typename T> struct TIsList { 
  static const bool sValue = false; 
  inline operator bool() const { return sValue; }
};

template <typename T> struct TIsBaseTypeList { 
  static const bool sValue = false; 
  inline operator bool() const { return sValue; }
};

// specialized in List.h

// =================================================================================================

//! TIsPair

template <typename T> struct TIsPair { 
  static const bool sValue = false; 
  inline operator bool() const { return sValue; }
};

template <typename T> struct TIsBaseTypePair { 
  static const bool sValue = false; 
  inline operator bool() const { return sValue; }
};

// specialized in Pair.h


#endif // _TypeTraits_h_

