#pragma once

#ifndef _SampleConverter_h_
#define _SampleConverter_h_

// =================================================================================================

#include "CoreTypes/Export/InlineMath.h"
#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/ByteOrder.h"

// =================================================================================================

namespace TSampleConverter
{
  // NOTE: 'Internal Float' in all conversions below is [-(2<<14) <> (2<<14) - 1] 
  // see also M16BitSampleMin, M16BitSampleMax in AudioTypes.h

  // ===============================================================================================

  // -----------------------------------------------------------------------------------------------

  // ... Internal Float -> 8bit signed [-128 <> 127]

  void InternalFloatToInt8BitSignedInterleaved(
    const float*  pSourceBufferLeft,
    const float*  pSourceBufferRight,
    TInt8*        pDestBuffer,
    int           NumberOfSampleFrames,
    bool          Dither);

  void InternalFloatToInt8BitSigned(
    const float*  pSourceBufferLeft,
    const float*  pSourceBufferRight,
    TInt8*        pDestBufferLeft,
    TInt8*        pDestBufferRight,
    int           NumberOfSampleFrames,
    bool          Dither);

  void InternalFloatToInt8BitSigned(
    const float*  pSourceBuffer,
    TInt8*        pDestBuffer,
    int           NumberOfSampleFrames,
    bool          Dither);
  
  
  // ... Internal Float -> 8bit unsigned [0 <> 256]

  void InternalFloatToInt8BitUnsignedInterleaved(
    const float*    pSourceBufferLeft,
    const float*    pSourceBufferRight,
    TUInt8*         pDestBuffer,
    int             NumberOfSampleFrames,
    bool            Dither);

  void InternalFloatToInt8BitUnsigned(
    const float*    pSourceBufferLeft,
    const float*    pSourceBufferRight,
    TUInt8*         pDestBufferLeft,
    TUInt8*         pDestBufferRight,
    int             NumberOfSampleFrames,
    bool            Dither);

  void InternalFloatToInt8BitUnsigned(
    const float*    pSourceBuffer,
    TUInt8*         pDestBuffer,
    int             NumberOfSampleFrames,
    bool            Dither);
          
  
  // ... Internal Float -> 16bit [-(2<<14) <> (2<<14) - 1]

  void InternalFloatToInt16BitInterleaved(
    const float*  pSourceBufferLeft,
    const float*  pSourceBufferRight,
    TInt16*       pDestBuffer,
    int           NumberOfSampleFrames,
    bool          Dither);

  void InternalFloatToInt16Bit(
    const float*  pSourceBufferLeft,
    const float*  pSourceBufferRight,
    TInt16*       pDestBufferLeft,
    TInt16*       pDestBufferRight,
    int           NumberOfSampleFrames,
    bool          Dither);

  void InternalFloatToInt16Bit(
    const float*  pSourceBuffer,
    TInt16*       pDestBuffer,
    int           NumberOfSampleFrames,
    bool          Dither);
  

  // ... Internal Float -> 24bit [-(2<<22) <> (2<<22) - 1]
 
  void InternalFloatToInt24BitInterleaved(
    const float*  pSourceBufferLeft,
    const float*  pSourceBufferRight,
    T24*          pDestBuffer,
    int           NumberOfSampleFrames,
    bool          Dither);

  void InternalFloatToInt24Bit(
    const float*  pSourceBufferLeft,
    const float*  pSourceBufferRight,
    T24*          pDestBufferLeft,
    T24*          pDestBufferRight,
    int           NumberOfSampleFrames,
    bool          Dither);

  void InternalFloatToInt24Bit(
    const float*  pSourceBuffer,
    T24*          pDestBuffer,
    int           NumberOfSampleFrames,
    bool          Dither);
  

  // ... Internal Float -> 32bit int [-(2<<30) <> (2<<30) - 1]
 
  void InternalFloatToInt32BitInterleaved(
    const float*  pSourceBufferLeft,
    const float*  pSourceBufferRight,
    TInt32*       pDestBuffer,
    int           NumberOfSampleFrames);

  void InternalFloatToInt32Bit(
    const float*  pSourceBufferLeft,
    const float*  pSourceBufferRight,
    TInt32*       pDestBufferLeft,
    TInt32*       pDestBufferRight,
    int           NumberOfSampleFrames);

  void InternalFloatToInt32Bit(
    const float*  pSourceBuffer,
    TInt32*       pDestBuffer,
    int           NumberOfSampleFrames);


  // ... Internal Float -> float [-1.0f <> 1.0f]
   
