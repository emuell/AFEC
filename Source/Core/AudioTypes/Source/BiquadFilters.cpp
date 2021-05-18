#include "AudioTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Memory.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TRbjBiquadFilter::TRbjBiquadFilter()
  : mType(kInvalidType),
    mFrequency(0.0),
    mSampleRate(0.0),
    mQ(0.0),
    mDbGain(0.0),
    mB0a0(0.0),
    mB1a0(0.0),
    mB2a0(0.0),
    mA0(0.0),
    mA1a0(0.0),
    mA2a0(0.0)
{
  FlushBuffers();
}

// -------------------------------------------------------------------------------------------------

void TRbjBiquadFilter::Setup(
   TType   Type,
   double  Frequency,
   double  SampleRate,
   double  Q,
   double  DbGain)
{
  MAssert(Type != kInvalidType, "");
  MAssert(Frequency >= 0.0 && Frequency <= MNyquistFrequency, "");
  MAssert(SampleRate >= 11025 && SampleRate < 16*192000, "");
  MAssert(Q >= 0.0, "");
  MAssert(DbGain >= MMinusInfInDb && DbGain < 40, "");
  
  // skip calculating coefs when nothing changes
  if (mType != Type ||
      mFrequency != Frequency ||
      mSampleRate != SampleRate ||
      mQ != Q ||
      mDbGain != DbGain)
  {
    // memorize settings
    mType = Type;
    mFrequency = Frequency;
    mSampleRate = SampleRate;
    mQ = Q;
    mDbGain = DbGain;

    // clip frequency
    TMathT<double>::Clip(Frequency, 0.0001 * SampleRate, 0.45 * SampleRate);

    // temp coeff vars
    double alpha = 0.0;
    double a0 = 0.0, a1 = 0.0, a2 = 0.0;
    double b0 = 0.0, b1 = 0.0, b2 = 0.0;

    // peaking, low shelf and hi shelf
    if (Type == kPeaking || Type == kHighShelf || Type == kLowShelf)
    {
      const double A = ::pow(10.0, DbGain / 40.0);
      const double omega = M2Pi * Frequency / SampleRate;
      const double tsin = ::sin(omega);
      const double tcos = ::cos(omega);

      #if 0
        if (QIsBandWidth)
        {
          alpha = tsin * ::sinh(::log(2.0) / 2.0 * QOrBandWidth * omega / tsin);
        }
      #endif

      alpha = tsin / (2.0 * MMax(0.04, Q));

      double const beta = ::sqrt(A) / MMax(0.04, Q);

      // peaking
      if (Type == kPeaking)
      {
        b0 = double(1.0 + alpha * A);
        b1 = double(-2.0 * tcos);
        b2 = double(1.0 - alpha * A);
        a0 = double(1.0 + alpha / A);
        a1 = double(-2.0 * tcos);
        a2 = double(1.0 - alpha / A);
      }

      // low shelf
      else if (Type == kLowShelf)
      {
        b0 = double(A * ((A + 1.0) - (A - 1.0) * tcos + beta * tsin));
        b1 = double(2.0 * A * ((A - 1.0) - (A + 1.0) * tcos));
        b2 = double(A * ((A + 1.0) - (A - 1.0) * tcos - beta * tsin));
        a0 = double((A + 1.0) + (A - 1.0) * tcos + beta * tsin);
        a1 = double(-2.0 * ((A - 1.0) + (A + 1.0) * tcos));
        a2 = double((A + 1.0) + (A - 1.0) * tcos - beta * tsin);
      }

      // hi shelf
      else if (Type == kHighShelf)
      {
        b0 = double(A * ((A + 1.0) + (A - 1.0) * tcos + beta * tsin));
        b1 = double(-2.0 * A * ((A - 1.0) + (A + 1.0) * tcos));
        b2 = double(A*((A + 1.0) + (A - 1.0) * tcos - beta * tsin));
        a0 = double((A + 1.0) - (A - 1.0) * tcos + beta * tsin);
        a1 = double(2.0 * ((A - 1.0) - (A + 1.0) * tcos));
        a2 = double((A + 1.0) - (A - 1.0) * tcos - beta * tsin);
      }

      else
      {
        MInvalid("unknown filter type");
      }
    }
    else
    {
      // other filters
      const double omega = M2Pi * Frequency / SampleRate;
      const double tsin = ::sin(omega);
      const double tcos = ::cos(omega);

      #if 0
        if (QIsBandWidth)
        {
          alpha = tsin * sinh(log(2.0) / 2.0 * QOrBandWidth * omega / tsin);
        }
      #endif

      alpha = tsin / (2.0 * MMax(0.0001, Q));


      // low pass
      if (Type == kLowPass)
      {
        /* BUTTERWORTH
        const double d = ::sqrt(2.0);

        const double beta = 0.5 * ( ( 1 - ( d / 2 ) * sin( M2Pi * (Frequency / SampleRate) ) ) / 
          ( 1 + ( d / 2 ) * sin( M2Pi * (Frequency / SampleRate) ) ) );

        double gamma = ( 0.5 + beta ) * cos ( M2Pi * (Frequency / SampleRate) );

        double alfa = ( 0.5 + beta - gamma ) / 4;

        a0 = 1;
        a1 = -2*gamma;
        a2 = 2*beta;
        b0 = 2*alfa;
        b1 = 4 * alfa;
        b2 = 2*alfa;

        SINGLE POLE
        a0 = 1.0;
        a1 = -exp(-omega);
        b0 = 1 + a1;*/

        b0 = (1.0 - tcos) / 2.0;
        b1 = 1.0 - tcos;
        b2 = (1.0 - tcos) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * tcos;
        a2 = 1.0 - alpha;
      }

      // hi pass
      else if (Type == kHighPass)
      {
        /*
        SINGLE POLE
        a0 = 1.0;
        a1 = -exp(-omega);
        b0 = (1 - a1) / 2;
        b1 = -b0;*/

        b0 = (1.0 + tcos) / 2.0;
        b1 = -(1.0 + tcos);
        b2 = (1.0 + tcos) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * tcos;
        a2 = 1.0 - alpha;
      }

      // bandpass csg
      else if (Type == kBandPassCsg)
      {
        b0 = tsin / 2.0;
        b1 = 0.0;
        b2 = -tsin / 2;
        a0 = 1.0 + alpha;
        a1 = -2.0 * tcos;
        a2 = 1.0 - alpha;
      }

      // bandpass czpg
      else if (Type == kBandPassCzpg)
      {
        b0 = alpha;
        b1 = 0.0;
        b2 = -alpha;
        a0 = 1.0+alpha;
        a1 = -2.0*tcos;
        a2 = 1.0-alpha;
      }

      // notch
      else if (Type == kNotch)
      {
        b0 = 1.0;
        b1 = -2.0 * tcos;
        b2 = 1.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * tcos;
        a2 = 1.0 - alpha;
      }

      // all pass
      else if (Type == kAllPass)
      {
        b0 = 1.0 - alpha;
        b1 = -2.0 * tcos;
        b2 = 1.0 + alpha;
        a0 = 1.0 + alpha;
        a1 = -2.0 * tcos;
        a2 = 1.0 - alpha;
      }

      else
      {
        MInvalid("unknown filter type");
      }
    }

    // set coeffs
    mA0 = a0;

    if (a0 != 0) 
    {
      mB0a0 = b0 / a0;
      mB1a0 = b1 / a0;
      mB2a0 = b2 / a0;
      mA1a0 = a1 / a0;
      mA2a0 = a2 / a0;
    }
    else
    {
      MInvalid("Invalid filter coeffs?");

      mB0a0 = 0.0;
      mB1a0 = 0.0;
      mB2a0 = 0.0;
      mA1a0 = 0.0;
      mA2a0 = 0.0;
    }
  }
}

