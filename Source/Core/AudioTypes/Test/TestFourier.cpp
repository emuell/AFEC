#include "AudioTypesPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"
#include "AudioTypes/Export/Fourier.h"
#include "AudioTypes/Test/TestFourier.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TAudioTypesTest::Fourier()
{
  // ... Test FFT transform 

  const int FftSize = 2048;
  TAlignedDoubleArray TestData(FftSize);
  for (int i = 0; i < FftSize; ++i) 
  {
    TestData[i] = TMath::RandFloat();
  }

  TFftTransformComplex FftTransform;
  FftTransform.Initialize(FftSize, true, TFftTransformComplex::kDivInvByN);

  TAudioMath::CopyBuffer(TestData.FirstRead(), FftTransform.Re(), FftSize);
  TAudioMath::ClearBuffer(FftTransform.Im(), FftSize);

  FftTransform.Forward();
  FftTransform.Inverse();

  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(
    TestData, TestData.Size(), 
    FftTransform.Re(), FftSize, 
    0.0001);
}

