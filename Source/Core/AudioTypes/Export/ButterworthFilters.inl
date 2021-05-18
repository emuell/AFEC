#pragma once

#ifndef _ButterworthFilters_inl_
#define _ButterworthFilters_inl_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"
#include "CoreTypes/Export/InlineMath.h"

#include <cmath>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
TButterworthFilter<sNumOrders>::TButterworthFilter()
: mType(kInvalidType),
  mFrequency(0.0),
  mSampleRate(0.0),
  mQ(0.0),
  mDbGain(0.0)
{
  mShelfGain = 0.0;

  for (int s = 0; s < kNumStages; ++s)
  {
    mB0a0[s] = mB1a0[s] = mB2a0[s] = 0.0;
    mA1a0[s] = mA2a0[s] = 0.0;
  }

  FlushBuffers();
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
MForceInline typename TButterworthFilter<sNumOrders>::TType 
  TButterworthFilter<sNumOrders>::Type()const 
{ 
  return mType; 
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
MForceInline double TButterworthFilter<sNumOrders>::Frequency()const
{
  return mFrequency;
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
MForceInline double TButterworthFilter<sNumOrders>::SampleRate()const
{
  return mSampleRate;
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
MForceInline double TButterworthFilter<sNumOrders>::Q()const
{
  return mQ;
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
MForceInline double TButterworthFilter<sNumOrders>::DbGain()const
{
  return mDbGain;  
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
MForceInline double TButterworthFilter<sNumOrders>::ProcessSampleLeftDFI(double Input)
{
  const double OldInput = Input;

  double Output = 0.0;

  for (int s = 0; s < kNumStages; ++s)
  {
    Output = mB0a0[s] * Input + mB1a0[s] * mZ1Left[s] + mB2a0[s] * mZ2Left[s] - 
      mA1a0[s] * mZ3Left[s] - mA2a0[s] * mZ4Left[s];
    MUnDenormalize(Output);

    mZ2Left[s] = mZ1Left[s];
    mZ1Left[s] = Input;
    mZ4Left[s] = mZ3Left[s];
    mZ3Left[s] = Output;

    Input = Output;
  }

  double y = Output + (OldInput - Output) * mShelfGain;
  MUnDenormalize(y);

  return y;
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
MForceInline double TButterworthFilter<sNumOrders>::ProcessSampleRightDFI(double Input)
{
  const double OldInput = Input;

  double Output = 0.0;

  for (int s = 0; s < kNumStages; ++s)
  {
    Output = mB0a0[s] * Input + mB1a0[s]* mZ1Right[s] + mB2a0[s] * mZ2Right[s] - 
      mA1a0[s] * mZ3Right[s] - mA2a0[s] * mZ4Right[s];
    MUnDenormalize(Output);

    mZ2Right[s] = mZ1Right[s];
    mZ1Right[s] = Input;
    mZ4Right[s] = mZ3Right[s];
    mZ3Right[s] = Output;

    Input = Output;
  }

  double y = Output + (OldInput - Output) * mShelfGain;
  MUnDenormalize(y);

  return y;
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
MForceInline double TButterworthFilter<sNumOrders>::ProcessSampleLeftDFII(double Input)
{
  const double OldInput = Input;

  double Output = Input;
  for (int s = 0; s < kNumStages; ++s)
  {
    Output = mB0a0[s]*Input + mZ1Left[s];
    MUnDenormalize(Output);
    
    mZ1Left[s] = mB1a0[s]*Input - mA1a0[s]*Output + mZ2Left[s];
    MUnDenormalize(mZ1Left[s]);
    
    mZ2Left[s] = mB2a0[s]*Input - mA2a0[s]*Output;
    MUnDenormalize(mZ2Left[s]);

    Input = Output;
  }

  double y = Output + (OldInput - Output) * mShelfGain;
  MUnDenormalize(y);

  return y;
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
MForceInline double TButterworthFilter<sNumOrders>::ProcessSampleRightDFII(double Input)
{
  const double OldInput = Input;

  double Output = 0.0;
  for (int s = 0; s < kNumStages; ++s)
  {
    Output = mB0a0[s]*Input + mZ1Right[s];
    MUnDenormalize(Output);

    mZ1Right[s] = mB1a0[s]*Input - mA1a0[s]*Output + mZ2Right[s];
    MUnDenormalize(mZ1Right[s]);

    mZ2Right[s] = mB2a0[s]*Input - mA2a0[s]*Output;
    MUnDenormalize(mZ2Right[s]);

    Input = Output;
  }

  double y = Output + (OldInput - Output) * mShelfGain;
  MUnDenormalize(y);

  return y;
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
double TButterworthFilter<sNumOrders>::FrequencyResponse(double Hz)const
{
  MAssert(Hz >= 0.0 && Hz <= 22050, "");
  TMathT<double>::Clip(Hz, 0.0001 * mSampleRate, 0.49 * mSampleRate);

  const double w = M2Pi * Hz / mSampleRate;

  double Response = 1.0;
  for (int s = 0; s < kNumStages; ++s)
  {
    double a0 = mB0a0[s]; 
    double a1 = mB1a0[s]; 
    double a2 = mB2a0[s]; 
    double b1 = mA1a0[s]; 
    double b2 = mA2a0[s]; 

    const double CosW = ::cos(w);
    const double Cos2W = ::cos(2.0*w);

    const double I1 = (a0*a0 + a1*a1 + a2*a2 + 2*(a0*a1 + a1*a2)*CosW + 2*(a0*a2)*Cos2W);
    const double I2 = (1 + b1*b1 + b2*b2 + 2*(b1 + b1*b2)*CosW + 2*b2*Cos2W);

    if (I2 > 0.0 && I1 / I2 > 0.0)
    {
      Response *= ::sqrt(I1 / I2);
    }
  }

  return Response + (1 - Response) * mShelfGain;
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
void TButterworthFilter<sNumOrders>::Setup(
  TType   Type,
  double  Frequency,
  double  SampleRate,
  double  Q,
  double  DbGain)
{
  MAssert(Type != kInvalidType, "");
  MAssert(Frequency >= 0.0 && Frequency <= 22050, "");
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
    const bool TypeChanged = (mType != Type);
    
    
    // . memorize settings

    mType = Type;
    mFrequency = Frequency;
    mSampleRate = SampleRate;
    mQ = Q;
    mDbGain = DbGain;


    // . set linear ShelfGain

    if (mType == kHighShelf || mType == kLowShelf)
    {
      mShelfGain = (double)TAudioMath::FastDbToLin((float)DbGain);
    }
    else
    {
      mShelfGain = 0.0;
    }


    // . clip frequency & Q

    // clip to sample rate and don't go below non audible HZ ranges
    TMathT<double>::Clip(Frequency, 0.0001 * SampleRate, 0.49 * SampleRate);
    
    // 0.08 min avoids DC offset "wobbles" when automating BP frequencies with low Qs
    TMathT<double>::Clip(Q, 0.08, 20.0);

    const double omega = M2Pi * Frequency / SampleRate;

    for (int s = 0; s < kNumStages; ++s)
    {
      double a0 = 0.0, a1 = 0.0, a2 = 0.0;
      double b0 = 0.0, b1 = 0.0, b2 = 0.0;

      // . kLowPass & kHighPass coeffs

      if (mType == kLowPass || mType == kHighPass || 
          mType == kLowShelf || mType == kHighShelf)
      {
        const double d = 2 * ::sin(((2.0*(s + 1) - 1) * MPi) / (2 * sNumOrders)); 

        const double beta = 0.5 * (( 1.0 - (d / 2.0) * ::sin(omega)) / 
          (1.0 + (d / 2.0) * ::sin(omega)));

        const double gamma = (0.5 + beta) * ::cos(omega);

        if (mType == kLowPass || mType == kHighShelf)
        {
          const double alpha = (0.5 + beta - gamma) / 4.0;

          a0 = 1;
          a1 = -2*gamma; 
          a2 = 2*beta;   
          b0 = 2*alpha;
          b1 = 4*alpha;
          b2 = 2*alpha;
        }
        else
        {
          MAssert(mType == kHighPass || mType == kLowShelf, "");

          const double alpha = (0.5 + beta + gamma) / 4.0;

          a0 = 1;
          a1 = -2*gamma; 
          a2 = 2*beta;   
          b0 = 2*alpha;
          b1 = -4*alpha;
          b2 = 2*alpha;
        }
      }

      // . kBandPass & kBandStop coeffs

      else
      {
        // limit Q so that the upper freq wont get invalid
        if (omega / Q > MPi / 2.0) 
        {
          Q = omega / (MPi / 2.0);
        }

        const double dE = ( 2.0 * ::tan(omega / (2.0 * Q))) / ::sin(omega);
        const double Dk = 2.0 * ::sin((((2.0 * (s + 1)) - 1.0) * MPi) / (2.0 * kNumStages));
        const double Ak = (1.0 + TMathT<double>::Square(dE / 2.0)) / (Dk * dE / 2.0);
        const double dk = ::sqrt((dE * Dk) / (Ak + ::sqrt(TMathT<double>::Square(Ak) - 1.0)));
        const double Bk = Dk * (dE / 2.0) / dk;
        const double Wk = Bk + ::sqrt(TMathT<double>::Square(Bk) - 1.0);

        const double theta_k = ((s & 1) == 0) ? 
          2.0 * ::atan((::tan(omega / 2.0)) * Wk) : 
          2.0 * ::atan((::tan(omega / 2.0)) / Wk);

        const double beta = 0.5 * (1.0 - (dk / 2.0)* ::sin(theta_k)) / 
          (1.0 + (dk / 2.0)* ::sin(theta_k));

        const double gamma = (0.5 + beta) * ::cos(theta_k);

        if (mType == kBandPass)
        {
          const double alpha = 0.5 * (0.5 - beta) * 
            ::sqrt(1.0 + TMathT<double>::Square((Wk - (1.0 / Wk)) / dk));

          a0 = 1;
          a1 = -2*gamma; 
          a2 = 2*beta;   
          b0 = 2*alpha;
          b1 = 0;
          b2 = -2*alpha;
        }
        else
        {
          MAssert(mType == kBandStop, "");

          const double alpha = 0.5 * (0.5 + beta) * 
            ((1 - ::cos(theta_k)) / (1 - ::cos(omega)));

          a0 = 1;
          a1 = -2*gamma; 
          a2 = 2*beta;   
          b0 = 2*alpha;
          b1 = -4*alpha * ::cos(omega);
          b2 = 2*alpha;
        }
      }


      // . set coeffs

      const double a0Inv = 1.0 / a0;
      
      mB0a0[s] = b0 * a0Inv;
      mB1a0[s] = b1 * a0Inv;
      mB2a0[s] = b2 * a0Inv;
      mA1a0[s] = a1 * a0Inv;
      mA2a0[s] = a2 * a0Inv;
    }


    // . flush buffers on type changes

    if (TypeChanged)
    {
      FlushBuffers();
    }
  }
}

// -------------------------------------------------------------------------------------------------

template <int sNumOrders>
void TButterworthFilter<sNumOrders>::ProcessSamplesDFI(
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

template <int sNumOrders>
void TButterworthFilter<sNumOrders>::ProcessSamplesDFII(
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

template <int sNumOrders>
void TButterworthFilter<sNumOrders>::FlushBuffers()
{
  for (int s = 0; s < kNumStages; ++s)
  {
    mZ1Left[s] = mZ2Left[s] = mZ3Left[s] = mZ4Left[s] = 0.0f;
    mZ1Right[s] = mZ2Right[s] = mZ3Right[s] = mZ4Right[s] = 0.0f;
  }
}


#endif // _ButterworthFilters_inl_

