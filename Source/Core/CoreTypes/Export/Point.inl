#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/TypeTraits.h"
#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/Str.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------
      
template <typename T>
inline TPointT<T>::TPointT(T X, T Y)
  : mX(X), mY(Y)
{
}

template <typename T>
inline TPointT<T>::TPointT()
  : mX(), mY()
{
}

// -------------------------------------------------------------------------------------------------

template <typename T>
inline TPointT<T>& TPointT<T>::operator+= (const TPointT<T>& Other)
{
  mX += Other.mX;
  mY += Other.mY;
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
inline TPointT<T>& TPointT<T>::operator-= (const TPointT<T>& Other)
{
  mX -= Other.mX;
  mY -= Other.mY;
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
inline TPointT<T>& TPointT<T>::operator*= (float Factor)
{
  mX = (T)(mX * Factor);
  mY = (T)(mY * Factor);

  return *this;
}

template <typename T>
inline TPointT<T>& TPointT<T>::operator*= (double Factor)
{
  mX = (T)(mX * Factor);
  mY = (T)(mY * Factor);

  return *this;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
inline TPointT<T>& TPointT<T>::operator/= (float Factor)
{
  return operator*= (1.0f / Factor);
}

template <typename T>
inline TPointT<T>& TPointT<T>::operator/= (double Factor)
{
  return operator*= (1.0 / Factor);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T>
inline bool operator== (const TPointT<T>& First, const TPointT<T>& Second)
{
  return (First.mX == Second.mX && First.mY == Second.mY);
}

template <typename T>
inline bool operator!= (const TPointT<T>& First, const TPointT<T>& Second)
{
  return (First.mX != Second.mX || First.mY != Second.mY);
}

// -------------------------------------------------------------------------------------------------

template <typename T>
inline TPointT<T> operator+ (const TPointT<T>& First, const TPointT<T>& Second)
{
  return TPointT<T>(First.mX + Second.mX, First.mY + Second.mY);
}

template <typename T>
inline TPointT<T> operator- (const TPointT<T>& First, const TPointT<T>& Second)
{
  return TPointT<T>(First.mX - Second.mX, First.mY - Second.mY);
}

// -------------------------------------------------------------------------------------------------

template <typename T>
inline TPointT<T> operator* (const TPointT<T>& Point, float Factor)
{
  return TPointT<T>(Point) *= Factor;
}

template <typename T>
inline TPointT<T> operator* (const TPointT<T>& Point, double Factor)
{
  return TPointT<T>(Point) *= Factor;
}

template <typename T>
inline TPointT<T> operator/ (const TPointT<T>& Point, float Factor)
{
  return TPointT<T>(Point) /= Factor;
}

template <typename T>
inline TPointT<T> operator/ (const TPointT<T>& Point, double Factor)
{
  return TPointT<T>(Point) /= Factor;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
TString ToString(const TPointT<T>& Point)
{
  return ToString(Point.mX) + "," + ToString(Point.mY);
}

// -------------------------------------------------------------------------------------------------

template <typename T>
bool StringToValue(TPointT<T>& Point, const TString &String)
{
  const TList<TString> Splits = String.SplitAt(',');

  if (Splits.Size() != 2)
  {
    MInvalid("Invalid source string for a TPoint");
    return false;
  }

  TPointT<T> Temp;

  if (!StringToValue(Temp.mX, Splits[0]) || 
      !StringToValue(Temp.mY, Splits[1]))
  {
    return false;
  }

  Point = Temp;
  return true;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

inline bool TPointSortCompare::operator()(
  const TIntPoint& Left, 
  const TIntPoint& Right)const
{ 
  MAssert(
    Left.mX >= -32768 && Left.mX <= 32767 && 
    Left.mY >= -32768 && Left.mY <= 32767 &&
    Right.mX >= -32768 && Right.mX <= 32767 && 
    Right.mY >= -32768 && Right.mY <= 32767, 
    "Can only sort 16 bit signed point values");

  return (((TInt16)Left.mY << 16) | (TInt16)Left.mX) <
    (((TInt16)Right.mY << 16) | (TInt16)Right.mX);
}

