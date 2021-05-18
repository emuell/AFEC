#pragma once

#ifndef _Range_h_
#define _Range_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"

// =================================================================================================

template <typename T>
class TRange
{
public:
  TRange();
  TRange(T Min, T Max);
  TRange(const TRange& Other);

  bool IsSet()const;
  
  bool Contains(T Value)const;
  bool Intersects(const TRange& Other)const;

  const T Min()const;
  void SetMin(T Min); 

  const T Max()const;
  void SetMax(T Max); 

  const T Diff()const;
  
  TRange<T>& Empty();
  TRange<T>& Set(T Min, T Max);
  TRange<T>& Shift(T Value);
  
private:
  T mMin;
  T mMax;
};

// =================================================================================================

template <typename T>
bool operator ==(const TRange<T>& First, const TRange<T>& Second);

template <typename T>
bool operator !=(const TRange<T>& First, const TRange<T>& Second);
  
// =================================================================================================

template <typename T>
TString ToString(const TRange<T>& Range);

template <typename T>
bool StringToValue(TRange<T>& Range, const TString& String);

// =================================================================================================

#include "CoreTypes/Export/Range.inl"


#endif // _Range_h_

