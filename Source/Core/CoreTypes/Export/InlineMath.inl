#pragma once

#ifndef _InlineMath_inl_
#define _InlineMath_inl_

// =================================================================================================

#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/ByteOrder.h"
#include "CoreTypes/Export/CompilerDefines.h"

#include <cmath>
#include <cfloat>
#include <cstdlib> // for rand

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T> 
MForceInline T MMin(const T& a, const T& b)
{
  return (a < b) ? a : b;
}

MForceInline int MMin(int a, int b) 
{
  return (b + ((a - b) & ((a - b) >> (sizeof(int) * 8 - 1))));
}

MForceInline long MMin(long a, long b) 
{ 
  return (b + ((a - b) & ((a - b) >> (sizeof(long) * 8 - 1))));
}

MForceInline long long MMin(long long a, long long b) 
{ 
  return (b + ((a - b) & ((a - b) >> (sizeof(long long) * 8 - 1))));
}

// -------------------------------------------------------------------------------------------------

template <typename T> 
MForceInline T MMax(const T& a, const T& b)
{
  return (a > b) ? a : b;
}

MForceInline int MMax(int a, int b) 
{
  return (a - ((a - b) & ((a - b) >> (sizeof(int) * 8 - 1))));
}

MForceInline long MMax(long a, long b) 
{
  return (a - ((a - b) & ((a - b) >> (sizeof(long) * 8 - 1))));
}

MForceInline long long MMax(long long a, long long b) 
{
  return (a - ((a - b) & ((a - b) >> (sizeof(long long) * 8 - 1))));
}

// -------------------------------------------------------------------------------------------------

template <typename T> 
MForceInline T MClip(const T& Value, const T& Min, const T& Max) 
{
  MAssert(Max >= Min, "Invalid Min/Max parameters");
  return (Value > Max) ? Max : (Value < Min) ? Min : Value;
}

MForceInline int MClip(int Value, int Min, int Max)
{
  return MMin(MMax(Value, Min), Max);
}

MForceInline long MClip(long Value, long Min, long Max)
{
  return MMin(MMax(Value, Min), Max);
}

MForceInline long long MClip(long long Value, long long Min, long long Max)
{
  return MMin(MMax(Value, Min), Max);
}

// -------------------------------------------------------------------------------------------------

template<typename T> 
MForceInline bool MIsEqual(const T &a,const T &b)
{
  return (a == b);
}

