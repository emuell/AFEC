#include "AudioTypesPrecompiledHeader.h"

#include "CoreTypes/Export/InlineMath.h"
#include "CoreTypes/Export/TestHelpers.h"
#include "AudioTypes/Export/Fourier.h"
#include "AudioTypes/Test/TestFourier.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TAudioTypesTest::Fourier()
{
  // ... Test FFT transform 

  constexpr int sFftSize = 2048; // 16
  double TestData[sFftSize];
  #if 1 // test on random data (noise)
    for (int i = 0; i < sFftSize; ++i)
    {
      TestData[i] = ((double)TMath::RandFloat() - 0.5) * 2.0;
    }
  #else // test on zero padded impulse
    TestData[0] = 1.0;
    for (int i = 1; i < sFftSize; ++i)
    {
      TestData[i] = 0.0;
    }
  #endif

  double TestDataCopy[sFftSize];
  TAudioMath::CopyBuffer(TestData, TestDataCopy, sFftSize);

  // . Complex
  {
    double BufferOouoraRe[sFftSize];
    double BufferOouoraIm[sFftSize];
    double BufferRe[sFftSize];
    double BufferIm[sFftSize];

    TFftTransformComplexOouora FftTransformComplexOouora;
    FftTransformComplexOouora.Initialize(sFftSize, true, TFftTransformComplex::kDivInvByN);

    TFftTransformComplex FftTransformComplex;
    FftTransformComplex.Initialize(sFftSize, true, TFftTransformComplex::kDivInvByN);

    TAudioMath::CopyBuffer(TestData, BufferOouoraRe, sFftSize);
    TAudioMath::ClearBuffer(BufferOouoraIm, sFftSize);

    TAudioMath::CopyBuffer(TestData, BufferRe, sFftSize);
    TAudioMath::ClearBuffer(BufferIm, sFftSize);

    // forward transform
    FftTransformComplexOouora.ForwardInplace(BufferOouoraRe, BufferOouoraIm);
    FftTransformComplex.ForwardInplace(BufferRe, BufferIm);

    // check if ooura and other impl behave the same
    BOOST_CHECK_ARRAYS_EQUAL_EPSILON(
      BufferOouoraRe, sFftSize,
      BufferRe, sFftSize,
      0.0001);

    // Im may have different signs...
    // BOOST_CHECK_ARRAYS_EQUAL_EPSILON(
    //   ProcessedBufferOouoraIm, sFftSize,
    //   ProcessedBufferIm, sFftSize,
    //   0.001);

    // inverse transform
    FftTransformComplexOouora.InverseInplace(BufferOouoraRe, BufferOouoraIm);
    FftTransformComplex.InverseInplace(BufferRe, BufferIm);

    // check if we got the original signal again
    BOOST_CHECK_ARRAYS_EQUAL_EPSILON(
      TestDataCopy, sFftSize,
      BufferRe, sFftSize,
      0.0001);

    BOOST_CHECK_ARRAYS_EQUAL_EPSILON(
      TestDataCopy, sFftSize,
      BufferOouoraRe, sFftSize,
      0.0001);
  }
 }
