#include "AudioTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Alloca.h"

#include "AudioTypes/Export/AudioMath.h"
#include "AudioTypes/Export/Fourier.h"

#include <cmath>
#include <algorithm>

#if defined(MHaveIntelIPP)
  #include "../../3rdParty/IPP/Export/IPPs.h"
#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline float SPhaseRewrap(float Phase)
{
  return (Phase > -(float)MPi && Phase < (float)MPi) ? Phase :
    Phase + (float)M2Pi * (1.f + ::floorf((-(float)MPi - Phase) * (float)MInv2Pi));
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TOnsetFftProcessor::TOnsetFftProcessor(
    float               SampleRate,
    unsigned int        FftSize,
    unsigned int        HopSize,
    bool                LogMagnitudes,
    TWhiteningType      WhiteningType,
    float               WhiteningRelaxTime)
  : mSampleRate(SampleRate),
    mFftsize(FftSize),
    mHopsize(HopSize),
    mNumbins((FftSize / 2) - 1),
    mLogmags(LogMagnitudes),
    mWhtype(WhiteningType),
    mWhiteningRelaxTime(0.0f),
    mWhiteningRelaxCoef(0.0f),
    mWhiteningFloor(0.1f)
{
  MAssert(mFftsize >= kMinFftSize && mFftsize <= kMaxFftSize, "Invalid FFT size");
  MAssert(mHopsize <= FftSize, "Invalid hop size");
  MAssert(SampleRate > 0.0f, "Invalid sample rate");
  MAssert(WhiteningRelaxTime >= 0.0f, "Invalid relax time");

  // Initialize the FFT transform
  const bool HighQuality = true;
  mFFT.Initialize(mFftsize, HighQuality, TFftTransformComplex::kNoDiv);

  // Calculate window coefs
  mWindowBuffer.SetSize(mFftsize);
  TFftWindow::SFillBuffer(TFftWindow::kHanning, mWindowBuffer.FirstWrite(), mFftsize);

  // Allocate whitening history
  const unsigned int RealNumBins = mNumbins + 2;
  mPsp.SetSize(RealNumBins);
  mPsp.Init(0.0f);

  // Reset polar buffer
  mPolarBuffer.mDC = 0;
  mPolarBuffer.mNyquist = 0;
  mPolarBuffer.mBin.SetSize(mNumbins);
  for (unsigned int i = 0; i < mNumbins; ++i) 
  {
    mPolarBuffer.mBin[i].mMagn = 0;
    mPolarBuffer.mBin[i].mPhase = 0;
  }
  
  // Default settings for Adaptive Whitening, user can set own values after init
  SetRelaxTime(WhiteningRelaxTime);
}

// -------------------------------------------------------------------------------------------------

const TOnsetFftProcessor::TPolarBuffer* TOnsetFftProcessor::Process(const float* pBuffer)
{
  TAllocaArray<double> TempBuffer(mFftsize);
  MInitAllocaArray(TempBuffer);

  for (unsigned int i = 0; i < mFftsize; ++i)
  {
    TempBuffer[i] = (double)pBuffer[i];
  }

  LoadFrame(TempBuffer.FirstRead());
  Whiten();

  return &mPolarBuffer;
}

const TOnsetFftProcessor::TPolarBuffer* TOnsetFftProcessor::Process(const double* pBuffer)
{
  LoadFrame(pBuffer);
  Whiten();

  return &mPolarBuffer;
}

// -------------------------------------------------------------------------------------------------

void TOnsetFftProcessor::SetRelaxTime(float Time)
{
  mWhiteningRelaxTime = Time;
  mWhiteningRelaxCoef = (Time == 0.0f) ? 0.0f : 
    (float)(::exp((-2.30258509 * (float)mHopsize) / (Time * mSampleRate)));
}

// -------------------------------------------------------------------------------------------------

void TOnsetFftProcessor::LoadFrame(const double* pBuffer)
{
  // ... copy and window the data

  TAudioMath::CopyBuffer(pBuffer, mFFT.Re(), mFftsize);
  TAudioMath::MultiplyBuffers(mWindowBuffer.FirstRead(), mFFT.Re(), mFFT.Re(), mFftsize);

  TAudioMath::ClearBuffer(mFFT.Im(), mFftsize);


  // ... do the FFT

  mFFT.Forward();


  // ... fetch magnitude and phase

  {
    TAllocaArray<double> Magnitude(mNumbins);
    MInitAllocaArray(Magnitude);

    TAudioMath::Magnitude(mFFT.Re(), mFFT.Im(),
      Magnitude.FirstWrite(), mNumbins);

    TAllocaArray<double> Phase(mNumbins);
    MInitAllocaArray(Phase);

    TAudioMath::Phase(mFFT.Re(), mFFT.Im(),
      Phase.FirstWrite(), mNumbins);

    mPolarBuffer.mDC = (float)mFFT.Re()[0];
    MUnDenormalize(mPolarBuffer.mDC);

    mPolarBuffer.mNyquist = (float)mFFT.Im()[0];
    MUnDenormalize(mPolarBuffer.mNyquist);

    for (unsigned int i = 0; i < mNumbins; i++)
    {
      mPolarBuffer.mBin[i].mMagn = (float)Magnitude[i];
      mPolarBuffer.mBin[i].mPhase = (float)Phase[i];
    }
  }

  // apply conversion to log-domain magnitudes
  ApplyLogMags();
}

// -------------------------------------------------------------------------------------------------

void TOnsetFftProcessor::ApplyLogMags()
{
  // ... conversion to log-domain magnitudes

  if (mLogmags)
  {
    static const double LogLowerLimit = 2e-42;
    static const double LogOfLowerLimit = -96.0154267;
    static const double AbsInfOfLowerLimit = 0.010414993;

    for (unsigned int i = 0; i < mNumbins; i++)
    {
      mPolarBuffer.mBin[i].mMagn = 
        (float)((::log(MMax((double)mPolarBuffer.mBin[i].mMagn, LogLowerLimit)) -
          LogOfLowerLimit) * AbsInfOfLowerLimit);
    }

    mPolarBuffer.mDC = 
      (float)((::log(MMax((double)TMathT<float>::Abs(mPolarBuffer.mDC), LogLowerLimit)) -
        LogOfLowerLimit) * AbsInfOfLowerLimit);

    mPolarBuffer.mNyquist = 
      (float)((::log(MMax((double)TMathT<float>::Abs(mPolarBuffer.mNyquist), LogLowerLimit)) -
        LogOfLowerLimit) * AbsInfOfLowerLimit);
  }
}

// -------------------------------------------------------------------------------------------------

void TOnsetFftProcessor::Whiten()
{
  if (mWhtype == kWhiteningNone)
  {
    return;
  }
  
  double Value, OldValue;

  double* pPsp = mPsp.FirstWrite();
  double* pPspp1 = mPsp.FirstWrite() + 1;
  
  // For each bin, update the record of the peak value
  
  Value = ::fabsf(mPolarBuffer.mDC);  // Grab current magnitude
  OldValue = pPsp[0];
  if (Value < OldValue)
  {
    // If it beats the amplitude stored then that's our new amplitude;
    // otherwise our new amplitude is a decayed version of the old one
    Value = Value + (OldValue - Value) * mWhiteningRelaxCoef;
  }
  pPsp[0] = Value; // Store the "amplitude trace" back
  
  Value = ::fabsf(mPolarBuffer.mNyquist);
  OldValue = pPspp1[mNumbins];
  if (Value < OldValue)
  {
    Value = Value + (OldValue - Value) * mWhiteningRelaxCoef;
  }
  pPspp1[mNumbins] = Value;
  
  for (unsigned int i = 0; i < mNumbins; ++i)
  {
    Value = ::fabsf(mPolarBuffer.mBin[i].mMagn);
    OldValue = pPspp1[i];
    if (Value < OldValue)
    {
      Value = Value + (OldValue - Value) * mWhiteningRelaxCoef;
    }
    pPspp1[i] = Value;
  }
  
  mPolarBuffer.mDC /= (float)MMax((double)mWhiteningFloor, pPsp[0]);
  mPolarBuffer.mNyquist /= (float)MMax((double)mWhiteningFloor, pPspp1[mNumbins]);

  for (unsigned int i = 0; i < mNumbins; ++i)
  {
    mPolarBuffer.mBin[i].mMagn /= (float)MMax((double)mWhiteningFloor, pPspp1[i]);
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TOnsetDetector::TOnsetDetector(
  TOnsetFunctionType  OdfType,
  float               SampleRate,
  unsigned int        FftSize,
  unsigned int        HopSize,
  float               Threshold,
  float               ThresholdMedianSpan,
  float               MinimumGap)
  : mOdftype(OdfType),
    mSampleRate(SampleRate),
    mFftsize(FftSize),
    mHopsize(HopSize),
    mThresh(Threshold),
    mMedSpan(0),
    mMedOdd(false),
    mMinGap(0),
    mGapLeft(0),
    mOdfparam(0.0f),
    mNormfactor(0.0f),
    mOdfvalpost(0.0f),
    mOdfvalpostprev(0),
    mNumbins((FftSize / 2) - 1)
{
  MAssert(FftSize >= TOnsetFftProcessor::kMinFftSize && FftSize <= TOnsetFftProcessor::kMaxFftSize, "");
  MAssert(HopSize <= FftSize, "Invalid hop size");
  MAssert(SampleRate > 0.0f, "Invalid sample rate");

  const unsigned int RealNumBins = mNumbins + 2;

  // Set mMedSpan to the number of FFT frames ThresholdMedianSpan corresponds to
  // TODO: this should use mHopsize?
  mMedSpan = (int)(((float)SampleRate * ThresholdMedianSpan) / (float)mHopsize + 0.5f);
  mMedSpan = MMax<int>(mMedSpan, 3); // cap to 3 as further code expects at least that
  mMedOdd = ((mMedSpan & 1) != 0);

  // Allocate temp arrays
  mpOdfvals.SetSize(mMedSpan);
  mpOdfvals.Init(0.0f);
  
  // Set mMinGap to the number of FFT frames MinimumGap corresponds to
  // TODO: this should use mHopsize?
  mMinGap = (int)(((float)SampleRate * MinimumGap) / (float)mHopsize + 0.5f);

  switch (mOdftype)
  {
    case kFunctionPower:
      mOdfparam = 0.01f; // "power threshold" in SC code
      mNormfactor = 2560.f / (float)(RealNumBins * mFftsize);
      // No old FFT frames needed:
      mpOther.Empty();
      break;

    case kFunctionMagSum:
      mOdfparam = 0.01f; // "power threshold" in SC code
      mNormfactor = 113.137085f / ((float)RealNumBins * ::sqrtf((float)mFftsize));
      // No old FFT frames needed:
      mpOther.Empty();
      break;
    
    case kFunctionComplex:
      mOdfparam = 0.01f; // "power threshold" in SC code
      mNormfactor = (float)(231.70475 / ::pow((double)mFftsize, 1.5)); // / mFftsize;
      // For each bin (NOT dc/nyq) we store mag, phase and d_phase
      mpOther.SetSize(mNumbins + mNumbins + mNumbins);
      mpOther.Init(0.0f);
      break;

    case kFunctionRComplex:
      mOdfparam = 0.01f; // "power threshold" in SC code
      mNormfactor = (float)(231.70475 / ::pow((double)mFftsize, 1.5)); // / mFftsize;
      // For each bin (NOT dc/nyq) we store mag, phase and d_phase
      mpOther.SetSize(mNumbins + mNumbins + mNumbins);
      mpOther.Init(0.0f);
      break;

    case kFunctionPhase:
      mOdfparam = 0.01f; // "power threshold" in SC code
      mNormfactor = 5.12f / (float)mFftsize;// / mFftsize;
      // For each bin (NOT dc/nyq) we store phase and d_phase
      mpOther.SetSize(mNumbins + mNumbins);
      mpOther.Init(0.0f);
      break;

    case kFunctionWPhase:
      mOdfparam = 0.0001f; // "power threshold" in SC code. Here it's superfluous.
      mNormfactor = (float)(115.852375 / ::pow((double)mFftsize, 1.5)); // / mFftsize;
      // For each bin (NOT dc/nyq) we store phase and d_phase
      mpOther.SetSize(mNumbins + mNumbins);
      mpOther.Init(0.0f);
      break;

    case kFunctionMKL:
      mOdfparam = 0.01f; // Epsilon parameter. Brossier recommends 1e-6
      mNormfactor = 7.68f * 0.25f / (float)mFftsize;
      // For each bin (NOT dc/nyq) we store mag
      mpOther.SetSize(mNumbins);
      mpOther.Init(0.0f);
      break;

    default:
      MInvalid("Unexpected odf function");
      break;
  }
}

// -------------------------------------------------------------------------------------------------

float TOnsetDetector::CurrentOdfValue()const  
{ 
  return mOdfvalpost; 
}

// -------------------------------------------------------------------------------------------------

bool TOnsetDetector::Process(const TOnsetFftProcessor::TPolarBuffer* pBuffer)
{
  CalculateOnsetFunction(pBuffer);
  return DetectOnset();
}

// -------------------------------------------------------------------------------------------------

void TOnsetDetector::CalculateOnsetFunction(const TOnsetFftProcessor::TPolarBuffer* pBuffer)
{
  int TBPointer;
  float Deviation, Diff, CurrentMagnitude;
  double TotalDeviation;
  
  bool Rectify = true;
  
  float* pOdfvals = mpOdfvals.FirstWrite();
  float* pOther = mpOther.FirstWrite();

  // Here we shunt the "old" ODF values down one place
  TMemory::Move(pOdfvals + 1, pOdfvals, (mMedSpan - 1) * sizeof(float));
  
  // Now calculate a new value and store in pOdfvals[0]
  switch (mOdftype)
  {
    case kFunctionPower:
      *pOdfvals = (pBuffer->mNyquist * pBuffer->mNyquist) + (pBuffer->mDC * pBuffer->mDC);
      for (unsigned int i = 0; i < mNumbins; i++)
      {
        const float Magnitude = pBuffer->mBin[i].mMagn;

        *pOdfvals += Magnitude * Magnitude;
      }
      break;
      
    case kFunctionMagSum:
      *pOdfvals = 0;
      for (unsigned int i = 0; i < mNumbins; i++)
      {
        *pOdfvals += TMathT<float>::Abs(pBuffer->mBin[i].mMagn);
      }
      break;
      
    case kFunctionComplex:
      Rectify = false;
    case kFunctionRComplex:
      // Note: "other" buf is stored in this format: mag[0],phase[0],d_phase[0],mag[1],phase[1],d_phase[1], ...
      // Iterate through, calculating the deviation from expected value.
      TotalDeviation = 0.0;
      TBPointer = 0;

      float PredMagnitude, PredPhase, YesterPhase, YesterPhaseDiff;

      for (unsigned int i = 0; i < mNumbins; ++i)
      {
        CurrentMagnitude = TMathT<float>::Abs(pBuffer->mBin[i].mMagn);
      
        // Predict mag as yestermag
        PredMagnitude   = pOther[TBPointer++];
        YesterPhase     = pOther[TBPointer++];
        YesterPhaseDiff = pOther[TBPointer++];
        
        // Thresholding as Brossier did - discard (ignore) bin's deviation if bin's power is minimal
        if (CurrentMagnitude > mOdfparam)
        {
          // If rectifying, ignore decreasing bins
          if (! Rectify || ! (CurrentMagnitude < PredMagnitude))
          {
            // Predict phase as YesterPhase + YesterPhaseDiff
            PredPhase = YesterPhase + YesterPhaseDiff;
            
            // Here temporarily using the "deviation" var to store the phase difference
            //  so that the rewrap macro can use it more efficiently
            Deviation = PredPhase - pBuffer->mBin[i].mPhase;
            
            // Deviation is Euclidean distance between predicted and actual.
            // In polar coords: sqrt(r1^2 +  r2^2 - r1r2 cos (theta1 - theta2))
            Deviation = ::sqrtf(PredMagnitude * PredMagnitude + CurrentMagnitude * CurrentMagnitude -
              PredMagnitude * CurrentMagnitude * ::cosf(SPhaseRewrap(Deviation)));
            
            TotalDeviation += Deviation;
          }
        }
      }
      
      // TotalDeviation will be the output, but first we need to fill tempbuf with today's values, ready for tomorrow.
      TBPointer = 0;

      for (unsigned int i = 0; i < mNumbins; ++i)
      {
        pOther[TBPointer++] = TMathT<float>::Abs(pBuffer->mBin[i].mMagn); // Storing mag
        Diff = pBuffer->mBin[i].mPhase - pOther[TBPointer]; // Retrieving yesterphase from buf
        pOther[TBPointer++] = pBuffer->mBin[i].mPhase; // Storing phase

        // Wrap onto +-PI range
        Diff = SPhaseRewrap(Diff);
        
        // Storing first diff to buf
        pOther[TBPointer++] = Diff;
      }

      *pOdfvals = (float)TotalDeviation;
      break;
      
    case kFunctionPhase:
      Rectify = false; // So, actually, "Rectify" means "useweighting" in this context
    case kFunctionWPhase:
      // Note: "other" buf is stored in this format: phase[0],d_phase[0],phase[1],d_phase[1], ...
      // Iterate through, calculating the deviation from expected value.
      TotalDeviation = 0.0;
      TBPointer = 0;

      for (unsigned int i = 0; i < mNumbins; ++i)
      {
        // Thresholding as Brossier did - discard (ignore) bin's phase deviation if bin's power is low
        if (TMathT<float>::Abs(pBuffer->mBin[i].mMagn) > mOdfparam)
        {
          int Thisptr = TBPointer++;
          int Nextptr = TBPointer++;
          
          // Deviation is the *second difference* of the phase, which is calc'ed as curval - yesterval - yesterfirstdiff
          Deviation = pBuffer->mBin[i].mPhase - pOther[Thisptr] - pOther[Nextptr];
          
          // Wrap onto +-PI range
          Deviation = SPhaseRewrap(Deviation);
          
          if (Rectify)
          {
            TotalDeviation += ::fabs(Deviation * TMathT<float>::Abs(pBuffer->mBin[i].mMagn));
          }
          else
          {
            TotalDeviation += ::fabs(Deviation);
          }
        }
      }
      
      // TotalDeviation will be the output, but first we need to fill tempbuf with today's values, ready for tomorrow.
      TBPointer = 0;

      for (unsigned int i = 0; i < mNumbins; ++i)
      {
        Diff = pBuffer->mBin[i].mPhase - pOther[TBPointer]; // Retrieving yesterphase from buf
        pOther[TBPointer++] = pBuffer->mBin[i].mPhase; // Storing phase

        // Wrap onto +-PI range
        Diff = SPhaseRewrap(Diff);

        // Storing first diff to buf
        pOther[TBPointer++] = Diff;
      }

      *pOdfvals = (float)TotalDeviation;
      break;
      
    case kFunctionMKL:
      // Iterate through, calculating the Modified Kullback-Liebler distance
      TotalDeviation = 0.0;
      TBPointer = 0;

      float YesterMagnitude;

      for (unsigned int i = 0; i < mNumbins; ++i)
      {
        CurrentMagnitude = TMathT<float>::Abs(pBuffer->mBin[i].mMagn);
        YesterMagnitude = pOther[TBPointer];
        
        // Here's the main implementation of Brossier's MKL equation:
        Deviation = TMathT<float>::Abs(CurrentMagnitude) / (TMathT<float>::Abs(YesterMagnitude) + mOdfparam);
        TotalDeviation += ::log(1.0 + Deviation);
        
        // Store the magnitude
        pOther[TBPointer++] = CurrentMagnitude;
      }

      *pOdfvals = (float)TotalDeviation;
      break;

    default:
      MInvalid("Unexpected odf function");
      break;
  }
    
  pOdfvals[0] *= mNormfactor;
}

// -------------------------------------------------------------------------------------------------

bool TOnsetDetector::DetectOnset()
{
  // Median removal

  // Shift the yesterval to its rightful place
  mOdfvalpostprev = mOdfvalpost;
    
  // Sort Odfvals
  MAssert((int)mMedSpan == mpOdfvals.Size(), "");
  TAllocaArray<float> SortedOdfvals(mMedSpan);
  MInitAllocaArray(SortedOdfvals);
  TMemory::Copy(SortedOdfvals.FirstWrite(), mpOdfvals.FirstRead(), mMedSpan * sizeof(float));
  std::sort(SortedOdfvals.FirstWrite(), SortedOdfvals.FirstWrite() + SortedOdfvals.Size());
      
  // Subtract the middlest value === the median
  const float Median = mMedOdd ? 
    SortedOdfvals[(mMedSpan - 1) >> 1] :
    ((SortedOdfvals[mMedSpan >> 1] + SortedOdfvals[(mMedSpan >> 1) - 1]) * 0.5f);

  mOdfvalpost = mpOdfvals[0] - Median;

  // Detection not allowed if we're too close to a previous detection.
  bool Detected;
  if (mGapLeft != 0)
  {
    mGapLeft--;
    Detected = false;
  }
  else
  {
    // Now do the detection.
    Detected = (mOdfvalpost > mThresh) && (mOdfvalpostprev <= mThresh);
    if (Detected)
    {
      mGapLeft = mMinGap;
    }
  }

  return Detected;
}

