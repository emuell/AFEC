#pragma once

#ifndef _Pair_inl_
#define _Pair_inl_

// =================================================================================================

#include "CoreTypes/Export/Debug.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T1, typename T2>
TPair<T1, T2>::TPair()
  : mFirst(), mSecond()
{
}

template <typename T1, typename T2>
TPair<T1, T2>::TPair(const T1& First, const T2& Second)
  : mFirst(First), mSecond(Second)
{
}

// -------------------------------------------------------------------------------------------------

template <typename T1, typename T2>
const T1& TPair<T1, T2>::First()const
{
  return mFirst;
}

template <typename T1, typename T2>
T1& TPair<T1, T2>::First()
{
  return mFirst;
}

// -------------------------------------------------------------------------------------------------

template <typename T1, typename T2>
const T2& TPair<T1, T2>::Second()const
{
  return mSecond;
}

template <typename T1, typename T2>
T2& TPair<T1, T2>::Second()
{
  return mSecond;
}

// -------------------------------------------------------------------------------------------------

template <typename T1, typename T2>
void TPair<T1, T2>::SetFirst(const T1& First)
{
  mFirst = First;
}

// -------------------------------------------------------------------------------------------------

template <typename T1, typename T2>
void TPair<T1, T2>::SetSecond(const T2& Second)
{
  mSecond = Second;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T1, typename T2>
TPair<T1, T2> MakePair(const T1& First, const T2& Second)
{
  return TPair<T1, T2>(First, Second);
}

// -------------------------------------------------------------------------------------------------

template <typename T1, typename T2>
bool operator ==(const TPair<T1, T2>& First, const TPair<T1, T2>& Second)
{
  return First.First() == Second.First() && First.Second() == Second.Second();
}

// -------------------------------------------------------------------------------------------------

template <typename T1, typename T2>
bool operator !=(const TPair<T1, T2>& First, const TPair<T1, T2>& Second)
{
  return !operator==(First, Second);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T1, typename T2>
TString ToString(const TPair<T1, T2>& Pair)
{
  return ToString(Pair.First()) + ":" + ToString(Pair.Second());
}

// -------------------------------------------------------------------------------------------------

template <typename T1, typename T2>
bool StringToValue(TPair<T1, T2>& Pair, const TString& String)
{
  if (String.CountChar(':') != 1)
  {
    return false;
  }

  TString FirstString, SecondString;
  int PosInString = 0;

  while (PosInString < String.Size() && String[PosInString] != ':')
  {
    FirstString.AppendChar(String[PosInString++]);
  }

  ++PosInString;

  while (PosInString < String.Size())
  {
    SecondString.AppendChar(String[PosInString++]);
  }
  
  T1 First;
  T2 Second;

  if (StringToValue(First, FirstString) && 
      StringToValue(Second, SecondString))
  {
    Pair.SetFirst(First);
    Pair.SetSecond(Second);
    return true;
  }

  return false;  
}


#endif

