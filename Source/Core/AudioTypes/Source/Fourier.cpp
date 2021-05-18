#include "AudioTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Alloca.h"

#if defined(MHaveIntelIPP)
  #include "../../3rdParty/IPP/Export/IPPs.h"

#else
  #include "AudioTypes/Source/OouraFFT8g.h"
#endif

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
  #else
  , mpTempDoubleBuffer(NULL)
  , mpTempIntBuffer(NULL)
  #endif
{
}

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
  
  #else
    mpTempDoubleBuffer = TAlignedAllocator<double>::SNew(mFftSize);

    mpTempIntBuffer = TAlignedAllocator<int>::SNew(mFftSize);
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
  #else

    TAlignedAllocator<double>::SDelete(mpTempDoubleBuffer, mFftSize);
    TAlignedAllocator<int>::SDelete(mpTempIntBuffer, mFftSize);

    TAlignedAllocator<double>::SDelete(mpProcessedReal, mFftSize);
    TAlignedAllocator<double>::SDelete(mpProcessedImag, mFftSize);

  #endif
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplex::Forward()
{
  TAudioMath::TDisableSseDenormals DisableDenormals;

  M__DisableFloatingPointAssertions

  #if defined(MHaveIntelIPP)

    MCheckIPPStatus(
      ::ippsFFTFwd_CToC_64f(mpProcessedReal, mpProcessedImag,
          mpProcessedReal, mpProcessedImag, mpFFTSpec, mpMemBuffer));

  #else

    TAllocaArray<double> InOutput(mFftSize * 2);
    MInitAllocaArray(InOutput);
    double* pInOutput = InOutput.FirstWrite();

    TAudioMath::CopyBuffer(mpProcessedReal, pInOutput, mFftSize);
    TAudioMath::ClearBuffer(pInOutput + mFftSize, mFftSize);

    ::ooura_rdft(
      mFftSize,
      1,
      pInOutput,
      mpTempIntBuffer,
      mpTempDoubleBuffer);

    for (unsigned int i = 0; i < mFftSize; ++i)
    {
      mpProcessedReal[i] = pInOutput[2 * i];
      mpProcessedImag[i] = pInOutput[2 * i + 1];
    }

    if ((mDivFlags & kDivFwdByN))
    {
      const double ScaleFactor = 1.0 / mFftSize;
      TAudioMath::ScaleBuffer(mpProcessedReal, mFftSize, ScaleFactor);
      TAudioMath::ScaleBuffer(mpProcessedImag, mFftSize, ScaleFactor);
    }
  #endif

  M__EnableFloatingPointAssertions
}

// -------------------------------------------------------------------------------------------------

void TFftTransformComplex::Inverse()
{
  TAudioMath::TDisableSseDenormals DisableDenormals;

  M__DisableFloatingPointAssertions

  #if defined(MHaveIntelIPP)

    MCheckIPPStatus(
      ::ippsFFTInv_CToC_64f(mpProcessedReal, mpProcessedImag,
          mpProcessedReal, mpProcessedImag, mpFFTSpec, mpMemBuffer));

  #else
    TAllocaArray<double> InOutput(mFftSize * 2);
    MInitAllocaArray(InOutput);
    double* pInOutput = InOutput.FirstWrite();

    for (unsigned int i = 0; i < mFftSize; ++i)
    {
      pInOutput[2 * i] = mpProcessedReal[i];
      pInOutput[2 * i + 1] = mpProcessedImag[i];
    }

    ::ooura_rdft(
      mFftSize,
      -1,
      InOutput.FirstWrite(),
      mpTempIntBuffer,
      mpTempDoubleBuffer);

    TAudioMath::CopyBuffer(pInOutput, mpProcessedReal, mFftSize);

    // fixed scaled output from ooura 
    double ScaleFactor = 2.0;

    // apply div options
    if ((mDivFlags & kDivInvByN))
    {
      ScaleFactor *= 1.0 / mFftSize;
    }

    TAudioMath::ScaleBuffer(mpProcessedReal, mFftSize, ScaleFactor);
  #endif

  M__EnableFloatingPointAssertions
}