// -------------------------------------------------------------------------------------------------

double TRbjBiquadFilter::FrequencyResponse(double Hz)const
{
  MAssert(Hz >= 0.0 && Hz <= MNyquistFrequency, "");
  TMathT<double>::Clip(Hz, 0.0001 * mSampleRate, 0.45 * mSampleRate);

  const double w = M2Pi * Hz / mSampleRate;

  double a0 = mB0a0; 
  double a1 = mB1a0; 
  double a2 = mB2a0; 
  double b1 = mA1a0; 
  double b2 = mA2a0; 

  const double CosW = ::cos(w);
  const double Cos2W = ::cos(2.0*w);
  
  const double I1 = (a0*a0 + a1*a1 + a2*a2 + 2*(a0*a1 + a1*a2)*CosW + 2*(a0*a2)*Cos2W);
  const double I2 = (1 + b1*b1 + b2*b2 + 2*(b1 + b1*b2)*CosW + 2*b2*Cos2W);
  
  if (I2 > 0.0 && I1 / I2 > 0.0)
  {
    return ::sqrt(I1 / I2);
  }
  else
  {
    return 0.0;
  }
}

// -------------------------------------------------------------------------------------------------

void TRbjBiquadFilter::ProcessSamplesDFI(
  float*  pLeftBuffer, 
  float*  pRightBuffer, 
  int     NumberOfSamples)
{
  MAssert(NumberOfSamples >= 0, "");

  while (NumberOfSamples--)
  {
    *pLeftBuffer = (float)ProcessSampleLeftDFI(*pLeftBuffer);
    MUnDenormalize(*pLeftBuffer);

    *pRightBuffer = (float)ProcessSampleRightDFI(*pRightBuffer);
    MUnDenormalize(*pRightBuffer);
    
    ++pLeftBuffer;
    ++pRightBuffer;
  }
}

