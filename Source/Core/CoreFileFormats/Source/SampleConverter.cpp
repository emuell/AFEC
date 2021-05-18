#include "CoreFileFormatsPrecompiledHeader.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt8BitSignedInterleaved(
  const float*  pSourceBufferLeft,
  const float*  pSourceBufferRight,
  TInt8*        pDestBuffer,
  int           NumberOfSampleFrames,
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  if (Dither)
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBuffer++ = S16BitFloatTo8BitSignedDither(pSourceBufferLeft[i]);
      *pDestBuffer++ = S16BitFloatTo8BitSignedDither(pSourceBufferRight[i]);
    }
  }
  else
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBuffer++ = S16BitFloatTo8BitSigned(pSourceBufferLeft[i]);
      *pDestBuffer++ = S16BitFloatTo8BitSigned(pSourceBufferRight[i]);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt8BitSigned(
  const float*  pSourceBufferLeft,
  const float*  pSourceBufferRight,
  TInt8*        pDestBufferLeft,
  TInt8*        pDestBufferRight,
  int           NumberOfSampleFrames,
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  if (Dither)
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBufferLeft++ = S16BitFloatTo8BitSignedDither(pSourceBufferLeft[i]);
      *pDestBufferRight++ = S16BitFloatTo8BitSignedDither(pSourceBufferRight[i]);
    }
  }
  else
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBufferLeft++ = S16BitFloatTo8BitSigned(pSourceBufferLeft[i]);
      *pDestBufferRight++ = S16BitFloatTo8BitSigned(pSourceBufferRight[i]);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt8BitSigned(
  const float*  pSourceBuffer,
  TInt8*        pDestBuffer,
  int           NumberOfSampleFrames,
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  if (Dither)
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBuffer++ = S16BitFloatTo8BitSignedDither(pSourceBuffer[i]);
    }
  }
  else
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBuffer++ = S16BitFloatTo8BitSigned(pSourceBuffer[i]);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt8BitUnsignedInterleaved(
  const float*  pSourceBufferLeft,
  const float*  pSourceBufferRight,
  TUInt8*       pDestBuffer,
  int           NumberOfSampleFrames,
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  if (Dither)
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {                  
      *pDestBuffer++ = S16BitFloatTo8BitUnsignedDither(pSourceBufferLeft[i]);
      *pDestBuffer++ = S16BitFloatTo8BitUnsignedDither(pSourceBufferRight[i]);
    }
  }
  else
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBuffer++ = S16BitFloatTo8BitUnsigned(pSourceBufferLeft[i]);
      *pDestBuffer++ = S16BitFloatTo8BitUnsigned(pSourceBufferRight[i]);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt8BitUnsigned(
  const float*  pSourceBufferLeft,
  const float*  pSourceBufferRight,
  TUInt8*       pDestBufferLeft,
  TUInt8*       pDestBufferRight,
  int           NumberOfSampleFrames,
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  if (Dither)
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBufferLeft++ = S16BitFloatTo8BitUnsignedDither(pSourceBufferLeft[i]);
      *pDestBufferRight++ = S16BitFloatTo8BitUnsignedDither(pSourceBufferRight[i]);
    }
  }
  else
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBufferLeft++ = S16BitFloatTo8BitUnsigned(pSourceBufferLeft[i]);
      *pDestBufferRight++ = S16BitFloatTo8BitUnsigned(pSourceBufferRight[i]);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt8BitUnsigned(
  const float*  pSourceBuffer,
  TUInt8*       pDestBuffer,
  int           NumberOfSampleFrames,
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  if (Dither)
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      pDestBuffer[i] = S16BitFloatTo8BitUnsignedDither(pSourceBuffer[i]);
    }
  }
  else
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      pDestBuffer[i] = S16BitFloatTo8BitUnsigned(pSourceBuffer[i]);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt16BitInterleaved(
  const float*  pSourceBufferLeft, 
  const float*  pSourceBufferRight, 
  TInt16*       pDestBuffer, 
  int           NumberOfSampleFrames, 
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  if (Dither)
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBuffer++ = S16BitFloatTo16BitSignedDither(pSourceBufferLeft[i]);
      *pDestBuffer++ = S16BitFloatTo16BitSignedDither(pSourceBufferRight[i]);
    }
  }
  else
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBuffer++ = S16BitFloatTo16BitSigned(pSourceBufferLeft[i]);
      *pDestBuffer++ = S16BitFloatTo16BitSigned(pSourceBufferRight[i]);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt16Bit(
  const float*  pSourceBufferLeft, 
  const float*  pSourceBufferRight, 
  TInt16*       pDestBufferLeft, 
  TInt16*       pDestBufferRight, 
  int           NumberOfSampleFrames, 
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  if (Dither)
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBufferRight++ = S16BitFloatTo16BitSignedDither(pSourceBufferRight[i]);
      *pDestBufferLeft++ = S16BitFloatTo16BitSignedDither(pSourceBufferLeft[i]);
    }
  }
  else
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      *pDestBufferRight++ = S16BitFloatTo16BitSigned(pSourceBufferRight[i]);
      *pDestBufferLeft++ = S16BitFloatTo16BitSigned(pSourceBufferLeft[i]);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt16Bit(
  const float*  pSourceBuffer,
  TInt16*       pDestBuffer,
  int           NumberOfSampleFrames,
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  if (Dither)
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      pDestBuffer[i] = S16BitFloatTo16BitSignedDither(pSourceBuffer[i]);
    }
  }
  else
  {
    for (int i = 0; i < NumberOfSampleFrames; ++i)
    {
      pDestBuffer[i] = S16BitFloatTo16BitSigned(pSourceBuffer[i]);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt24BitInterleaved(
  const float*  pSourceBufferLeft,
  const float*  pSourceBufferRight,
  T24*          pDestBuffer,
  int           NumberOfSampleFrames,
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  T24Pack *pT24Buffer = (T24Pack*)pDestBuffer;
    
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    *pT24Buffer++ = S16BitFloatTo24Bit(pSourceBufferLeft[i]);
    *pT24Buffer++ = S16BitFloatTo24Bit(pSourceBufferRight[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt24Bit(
  const float*  pSourceBufferLeft,
  const float*  pSourceBufferRight,
  T24*          pDestBufferLeft,
  T24*          pDestBufferRight,
  int           NumberOfSampleFrames,
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  T24Pack *pT24BufferLeft = (T24Pack*)pDestBufferLeft;
  T24Pack *pT24BufferRight = (T24Pack*)pDestBufferRight;
    
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pT24BufferLeft[i] = S16BitFloatTo24Bit(pSourceBufferLeft[i]);
    pT24BufferRight[i] = S16BitFloatTo24Bit(pSourceBufferRight[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt24Bit(
  const float*  pSourceBuffer,
  T24*          pDestBuffer,
  int           NumberOfSampleFrames,
  bool          Dither)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  T24Pack *pT24Buffer = (T24Pack*)pDestBuffer;
    
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pT24Buffer[i] = S16BitFloatTo24Bit(pSourceBuffer[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt32BitInterleaved(
  const float*  pSourceBufferLeft,
  const float*  pSourceBufferRight,
  TInt32*       pDestBuffer,
  int           NumberOfSampleFrames)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    *pDestBuffer++ = S16BitFloatTo32BitSigned(pSourceBufferLeft[i]);
    *pDestBuffer++ = S16BitFloatTo32BitSigned(pSourceBufferRight[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt32Bit(
  const float*  pSourceBuffer,
  TInt32*       pDestBuffer,
  int           NumberOfSampleFrames)
{
  MAssert(NumberOfSampleFrames >= 0, "");

  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBuffer[i] = S16BitFloatTo32BitSigned(pSourceBuffer[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToInt32Bit(
  const float*  pSourceBufferLeft,
  const float*  pSourceBufferRight,
  TInt32*       pDestBufferLeft,
  TInt32*       pDestBufferRight,
  int           NumberOfSampleFrames)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBufferLeft[i] = S16BitFloatTo32BitSigned(pSourceBufferLeft[i]);
    pDestBufferRight[i] = S16BitFloatTo32BitSigned(pSourceBufferRight[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToFloat32Interleaved(
  const float*  pSourceBufferLeft, 
  const float*  pSourceBufferRight, 
  float*        pDestBuffer, 
  int           NumberOfSampleFrames)
{
  MAssert(NumberOfSampleFrames >= 0, "");
  
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    *pDestBuffer++ = (float)S16BitFloatTo0To1Float(pSourceBufferLeft[i]);
    *pDestBuffer++ = (float)S16BitFloatTo0To1Float(pSourceBufferRight[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToFloat32(
  const float*  pSourceBufferLeft, 
  const float*  pSourceBufferRight, 
  float*        pBufferLeft, 
  float*        pBufferRight, 
  int           NumberOfSampleFrames)
{
  MAssert(NumberOfSampleFrames >= 0, "");

  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pBufferLeft[i] = (float)S16BitFloatTo0To1Float(pSourceBufferLeft[i]);
    pBufferRight[i] = (float)S16BitFloatTo0To1Float(pSourceBufferRight[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToFloat32(
  const float*  pSourceBuffer, 
  float*        pBuffer, 
  int           NumberOfSampleFrames)
{
  MAssert(NumberOfSampleFrames >= 0, "");

  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pBuffer[i] = (float)S16BitFloatTo0To1Float(pSourceBuffer[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToFloat64Interleaved(
  const float*  pSourceBufferLeft, 
  const float*  pSourceBufferRight, 
  double*       pDestBuffer, 
  int           NumberOfSampleFrames)
{
  MAssert(NumberOfSampleFrames >= 0, "");

  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    *pDestBuffer++ = S16BitFloatTo0To1Float(pSourceBufferLeft[i]);
    *pDestBuffer++ = S16BitFloatTo0To1Float(pSourceBufferRight[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToFloat64(
  const float*  pSourceBufferLeft,
  const float*  pSourceBufferRight,
  double*       pBufferLeft,
  double*       pBufferRight,
  int           NumberOfSampleFrames)
{
  MAssert(NumberOfSampleFrames >= 0, "");

  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pBufferLeft[i] = S16BitFloatTo0To1Float(pSourceBufferLeft[i]);
    pBufferRight[i] = S16BitFloatTo0To1Float(pSourceBufferRight[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::InternalFloatToFloat64(
  const float*  pSourceBuffer,
  double*       pBuffer,
  int           NumberOfSampleFrames)
{
  MAssert(NumberOfSampleFrames >= 0, "");

  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pBuffer[i] = S16BitFloatTo0To1Float(pSourceBuffer[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int8BitSignedToInternalFloat(
  const TInt8*  pSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBuffer[i] = S8BitSignedTo16BitFloat(pSrcBuffer[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int8BitSignedInterleavedToInternalFloat(
  const TInt8*  pInterleavedSrcBuffer, 
  float*        pDestBufferLeft, 
  float*        pDestBufferRight, 
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBufferLeft[i] = S8BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
    pDestBufferRight[i] = S8BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int8BitSignedInterleavedToInternalFloat(
  const TInt8*          pInterleavedSrcBuffer,
  const TArray<float*>& pDestBufferPointers,
  int                   NumberOfSampleFrames)
{
  const int NumberOfChannels = pDestBufferPointers.Size();
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    for (int c = 0; c < NumberOfChannels; ++c)
    {
      pDestBufferPointers[c][i] = S8BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int8BitSignedInterleavedStereoToInternalFloatMono(
  const TInt8*  pInterleavedSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{
  for ( int i = 0; i < NumberOfSampleFrames; ++i )
  {
    const float Left = S8BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
    const float Right = S8BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);

    pDestBuffer[i] = (Left + Right) * 0.5f;
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int8BitUnsignedToInternalFloat(
  const TUInt8* pSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBuffer[i] = S8BitUnsignedTo16BitFloat(pSrcBuffer[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int8BitUnsignedInterleavedToInternalFloat(
  const TUInt8* pInterleavedSrcBuffer, 
  float*        pDestBufferLeft, 
  float*        pDestBufferRight, 
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBufferLeft[i] = S8BitUnsignedTo16BitFloat(*pInterleavedSrcBuffer++);
    pDestBufferRight[i] = S8BitUnsignedTo16BitFloat(*pInterleavedSrcBuffer++);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int8BitUnsignedInterleavedToInternalFloat(
  const TUInt8*         pInterleavedSrcBuffer,
  const TArray<float*>& pDestBufferPointers,
  int                   NumberOfSampleFrames)
{
  const int NumberOfChannels = pDestBufferPointers.Size();
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    for (int c = 0; c < NumberOfChannels; ++c)
    {
      pDestBufferPointers[c][i] = S8BitUnsignedTo16BitFloat(*pInterleavedSrcBuffer++);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int8BitUnsignedInterleavedStereoToInternalFloatMono(
  const TUInt8* pInterleavedSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    const float Left = S8BitUnsignedTo16BitFloat(*pInterleavedSrcBuffer++);
    const float Right = S8BitUnsignedTo16BitFloat(*pInterleavedSrcBuffer++);

    pDestBuffer[i] = (Left + Right) * 0.5f;
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int16BitToInternalFloat(
  const TInt16* pSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBuffer[i] = S16BitSignedTo16BitFloat(pSrcBuffer[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int16BitInterleavedToInternalFloat(
  const TInt16* pInterleavedSrcBuffer,
  float*        pDestBufferLeft,
  float*        pDestBufferRight,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBufferLeft[i] = S16BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
    pDestBufferRight[i] = S16BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int16BitInterleavedToInternalFloat(
  const TInt16*         pInterleavedSrcBuffer,
  const TArray<float*>& pDestBufferPointers,
  int                   NumberOfSampleFrames)
{
  const int NumberOfChannels = pDestBufferPointers.Size();
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    for (int c = 0; c < NumberOfChannels; ++c)
    {
      pDestBufferPointers[c][i] = S16BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int16BitInterleavedStereoToInternalFloatMono(
  const TInt16* pInterleavedSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    const float Left = S16BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
    const float Right = S16BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
    
    pDestBuffer[i] = (Left + Right) * 0.5f;
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int24BitToInternalFloat(
  const T24*  pSrcBuffer,
  float*      pDestBuffer,
  int         NumberOfSampleFrames)
{
  T24Pack* pT24Buffer = (T24Pack*)pSrcBuffer;

  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBuffer[i] = S24BitTo16BitFloat(pT24Buffer[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int24BitInterleavedToInternalFloat(
  const T24*  pInterleavedSrcBuffer,
  float*      pDestBufferLeft,
  float*      pDestBufferRight,
  int         NumberOfSampleFrames)
{
  T24Pack* pT24Buffer = (T24Pack*)pInterleavedSrcBuffer;

  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBufferLeft[i] = S24BitTo16BitFloat(*pT24Buffer++);
    pDestBufferRight[i] = S24BitTo16BitFloat(*pT24Buffer++);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int24BitInterleavedToInternalFloat(
  const T24*            pInterleavedSrcBuffer,
  const TArray<float*>& pDestBufferPointers,
  int                   NumberOfSampleFrames)
{
  T24Pack* pT24Buffer = (T24Pack*)pInterleavedSrcBuffer;
  const int NumberOfChannels = pDestBufferPointers.Size();

  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    for (int c = 0; c < NumberOfChannels; ++c)
    {
      pDestBufferPointers[c][i] = S24BitTo16BitFloat(*pT24Buffer++);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int24BitInterleavedStereoToInternalFloatMono(
  const T24*  pInterleavedSrcBuffer,
  float*      pDestBuffer,
  int         NumberOfSampleFrames)
{
  T24Pack* pT24Buffer = (T24Pack*)pInterleavedSrcBuffer;
  
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    const float Left = S24BitTo16BitFloat(*pT24Buffer++);
    const float Right = S24BitTo16BitFloat(*pT24Buffer++);
    
    pDestBuffer[i] = (Left + Right) * 0.5f;
  }
}
  
// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int32BitToInternalFloat(
  const TInt32* pSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{  
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBuffer[i] = S32BitSignedTo16BitFloat(pSrcBuffer[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int32BitInterleavedToInternalFloat(
  const TInt32* pInterleavedSrcBuffer,
  float*        pDestBufferLeft,
  float*        pDestBufferRight,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBufferLeft[i] = S32BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
    pDestBufferRight[i] = S32BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int32BitInterleavedToInternalFloat(
  const TInt32*         pInterleavedSrcBuffer,
  const TArray<float*>& pDestBufferPointers,
  int                   NumberOfSampleFrames)
{
  const int NumberOfChannels = pDestBufferPointers.Size();
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    for (int c = 0; c < NumberOfChannels; ++c)
    {
      pDestBufferPointers[c][i] = S32BitSignedTo16BitFloat( *pInterleavedSrcBuffer++ );
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Int32BitInterleavedStereoToInternalFloatMono(
  const TInt32* pInterleavedSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    const float Left = S32BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
    const float Right = S32BitSignedTo16BitFloat(*pInterleavedSrcBuffer++);
    
    pDestBuffer[i] = (Left + Right) * 0.5f;
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Float32BitToInternalFloat(
  const float*  pSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{  
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBuffer[i] = S0To1FloatTo16BitFloat(pSrcBuffer[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Float32BitInterleavedToInternalFloat(
  const float*  pInterleavedSrcBuffer,
  float*        pDestBufferLeft,
  float*        pDestBufferRight,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBufferLeft[i] = S0To1FloatTo16BitFloat(*pInterleavedSrcBuffer++);
    pDestBufferRight[i] = S0To1FloatTo16BitFloat(*pInterleavedSrcBuffer++);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Float32BitInterleavedToInternalFloat(
  const float*          pInterleavedSrcBuffer,
  const TArray<float*>& pDestBufferPointers,
  int                   NumberOfSampleFrames)
{
  const int NumberOfChannels = pDestBufferPointers.Size();
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    for (int c = 0; c < NumberOfChannels; ++c)
    {
      pDestBufferPointers[c][i] = S0To1FloatTo16BitFloat(*pInterleavedSrcBuffer++);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Float32BitInterleavedStereoToInternalFloatMono(
  const float*  pInterleavedSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    const float Left = S0To1FloatTo16BitFloat(*pInterleavedSrcBuffer++);
    const float Right = S0To1FloatTo16BitFloat(*pInterleavedSrcBuffer++);
    
    pDestBuffer[i] = (Left + Right) * 0.5f;
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Float64BitToInternalFloat(
  const double* pSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{  
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBuffer[i] = S0To1FloatTo16BitFloat(pSrcBuffer[i]);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Float64BitInterleavedToInternalFloat(
  const double* pInterleavedSrcBuffer,
  float*        pDestBufferLeft,
  float*        pDestBufferRight,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    pDestBufferLeft[i] = S0To1FloatTo16BitFloat(*pInterleavedSrcBuffer++);
    pDestBufferRight[i] = S0To1FloatTo16BitFloat(*pInterleavedSrcBuffer++);
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Float64BitInterleavedToInternalFloat(
  const double*         pInterleavedSrcBuffer,
  const TArray<float*>& pDestBufferPointers,
  int                   NumberOfSampleFrames)
{
  const int NumberOfChannels = pDestBufferPointers.Size();
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    for (int c = 0; c < NumberOfChannels; ++c)
    {
      pDestBufferPointers[c][i] = S0To1FloatTo16BitFloat(*pInterleavedSrcBuffer++);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TSampleConverter::Float64BitInterleavedStereoToInternalFloatMono(
  const double* pInterleavedSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSampleFrames)
{
  for (int i = 0; i < NumberOfSampleFrames; ++i)
  {
    const float Left = S0To1FloatTo16BitFloat(*pInterleavedSrcBuffer++);
    const float Right = S0To1FloatTo16BitFloat(*pInterleavedSrcBuffer++);
    
    pDestBuffer[i] = (Left + Right) * 0.5f;
  }
}