  void InternalFloatToFloat32(
    const float*  pSourceBuffer,
    float*        pBuffer,
    int           NumberOfSampleFrames);

  void InternalFloatToFloat32(
    const float*  pSourceBufferLeft,
    const float*  pSourceBufferRight,
    float*        pBufferLeft,
    float*        pBufferRight,
    int           NumberOfSampleFrames);
  
  void InternalFloatToFloat32Interleaved(
    const float*  pSourceBufferLeft, 
    const float*  pSourceBufferRight, 
    float*        pDestBuffer, 
    int           NumberOfSampleFrames);


  // ... Internal Float -> double [-1.0 <> 1.0]

  void InternalFloatToFloat64(
    const float*  pSourceBuffer,
    double*       pBuffer,
    int           NumberOfSampleFrames);

  void InternalFloatToFloat64(
    const float*  pSourceBufferLeft,
    const float*  pSourceBufferRight,
    double*       pBufferLeft,
    double*       pBufferRight,
    int           NumberOfSampleFrames);

  void InternalFloatToFloat64Interleaved(
    const float*  pSourceBufferLeft, 
    const float*  pSourceBufferRight, 
    double*       pDestBuffer, 
    int           NumberOfSampleFrames);

  // -----------------------------------------------------------------------------------------------

  // ... 8 Bit Signed [-128 <> 127] -> Internal Float 

  // mono -> mono
  void Int8BitSignedToInternalFloat(
    const TInt8* pSrcBuffer,
    float*        pDestBuffer,
    int           NumberOfSampleFrames);

  // stereo -> stereo
  void Int8BitSignedInterleavedToInternalFloat(
    const TInt8*  pInterleavedSrcBuffer,
    float*        pDestBufferLeft,
    float*        pDestBufferRight,
    int           NumberOfSampleFrames);

  // X channels -> X channels
  void Int8BitSignedInterleavedToInternalFloat(
    const TInt8*          pInterleavedSrcBuffer,
    const TArray<float*>& pDestBufferPointers,
    int                   NumberOfSampleFrames );

  // stereo -> mono
  void Int8BitSignedInterleavedStereoToInternalFloatMono(
    const TInt8*  pInterleavedSrcBuffer,
    float*        pDestBuffer,
    int           NumberOfSampleFrames);


  // ... 8 Bit Unsigned [0 <> 256] -> Internal Float

  // mono -> mono
  void Int8BitUnsignedToInternalFloat(
    const TUInt8*         pSrcBuffer,
    float*                pDestBuffer,
    int                   NumberOfSampleFrames);

  // stereo -> stereo
  void Int8BitUnsignedInterleavedToInternalFloat(
    const TUInt8*         pInterleavedSrcBuffer,
    float*                pDestBufferLeft,
    float*                pDestBufferRight,
    int                   NumberOfSampleFrames);

  // X channels -> X channels
  void Int8BitUnsignedInterleavedToInternalFloat(
    const TUInt8*         pInterleavedSrcBuffer,
    const TArray<float*>& pDestBufferPointers,
    int                   NumberOfSampleFrames);

  // stereo -> mono
  void Int8BitUnsignedInterleavedStereoToInternalFloatMono(
    const TUInt8*         pInterleavedSrcBuffer,
    float*                pDestBuffer,
    int                   NumberOfSampleFrames);


  // ... 16 Bit signed [-(2<<14) <> (2<<14) - 1] -> Internal Float

  // mono -> mono
  void Int16BitToInternalFloat(
    const TInt16* pSrcBuffer,
    float*        pDestBuffer,
    int           NumberOfSampleFrames);
  
  // stereo -> stereo
  void Int16BitInterleavedToInternalFloat(
    const TInt16* pInterleavedSrcBuffer,
    float*        pDestBufferLeft,
    float*        pDestBufferRight,
    int           NumberOfSampleFrames);

  // X channels -> X channels
  void Int16BitInterleavedToInternalFloat(
    const TInt16*         pInterleavedSrcBuffer,
    const TArray<float*>& pDestBufferPointers,
    int                   NumberOfSampleFrames );

  // stereo -> mono
  void Int16BitInterleavedStereoToInternalFloatMono(
    const TInt16* pInterleavedSrcBuffer,
    float*        pDestBuffer,
    int           NumberOfSampleFrames);


  // ... 24Bit [-(2<<22) <> (2<<22) - 1] -> Internal Float

