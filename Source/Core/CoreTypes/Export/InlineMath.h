#pragma once

#ifndef _InlineMath_h_
#define _InlineMath_h_

// =================================================================================================

//@{ ... standard math consts

#define MPiDiv2 1.5707963267948966192313216916398 // PI/2
#define MInvPiDiv2 0.63661977236758134307553505349006 // 1.0/(PI/2)

#define MPi 3.1415926535897932384626433832795 // PI
#define MInvPi 0.31830988618379067153776752674503 // 1.0/PI

#define M2Pi 6.2831853071795864769252867665590 // 2*PI
#define MInv2Pi 0.15915494309189533576888376337251 // 1.0/(2*PI)

#define MLn2 0.69314718055994530941723212145818 // ln(2)
#define MLn10 2.3025850929940456840179914546844 // ln(10)

#define MLog10f2 3.3219280948873623478703194294894 // log10(2)
#define MInvLog10f2 3.3219280948873623478703194294894 // 1.0/log10(2)

#define M2Raised32 4294967296.0 // pow(2, 32)
//@}


//@{ ... denormal numbers

// !DEPRECATED - use an epsilon depending on the context instead!
#define MEpsilon 1e-12f 

//! default compare quantum to use with TMath::FloatsAreEqual. Choose 
//! appropriate values in your context when possible
#define MDefaultFloatCompareQuantum 0.000001f

#if defined(MArch_X86)
  //! add this to your float to avoid denormalized numbers
  #define MAntiDenormal 1.0E-25f

  //! fast way to undenormalize a float or double value
  #define MUnDenormalize(FloatValue) \
    FloatValue += 1.0E-18f; FloatValue -= 1.0E-18f;
    
#else
  //! noops on other architectures, cause only old x86 processors are
  //! vulnerable to denormalisation problems
  #define MAntiDenormal 0.0f
  #define MUnDenormalize(FloatValue)
  
#endif

// =================================================================================================

//! generic global min,max(a, b) templates
template <typename T> 
T MMin(const T& a, const T& b);
int MMin(int a, int b);
long MMin(long a, long b); 
long long MMin(long long a, long long b); 

template <typename T> 
T MMax(const T& a, const T& b);
int MMax(int a, int b);
long MMax(long a, long b); 
long long MMax(long long a, long long b); 

//! generic global clip template
template <typename T> 
T MClip(const T& Value, const T& Min, const T& Max);
int MClip(int Value, int Min, int Max);
long MClip(long Value, long Min, long Max); 
long long MClip(long long Value, long long Min, long long Max); 

//! Test two values for equality. 
//! Also compares floats byte per byte to avoid strange things that 
//! happen on Intel CPUs when comparing single precision FP values.
template<typename T> 
bool MIsEqual(const T& a,const T& b);
bool MIsEqual(float a, float b);

// =================================================================================================

/*!
 *  Fast basic math for floats and doubles
!*/ 

namespace TMath
{
  //! project internal init function
  void Init();

  // return greatest common divider of two numbers
  int Gcd(int a, int b);
  
  //! return true if value is a power of two (valid for 0 also)
  bool IsPowerOfTwo(int Value);
  //! return the next pow2 value, that param Value matches. 
  //! If value already is a pow2 value, value will be returned unchanged.
  int NextPow2(int Value);
  //! return the order of a pow2 value: log2(Pow2Value)
  int Pow2Order(int Pow2Value);

  //! Degrees <-> Radians conversion
  double DegreesToRadians(double Degrees);
  float DegreesToRadians(float Degrees);
  double RadiansToDegrees(double Radians);
  float RadiansToDegrees(float Radians);

  //! fast, fuzzy approximation of pow, pow2, log, log2 and exp with an error of ~1%. 
  //! NB: Negative input values will result in most cases in completely bad values.
  //! See http://www.machinedlearnings.com/2011/06/fast-approximate-logarithm-exponential.html
  float FastExp(float f);
  float FastPow(float x, float p);
  float FastPow2(float x);
  float FastLog(float x);
  float FastLog2(float x);

  //! fast, precise sin/cos/tan approximation (with 20 bits of precision).
  //! See http://www.musicdsp.org/archive.php?classid=0#115
  float FastSin(float x);
  double FastSin(double x);
  float FastCos(float x);
  double FastCos(double x);
  float FastTan(float x);
  double FastTan(double x);

  //! float quantization
  enum TQuantizeMode
  {
    kChop,  // towards zero (-0.5 ->  0, 0.5 -> 0)
    kRoundUp,   // to +infinity (-0.5 ->  0, 0.5 -> 1)
    kRoundDown, // to -infinity (-0.5 -> -1, 0.5 -> 0)
    kRoundToNearest, // nearest towards zero (-0.5 -> -1, 0.5 -> 1)

