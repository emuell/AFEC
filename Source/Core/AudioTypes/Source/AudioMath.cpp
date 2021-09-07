#include "AudioTypesPrecompiledHeader.h"


#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/Memory.h"

#if defined(MHaveIntelIPP)
  #include "../../3rdParty/IPP/Export/IPPs.h"
#endif
  
#if defined(MMac)
  #include <Accelerate/Accelerate.h>
#endif

#if defined(MArch_X86) || defined(MArch_X64)
  #include <xmmintrin.h>
#endif

#include <cmath>

// =================================================================================================

#if defined(MArch_X86) || defined(MArch_X64)

// -------------------------------------------------------------------------------------------------

TAudioMath::TDisableSseDenormals::TDisableSseDenormals()
{
  // DAZ and FZ bits
  static const unsigned int sMask = 0x8040;

  M__DisableFloatingPointAssertions

  mOldMXCSR = ::_mm_getcsr();
  ::_mm_setcsr(mOldMXCSR | sMask);
}
  
// -------------------------------------------------------------------------------------------------

TAudioMath::TDisableSseDenormals::~TDisableSseDenormals()
{
  ::_mm_setcsr(mOldMXCSR);

  M__EnableFloatingPointAssertions
}

#endif // defined(MArch_X86) || defined(MArch_X64)

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TAudioMath::Init()
{
  #if defined(MHaveIntelIPP)
    TLog::SLog()->AddLine("IPP", "Detected CPU type: 0x%x", (int)::ippGetCpuType());
  #endif
}


// -------------------------------------------------------------------------------------------------

void TAudioMath::AddBuffer(
  const float*  pSourceBuffer,
  float*        pDestBuffer,
  int           NumberOfSamples)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsAdd_32f(pSourceBuffer, pDestBuffer, pDestBuffer, NumberOfSamples));
  
  #elif defined(MMac)
    ::vDSP_vadd(pSourceBuffer, 1, pDestBuffer, 1, pDestBuffer, 1, NumberOfSamples);
    
  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pDestBuffer[i] += pSourceBuffer[i];
    }
  #endif
}

void TAudioMath::AddBuffer(
  const double* pSourceBuffer,
  double*       pDestBuffer,
  int           NumberOfSamples)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsAdd_64f(pSourceBuffer, pDestBuffer, pDestBuffer, NumberOfSamples));

  #elif defined(MMac)
    ::vDSP_vaddD(pSourceBuffer, 1, pDestBuffer, 1, pDestBuffer, 1, NumberOfSamples);
    
  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pDestBuffer[i] += pSourceBuffer[i];
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

void TAudioMath::CopyBuffer(
  const float*  pSrcBuffer, 
  float*        pDestBuffer, 
  int           NumberOfSamples)
{
  // no TDisableSseDenormals needed here
  
  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsCopy_32f(pSrcBuffer, pDestBuffer, NumberOfSamples));
  
  #else
    MAssert(NumberOfSamples >= 0, "");
    TMemory::Copy(pDestBuffer, pSrcBuffer, sizeof(float) * NumberOfSamples);
  #endif
}

void TAudioMath::CopyBuffer(
  const double* pSrcBuffer, 
  double*       pDestBuffer, 
  int           NumberOfSamples)
{
  // no TDisableSseDenormals needed here
  
  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsCopy_64f(pSrcBuffer, pDestBuffer, NumberOfSamples));
  
  #else
    MAssert(NumberOfSamples >= 0, "");
    TMemory::Copy(pDestBuffer, pSrcBuffer, sizeof(double) * NumberOfSamples);
  #endif
}

// -------------------------------------------------------------------------------------------------

void TAudioMath::CopyBufferScaled(
  const float*  pSourceBuffer,
  float*        pDestBuffer,
  int           NumberOfSamples,
  float         ScaleFactor)
{
  if (ScaleFactor == 1.0f)
  {
    CopyBuffer(pSourceBuffer, pDestBuffer, NumberOfSamples);
    return;
  }

  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMulC_32f(pSourceBuffer, ScaleFactor, pDestBuffer, NumberOfSamples));
  
  #elif defined(MMac)
    ::vDSP_vsmul(pSourceBuffer, 1, &ScaleFactor, pDestBuffer, 1, NumberOfSamples);
  
  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pDestBuffer[i] = pSourceBuffer[i] * ScaleFactor;
    }
  #endif
}