  // mono -> mono
  void Int24BitToInternalFloat(
    const T24*  pSrcBuffer,
    float*      pDestBuffer,
    int         NumberOfSampleFrames);
  
  // stereo -> stereo
  void Int24BitInterleavedToInternalFloat(
    const T24*  pInterleavedSrcBuffer,
    float*      pDestBufferLeft,
    float*      pDestBufferRight,
    int         NumberOfSampleFrames);
  
  // X channel -> X channel
  void Int24BitInterleavedToInternalFloat(
    const T24*            pInterleavedSrcBuffer,
    const TArray<float*>& pDestBufferPointers,
    int                   NumberOfSampleFrames );
  
  // stereo -> mono
  void Int24BitInterleavedStereoToInternalFloatMono(
    const T24*  pInterleavedSrcBuffer,
    float*      pDestBuffer,
    int         NumberOfSampleFrames);


  // ... 32 Bit Int [-(2<<30) <> (2<<30) - 1] -> Internal Float

  // mono -> mono
  void Int32BitToInternalFloat(
    const TInt32* pSrcBuffer,
    float*        pDestBuffer,
    int           NumberOfSampleFrames);

  // stereo -> stereo
  void Int32BitInterleavedToInternalFloat(
    const TInt32* pInterleavedSrcBuffer,
    float*        pDestBufferLeft,
    float*        pDestBufferRight,
    int           NumberOfSampleFrames);

  // X channels -> X channels
  void Int32BitInterleavedToInternalFloat(
    const TInt32*         pInterleavedSrcBuffer,
    const TArray<float*>& pDestBufferPointers,
    int                   NumberOfSampleFrames );

  // stereo -> mono
  void Int32BitInterleavedStereoToInternalFloatMono(
    const TInt32* pInterleavedSrcBuffer,
    float*        pDestBuffer,
    int           NumberOfSampleFrames);


  // ... 32 Bit Float [-1.0f <> 1.0f] -> Internal Float

  // mono -> mono
  void Float32BitToInternalFloat(
    const float*  pSrcBuffer,
    float*        pDestBuffer,
    int           NumberOfSampleFrames);

  // stereo -> stereo
  void Float32BitInterleavedToInternalFloat(
    const float*  pInterleavedSrcBuffer,
    float*        pDestBufferLeft,
    float*        pDestBufferRight,
    int           NumberOfSampleFrames);

  // X channels -> X channels
  void Float32BitInterleavedToInternalFloat(
    const float*          pInterleavedSrcBuffer,
    const TArray<float*>& pDestBufferPointers,
    int                   NumberOfSampleFrames );

  // stereo -> mono
  void Float32BitInterleavedStereoToInternalFloatMono(
    const float*  pInterleavedSrcBuffer,
    float*        pDestBuffer,
    int           NumberOfSampleFrames);

  
  // ... 64 Bit Float (double) [-1.0 <> 1.0] -> Internal Float

  // mono -> mono
  void Float64BitToInternalFloat(
    const double* pSrcBuffer,
    float*        pDestBuffer,
    int           NumberOfSampleFrames);

  // stereo -> stereo
  void Float64BitInterleavedToInternalFloat(
    const double* pInterleavedSrcBuffer,
    float*        pDestBufferLeft,
    float*        pDestBufferRight,
    int           NumberOfSampleFrames);

  // X channels -> X channels
  void Float64BitInterleavedToInternalFloat(
    const double*         pInterleavedSrcBuffer,
    const TArray<float*>& pDestBufferPointers,
    int                   NumberOfSampleFrames );
  
  // stereo -> mono
  void Float64BitInterleavedStereoToInternalFloatMono(
    const double* pInterleavedSrcBuffer,
    float*        pDestBuffer,
    int           NumberOfSampleFrames);

  // ===============================================================================================

  // ... 8 Bit Unsigned

  MForceInline TUInt8 S16BitFloatTo8BitUnsigned(float Value)
  {
    return (TUInt8)((TMath::f2i(Value) >> 8) + 128);
  }
  
  MForceInline TUInt8 S16BitFloatTo8BitUnsignedDither(float Value)
  {
    return (TUInt8)((TMath::Dither(Value) >> 8) + 128);
  }

  MForceInline float S8BitUnsignedTo16BitFloat(TUInt8 Value)
  {
    return (float)((int)(Value - 128) << 8);
  }
  
  
  // ... 8 Bit Signed
  
  MForceInline TInt8 S16BitFloatTo8BitSigned(float Value)
  {
    return (TInt8)(TMath::f2i(Value) >> 8);
  }
  