// -------------------------------------------------------------------------------------------------

void TRbjBiquadFilter::ProcessSamplesDFII(
  float*  pLeftBuffer, 
  float*  pRightBuffer, 
  int     NumberOfSamples)
{
  MAssert(NumberOfSamples >= 0, "");

  while (NumberOfSamples--)
  {
    *pLeftBuffer = (float)ProcessSampleLeftDFII(*pLeftBuffer);
    MUnDenormalize(*pLeftBuffer);

    *pRightBuffer = (float)ProcessSampleRightDFII(*pRightBuffer);
    MUnDenormalize(*pRightBuffer);
    
    ++pLeftBuffer;
    ++pRightBuffer;
  }
}

// -------------------------------------------------------------------------------------------------

void TRbjBiquadFilter::FlushBuffers()
{
  mZ1Left = mZ2Left = mZ3Left = mZ4Left = 0.0f;
  mZ1Right = mZ2Right = mZ3Right = mZ4Right = 0.0f;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TTptBiquadFilter::TTptBiquadFilter()
  : mType(kInvalidType),
    mFrequency(0.0),
    mSampleRate(44100.0),
    mQ(0.0),
    mG(0.0),
    mH(0.0),
    mR2(0.0),
    mLowOut(0.0),
    mBandOut(0.0),
    mHighOut(0.0),
    mDbGain(0.0)
{
  FlushBuffers();
}

// -------------------------------------------------------------------------------------------------

void TTptBiquadFilter::Setup(
  TType   Type, 
  double  Frequency, 
  double  SampleRate, 
  double  Q, 
  double  DbGain)
{
  MAssert(Type != kInvalidType, "");
  MAssert(Frequency >= 0.0 && Frequency <= MNyquistFrequency, "");
  MAssert(SampleRate >= 11025 && SampleRate < 16*192000, "");
  MAssert(Q >= 0.0 && Q <= 4.0, "");

  // skip calculating coefs when nothing changes
  if (mType != Type ||
      mFrequency != Frequency ||
      mSampleRate != SampleRate ||
      mQ != Q ||
      mDbGain != DbGain)
  {
    // memorize settings
    mType = Type;
    mFrequency = Frequency;
    mSampleRate = SampleRate;
    mQ = Q;
    mDbGain = DbGain;

    // clip frequency
    TMathT<double>::Clip(Frequency, 0.0001 * SampleRate, 0.49 * SampleRate);

    // calc G
    mG = ::tan(MPi * Frequency / SampleRate);

    // NB: TRbjfilter's real applied max is 14.0
    const double MaxQ = 4.0; // resonance peak at ~12dB

    // calc out gains and R2
    switch (Type)
    {
    default:
      MInvalid("");

    case kLowPass:
    case kHighPass:
    {
      // min Q is 0.2
      Q = 0.2 + (Q / 4.0) * (MaxQ - 0.2);

      // Q = resonance gain
      mR2 = 1.0 / Q;

      mLowOut = (float)(Type == kLowPass);
      mBandOut = 0.0;
      mHighOut = (float)(Type == kHighPass);
      
    } break;

    case kLowShelf:
    case kHighShelf:
    {
      // min Q is 0.15
      Q = 0.150 + (Q / 4.0) * (MaxQ - 0.150);

      const double Gain = (double)TAudioMath::FastDbToLin((float)DbGain);
      
      // scale SVF-cutoff frequency for shelvers
      const double a = ::sqrt(Gain);
      mG /= ::sqrt(a);

      mR2 = 1.0 / Q;

      if (Type == kLowShelf)
      {
        mLowOut = Gain;
        mBandOut = mR2;
        mHighOut = 1.0;
      }
      else
      {
        MAssert(Type == kHighShelf, "");

        mLowOut = 1.0;
        mBandOut = mR2;
        mHighOut = Gain;
      }
    } break;

    case kBandPassCsg: // constant skirt
    {
      // min Q is 0.2
      Q = 0.2 + (Q / 4.0) * (MaxQ - 0.2);

      // Q = resonance gain
      mR2 = 1.0 / Q;

      mLowOut = 0.0;
      mBandOut = 1.0;
      mHighOut = 0.0;
    } break;

    case kBandPassCpg: // constant peak
    {
      // min Q is 0.2
      Q = 0.20 + ((4.0 - Q) / 4.0) * (MaxQ - 0.20);

      // lower bandedge frequency (in Hz)
      const double fl = Frequency * pow(2.0, -Q/2); 
      // warped radian lower bandedge frequency /(2*fs)
      const double gl = ::tan(MPi * fl / SampleRate);
      // ratio between warped lower bandedge- and center-frequencies
      const double r = gl / mG; 

      // unwarped: r = pow(2, -B/2) -> approximation for low
      // center-frequencies
      mR2 = 2.0 * sqrt((1-r*r)*(1-r*r)/(4*r*r));

      mLowOut = 0.0; 
      mBandOut = mR2; 
      mHighOut = 0.0;
      
    } break;

    case kNotch:
    case kPeaking:
    case kAllPass:
    {
      if (Type == kNotch)
      {
        // min Q is 0.3, max 7.7
        Q = 0.3 + ((4.0 - Q) / 4.0) * (2*MaxQ - 0.3);
      }
      else if (Type == kPeaking)
      {
        // min Q is 0.3
        Q = 0.3 + (Q / 4.0) * (MaxQ - 0.3);
      }

      // lower band edge frequency (in Hz)
      const double fl = Frequency * ::pow(2, -0.5 * Q);
      // warped radian lower band edge frequency /(2*fs)
      const double gl = ::tan(MPi * fl / SampleRate);
      // ratio between warped lower band edge- and center-frequencies
      // unwarped: r = 2^(-0.5 * bw) -> approximation for low center-frequencies
      const double r = (gl / mG) * (gl / mG);

      mR2 = 2.0 * ::sqrt((1.0 - r) * (1.0 - r) / (MaxQ * r));

      if (Type == kBandPassCpg)
      {
        mLowOut = 0.0;
        mBandOut = 1.0;
        mHighOut = 0.0;
      }
      else if (Type == kNotch)
      {
        mLowOut = 1.0;
        mBandOut = 0.0;
        mHighOut = 1.0;
      }
      else if (Type == kPeaking)
      {
        mLowOut = 1.0;
        mBandOut = mR2 * Q;
        mHighOut = 1.0;
      }
      else
      {
        MAssert(Type == kAllPass, "Unexpected type");

        mLowOut = 1.0;
        mBandOut = mR2;
        mHighOut = 1.0;
      }
    } break;

    }

    // calc H
    mH = 1.0 / (1.0 + mR2 * mG + mG * mG);
  }
}

// -------------------------------------------------------------------------------------------------

void TTptBiquadFilter::ProcessSamples(
  float*  pBuffer, 
  int     NumberOfSamples)
{
  MAssert(NumberOfSamples >= 0, "");

  while (NumberOfSamples--)
  {
    *pBuffer = (float)ProcessSampleLeft(*pBuffer);
    MUnDenormalize(*pBuffer);

    ++pBuffer;
  }

  // keep filter states in sync when processing mono
  mS1Right = mS1Left; 
  mS2Right = mS2Left; 
}

void TTptBiquadFilter::ProcessSamples(
  float*  pLeftBuffer, 
  float*  pRightBuffer, 
  int     NumberOfSamples)
{
  MAssert(NumberOfSamples >= 0, "");

  while (NumberOfSamples--)
  {
    *pLeftBuffer = (float)ProcessSampleLeft(*pLeftBuffer);
    MUnDenormalize(*pLeftBuffer);

    *pRightBuffer = (float)ProcessSampleRight(*pRightBuffer);
    MUnDenormalize(*pRightBuffer);

    ++pLeftBuffer;
    ++pRightBuffer;
  }
}

// -------------------------------------------------------------------------------------------------

void TTptBiquadFilter::FlushBuffers()
{
  mS1Left = mS1Right = 0.0;
  mS2Left = mS2Right = 0.0;
}