    kNumberOfQuantizeModes
  };

  void Quantize(int& Value, int Step, TQuantizeMode Mode = kChop);
  void Quantize(float& Value, float Step, TQuantizeMode Mode = kChop);
  void Quantize(double& Value, double Step, TQuantizeMode Mode = kChop);

  //! quantized float compare
  bool FloatsAreEqual(float Value1, float Value2, float Quantum);
  bool FloatsAreEqual(double Value1, double Value2, double Quantum);

  //! check if the float or double is a NAN (not a number)
  bool IsNaN(float Value);
  bool IsNaN(double Value);

  //! check if a float or double is an INF or -INF (infinite value)
  bool IsInf(float Value);
  bool IsInf(double Value);

  //! fast truncation of a float to an int, behaves exactly like inbuilt (int)flt
  int f2i(float flt);
  //! fast round to nearest towards zero -> (int)(flt + sign(flt)*0.5f)
  int f2iRound(float flt);
  //! fast upwards rounding for floats -> (int)ceil(flt)
  int f2iRoundUp(float flt);
  //! fast downwards rounding for floats -> (int)floor(flt)
  int f2iRoundDown(float flt);

  //! fast truncation of a double to an int, behaves exactly like inbuilt (int)dbl
  int d2i(double dbl);
  //! fast round to nearest towards zero -> (int)(dbl + sign(dbl)*0.5)
  int d2iRound(double dbl);
  //! fast upwards rounding -> (int)ceil(dbl)
  int d2iRoundUp(double dbl);
  //! fast downwards rounding -> (int)floor(dbl)
  int d2iRoundDown(double dbl);

  //! Min/Max values for our Rand() implemementation. Note that the std c rand()
  //! may have a different range!
  enum { 
    kRandMin = 0, 
    kRandMax = 0x7fffffff
  };
  
  //! State for \function RandFloat and \function Rand. 
  //! Change it to anything > 1 at any time to seed. 0x16BA2118 by default.
  extern int sRandState;

  //! Fast implementation of getting a random value between kRandMin and kRandMax.
  int Rand(); 
  //! Just like Rand() a fast implementation of getting a random value, but 
  //! unlike Rand() returning floats in the range (0.0f - 1.0f)
  double RandFloat();  
  
  //! simple dithering, based on RandFloat(). assuming 16bit Min/Max float input
  short Dither(float Input);

  //! Linear interpolation of 2 values
  float InterpolateLinear(float y0, float y1, float x);
  double InterpolateLinear(double y0, double y1, double x);

  //! BSpline interpolation with 4 values. Does not necessary match destination value y1.
  float InterpolateCubicBSpline(float ym1, float y0, float y1, float y2, float x);
  double InterpolateCubicBSpline(double ym1, double y0, double y1, double y2, double x);

  //! Hermite interpolation with 4 values. Matches destination value y1, unlike BSpline.
  float InterpolateCubicHermite(float ym1, float y0, float y1, float y2, float x);
  double InterpolateCubicHermite(double ym1, double y0, double y1, double y2, double x);
}

// =================================================================================================

/*!
 * Common basic template math functions, using fast, specialized 
 * non-branching implementations for floats where possible
!*/ 

template<typename T>
class TMathT
{
public:
  //! swap two values (of any type).
  static void Swap(T& a , T& b);
  
  //! clamp a value.
  static void Clip(T& Value, const T& Min, const T& Max);

  //! sign. Optimized for floats.
  static T Sign(T Entry);
  //! abs value. Optimized for floats.
  static T Abs(T entry);
  //! Value * Value.
  static T Square(T Value);

  //! sum of all buffer entries / Size.
  static T Average(const T* pBuffer, int Size);
  
  //! fast query of Min/Max within a buffer.
  static void GetMinMax(
    T&        Min, 
    T&        Max, 
    int       StartPosition, 
    int       Count, 
    const T*  pBufferBlock);
  static void GetMin(
    T&        Min, 
    int       StartPosition, 
    int       Count, 
    const T*  pBufferBlock);
  static void GetMax(
    T&        Max, 
    int       StartPosition, 
    int       Count, 
    const T*  pBufferBlock);

  // get the nearer T of both, after wrapping into the specified range
  static T NearestWrappedDistance(
    T Position1, 
    T Position2, 
    T WrapAmount);

  // wrap T into the given range, if needed
  static T WrapIntoRange(
    T Value, 
    T RangeStart, 
    T RangeEnd);
};

// =================================================================================================

namespace TRandom
{
  //! return a positive integer between 0 and MaxValue
  int Integer(int MaxValue);
}

// =================================================================================================

#include "CoreTypes/Export/InlineMath.inl"


#endif // _InlineMath_h_

