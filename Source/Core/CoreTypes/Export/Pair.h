#pragma once

#ifndef _Pair_h_
#define _Pair_h_

// =================================================================================================

#include "CoreTypes/Export/TypeTraits.h"
#include "CoreTypes/Export/Str.h"

// =================================================================================================

/*!
 *  A std::pair like structure that allows you to carry around two
 *  two different types, without having to define a new type
!*/
 
template <typename T1, typename T2 = T1>
class TPair
{
public:
  TPair();
  TPair(const T1& First, const T2& Second);

  const T1& First()const;
  T1& First();

  const T2& Second()const;
  T2& Second();
  
  void SetFirst(const T1& First); 
  void SetSecond(const T2& Second); 

private:
  T1 mFirst;
  T2 mSecond;
};

// =================================================================================================

template <typename T1, typename T2> struct TIsPair< TPair<T1, T2> > { 
  static const bool sValue = true; 
  inline operator bool() const { return sValue; }
};

template <typename T1, typename T2>
struct TIsBaseTypePair< TPair<T1, T2> > { 
  static const bool sValue = 
    (TIsBaseType<T1>::sValue && TIsBaseType<T2>::sValue);
  inline operator bool() const { return sValue; }
};

// =================================================================================================

template <typename T1, typename T2>
TPair<T1, T2> MakePair(const T1& First, const T2& Second);

template <typename T1, typename T2>
bool operator ==(const TPair<T1, T2>& First, const TPair<T1, T2>& Second);

template <typename T1, typename T2>
bool operator !=(const TPair<T1, T2>& First, const TPair<T1, T2>& Second);
  
// =================================================================================================

template <typename T1, typename T2>
TString ToString(const TPair<T1, T2>& Pair);

template <typename T1, typename T2>
bool StringToValue(TPair<T1, T2>& Pair, const TString& String);

// =================================================================================================

#include "CoreTypes/Export/Pair.inl"


#endif // _Pair_h_

