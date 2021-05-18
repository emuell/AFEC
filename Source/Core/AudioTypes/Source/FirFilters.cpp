#include "AudioTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Alloca.h"
#include "CoreTypes/Export/Memory.h"

#if defined(MHaveIntelIPP)
  #include "../../3rdParty/IPP/Export/IPPs.h"
#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static double SZeroOrderBessel(double x)
{
  // This function calculates the zeroth order Bessel function

  double d = 0.0;
  double ds = 1.0;
  double s = 1.0;
  do
  {
    d += 2.0;
    ds *= (x * x) / (d * d);
    s += ds;
  }
  while (ds > s * 1e-6);
  return s;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TFirFilter::SCalcKaiserWindowedCoeffs(
  double* pCoefs, 
  int     NumberOfTaps,
  double  SampleRate, 
  double  FrequencyA, 
  double  FrequencyB, 
  double  Attenuation)
{
  // This function calculates Kaiser windowed
  // FIR filter coefficients for a single passband based on
  // "DIGITAL SIGNAL PROCESSING, II" IEEE Press pp 123-126.

  MAssert(NumberOfTaps > 2, "Invalid taps");
  MAssert((NumberOfTaps & 1) == 1, "Order should be odd");
  
  MAssert(SampleRate > 0.0 && SampleRate < 16*192000, "Invalid samplerate");
  MAssert(FrequencyA >= 0.0 && FrequencyA <= SampleRate / 2.0, "Invalid frequency");
  MAssert(FrequencyB >= 0.0 && FrequencyB <= SampleRate / 2.0, "Invalid frequency");
  MAssert(Attenuation > 21.0, "Invalid attenuation");

  const int Np = (NumberOfTaps - 1) / 2;

  TAllocaArray<double> TempCoeffs(NumberOfTaps / 2 + 1, 32);
  MInitAllocaArray(TempCoeffs);

  // Calculate the impulse response of the ideal filter
  TempCoeffs[0] = 2.0 * (FrequencyB - FrequencyA) / SampleRate;
  for (int j = 1; j <= Np; ++j)
  {
    TempCoeffs[j] = (::sin(2.0 * j * MPi * FrequencyB / SampleRate) -
      ::sin(2.0 * j * MPi * FrequencyA / SampleRate)) / (double)(j * MPi);
  }

  // Calculate the desired shape factor for the Kaiser-Bessel window
  double Alpha;
  if (Attenuation < 21.0)
  {
    Alpha = 0.0;
  }
  else if (Attenuation > 50.0)
  {
    Alpha = 0.1102 * (Attenuation - 8.7);
  }
  else
  {
    Alpha = 0.5842 * ::pow((Attenuation - 21.0), 0.4) + 0.07886 * (Attenuation - 21.0);
  }

  // Window the ideal response with the Kaiser-Bessel window
  const double AlphaBessel = SZeroOrderBessel(Alpha);

  for (int j = 0; j <= Np; j++)
  {
    pCoefs[Np + j] = TempCoeffs[j] * SZeroOrderBessel(
      Alpha * ::sqrt(1.0 - (j * j / (double)(Np * Np)))) / AlphaBessel;
  }

  for (int j = 0; j < Np; j++)
  {
    pCoefs[j] = pCoefs[NumberOfTaps - 1 - j];
  }
}

// -------------------------------------------------------------------------------------------------

TFirFilter::TFirFilter(int NumberOfTaps)
  : mNumberOfTaps(NumberOfTaps),
    mSampleRate(0.0),
    mFrequencyA(0.0),
    mFrequencyB(0.0),
    mAttenuation(48.0),
    mDelayLinePosLeft(0),
    mDelayLinePosRight (0)
{
  MAssert(NumberOfTaps >= 3 && NumberOfTaps <= 4096, "Invalid tap size");

  mCoeffs.SetSize(NumberOfTaps);
  mCoeffs.Init(0.0);

  mDelayLineLeft.SetSize(NumberOfTaps);
  mDelayLineLeft.Init(0.0);

  mDelayLineRight.SetSize(NumberOfTaps);
  mDelayLineRight.Init(0.0);

  #if defined(MHaveIntelIPP)

    // Get sizes of the IPP spec structure and the work buffer.
    // NB: must calc FIRs in doubles to be precise enough with windowed taps!

    int SpecSize, BufferSize;
    MCheckIPPStatus(::ippsFIRSRGetSize(
      mNumberOfTaps, ipp64f, &SpecSize, &BufferSize));

    mpIppSpecs = (IppsFIRSpec_64f*)ippsMalloc_8u(SpecSize);
    mpIppSpecsBuffer = ippsMalloc_8u(BufferSize);
  #endif
}

// -------------------------------------------------------------------------------------------------

TFirFilter::~TFirFilter()
{
  #if defined(MHaveIntelIPP)
    ::ippsFree(mpIppSpecs);
    ::ippsFree(mpIppSpecsBuffer);
  #endif
}

// -------------------------------------------------------------------------------------------------

int TFirFilter::NumberOfTaps()const
{
  return mNumberOfTaps;
}

// -------------------------------------------------------------------------------------------------

int TFirFilter::Latency()const
{
  return mNumberOfTaps / 2;
}

// -------------------------------------------------------------------------------------------------

const double* TFirFilter::Coeffs()const
{
  return mCoeffs.FirstRead();
}

// -------------------------------------------------------------------------------------------------

void TFirFilter::SetCoeffs(const double* pCoeffs)
{
  mFrequencyA = 0.0;
  mFrequencyB = 0.0;
  mSampleRate = 0.0;
  mAttenuation = 0.0;

  TAudioMath::CopyBuffer(pCoeffs, mCoeffs.FirstWrite(), mNumberOfTaps);

  #if defined(MHaveIntelIPP)
    // initialize/update the IPP spec structures
    MCheckIPPStatus(::ippsFIRSRInit_64f(
      mCoeffs.FirstWrite(), mNumberOfTaps, ippAlgDirect, mpIppSpecs));
  #endif
}

// -------------------------------------------------------------------------------------------------

void TFirFilter::SetupCoeffs(
  double FrequencyA, 
  double FrequencyB, 
  double SampleRate, 
  double Attenuation)
{
  if (mFrequencyA != FrequencyA ||
      mFrequencyB != FrequencyB ||
      mSampleRate != SampleRate ||
      mAttenuation != Attenuation)
  {
    mFrequencyA = FrequencyA;
    mFrequencyB = FrequencyB;
    mSampleRate = SampleRate;
    mAttenuation = Attenuation;
    
    SCalcKaiserWindowedCoeffs(mCoeffs.FirstWrite(), mNumberOfTaps,
      SampleRate, FrequencyA, FrequencyB, Attenuation);

    #if defined(MHaveIntelIPP)
      // initialize/update the IPP spec structures
      MCheckIPPStatus(::ippsFIRSRInit_64f(
        mCoeffs.FirstWrite(), mNumberOfTaps, ippAlgDirect, mpIppSpecs));
    #endif
  }
}

// -------------------------------------------------------------------------------------------------

double TFirFilter::ProcessSampleLeft(double InputSample)
{
  #if defined(MHaveIntelIPP)

    const TAudioMath::TDisableSseDenormals DisableDenormals;

    double OutputSample;
    MCheckIPPStatus(::ippsFIRSR_64f(
      &InputSample, &OutputSample, 1, mpIppSpecs, 
      mDelayLineLeft.FirstWrite(), mDelayLineLeft.FirstWrite(), 
      mpIppSpecsBuffer));

    return OutputSample;

  #else
    double Result = 0.0;

    mDelayLineLeft[mDelayLinePosLeft] = InputSample;

    int Index = mDelayLinePosLeft++;
    for (int i = 0; i < mNumberOfTaps; i++)
    {
      Result += (double)mCoeffs[i] * (double)mDelayLineLeft[Index];

      if (--Index < 0)
      {
        Index = mNumberOfTaps - 1;
      }
    }

    mDelayLinePosLeft %= mNumberOfTaps;

    MUnDenormalize(Result);
    return Result;

  #endif
}

// -------------------------------------------------------------------------------------------------

double TFirFilter::ProcessSampleRight(double InputSample)
{
  #if defined(MHaveIntelIPP)

    const TAudioMath::TDisableSseDenormals DisableDenormals;

    double OutputSample;
    MCheckIPPStatus(::ippsFIRSR_64f(
      &InputSample, &OutputSample, 1, mpIppSpecs, 
      mDelayLineRight.FirstWrite(), mDelayLineRight.FirstWrite(), 
      mpIppSpecsBuffer));

    return OutputSample;

  #else
    double Result = 0.0;

    mDelayLineRight[mDelayLinePosRight] = InputSample;

    int Index = mDelayLinePosRight++;
    for (int i = 0; i < mNumberOfTaps; i++)
    {
      Result += (double)mCoeffs[i] * (double)mDelayLineRight[Index];

      if (--Index < 0)
      {
        Index = mNumberOfTaps - 1;
      }
    }

    mDelayLinePosRight %= mNumberOfTaps;

    MUnDenormalize(Result);
    return Result;

  #endif
}

// -------------------------------------------------------------------------------------------------

void TFirFilter::ProcessSamples(
  const float*  pInputBuffer,
  float*        pOutputBuffer,
  int           NumberOfSamples)
{
  #if defined(MHaveIntelIPP)

    const TAudioMath::TDisableSseDenormals DisableDenormals;

    TAllocaArray<double> TempBuffer(NumberOfSamples, 32);
    MInitAllocaArray(TempBuffer);

    MCheckIPPStatus(::ippsConvert_32f64f(
      pInputBuffer, TempBuffer.FirstWrite(), NumberOfSamples));

    MCheckIPPStatus(::ippsFIRSR_64f(
      TempBuffer.FirstRead(), TempBuffer.FirstWrite(), NumberOfSamples, 
      mpIppSpecs, mDelayLineLeft.FirstWrite(), mDelayLineLeft.FirstWrite(), 
      mpIppSpecsBuffer));

    MCheckIPPStatus(::ippsConvert_64f32f(
      TempBuffer.FirstRead(), pOutputBuffer, NumberOfSamples));

  #else
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pOutputBuffer[i] = (float)ProcessSampleLeft(pInputBuffer[i]);
      MUnDenormalize(pOutputBuffer[i]);
    }

  #endif
}