void TAudioMath::CopyBufferScaled(
  const double* pSourceBuffer,
  double*       pDestBuffer,
  int           NumberOfSamples,
  float         ScaleFactor)
{
  if (ScaleFactor == 1.0f)
  {
    CopyBuffer(pSourceBuffer, pDestBuffer, NumberOfSamples);
    return;
  }

  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMulC_64f(pSourceBuffer, ScaleFactor, pDestBuffer, NumberOfSamples));
  
  #elif defined(MMac)
    double ScaleFactorD =  ScaleFactor;
    ::vDSP_vsmulD(pSourceBuffer, 1, &ScaleFactorD, pDestBuffer, 1, NumberOfSamples);
  
  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pDestBuffer[i] = pSourceBuffer[i] * ScaleFactor;
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

void TAudioMath::MultiplyBuffers(
  const float*  pSourceBufferA,
  const float*  pSourceBufferB,
  float*        pDestBuffer,
  int           NumberOfSamples)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMul_32f(pSourceBufferA, pSourceBufferB, pDestBuffer, NumberOfSamples));
  
  #elif defined(MMac)
    ::vDSP_vmul(pSourceBufferA, 1, pSourceBufferB, 1, pDestBuffer, 1, NumberOfSamples);
  
  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pDestBuffer[i] = pSourceBufferA[i] * pSourceBufferB[i];
    }
  #endif
}

void TAudioMath::MultiplyBuffers(
  const double* pSourceBufferA,
  const double* pSourceBufferB,
  double*       pDestBuffer,
  int           NumberOfSamples)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMul_64f(pSourceBufferA, pSourceBufferB, pDestBuffer, NumberOfSamples));
  
  #elif defined(MMac)
    ::vDSP_vmulD(pSourceBufferA, 1, pSourceBufferB, 1, pDestBuffer, 1, NumberOfSamples);
  
  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pDestBuffer[i] = pSourceBufferA[i] * pSourceBufferB[i];
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

void TAudioMath::ScaleBuffer(
  float*  pSourceBuffer,
  int     NumberOfSamples,
  float   ScaleFactor)
{
  if (ScaleFactor == 1.0f)
  {
    return;
  }

  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMulC_32f(pSourceBuffer, ScaleFactor, pSourceBuffer, NumberOfSamples));
  
  #elif defined(MMac)
    ::vDSP_vsmul(pSourceBuffer, 1, &ScaleFactor, pSourceBuffer, 1, NumberOfSamples);

  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pSourceBuffer[i] *= ScaleFactor;
    }
  #endif
}
  
void TAudioMath::ScaleBuffer(
  double* pSourceBuffer,
  int     NumberOfSamples,
  double  ScaleFactor)
{
  if (ScaleFactor == 1.0f)
  {
    return;
  }

  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMulC_64f(pSourceBuffer, ScaleFactor, pSourceBuffer, NumberOfSamples));
  
  #elif defined(MMac)
    ::vDSP_vsmulD(pSourceBuffer, 1, &ScaleFactor, pSourceBuffer, 1, NumberOfSamples);

  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pSourceBuffer[i] *= ScaleFactor;
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

void TAudioMath::AddBufferScaled(
  const float*  pSrcBuffer,
  float*        pDestBuffer,
  int           NumberOfSamples,
  float         SrcScaleFactor)
{
  if (SrcScaleFactor == 1.0f)
  {
    AddBuffer(pSrcBuffer, pDestBuffer, NumberOfSamples);
    return;
  }

  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsAddProductC_32f(pSrcBuffer, SrcScaleFactor, pDestBuffer, NumberOfSamples));

  #elif defined(MMac)
    ::vDSP_vsma(pSrcBuffer, 1, &SrcScaleFactor, pDestBuffer, 1, pDestBuffer, 1, NumberOfSamples);
    
  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pDestBuffer[i] += pSrcBuffer[i] * SrcScaleFactor;
    }
  #endif
}

