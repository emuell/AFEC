#pragma once

#ifndef _Point_h_
#define _Point_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"

// =================================================================================================

/*!
 * Template base class for a point. Avoid using the template directly. 
 * See TPoint, TInt/Float/Double/Point typedefs for specializations. 
!*/

template <typename T>
class TPointT
{
public:
  //! Create and empty point.
  TPointT();
  //! Create a point wit the given coordinates.
  TPointT(T PosX, T PosY);
  
  //! Allow explicit conversion a point with another base type.
  template <typename Y>
  explicit TPointT(const TPointT<Y>& Other)
    : mX((T)Other.mX), mY((T)Other.mY) { }

  //! Add/subtract a point value to a point value.
  TPointT& operator+= (const TPointT& Other);
  TPointT& operator-= (const TPointT& Other);

  //! Scale (when used as size) or transform (when used as position) 
  //! the point by the given factor.
  TPointT& operator*= (float Factor);
  TPointT& operator*= (double Factor);
  TPointT& operator/= (float Factor);
  TPointT& operator/= (double Factor);

  T mX;
  T mY;
};

// =================================================================================================

//! Integer point types
typedef TPointT<int> TPoint; // "default" point
typedef TPointT<int> TIntPoint; // explcitly int

//! Floating point types
typedef TPointT<float> TFloatPoint;
typedef TPointT<double> TDoublePoint;

// =================================================================================================

template <typename T>
bool operator== (const TPointT<T>& First, const TPointT<T>& Second);

template <typename T>
bool operator!= (const TPointT<T>& First, const TPointT<T>& Second);

template <typename T>
TPointT<T> operator+ (const TPointT<T>& First, const TPointT<T>& Second);

template <typename T>
TPointT<T> operator- (const TPointT<T>& First, const TPointT<T>& Second);

// -------------------------------------------------------------------------------------------------

template <typename T>
TPointT<T> operator* (const TPointT<T>& Point, float Factor);

template <typename T>
TPointT<T> operator* (const TPointT<T>& Point, double Factor);

template <typename T>
TPointT<T> operator/ (const TPointT<T>& Point, float Factor);

template <typename T>
TPointT<T> operator/ (const TPointT<T>& Point, double Factor);

// -------------------------------------------------------------------------------------------------

template <typename T>
TString ToString(const TPointT<T>& Point);

template <typename T>
bool StringToValue(TPointT<T>& Point, const TString &String);

// =================================================================================================

//! Binary function for sorting integer points in sorted associative containers

struct TPointSortCompare {  
  bool operator()(const TIntPoint& Left, const TIntPoint& Right)const;
};

// =================================================================================================

#include "CoreTypes/Export/Point.inl"


#endif // _Point_h_