void TFirFilter::ProcessSamples(
  const float*  pInputBufferLeft,
  const float*  pInputBufferRight,
  float*        pOutputBufferLeft,
  float*        pOutputBufferRight,
  int         NumberOfSamples)
{
  #if defined(MHaveIntelIPP)

    const TAudioMath::TDisableSseDenormals DisableDenormals;

    TAllocaArray<double> TempBuffer(NumberOfSamples, 32);
    MInitAllocaArray(TempBuffer);
    
    MCheckIPPStatus(::ippsConvert_32f64f(
      pInputBufferLeft, TempBuffer.FirstWrite(), NumberOfSamples));

    MCheckIPPStatus(::ippsFIRSR_64f(
      TempBuffer.FirstRead(), TempBuffer.FirstWrite(), NumberOfSamples, 
      mpIppSpecs, mDelayLineLeft.FirstWrite(), mDelayLineLeft.FirstWrite(), 
      mpIppSpecsBuffer));

    MCheckIPPStatus(::ippsConvert_64f32f(
      TempBuffer.FirstRead(), pOutputBufferLeft, NumberOfSamples));


    MCheckIPPStatus(::ippsConvert_32f64f(
      pInputBufferRight, TempBuffer.FirstWrite(), NumberOfSamples));

    MCheckIPPStatus(::ippsFIRSR_64f(
      TempBuffer.FirstRead(), TempBuffer.FirstWrite(), NumberOfSamples, 
      mpIppSpecs, mDelayLineRight.FirstWrite(), mDelayLineRight.FirstWrite(), 
      mpIppSpecsBuffer));

    MCheckIPPStatus(::ippsConvert_64f32f(
      TempBuffer.FirstRead(), pOutputBufferRight, NumberOfSamples));

  #else
    for (int i = 0; i < NumberOfSamples; ++i)
    {
      pOutputBufferLeft[i] = (float)ProcessSampleLeft(pInputBufferLeft[i]);
      MUnDenormalize(pOutputBufferLeft[i]);

      pOutputBufferRight[i] = (float)ProcessSampleRight(pInputBufferRight[i]);
      MUnDenormalize(pOutputBufferRight[i]);
    }

  #endif
}

// -------------------------------------------------------------------------------------------------

void TFirFilter::FlushBuffers()
{
  mDelayLinePosLeft = 0;
  mDelayLinePosRight = 0;

  mDelayLineLeft.Init(0.0);
  mDelayLineRight.Init(0.0);
}

