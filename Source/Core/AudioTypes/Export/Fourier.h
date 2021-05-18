#pragma once

#ifndef _Fourier_h_
#define _Fourier_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

template <typename TEnumType> struct SEnumStrings;

// =================================================================================================

class TFftTransformComplex
{
public:
  enum
  {
    kDivFwdByN = 1 << 0,
    kDivInvByN = 1 << 1,
    kNoDiv     = 1 << 3
  };
  typedef int TDivFlags;

  TFftTransformComplex();
  ~TFftTransformComplex();

  void Initialize(
    int       FftSize,
    bool      HighQuality = true,
    TDivFlags Flags = kNoDiv);

  void Forward();
  void Inverse();

  double* Re();
  double* Im();

private:
  // private and not implemented. 
  TFftTransformComplex(const TFftTransformComplex& Other);
  TFftTransformComplex& operator=(const TFftTransformComplex& Other);

  void Free();

  double* mpProcessedReal;
  double* mpProcessedImag;

  #if defined(MHaveIntelIPP)
    struct FFTSpec_C_64f* mpFFTSpec;
    TUInt8* mpMemSpec;
    TUInt8* mpMemInit;
    TUInt8* mpMemBuffer;
  #else
    double* mpTempDoubleBuffer;
    int* mpTempIntBuffer;
  #endif

  unsigned int mFftSize;
  TDivFlags mDivFlags;
};

// =================================================================================================

class TFftWindow
{
public:
  enum TWindowType
  {
    kInvalidType = -1,

    kNone,
    kRectangular,
    kRaisedCosine,
    kHamming,
    kHanning,
    kBlackman,
    kBlackmanHarris,
    kGaussian,
    kFlatTop,

    kNumberOfWindowTypes
  };

  template<typename SampleType>
  static SampleType SFillBuffer(const TWindowType& Type, SampleType* pBuffer, int Size);
};

// =================================================================================================

template <> struct SEnumStrings<enum TFftWindow::TWindowType>
{
  operator TList<TString>()const
  {
    MStaticAssert(TFftWindow::kNumberOfWindowTypes == 9);

    TList<TString> Ret;
    Ret.Append("None");
    Ret.Append("Rectangular");
    Ret.Append("Raised Cosine");
    Ret.Append("Hamming");
    Ret.Append("Hanning");
    Ret.Append("Blackman");
    Ret.Append("Blackman-Harris");
    Ret.Append("Gaussian");
    Ret.Append("Flat-Top");

    return Ret;
  }
};

// =================================================================================================

#include "AudioTypes/Export/Fourier.inl"


#endif // _Fourier_h_

