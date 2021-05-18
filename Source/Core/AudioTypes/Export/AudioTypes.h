#pragma once

#ifndef _AudioTypes_h_
#define _AudioTypes_h_

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/ByteOrder.h"
#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/Str.h"

template <typename TEnumType> struct SEnumStrings;

// =================================================================================================

typedef float TSampleType; 

#define M16BitSampleMin (-(2 << 14))
#define M16BitSampleMax ((2 << 14) - 1)
#define M16BitSampleRange (2 << 15)

// =================================================================================================

namespace TAudioTypes
{
  // ===============================================================================================

  enum TSampleInterpolationMode
  {
    kNoInterpolation,
    kInterpolateLinear,
    kInterpolateCubic,
    kInterpolateSinc,

    kNumberOfSampleInterpolationModes
  };

  // ===============================================================================================

  enum TSampleVolumeRampMode
  {
    kRampNormal,
    kRampFast,
    kRampVeryFast,
    kRampSuperFast,

    kNumberOfSampleVolumeRampModes
  };
  
  // ===============================================================================================

  enum TLoopMode
  {
    kNoLoop,
    kLoopForward,
    kLoopBackward,
    kLoopPingPong,
    
    kNumberOfLoopModes
  };
}

// =================================================================================================

/**
 * Represents a Floating number, with `mFirst` as the integer part
 * and `mLast` as the fractional part.
 *
 * This involves a resolution (biggest possible stored number in mLast).
 * biggest number on 32 unsigned bit can be 2^32 = 4294967296 = `M2Raised32`
 *
 * to write a double (3.14) to SamplePosition
 * ```
 *  mAbsolut = (unit64_t)(3.14 * M2Raised32);
 * ```
 * or
 * ```
 * mHalf.mFirst = 3;
 * mHalf.mLast = (uint32_t)(0.14 * M2Raised32); 
 * ```
 */

union TSamplePosition
{
  struct
  {
    #if (MSystemByteOrder == MMotorolaByteOrder)
      TUInt32 mFirst;
      TUInt32 mLast;
    #else
      TUInt32 mLast;
      TUInt32 mFirst;
    #endif
  }
  mHalf;

  TUInt64 mAbsolut;
};

// =================================================================================================

template <> struct SEnumStrings<TAudioTypes::TSampleInterpolationMode>
{
  operator TList<TString>()const
  {
    MStaticAssert(TAudioTypes::kNumberOfSampleInterpolationModes == 4);

    TList<TString> Ret;
    Ret.Append("None");
    Ret.Append("Linear");
    Ret.Append("Cubic");
    Ret.Append("Sinc");

    return Ret;
  }
};

// =================================================================================================

template <> struct SEnumStrings<TAudioTypes::TLoopMode>
{
  operator TList<TString>()const
  {
    MStaticAssert(TAudioTypes::kNumberOfLoopModes == 4);

    TList<TString> Ret;
    Ret.Append("Off");
    Ret.Append("Forward");
    Ret.Append("Backward");
    Ret.Append("PingPong");

    return Ret;
  }
};


#endif // _AudioTypes_h_

