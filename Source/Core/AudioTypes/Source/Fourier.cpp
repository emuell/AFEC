#include "AudioTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Alloca.h"

#if defined(MHaveIntelIPP)
  #include "../../3rdParty/IPP/Export/IPPs.h"
#elif defined(MMac)
  #include <Accelerate/Accelerate.h>
#endif

#include "AudioTypes/Source/OouraFFT8g.h"

// =================================================================================================

#if defined(MHaveIntelIPP)
  MStaticAssert((int)TFftTransformComplex::kDivFwdByN == (int)IPP_FFT_DIV_FWD_BY_N);
  MStaticAssert((int)TFftTransformComplex::kDivInvByN == (int)IPP_FFT_DIV_INV_BY_N);
  MStaticAssert((int)TFftTransformComplex::kNoDiv == (int)IPP_FFT_NODIV_BY_ANY);
#endif

// -------------------------------------------------------------------------------------------------

TFftTransformComplex::TFftTransformComplex()
  : mFftSize(0)
  , mDivFlags(0)
  , mpProcessedReal(NULL)
  , mpProcessedImag(NULL)
#if defined(MHaveIntelIPP)
  , mpFFTSpec(NULL)
  , mpMemSpec(NULL)
  , mpMemInit(NULL)
  , mpMemBuffer(NULL)
#elif defined(MMac)
  , mpDFTSetupForward(NULL)
  , mpDFTSetupInverse(NULL)
#else
  , mpTempInterleavedBuffer(NULL)
  , mpTempDoubleBuffer(NULL)
  , mpTempIntBuffer(NULL)
#endif
{ }

// -------------------------------------------------------------------------------------------------