void TAudioMath::AddBufferScaled(
  const double* pSrcBuffer,
  double*       pDestBuffer,
  int           NumberOfSamples,
  double        SrcScaleFactor)
{
  if (SrcScaleFactor == 1.0f)
  {
    AddBuffer(pSrcBuffer, pDestBuffer, NumberOfSamples);
    return;
  }

  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

 // there is no ippsAddProductC_64f
 
  #if defined(MMac)
    ::vDSP_vsmaD(pSrcBuffer, 1, &SrcScaleFactor, pDestBuffer, 1, pDestBuffer, 1, NumberOfSamples);
  
  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pDestBuffer[i] += pSrcBuffer[i] * SrcScaleFactor;
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

void TAudioMath::ClearBuffer(
  float*  pBuffer,
  int     NumberOfSamples)
{
  // no TDisableSseDenormals needed here

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsZero_32f(pBuffer, NumberOfSamples));
  
  #elif defined(Mac)
    ::vDSP_vclr(pDestBuffer, ByteCount);
  
  #else
  
    MAssert(NumberOfSamples >= 0, "");
    TMemory::Zero(pBuffer, NumberOfSamples * sizeof(float));
  #endif
}

void TAudioMath::ClearBuffer(
  double* pBuffer,
  int     NumberOfSamples)
{
  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsZero_64f(pBuffer, NumberOfSamples));
  
  #elif defined(Mac)
    ::vDSP_vclrD(pDestBuffer, ByteCount);
  
  #else
    MAssert(NumberOfSamples >= 0, "");
    TMemory::Zero(pBuffer, NumberOfSamples * sizeof(double));
  #endif
}

// -------------------------------------------------------------------------------------------------

void TAudioMath::Magnitude(
  const float* pComplexBufferReal,
  const float* pComplexBufferImag,
  float*       pDestBinBuffer,
  int          NumberOfBins)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMagnitude_32f((Ipp32f*)pComplexBufferReal, (Ipp32f*)pComplexBufferImag,
        (Ipp32f*)pDestBinBuffer, NumberOfBins));

  #elif defined(MMac)
    DSPSplitComplex Complex {
      const_cast<float*>(pComplexBufferReal),
      const_cast<float*>(pComplexBufferImag)
    };
    ::vDSP_zvmags(&Complex, 1, pDestBinBuffer, 1, NumberOfBins);
    ::vvsqrtf(pDestBinBuffer, pDestBinBuffer, &NumberOfBins);
        
  #else
    MAssert(NumberOfBins > 0, "");
    for (int i = 0; i < NumberOfBins; i++)
    {
      const float Real = *pComplexBufferReal++;
      const float Imag = *pComplexBufferImag++;

      *pDestBinBuffer++ = ::sqrtf(Real * Real + Imag * Imag);
    }
  #endif
}

void TAudioMath::Magnitude(
  const double* pComplexBufferReal,
  const double* pComplexBufferImag,
  double*       pDestBinBuffer,
  int           NumberOfBins)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMagnitude_64f((Ipp64f*)pComplexBufferReal, (Ipp64f*)pComplexBufferImag,
        (Ipp64f*)pDestBinBuffer, NumberOfBins));

  #elif defined(MMac)
    DSPDoubleSplitComplex Complex {
      const_cast<double*>(pComplexBufferReal),
      const_cast<double*>(pComplexBufferImag)
    };
    ::vDSP_zvmagsD(&Complex, 1, pDestBinBuffer, 1, NumberOfBins);
    ::vvsqrt(pDestBinBuffer, pDestBinBuffer, &NumberOfBins);
        
  #else
    MAssert(NumberOfBins > 0, "");
    for (int i = 0; i < NumberOfBins; i++)
    {
      const double Real = *pComplexBufferReal++;
      const double Imag = *pComplexBufferImag++;

      *pDestBinBuffer++ = ::sqrt(Real * Real + Imag * Imag);
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

void TAudioMath::PowerSpectrum(
  const float* pComplexBufferReal,
  const float* pComplexBufferImag,
  float*       pDestBinBuffer,
  int          NumberOfBins)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsPowerSpectr_32f((Ipp32f*)pComplexBufferReal, (Ipp32f*)pComplexBufferImag,
        (Ipp32f*)pDestBinBuffer, NumberOfBins));

  #elif defined(MMac)
    DSPSplitComplex Complex {
      const_cast<float*>(pComplexBufferReal),
      const_cast<float*>(pComplexBufferImag)
    };
    ::vDSP_zvmags(&Complex, 1, pDestBinBuffer, 1, NumberOfBins);
        
  #else
    MAssert(NumberOfBins > 0, "");
    for (int i = 0; i < NumberOfBins; i++)
    {
      const float Real = *pComplexBufferReal++;
      const float Imag = *pComplexBufferImag++;

      *pDestBinBuffer++ = Real * Real + Imag * Imag;
    }
  #endif
}

