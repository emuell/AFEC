#include "AudioTypesPrecompiledHeader.h"


#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/Memory.h"

#if defined(MHaveIntelIPP)
  #include "../../3rdParty/IPP/Export/IPPs.h"
#endif

#if defined(MArch_X86) || defined(MArch_X64)
  #include <xmmintrin.h>
#endif

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

void TAudioMath::FrequencyToNote(double Frequency, int& Note, int& CentsDeviation)
{
  MAssert(Frequency >= 20.0 && Frequency <= 22050.0, "Invalid frequency value");

  const double BaseFrequency = 440.0;

  const double NoteFrac = 69.0 + 12.0 *
    ::log(Frequency / BaseFrequency) / ::log(2.0);

  Note = TMath::d2iRound(NoteFrac);
  CentsDeviation = TMath::d2iRound((NoteFrac - Note) * 100);

  if (CentsDeviation >= 50)
  {
    CentsDeviation -= 100;
    Note++;
  }
}

// -------------------------------------------------------------------------------------------------

void TAudioMath::AddBuffer(
  const float*  pSourceBuffer,
  float*        pDestBuffer,
  int           NumberOfSamples)
{
  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsAdd_32f(pSourceBuffer, pDestBuffer, pDestBuffer, NumberOfSamples));
  
  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pDestBuffer[i] *= pSourceBuffer[i];
    }
  #endif
}

void TAudioMath::AddBuffer(
  const double* pSourceBuffer,
  double*       pDestBuffer,
  int           NumberOfSamples)
{
  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsAdd_64f(pSourceBuffer, pDestBuffer, pDestBuffer, NumberOfSamples));

  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pDestBuffer[i] *= pSourceBuffer[i];
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

  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMulC_32f(pSourceBuffer, ScaleFactor, pDestBuffer, NumberOfSamples));
  
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

  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMulC_64f(pSourceBuffer, ScaleFactor, pDestBuffer, NumberOfSamples));
  
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
  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMul_32f(pSourceBufferA, pSourceBufferB, pDestBuffer, NumberOfSamples));
  
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
  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMul_64f(pSourceBufferA, pSourceBufferB, pDestBuffer, NumberOfSamples));
  
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

  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMulC_32f(pSourceBuffer, ScaleFactor, pSourceBuffer, NumberOfSamples));
  
  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pSourceBuffer[i] = pSourceBuffer[i] * ScaleFactor;
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

  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMulC_64f(pSourceBuffer, ScaleFactor, pSourceBuffer, NumberOfSamples));
  
  #else
    MAssert(NumberOfSamples >= 0, "");
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pSourceBuffer[i] = pSourceBuffer[i] * ScaleFactor;
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
  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    if (SrcScaleFactor == 1.0f)
    {
      MCheckIPPStatus(
        ::ippsAdd_32f(pSrcBuffer, pDestBuffer, pDestBuffer, NumberOfSamples));
    }
    else
    {
      MCheckIPPStatus(
        ::ippsAddProductC_32f(pSrcBuffer, SrcScaleFactor, pDestBuffer, NumberOfSamples));
    }
  
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
  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMagnitude_32f((Ipp32f*)pComplexBufferReal, (Ipp32f*)pComplexBufferImag,
        (Ipp32f*)pDestBinBuffer, NumberOfBins));
  #else
    MAssert(NumberOfBins > 0, "Invalid bin size");

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
  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsMagnitude_64f((Ipp64f*)pComplexBufferReal, (Ipp64f*)pComplexBufferImag,
        (Ipp64f*)pDestBinBuffer, NumberOfBins));
  #else
    MAssert(NumberOfBins > 0, "Invalid bin size");

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
  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsPowerSpectr_32f((Ipp32f*)pComplexBufferReal, (Ipp32f*)pComplexBufferImag,
        (Ipp32f*)pDestBinBuffer, NumberOfBins));
  #else
    MAssert(NumberOfBins > 0, "Invalid bin size");

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
  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsPowerSpectr_64f((Ipp64f*)pComplexBufferReal, (Ipp64f*)pComplexBufferImag,
        (Ipp64f*)pDestBinBuffer, NumberOfBins));
  #else
    MAssert(NumberOfBins > 0, "Invalid bin size");

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
  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsPhase_32f((Ipp32f*)pComplexBufferReal, (Ipp32f*)pComplexBufferImag,
        (Ipp32f*)pDestBinBuffer, NumberOfBins));
  
  #else
    MAssert(NumberOfBins > 0, "Invalid bin size");

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
  const TDisableSseDenormals DisableDenormals;

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsPhase_64f((Ipp64f*)pComplexBufferReal, (Ipp64f*)pComplexBufferImag,
        (Ipp64f*)pDestBinBuffer, NumberOfBins));
  
  #else
    MAssert(NumberOfBins > 0, "Invalid bin size");

    for (int i = 0; i < NumberOfBins; i++)
    {
      const double Real = *pComplexBufferReal++;
      const double Imag = *pComplexBufferImag++;

      *pDestBinBuffer++ = ::atan2(Imag, Real);
    }
  #endif
}