TFftTransformComplex::~TFftTransformComplex()
{
  Free();
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplex::Initialize(
  int       FftSize, 
  bool      HighQuality, 
  TDivFlags Flags)
{
  Free();

  MAssert(FftSize > 4 && TMath::IsPowerOfTwo(FftSize), "Invalid fft size");

  mDivFlags = Flags;
  mFftSize = FftSize;

  #if defined(MHaveIntelIPP)
    const int Order = TMath::Pow2Order(FftSize);

    const IppHintAlgorithm QualityHint =
      HighQuality ? ippAlgHintAccurate : ippAlgHintFast;

    int SizeSpec, SizeInit, SizeBuffer;
    MCheckIPPStatus(
      ::ippsFFTGetSize_C_64f(Order, Flags, QualityHint,
        &SizeSpec, &SizeInit, &SizeBuffer));

    mpMemSpec = ::ippsMalloc_8u(SizeSpec);
    mpMemBuffer = ::ippsMalloc_8u(SizeBuffer);
    if (SizeInit > 0)
    {
       mpMemInit = ::ippsMalloc_8u(SizeInit);
    }

    MCheckIPPStatus(
      ::ippsFFTInit_C_64f(&mpFFTSpec, Order, Flags, QualityHint,
          mpMemSpec, mpMemInit));

    mpProcessedReal = ::ippsMalloc_64f(FftSize);
    mpProcessedImag = ::ippsMalloc_64f(FftSize);
  
  #elif defined(MMac)
    mpDFTSetupForward = ::vDSP_DFT_zop_CreateSetupD(NULL, FftSize, vDSP_DFT_FORWARD);
    MAssert(mpDFTSetupForward != NULL, "DFT setup failed!");

    mpDFTSetupInverse = ::vDSP_DFT_zop_CreateSetupD(NULL, FftSize, vDSP_DFT_INVERSE);
    MAssert(mpDFTSetupInverse != NULL, "DFT setup failed!");

    mpProcessedReal = TAlignedAllocator<double>::SNew(FftSize);
    mpProcessedImag = TAlignedAllocator<double>::SNew(FftSize);
    
  #else
    mpTempInterleavedBuffer = TAlignedAllocator<double>::SNew(2 * FftSize);
    mpTempDoubleBuffer = TAlignedAllocator<double>::SNew(FftSize);

    mpTempIntBuffer = TAlignedAllocator<int>::SNew(FftSize);
    mpTempIntBuffer[0] = 0;

    mpProcessedReal = TAlignedAllocator<double>::SNew(FftSize);
    mpProcessedImag = TAlignedAllocator<double>::SNew(FftSize);
  #endif
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplex::Free()
{
  #if defined(MHaveIntelIPP)
    if (mpMemSpec)
    {
      ::ippFree(mpMemSpec);
      mpMemSpec = NULL;
    }

    if (mpMemInit)
    {
      ::ippFree(mpMemInit);
      mpMemInit = NULL;
    }

    if (mpMemBuffer)
    {
      ::ippFree(mpMemBuffer);
      mpMemBuffer = NULL;
    }

    if (mpProcessedReal)
    {
      ::ippFree(mpProcessedReal);
      mpProcessedReal = NULL;
    }

    if (mpProcessedImag)
    {
      ::ippFree(mpProcessedImag);
      mpProcessedImag = NULL;
    }

  #elif defined(MMac)
    ::vDSP_DFT_DestroySetupD(mpDFTSetupForward);
    mpDFTSetupForward = NULL;

    ::vDSP_DFT_DestroySetupD(mpDFTSetupInverse);
    mpDFTSetupInverse = NULL;

    TAlignedAllocator<double>::SDelete(mpProcessedReal, mFftSize);
    mpProcessedReal = NULL;
    
    TAlignedAllocator<double>::SDelete(mpProcessedImag, mFftSize);
    mpProcessedImag = NULL;
  
  #else
    TAlignedAllocator<double>::SDelete(mpTempInterleavedBuffer, 2*mFftSize);
    mpTempInterleavedBuffer = NULL;
  
    TAlignedAllocator<double>::SDelete(mpTempDoubleBuffer, mFftSize);
    mpTempDoubleBuffer = NULL;
  
    TAlignedAllocator<int>::SDelete(mpTempIntBuffer, mFftSize);
    mpTempIntBuffer = NULL;
  
    TAlignedAllocator<double>::SDelete(mpProcessedReal, mFftSize);
    mpProcessedReal = NULL;
    
    TAlignedAllocator<double>::SDelete(mpProcessedImag, mFftSize);
    mpProcessedImag = NULL;
  
  #endif
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplex::Forward(
  const double* pReIn,
  const double* pImIn,
  double*       pReOut,
  double*       pImOut)
{
  TAudioMath::CopyBuffer(pReIn, mpProcessedReal, mFftSize);
  TAudioMath::CopyBuffer(pImIn, mpProcessedImag, mFftSize);

  ForwardInplace();

  TAudioMath::CopyBuffer(mpProcessedReal, pReOut, mFftSize);
  TAudioMath::CopyBuffer(mpProcessedImag, pImOut, mFftSize);
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplex::Inverse(
  const double* pReIn,
  const double* pImIn,
  double*       pReOut,
  double*       pImOut)
{
  TAudioMath::CopyBuffer(pReIn, mpProcessedReal, mFftSize);
  TAudioMath::CopyBuffer(pImIn, mpProcessedImag, mFftSize);

  InverseInplace();

  TAudioMath::CopyBuffer(mpProcessedReal, pReOut, mFftSize);
  TAudioMath::CopyBuffer(mpProcessedImag, pImOut, mFftSize);
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplex::ForwardInplace()
{
  ForwardInplace(mpProcessedReal, mpProcessedImag);
}

void TFftTransformComplex::ForwardInplace(double* pRe, double* pIm)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TAudioMath::TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsFFTFwd_CToC_64f(pRe, pIm, pRe, pIm, mpFFTSpec, mpMemBuffer));

  #elif defined(MMac)
    ::vDSP_DFT_ExecuteD(mpDFTSetupForward, pRe, pIm, pRe, pIm);
    
    // apply div options
    if (mDivFlags & kDivFwdByN)
    {
      const double ScaleFactor = 1.0f / mFftSize;
      TAudioMath::ScaleBuffer(pRe, mFftSize, ScaleFactor);
      TAudioMath::ScaleBuffer(pIm, mFftSize, ScaleFactor);
    }
    
  #else
    M__DisableFloatingPointAssertions

    double* pInterleavedTemp = mpTempInterleavedBuffer;

    for (unsigned int i = 0; i < mFftSize; ++i)
    {
      pInterleavedTemp[2*i + 0] = pRe[i];
      pInterleavedTemp[2*i + 1] = pIm[i];
    }

    mpTempIntBuffer[0] = 0;
    ::ooura_cdft(
      2*mFftSize,
      1,
      pInterleavedTemp,
      mpTempIntBuffer,
      mpTempDoubleBuffer);

    for (unsigned int i = 0; i < mFftSize; ++i)
    {
      pRe[i] = pInterleavedTemp[2*i + 0];
      pIm[i] = pInterleavedTemp[2*i + 1];
    }

    if (mDivFlags & kDivFwdByN)
    {
      const double ScaleFactor = 1.0f / mFftSize;
      TAudioMath::ScaleBuffer(pRe, mFftSize, ScaleFactor);
      TAudioMath::ScaleBuffer(pIm, mFftSize, ScaleFactor);
    }

    M__EnableFloatingPointAssertions
  #endif
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplex::InverseInplace()
{
  InverseInplace(mpProcessedReal, mpProcessedImag);
}

void TFftTransformComplex::InverseInplace(double* pRe, double* pIm)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TAudioMath::TDisableSseDenormals DisableDenormals;
  #endif

  #if defined(MHaveIntelIPP)
    MCheckIPPStatus(
      ::ippsFFTInv_CToC_64f(pRe, pIm, pRe, pIm, mpFFTSpec, mpMemBuffer));

  #elif defined(MMac)
    ::vDSP_DFT_ExecuteD(mpDFTSetupInverse, pRe, pIm, pRe, pIm);
    
    // apply div options
    if (mDivFlags & kDivInvByN)
    {
      const double ScaleFactor = 1.0f / mFftSize;
      TAudioMath::ScaleBuffer(pRe, mFftSize, ScaleFactor);
      TAudioMath::ScaleBuffer(pIm, mFftSize, ScaleFactor);
    }
    
  #else
    M__DisableFloatingPointAssertions

    double* pInterleavedTemp = mpTempInterleavedBuffer;

    for (unsigned int i = 0; i < mFftSize; ++i)
    {
      pInterleavedTemp[2*i + 0] = pRe[i];
      pInterleavedTemp[2*i + 1] = pIm[i];
    }

    ::ooura_cdft(
      2*mFftSize,
      -1,
      pInterleavedTemp,
      mpTempIntBuffer,
      mpTempDoubleBuffer);

    for (unsigned int i = 0; i < mFftSize; ++i)
    {
      pRe[i] = pInterleavedTemp[2*i + 0];
      pIm[i] = pInterleavedTemp[2*i + 1];
    }

    // apply div options
    if (mDivFlags & kDivInvByN)
    {
      const double ScaleFactor = 1.0f / mFftSize;
      TAudioMath::ScaleBuffer(pRe, mFftSize, ScaleFactor);
      TAudioMath::ScaleBuffer(pIm, mFftSize, ScaleFactor);
    }

    M__EnableFloatingPointAssertions
  #endif
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TFftTransformComplexOouora::TFftTransformComplexOouora()
  : mFftSize(0)
  , mDivFlags(0)
  , mpProcessedReal(NULL)
  , mpProcessedImag(NULL)
  , mpTempInterleavedBuffer(NULL)
  , mpTempDoubleBuffer(NULL)
  , mpTempIntBuffer(NULL)
{
}

// -------------------------------------------------------------------------------------------------

TFftTransformComplexOouora::~TFftTransformComplexOouora()
{
  Free();
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplexOouora::Initialize(
  int       FftSize, 
  bool      HighQuality, 
  TDivFlags Flags)
{
  Free();

  MAssert(FftSize > 4 && TMath::IsPowerOfTwo(FftSize), "Invalid fft size");

  mDivFlags = Flags;
  mFftSize = FftSize;

  mpTempInterleavedBuffer = TAlignedAllocator<double>::SNew(2 * FftSize);
  mpTempDoubleBuffer = TAlignedAllocator<double>::SNew(FftSize);

  mpTempIntBuffer = TAlignedAllocator<int>::SNew(FftSize);
  mpTempIntBuffer[0] = 0;

  mpProcessedReal = TAlignedAllocator<double>::SNew(FftSize);
  mpProcessedImag = TAlignedAllocator<double>::SNew(FftSize);
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplexOouora::Free()
{
  TAlignedAllocator<double>::SDelete(mpTempInterleavedBuffer, 2*mFftSize);
  mpTempInterleavedBuffer = NULL;
  
  TAlignedAllocator<double>::SDelete(mpTempDoubleBuffer, mFftSize);
  mpTempDoubleBuffer = NULL;
  
  TAlignedAllocator<int>::SDelete(mpTempIntBuffer, mFftSize);
  mpTempIntBuffer = NULL;

  TAlignedAllocator<double>::SDelete(mpProcessedReal, mFftSize);
  mpProcessedReal = NULL;
  
  TAlignedAllocator<double>::SDelete(mpProcessedImag, mFftSize);
  mpProcessedImag = NULL;
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplexOouora::ForwardInplace()
{
  ForwardInplace(mpProcessedReal, mpProcessedImag);
}

void TFftTransformComplexOouora::ForwardInplace(double* pRe, double* pIm)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TAudioMath::TDisableSseDenormals DisableDenormals;
  #endif

  M__DisableFloatingPointAssertions

  double* pInterleavedTemp = mpTempInterleavedBuffer;

  for (unsigned int i = 0; i < mFftSize; ++i)
  {
    pInterleavedTemp[2*i + 0] = pRe[i];
    pInterleavedTemp[2*i + 1] = pIm[i];
  }

  mpTempIntBuffer[0] = 0;
  ::ooura_cdft(
    2*mFftSize,
    1,
    pInterleavedTemp,
    mpTempIntBuffer,
    mpTempDoubleBuffer);

  for (unsigned int i = 0; i < mFftSize; ++i)
  {
    pRe[i] = pInterleavedTemp[2*i + 0];
    pIm[i] = pInterleavedTemp[2*i + 1];
  }

  if (mDivFlags & kDivFwdByN)
  {
    const double ScaleFactor = 1.0f / mFftSize;
    TAudioMath::ScaleBuffer(pRe, mFftSize, ScaleFactor);
    TAudioMath::ScaleBuffer(pIm, mFftSize, ScaleFactor);
  }

  M__EnableFloatingPointAssertions
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplexOouora::InverseInplace()
{
  InverseInplace(mpProcessedReal, mpProcessedImag);
}

void TFftTransformComplexOouora::InverseInplace(double* pRe, double* pIm)
{
  #if defined(MArch_X86) || defined(MArch_X64)
    const TAudioMath::TDisableSseDenormals DisableDenormals;
  #endif

  M__DisableFloatingPointAssertions

  double* pInterleavedTemp = mpTempInterleavedBuffer;

  for (unsigned int i = 0; i < mFftSize; ++i)
  {
    pInterleavedTemp[2*i + 0] = pRe[i];
    pInterleavedTemp[2*i + 1] = pIm[i];
  }

  ::ooura_cdft(
    2*mFftSize,
    -1,
    pInterleavedTemp,
    mpTempIntBuffer,
    mpTempDoubleBuffer);

  for (unsigned int i = 0; i < mFftSize; ++i)
  {
    pRe[i] = pInterleavedTemp[2*i + 0];
    pIm[i] = pInterleavedTemp[2*i + 1];
  }

  // apply div options
  if (mDivFlags & kDivInvByN)
  {
    const double ScaleFactor = 1.0f / mFftSize;
    TAudioMath::ScaleBuffer(pRe, mFftSize, ScaleFactor);
    TAudioMath::ScaleBuffer(pIm, mFftSize, ScaleFactor);
  }

  M__EnableFloatingPointAssertions
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

double TFftWindow::SFillBuffer(
  const TWindowType& Type, double* pBuffer, int Size)
{
  const double DeltaSize = 1.0 / (double)(Size - 1);

  double WindowFactor = 0.0;

  switch(Type)
  {
  case kNone:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = 1.0;
      WindowFactor += pBuffer[i];
    }
    break;

  case kRectangular:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = 0.707;
      WindowFactor += pBuffer[i];
    }
    break;

  case kRaisedCosine:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = -0.5 * ::cos(M2Pi * (double)i * DeltaSize) + 0.5;
      WindowFactor += pBuffer[i];
    }
    break;

  case kHamming:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = 0.54 - (0.46 * ::cos(M2Pi * (double)i * DeltaSize));
      WindowFactor += pBuffer[i];
    }
    break;

  case kHanning:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = 0.5 * (1.0 - ::cos(M2Pi * (double)i * DeltaSize));
      WindowFactor += pBuffer[i];
    }
    break;

  case kBlackman:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = 0.42 - (0.5 * ::cos(M2Pi * (double)i * DeltaSize)) +
          (0.08 * ::cos(4.0 * MPi * (double)i * DeltaSize));

      WindowFactor += pBuffer[i];
    }
    break;

  case kBlackmanHarris:
    for (int i = 0; i < Size; ++i)
    {
      pBuffer[i] = 0.42323 - (0.49755 * ::cos(M2Pi * (double)i * DeltaSize)) +
          (0.07922 * ::cos(4.0 * MPi * (double)i * DeltaSize));

      WindowFactor += pBuffer[i];
    }
    break;

  case kGaussian:
    for (int i = 0; i < Size; ++i)
    {
      constexpr double SConstE = 2.7182818284590452353602875;

      pBuffer[i] = ::pow(SConstE, -0.5 *
        ::pow(((double)i - (double)(Size - 1) / 2.0) /
          (0.1 * (double)(Size - 1) / 2.0), 2.0));

      WindowFactor += pBuffer[i];
    }
    break;

  case kFlatTop:
    for (int i = 0; i < Size; ++i)
    {
      constexpr double SA0 = 1.0, SA1 = 1.93, SA2 = 1.29, SA3 = 0.388, SA4 = 0.032;

      pBuffer[i] = (SA0 -
        SA1 * ::cos(2.0 * MPi * i * DeltaSize) +
        SA2 * ::cos(4.0 * MPi * i * DeltaSize) -
        SA3 * ::cos(6.0 * MPi * i * DeltaSize) +
        SA4 * ::cos(8.0 * MPi * i * DeltaSize));

      WindowFactor += pBuffer[i];
    }
    break;

  default:
    MInvalid("Wrong fft window type");
    break;
  }

  return WindowFactor;
}
