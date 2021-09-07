#pragma once

#ifndef _Fourier_h_
#define _Fourier_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

template <typename TEnumType> struct SEnumStrings;

// =================================================================================================

/*
 * Perform complex to complex forward or inverse DFT transforms
!*/

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

  // internal in-place buffers, used by ForwardInplace() and InverseInplace()
  double* Re();
  double* Im();

  // process out-of-place on given buffers
  void Forward(const double* pReIn, const double* pImIn, double* pReOut, double* pImOut);
  void Inverse(const double* pReIn, const double* pImIn, double* pReOut, double* pImOut);

  // process with internal \function Re() and Im() as in and output
  void ForwardInplace();
  // process on given external Re and Im buffers as in and output
  void ForwardInplace(double* pRe, double* pIm);
  // process with internal \function Re() and Im() as in and output
  void InverseInplace();
  // process on given external Re and Im buffers as in and output
  void InverseInplace(double* pRe, double* pIm);
  
private:
  // private and not implemented. 
  TFftTransformComplex(const TFftTransformComplex& Other) = delete;
  TFftTransformComplex& operator=(const TFftTransformComplex& Other) = delete;

  void Free();

  double* mpProcessedReal;
  double* mpProcessedImag;

  #if defined(MHaveIntelIPP)
    struct FFTSpec_C_64f* mpFFTSpec;
    TUInt8* mpMemSpec;
    TUInt8* mpMemInit;
    TUInt8* mpMemBuffer;
  #elif defined(MMac)
    struct vDSP_DFT_SetupStructD* mpDFTSetupForward;
    struct vDSP_DFT_SetupStructD* mpDFTSetupInverse;
  #else
    double* mpTempInterleavedBuffer;
    double* mpTempDoubleBuffer;
    int* mpTempIntBuffer;
  #endif

  unsigned int mFftSize;
  TDivFlags mDivFlags;
};

// =================================================================================================

/*!
 * Reference complex transform implementation. Use TFftTransformComplex in production instead.
!*/

class TFftTransformComplexOouora
{
public:
  enum
  {
    kDivFwdByN = 1 << 0,
    kDivInvByN = 1 << 1,
    kNoDiv     = 1 << 3
  };
  typedef int TDivFlags;

  TFftTransformComplexOouora();
  ~TFftTransformComplexOouora();

  void Initialize(
    int       FftSize,
    bool      HighQuality = true,
    TDivFlags Flags = kNoDiv);

  void ForwardInplace();
  void ForwardInplace(double* pRe, double* pIm);
  void InverseInplace();
  void InverseInplace(double* pRe, double* pIm);

  double* Re();
  double* Im();

private:
  // private and not implemented. 
  TFftTransformComplexOouora(const TFftTransformComplexOouora& Other) = delete;
  TFftTransformComplexOouora& operator=(const TFftTransformComplexOouora& Other) = delete;

  void Free();

  double* mpProcessedReal;
  double* mpProcessedImag;

  double* mpTempInterleavedBuffer;
  double* mpTempDoubleBuffer;
  int* mpTempIntBuffer;

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

  // returns the sum of the window for normalizing purposes
  static double SFillBuffer(const TWindowType& Type, double* pBuffer, int Size);
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