  MForceInline TInt8 S16BitFloatTo8BitSignedDither(float Value)
  {
    return (TInt8)(TMath::Dither(Value) >> 8);
  }
  
  MForceInline float S8BitSignedTo16BitFloat(TInt8 Value)
  {
    return (float)((int)Value << 8);
  }
  
  
  // ... 16 Bit Unsigned

  MForceInline TUInt16 S16BitFloatTo16BitUnsigned(float Value)
  {
    return (TUInt16)(TMath::f2i(Value) + (2<<14));
  }

  MForceInline TUInt16 S16BitFloatTo16BitUnsignedDither(float Value)
  {
    return (TUInt16)(TMath::Dither(Value) + (2<<14));
  }   

  MForceInline float S16BitUnsignedTo16BitFloat(TUInt16 Value)
  {
    return (float)((signed)Value - (2<<14));
  }


  // ... 16 Bit Signed
  
  MForceInline TInt16 S16BitFloatTo16BitSigned(float Value)
  {
    return (TInt16)TMath::f2i(Value);
  }
  
  MForceInline TInt16 S16BitFloatTo16BitSignedDither(float Value)
  {
    return (TInt16)TMath::Dither(Value);
  }   

  MForceInline float S16BitSignedTo16BitFloat(TInt16 Value)
  {
    return (float)Value;
  }
  
  
  // ... 24 Bit
  
  struct T24Pack {
    TInt8 mFirst, mSecond, mThird;
  };
  
  MForceInline T24Pack S16BitFloatTo24Bit(float Value)
  {
    double DoubleValue = (double)Value * 2147483648.0 / 32768.0;
    const TInt32 Int32 = (TInt32)MMax(-2147483648.0, MMin(2147483647.0, DoubleValue));
  
    const T24Pack Ret = {
      #if (MSystemByteOrder == MMotorolaByteOrder)
        (TInt8)(Int32 >> 24), (TInt8)(Int32 >> 16), (TInt8)(Int32 >> 8)
      #else
        (TInt8)(Int32 >> 8), (TInt8)(Int32 >> 16), (TInt8)(Int32 >> 24)
      #endif
    };

    return Ret;
  }
  
  MForceInline float S24BitTo16BitFloat(const T24Pack& Value)
  {
    const TInt32 Int32 = 
      #if (MSystemByteOrder == MMotorolaByteOrder)
        ((TUInt8)Value.mThird | 
        ((TUInt8)Value.mSecond << 8) | 
        ((TUInt8)Value.mFirst << 16)) << 8;
      #else
        ((TUInt8)Value.mFirst | 
        ((TUInt8)Value.mSecond << 8) | 
        ((TUInt8)Value.mThird << 16)) << 8;
      #endif
      
    return (float)(Int32 * 32768.0 / 2147483648.0);
  }
  
  
  // ... 32Bit Signed

  MForceInline TUInt32 S16BitFloatTo32BitUnsigned(float Value)
  {
    double DoubleValue = (double)(Value + 32768.0) * 2147483648.0 / 32768.0;
    return (TUInt32)MMax(0.0f, MMin(4294967296.0f, (float)DoubleValue));
  }

  MForceInline float S32BitUnsignedTo16BitFloat(TUInt32 Value)
  {
    double DoubleValue = (double)Value * 32768.0 / 2147483648.0 - 32768.0;
    return MMax(-32768.0f, MMin(32767.0f, (float)DoubleValue));
  }


  // ... 32Bit Signed
  
  MForceInline TInt32 S16BitFloatTo32BitSigned(float Value)
  {
    double DoubleValue = (double)Value * 2147483648.0 / 32768.0;
    return (TInt32)MMax(-2147483648.0, MMin(2147483647.0, DoubleValue));
  }

  MForceInline float S32BitSignedTo16BitFloat(TInt32 Value)
  {
    double DoubleValue = (double)Value * 32768.0 / 2147483648.0;
    return MMax(-32768.0f, MMin(32767.0f, (float)DoubleValue));
  }


  // ... Normalized Floats

  MForceInline double S16BitFloatTo0To1Float(float Value)
  {
    double DoubleValue = Value / 32768.0;
    return MMax(-1.0, MMin(1.0, DoubleValue));
  }

  MForceInline float S0To1FloatTo16BitFloat(double Value)
  {
    double DoubleValue = Value * 32768.0;
    return (float)MMax(-32768.0, MMin(32767.0, DoubleValue));
  }
}


#endif // _SampleConverter_h_