void TAudioMath::PowerSpectrum(
  const double* pComplexBufferReal,
  const double* pComplexBufferImag,
  double*       pDestBinBuffer,
  int           NumberOfBins)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsPowerSpectr_64f((Ipp64f*)pComplexBufferReal, (Ipp64f*)pComplexBufferImag,
        (Ipp64f*)pDestBinBuffer, NumberOfBins));

  #elif defined(MMac)
    DSPDoubleSplitComplex Complex {
      const_cast<double*>(pComplexBufferReal),
      const_cast<double*>(pComplexBufferImag)
    };
    ::vDSP_zvmagsD(&Complex, 1, pDestBinBuffer, 1, NumberOfBins);
        
  #else
    MAssert(NumberOfBins > 0, "");
    for (int i = 0; i < NumberOfBins; i++)
    {
      const double Real = *pComplexBufferReal++;
      const double Imag = *pComplexBufferImag++;

      *pDestBinBuffer++ = Real * Real + Imag * Imag;
    }
  #endif
}

// -------------------------------------------------------------------------------------------------

void TAudioMath::Phase(
  const float* pComplexBufferReal,
  const float* pComplexBufferImag,
  float*       pDestBinBuffer,
  int          NumberOfBins)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsPhase_32f((Ipp32f*)pComplexBufferReal, (Ipp32f*)pComplexBufferImag,
        (Ipp32f*)pDestBinBuffer, NumberOfBins));
  
  #elif defined(MMac)
    DSPSplitComplex Complex {
      const_cast<float*>(pComplexBufferReal),
      const_cast<float*>(pComplexBufferImag)
    };
    ::vDSP_zvphas(&Complex, 1, pDestBinBuffer, 1, NumberOfBins);
  
  #else
    MAssert(NumberOfBins > 0, "");
    for (int i = 0; i < NumberOfBins; i++)
    {
      const float Real = *pComplexBufferReal++;
      const float Imag = *pComplexBufferImag++;

      *pDestBinBuffer++ = ::atan2f(Imag, Real);
    }
  #endif
}

void TAudioMath::Phase(
  const double* pComplexBufferReal,
  const double* pComplexBufferImag,
  double*       pDestBinBuffer,
  int           NumberOfBins)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsPhase_64f((Ipp64f*)pComplexBufferReal, (Ipp64f*)pComplexBufferImag,
        (Ipp64f*)pDestBinBuffer, NumberOfBins));
  
  #elif defined(MMac)
    DSPDoubleSplitComplex Complex {
      const_cast<double*>(pComplexBufferReal),
      const_cast<double*>(pComplexBufferImag)
    };
    ::vDSP_zvphasD(&Complex, 1, pDestBinBuffer, 1, NumberOfBins);
  
  #else
    MAssert(NumberOfBins > 0, "");
    for (int i = 0; i < NumberOfBins; i++)
    {
      const double Real = *pComplexBufferReal++;
      const double Imag = *pComplexBufferImag++;

      *pDestBinBuffer++ = ::atan2(Imag, Real);
    }
  #endif
}

