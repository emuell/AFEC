#pragma once

#ifndef _Range_inl_
#define _Range_inl_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"
#include "CoreTypes/Export/Debug.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T>
TRange<T>::TRange()
  : mMin(), mMax() 
{ 
}

template <typename T>
TRange<T>::TRange(T Min, T Max)
  : mMin(Min), mMax(Max) 
{
  MAssert(mMax >= mMin, "Invalid Range");
}

template <typename T>
TRange<T>::TRange(const TRange& Other)
  : mMin(Other.mMin), mMax(Other.mMax) 
{
}

// -------------------------------------------------------------------------------------------------

template <typename T>
bool TRange<T>::IsSet()const 
{
  return mMin != T() || mMax != T();
}

// -------------------------------------------------------------------------------------------------

template <typename T>
bool TRange<T>::Contains(T Value)const 
{
  return Value >= mMin && Value <= mMax;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
bool TRange<T>::Intersects(const TRange& Other)const 
{
  return Other.mMax >= mMin && Other.mMin <= mMax;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
inline const T TRange<T>::Diff()const 
{
  return mMax - mMin;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
inline const T TRange<T>::Min()const 
{
  return mMin;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
inline const T TRange<T>::Max()const 
{
  return mMax;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
TRange<T>& TRange<T>::Empty()
{
  mMin = mMax = T();
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
TRange<T>& TRange<T>::Set(T Min, T Max) 
{
  mMin = Min;
  mMax = Max;

  MAssert(mMax >= mMin, "Invalid Range");
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
void TRange<T>::SetMin(T Min) 
{
  mMin = Min;
  MAssert(mMax >= mMin, "Invalid Range");
}

// -------------------------------------------------------------------------------------------------

template <typename T>
void TRange<T>::SetMax(T Max) 
{
  mMax = Max;
  MAssert(mMax >= mMin, "Invalid Range");
}

// -------------------------------------------------------------------------------------------------

template <typename T>
TRange<T>& TRange<T>::Shift(T Value)
{
  mMin += Value;
  mMax += Value;
  
  return *this;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T>
bool operator ==(const TRange<T>& First, const TRange<T>& Second)
{
  return First.Min() == Second.Min() && First.Max() == Second.Max();
}

// -------------------------------------------------------------------------------------------------

template <typename T>
bool operator !=(const TRange<T>& First, const TRange<T>& Second)
{
  return !operator==(First, Second);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T>
TString ToString(const TRange<T>& Range)
{
  return ToString(Range.Min()) + ":" + ToString(Range.Max());
}

// -------------------------------------------------------------------------------------------------

template <typename T>
bool StringToValue(TRange<T>& Range, const TString& String)
{
  const TList<TString> Splits = String.SplitAt(':');

  if (Splits.Size() != 2)
  {
    MInvalid("Invalid source string for a Range!");
    return false;
  }
  
  T Min = T(), Max = T();
  
  if (!StringToValue(Min, Splits[0]) || 
      !StringToValue(Max, Splits[1]))
  {
    return false;
  }
  
  Range.Set(Min, Max);
  return true;  
}


#endif // _Range_inl_

