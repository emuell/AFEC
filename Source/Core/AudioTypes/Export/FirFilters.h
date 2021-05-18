#pragma once

#ifndef _FirFilters_h_
#define _FirFilters_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

// =================================================================================================

/*!
 * A generic (and fast) mono or stereo FIR filter.
 * Taps can either be setup manually or calculated by TFirFilter.
!*/

class TFirFilter
{
public:
  //! Create coefficients with given frequencies to create a kaiser-bessel 
  //! windowed LP/HP or BP filter with the given stop band attenuation:
  //! \param pCoefs: dest coef buffer
  //! \param NumberOfTaps: number of points in filter (must be ODD)
  //! \param SampleRate: SampleRate in Hz
  //! \param FrequencyA: Lower frequency in Hz (0=low pass)
  //! \param FrequencyB: Upper frequency in Hz (SampleRate/2=high pass)
  //! \param Attenuation: Attenuation in dB for Kaiser-Bessel window (>21dB)
  static void SCalcKaiserWindowedCoeffs(
    double* pCoefs, 
    int     NumberOfTaps,
    double  SampleRate, 
    double  FrequencyA, 
    double  FrequencyB, 
    double  Attenuation);

  TFirFilter(int NumberOfTaps);
  ~TFirFilter();

  //! Number of taps.
  int NumberOfTaps()const;
  //! Introduced latency in samples.
  int Latency()const;

  //! Const access to the current filter coeffs.
  const double* Coeffs()const;
  //! Set custom filter coefs. pCoefs must be of NumberOfTaps size.
  void SetCoeffs(const double* pCoefs);

  //! Generate and srt coefficients with given frequencies to create a 
  //! kaiser-bessel windowed LP/HP or BP filter. 
  //! See SCalcKaiserWindowedCoeffs for parameter info
  void SetupCoeffs(
    double FrequencyA, 
    double FrequencyB, 
    double SampleRate, 
    double Attenuation);

  //! Process a single sample
  double ProcessSampleLeft(double InputSample);
  double ProcessSampleRight(double InputSample);

  //! Process a mono or stereo float buffer
  void ProcessSamples(
    const float*  pInputBuffer,
    float*        pOutputBuffer,
    int           NumberOfSamples);
  void ProcessSamples(
    const float*  pInputBufferLeft,
    const float*  pInputBufferRight,
    float*        pOutputBufferLeft,
    float*        pOutputBufferRight,
    int           NumberOfSamples);

  //! reset delay lines
  void FlushBuffers();

private:
  const int mNumberOfTaps;

  double mSampleRate;
  double mFrequencyA;
  double mFrequencyB;
  double mAttenuation;

  int mDelayLinePosLeft;
  int mDelayLinePosRight;

  TAlignedDoubleArray mDelayLineLeft;
  TAlignedDoubleArray mDelayLineRight;

  TAlignedDoubleArray mCoeffs;

  #if defined(MHaveIntelIPP)
    // use IPP, when available
    struct FIRSpec_64f* mpIppSpecs;
    TUInt8* mpIppSpecsBuffer;
  #endif
};

// =================================================================================================

/*!
 * FIR mono filter for up-sampling. Optimized to not calc the 0-padded samples.
 * Templated, so the compiler can optimize/unroll loops more aggressively.
!*/

template <int sFirSize, int sRatio>
class TFirUpsampler
{
public:
  TFirUpsampler();

  //! Setup FIR filter coefs. pCoefs must be of sFirSize size.
  //! NB: !! Buffer will not be copied, so make sure it stays valid as long 
  //! as the TFirUpsampler object is using them. !!
  const double* Coeffs()const;
  void SetCoeffs(const double* pCoefs);

  //! Upsample the given input sample
  double Upsample(double Sample);
  //! Upsample a zero sample (interleaving),
  //! Z being the time in samples since the last non-0 sample.
  double Pad(int Z);

  //! Flush buffers
  void FlushBuffers();

private:
  // Make buffer a pow2 value, so we can wrap indices more easily
  enum {
    kBufferSize = TNextPowerOfTwo<sFirSize>::sValue,
    kBufferWrap = kBufferSize - 1
  };

  // FIR coefficients pointer
  const double* mpCoeffs;

  // history
  double mBuffer[kBufferSize];
  int mBufferIndex; 
};

// =================================================================================================

/*!
 * FIR mono filter for down-sampling. Optimized to not calc the 0-padded samples.
 * Templated, so the compiler can optimize/unroll loops more aggressively. 
!*/

template <int sFirSize>
class TFirDownsampler
{
public:
  TFirDownsampler();
  
  //! Setup FIR filter coefs. pCoefs must be of sFirSize size.
  //! NB: !! Buffer will not be copied, so make sure it stays valid as long 
  //! as the TFirDownsampler object is using them. !!
  const double* Coeffs()const;
  void SetCoeffs(const double* pCoefs);

  //! Get downsampled first sample
  double Downsample(double Sample);
  //! Flush unused N-1 samples after fetching sample with "Downsample"
  void Flush(double Sample);

  //! Flush buffers
  void FlushBuffers();
  //! Copy history buffers from another downsampler instance
  void CopyBuffers(const TFirDownsampler<sFirSize>& Other);

private:
  // Make buffer a pow2 value, so we can wrap indices more easily
  enum {
    kBufferSize = TNextPowerOfTwo<sFirSize>::sValue,
    kBufferWrap = kBufferSize - 1
  };

  // FIR coefficients pointer
  const double* mpCoeffs;

  // history
  double mBuffer[kBufferSize];
  // index in history
  int mBufferIndex; 
};

// =================================================================================================

#include "AudioTypes/Export/FirFilters.inl"


#endif // _Filters_h_