MForceInline bool MIsEqual(float a, float b)
{
  // Special instantiation for floats because floats can and should never be 
  // directly compared without a quantum. When comparing them directly, the 
  // compiler may also generate code which compares truncated values in RAM with
  // untruncated values from floating-point register or vice versa. 
  // By comparing the floating point values bit by bit we can avoid that.

  #if defined(MArch_X86) || defined(MArch_X64) // assuming Intel

    MStaticAssert(sizeof(float) == sizeof(TInt32));
    union { float f; TInt32 i; } ua = { a };
    union { float f; TInt32 i; } ub = { b };

    return (ua.i == ub.i);

  #else
    return (a == b);
    
  #endif  
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline void TMathT<T>::Swap(T& a, T& b)
{
  const T Temp(a);
  a = b;
  b = Temp;
}

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline T TMathT<T>::Square(T Value)
{
  return Value * Value;
}

// -------------------------------------------------------------------------------------------------

template<typename T>
T TMathT<T>::Average(const T* pBuffer, int Size)
{
  T Sum = (T)0;

  for (int i = 0; i < Size; i++)
  {
    Sum += pBuffer[i];
  }

  return (T)((double)Sum / (double)Size);
}

// -------------------------------------------------------------------------------------------------

template<> MForceInline float TMathT<float>::Sign(float Float)
{
  MStaticAssert(sizeof(float) == sizeof(TInt32));
  union { float f; TInt32 i; } u = { Float };
  
  return 1.0f + (float)(((u.i) >> 31) << 1);
}

template<> MForceInline double TMathT<double>::Sign(double Double)
{
  MStaticAssert(sizeof(double) == sizeof(TInt64));
  union { double d; TInt64 i; } u = { Double };
  
  return 1.0 + (double)(((u.i) >> 63LL) << 1LL);
}

template<typename T>
MForceInline T TMathT<T>::Sign(T Value)
{
  return (Value < (T)0) ? (T)-1 : (T)1;
}

// -------------------------------------------------------------------------------------------------

template<> MForceInline float TMathT<float>::Abs(float Float)
{ 
  return ::fabsf(Float); // single instruction as intrinsic
}

template<> MForceInline double TMathT<double>::Abs(double Double)
{
  return ::fabs(Double); // single instruction as intrinsic
}

template<> MForceInline int TMathT<int>::Abs(int Integer)
{ 
  return ::abs(Integer); // single instruction as intrinsic
}

template<typename T>
MForceInline T TMathT<T>::Abs(T Entry)
{
  return (Entry < (T)0) ? -Entry : Entry;
}

// -------------------------------------------------------------------------------------------------

template<typename T>
MForceInline void TMathT<T>::Clip(
  T&        Value, 
  const T&  Min, 
  const T&  Max)
{
  Value = MClip(Value, Min, Max);
}

// -------------------------------------------------------------------------------------------------

template<typename T>
void TMathT<T>::GetMinMax(
  T&        Min, 
  T&        Max, 
  int       StartPosition, 
  int       Count, 
  const T*  pBufferBlock)
{
  if (Count == 0)
  {
    return;
  }
  
  const T* pBuffer = pBufferBlock + StartPosition;

  T MinResult = *pBuffer;
  T MaxResult = *pBuffer;
  
  if (Count > 1)
  {
    // ... treat First pair separately (only one comparison for First two elements)
  
    int First = 0;
    int Second = 1;
    int PotentialMinIndex = -1;
    
    if (pBuffer[First] < pBuffer[Second])
    {
      MaxResult = pBuffer[Second];
    }
    else 
    {
      MinResult = pBuffer[Second];
      PotentialMinIndex = First;
    }


    // ... then each element by pairs, with at most 3 comparisons per pair
    
    First = 2;
    Second = 3; 
    
    while (Second < Count)
    {
      if (pBuffer[First] < pBuffer[Second]) 
      {
        if (pBuffer[First] < MinResult) 
        {
          MinResult = pBuffer[First];
          PotentialMinIndex = -1;
        }
       
        if (MaxResult < pBuffer[Second])
        {
          MaxResult = pBuffer[Second];
        }
      } 
      else 
      {
        if (pBuffer[Second] < MinResult) 
        {
          MinResult = pBuffer[Second];
          PotentialMinIndex = First;
        }
        
        if (MaxResult < pBuffer[First])
        {
          MaxResult = pBuffer[First];
        }
      }
      
      First += 2;
      Second += 2;
    }

    // ... if odd number of elements, treat last element
    
    if ((Count&1) == 1) 
    { 
      // odd number of elements
      if (pBuffer[Count - 1] < MinResult)
      {
        MinResult = pBuffer[Count - 1];
        PotentialMinIndex = -1;
      }
      else if (MaxResult < pBuffer[Count - 1])
      {
        MaxResult = pBuffer[Count - 1];
      }
    }


    // ... resolve MinResult being incorrect with one extra comparison
    //     (in which case potential_min_result is necessarily the correct result)
    
    if (PotentialMinIndex != -1 && pBuffer[PotentialMinIndex] < MinResult)
    {
      MinResult = pBuffer[PotentialMinIndex];
    }
  }
  
  
  // ... the final comparison with the arguments
  
  if (MinResult < Min)
  {
    Min = MinResult;
  }
  
  if (MaxResult > Max)
  {
    Max = MaxResult;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T>
void TMathT<T>::GetMin(
  T&        Min, 
  int       StartPosition, 
  int       Count, 
  const T*  pBufferBlock)
{
  const T *pBuffer = pBufferBlock + StartPosition;

  while (Count--)
  {
    if (Min > *pBuffer)
    {
      Min = *pBuffer;
    }

    ++pBuffer;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T>
void TMathT<T>::GetMax(
  T&        Max, 
  int       StartPosition, 
  int       Count, 
  const T*  pBufferBlock)
{
  const T *pBuffer = pBufferBlock + StartPosition;

  while (Count--)
  {
    if (Max < *pBuffer)
    {
      Max = *pBuffer;
    }

    ++pBuffer;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T>
T TMathT<T>::NearestWrappedDistance(T Position1, T Position2, T WrapAmount)
{
  MAssert(Position1 >= 0 && Position1 < WrapAmount, 
    "Position must be within the wrap amount");

  MAssert(Position2 >= 0 && Position2 < WrapAmount, 
    "Position must be within the wrap amount");

  const T Wrapped1 = (Position2 + WrapAmount) - Position1;
  const T Wrapped2 = Position2 - (Position1 + WrapAmount);
  const T UnWrapped = Position2 - Position1;

  if (TMathT<T>::Abs(Wrapped1) < TMathT<T>::Abs(Wrapped2) && 
      TMathT<T>::Abs(Wrapped1) < TMathT<T>::Abs(UnWrapped))
  {
    return Wrapped1;
  }
  else if (TMathT<T>::Abs(Wrapped2) < TMathT<T>::Abs(Wrapped1) && 
    TMathT<T>::Abs(Wrapped2) < TMathT<T>::Abs(UnWrapped))
  {
    return Wrapped2;
  }
  else
  {
    return UnWrapped;
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T>
T TMathT<T>::WrapIntoRange(T Value, T RangeStart, T RangeEnd)
{
  const T RangeLength = RangeEnd - RangeStart;
  MAssert(RangeLength > 0, "");

  while (Value < RangeStart)
  {
    Value += RangeLength;
  }

  while (Value >= RangeEnd)
  {
    Value -= RangeLength;
  }

  return Value;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

inline int TMath::Gcd(int a, int b) 
{
  if (b == 0) 
  {
    return a;
  }
  
  return TMathT<int>::Abs(Gcd(b, (a % b)));
}

// -------------------------------------------------------------------------------------------------

MForceInline int TMath::NextPow2(int Value)
{
  MAssert(Value >= 0, "Expected a positive value");

  if (Value & (Value - 1))
  {
    int PrevN;

    do
    {
      PrevN = Value;
    }
    while (Value &= Value - 1);

    return (int)(PrevN * 2);
  }
  else
  {
    return Value;
  }
}

// -------------------------------------------------------------------------------------------------

MForceInline int TMath::Pow2Order(int Pow2Value)
{
  MAssert(TMath::IsPowerOfTwo(Pow2Value), "Expected a pow 2 value");

  int Order = 0;
  while (Pow2Value >>= 1)
  {
    ++Order;
  }

  return Order;
}

// -------------------------------------------------------------------------------------------------

MForceInline bool TMath::IsPowerOfTwo(int Value)
{
  return (Value) && !(Value & (Value - 1));
}

// -------------------------------------------------------------------------------------------------

MForceInline double TMath::DegreesToRadians(double Degrees)
{
  return Degrees * MPi / 180.0;
}

MForceInline float TMath::DegreesToRadians(float Degrees)
{
  return (float)(Degrees * MPi / 180.0);
}

// -------------------------------------------------------------------------------------------------

MForceInline double TMath::RadiansToDegrees(double Radians)
{
  return Radians * 180.0 / MPi;
}

MForceInline float TMath::RadiansToDegrees(float Radians)
{
  return (float)(Radians * 180.0 / MPi);
}

// -------------------------------------------------------------------------------------------------

MForceInline float TMath::FastExp(float p) 
{
  return FastPow2(1.442695040f * p);
}

// -------------------------------------------------------------------------------------------------

MForceInline float TMath::FastPow(float x, float p) 
{
  return FastPow2(p * FastLog2(x));
}

// -------------------------------------------------------------------------------------------------

MForceInline float TMath::FastPow2(float p)
{
  float offset = (p < 0) ? 1.0f : 0.0f;
  float clipp = (p < -126) ? -126.0f : p;
  int w = f2i(clipp);
  float z = clipp - w + offset;
  
  MStaticAssert(sizeof(float) == sizeof(TInt32));
  
  union { TUInt32 i; float f; } v = { 
    (TUInt32) ( (1 << 23) * (clipp + 121.2740575f + 27.7280233f / 
      (4.84252568f - z) - 1.49012907f * z) ) 
  };

  return v.f;
}

// -------------------------------------------------------------------------------------------------

MForceInline float TMath::FastLog(float x)
{
  return 0.69314718f * FastLog2(x);
}

// -------------------------------------------------------------------------------------------------

MForceInline float TMath::FastLog2(float x)
{
  MStaticAssert(sizeof(float) == sizeof(TInt32));

  union { float f; TUInt32 i; } vx = { x };
  union { TUInt32 i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
  
  float y = (float)vx.i;
  y *= 1.1920928955078125e-7f;

  return y - 124.22551499f - 1.498030302f * mx.f - 
    1.72587999f / (0.3520887068f + mx.f);
}

// -------------------------------------------------------------------------------------------------

MForceInline void TMath::Quantize(int& Value, int Step, TQuantizeMode Mode)
{
  double Dbl = (double)Value;
  Quantize(Dbl, (double)Step, Mode);
  Value = d2i(Dbl);
}

MForceInline void TMath::Quantize(float& Value, float Step, TQuantizeMode Mode)
{
  double Dbl = (double)Value;
  Quantize(Dbl, (double)Step, Mode);
  Value = (float)Dbl;
}

MForceInline void TMath::Quantize(double &Value, double Step, TQuantizeMode Mode)
{
  MAssert(Step >= 0.0f, "Invalid quantize step");

  if (Step != 0.0)
  {
    switch (Mode)
    {
    default: MInvalid("");

    case kChop:
      Value = (double)((double)d2i(Value / Step) * Step);
      break;

    case kRoundUp:
    {
      const double TruncatedValue = (double)((double)d2i(Value / Step) * Step);

      if (TruncatedValue < Value)
      {
        Value = TruncatedValue + Step;
      }
      else
      {
        Value = TruncatedValue;
      }
    }
    break;

    case kRoundDown:
    {
      const double TruncatedValue = (double)((double)d2i(Value / Step) * Step);

      if (Value < 0.0 && Value < TruncatedValue)
      {
        Value = TruncatedValue - Step;
      }
      else
      {
        Value = TruncatedValue;
      }
    }
    break;

    case kRoundToNearest:  
      if (Value > 0.0)
      {
        Value += Step / 2.0;
      }
      else
      {
        Value -= Step / 2.0;
      }

      Value = (double)((double)d2i(Value / Step) * Step);
      break;
    }
  }
}

// -------------------------------------------------------------------------------------------------

MForceInline bool TMath::FloatsAreEqual(float Value1, float Value2, float Step)
{
  const float Diff = Value2 - Value1;

  return !(Diff < -Step || Diff > Step);
}

// -------------------------------------------------------------------------------------------------

MForceInline bool TMath::FloatsAreEqual(double Value1, double Value2, double Step)
{
  const double Diff = Value2 - Value1;

  return !(Diff < -Step || Diff > Step);
}

// -------------------------------------------------------------------------------------------------

MForceInline bool TMath::IsNaN(float Value)
{
  #if defined(MCompiler_VisualCPP)
    return !! ::_isnan(Value);
    
  #elif defined(MCompiler_GCC)
    return std::isnan(Value);
    
  #else
    #error "Unknown Compiler"
  #endif
}

MForceInline bool TMath::IsNaN(double Value)
{
  #if defined(MCompiler_VisualCPP)
    return !! ::_isnan(Value);
    
  #elif defined(MCompiler_GCC)
    return std::isnan(Value);
    
  #else
    #error "Unknown Compiler"
  #endif
}
  
// -------------------------------------------------------------------------------------------------

MForceInline bool TMath::IsInf(float Value)
{
  #if defined(MCompiler_VisualCPP)
    return ! ::_finite(Value);
    
  #elif defined(MCompiler_GCC)
    return std::isinf(Value);
    
  #else
    #error "Unknown Compiler"
  #endif
}

MForceInline bool TMath::IsInf(double Value)
{
  #if defined(MCompiler_VisualCPP)
    return ! ::_finite(Value);
    
  #elif defined(MCompiler_GCC)
    return std::isinf(Value);
    
  #else
    #error "Unknown Compiler"
  #endif
}

// -------------------------------------------------------------------------------------------------

MForceInline int TMath::f2i(float f)
{
  #if defined(MCompiler_VisualCPP)
    
    #if defined(MArch_X86)
      __asm cvttss2si eax,f

    #elif defined(MArch_X64)
      // compiler will always use cvttss2si here
      return (int)f;
    
    #else
      #error "Unknown Architecture"
    #endif

  #elif defined(MCompiler_GCC)
    
    #if defined(MArch_X86)
      int i; 
      __asm__ ( "cvttss2si %1, %0" : "=r" (i) : "m" (f) ); 
      return i;
  
    #elif defined(MArch_X64)
      // compiler will always use cvttss2si here
      return (int)f;

    #elif defined(MArch_PPC)
      // PPC cast is fast enough...
      return (int)f;
    
    #else
      #error "Unknown Architecture"
    #endif
  
  #else
    #error "Unknown Compiler"
  #endif
}

// -------------------------------------------------------------------------------------------------

MForceInline int TMath::f2iRound(float flt)
{
  return f2i(flt + TMathT<float>::Sign(flt) * 0.5f);
}

// -------------------------------------------------------------------------------------------------

MForceInline int TMath::f2iRoundUp(float flt)
{
  return f2i(::ceilf(flt));
}

// -------------------------------------------------------------------------------------------------

MForceInline int TMath::f2iRoundDown(float flt)
{
  return f2i(::floorf(flt));
}

// -------------------------------------------------------------------------------------------------

MForceInline int TMath::d2i(double dbl)
{
  // Note: f2i((float)dbl) would use the current rounding 
  // mode to cast to the float, which is not what we want...

  #if defined(MCompiler_VisualCPP)
    
    #if defined(MArch_X86)
      __asm cvttsd2si eax,dbl

    #elif defined(MArch_X64)
      // compiler will always use cvttsd2si here
      return (int)dbl;
    
    #else
      #error "Unknown Architecture"
    #endif

  #elif defined(MCompiler_GCC)
    
    #if defined(MArch_X86)
      int i; 
      __asm__ ( "cvttsd2si %1, %0" : "=r" (i) : "m" (dbl) ); 
      return i;
  
    #elif defined(MArch_X64)
      // compiler will always use cvttss2si here
      return (int)dbl;

    #elif defined(MArch_PPC)
      // PPC cast is fast enough...
      return (int)dbl;
    
    #else
      #error "Unknown Architecture"
    #endif
  
  #else
    #error "Unknown Compiler"
  #endif
}

// -------------------------------------------------------------------------------------------------

MForceInline int TMath::d2iRound(double dbl)
{
  return d2i(dbl + TMathT<double>::Sign(dbl) * 0.5);
}

// -------------------------------------------------------------------------------------------------

MForceInline int TMath::d2iRoundUp(double dbl)
{
  return d2i(::ceil(dbl));
}

// -------------------------------------------------------------------------------------------------

MForceInline int TMath::d2iRoundDown(double dbl)
{
  return d2i(::floor(dbl));
}

// -------------------------------------------------------------------------------------------------

MForceInline int TMath::Rand()
{
  sRandState = (sRandState * 1103515245 + 12345) & 0x7fffffff;
  return sRandState;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TMath::RandFloat()
{
  static const double sScaleFactor = (1.0 / kRandMax);
  return Rand() * sScaleFactor;
}

// -------------------------------------------------------------------------------------------------

MForceInline short TMath::Dither(float flt)
{
  return (short)f2i((flt + (float)RandFloat() - 0.5f));
}

// -------------------------------------------------------------------------------------------------

MForceInline float TMath::InterpolateLinear(float y0, float y1, float x)
{
  MAssert(x >= 0.0f && x <= 1.0f, "Invalid fraction");
  return y0 + (y1 - y0) * x;
}

MForceInline double TMath::InterpolateLinear(double y0, double y1, double x)
{
  MAssert(x >= 0.0 && x <= 1.0, "Invalid fraction");
  return y0 + (y1 - y0) * x;
}

// -------------------------------------------------------------------------------------------------

MForceInline float TMath::InterpolateCubicBSpline(
  float ym1,
  float y0,
  float y1,
  float y2,
  float x)
{
  MAssert(x >= 0.0 && x <= 1.0, "Invalid fraction");

  const double ym1py1 = ym1 + y1;
  const double c0 = 1.0 / 6.0 * ym1py1 + 2.0 / 3.0 * y0;
  const double c1 = 1.0 / 2.0 * (y1 - ym1);
  const double c2 = 1.0 / 2.0 * ym1py1 - y0;
  const double c3 = 1.0 / 2.0 * (y0 - y1) + 1.0 / 6.0 * (y2 - ym1);

  double y = ((c3 * x + c2) * x + c1) * x + c0;
  MUnDenormalize(y);

  return (float)y;
}

MForceInline double TMath::InterpolateCubicBSpline(
  double ym1,
  double y0,
  double y1,
  double y2,
  double x)
{
  MAssert(x >= 0.0 && x <= 1.0, "Invalid fraction");

  const double ym1py1 = ym1 + y1;
  const double c0 = 1.0 / 6.0 * ym1py1 + 2.0 / 3.0 * y0;
  const double c1 = 1.0 / 2.0 * (y1 - ym1);
  const double c2 = 1.0 / 2.0 * ym1py1 - y0;
  const double c3 = 1.0 / 2.0 * (y0 - y1) + 1.0 / 6.0 * (y2 - ym1);
  
  double y = ((c3 * x + c2) * x + c1) * x + c0;
  MUnDenormalize(y);

  return y;
}

// -------------------------------------------------------------------------------------------------

MForceInline float TMath::InterpolateCubicHermite(
  float ym1,
  float y0,
  float y1,
  float y2,
  float x)
{
  MAssert(x >= 0.0f && x <= 1.0f, "Invalid fraction");

  // 4-point, 3rd-order Hermite (x-form)
  float c = (y1 - ym1) * 0.5f;
  MUnDenormalize(c);

  const float v = y0 - y1;
  const float w = c + v;
  
  float a = w + v + (y2 - y0) * 0.5f;
  MUnDenormalize(a);

  const float b_neg = w + a;

  float y = ((((a * x) - b_neg) * x + c) * x + y0);
  MUnDenormalize(y);

  return y;
}

MForceInline double TMath::InterpolateCubicHermite(
  double ym1,
  double y0,
  double y1,
  double y2,
  double x)
{
  MAssert(x >= 0.0 && x <= 1.0, "Invalid fraction");

  // 4-point, 3rd-order Hermite (x-form)
  double c = (y1 - ym1) * 0.5;
  MUnDenormalize(c);

  const double v = y0 - y1;
  const double w = c + v;
  
  double a = w + v + (y2 - y0) * 0.5;
  MUnDenormalize(a);

  const double b_neg = w + a;

  double y = ((((a * x) - b_neg) * x + c) * x + y0);
  MUnDenormalize(y);

  return y;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline int TRandom::Integer(int MaxValue)
{
  MAssert(MaxValue > 0, "Invalid max value");
  return ::rand() % MaxValue;
}


#endif // _InlineMath_inl_

