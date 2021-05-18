#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"

#include "CoreTypes/Test/TestInlineMath.h"
#include "CoreTypes/Export/InlineMath.h"

// =================================================================================================

struct TQuantizeModeTestRecord 
{
  double mValue;
  double mQuantum;
  TMath::TQuantizeMode mMode;
  double mResult;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreTypesTest::InlineMath()
{
  // ... fti and d2i

  BOOST_CHECK_EQUAL(TMath::f2i(2.0f), 2);
  BOOST_CHECK_EQUAL(TMath::f2i(-2.0f), -2);
  BOOST_CHECK_EQUAL(TMath::f2i(1.8f), 1);
  BOOST_CHECK_EQUAL(TMath::f2i(-1.8f), -1);
  
  BOOST_CHECK_EQUAL(TMath::f2iRound(2.0f), 2);
  BOOST_CHECK_EQUAL(TMath::f2iRound(-2.0f), -2);
  BOOST_CHECK_EQUAL(TMath::f2iRound(1.5f), 2);
  BOOST_CHECK_EQUAL(TMath::f2iRound(-1.5f), -2);

  BOOST_CHECK_EQUAL(TMath::f2iRoundUp(2.0f), 2);
  BOOST_CHECK_EQUAL(TMath::f2iRoundUp(-2.0f), -2);
  BOOST_CHECK_EQUAL(TMath::f2iRoundUp(1.25f), 2);
  BOOST_CHECK_EQUAL(TMath::f2iRoundUp(-1.25f), -1);

  BOOST_CHECK_EQUAL(TMath::f2iRoundDown(2.0f), 2);
  BOOST_CHECK_EQUAL(TMath::f2iRoundDown(-2.0f), -2);
  BOOST_CHECK_EQUAL(TMath::f2iRoundDown(1.25f), 1);
  BOOST_CHECK_EQUAL(TMath::f2iRoundDown(-1.25f), -2);


  BOOST_CHECK_EQUAL(TMath::d2i(2.0), 2);
  BOOST_CHECK_EQUAL(TMath::d2i(-2.0), -2);
  BOOST_CHECK_EQUAL(TMath::d2i(1.8), 1);
  BOOST_CHECK_EQUAL(TMath::d2i(-1.8), -1);

  BOOST_CHECK_EQUAL(TMath::d2iRound(2.0), 2);
  BOOST_CHECK_EQUAL(TMath::d2iRound(-2.0), -2);
  BOOST_CHECK_EQUAL(TMath::d2iRound(1.5), 2);
  BOOST_CHECK_EQUAL(TMath::d2iRound(-1.5), -2);

  BOOST_CHECK_EQUAL(TMath::d2iRoundUp(2.0), 2);
  BOOST_CHECK_EQUAL(TMath::d2iRoundUp(-2.0), -2);
  BOOST_CHECK_EQUAL(TMath::d2iRoundUp(1.25), 2);
  BOOST_CHECK_EQUAL(TMath::d2iRoundUp(-1.25), -1);

  BOOST_CHECK_EQUAL(TMath::d2iRoundDown(2.0), 2);
  BOOST_CHECK_EQUAL(TMath::d2iRoundDown(-2.0), -2);
  BOOST_CHECK_EQUAL(TMath::d2iRoundDown(1.25), 1);
  BOOST_CHECK_EQUAL(TMath::d2iRoundDown(-1.25), -2);


  // ... Quantize

  MStaticAssert(TMath::kNumberOfQuantizeModes == 4);

  static const TQuantizeModeTestRecord sQuantizeTests[] = {
    {33.0, 1.0, TMath::kChop, 33.0}, 
    {33.0, 1.0, TMath::kRoundUp, 33.0}, 
    {33.0, 1.0, TMath::kRoundDown, 33.0}, 
    {33.0, 1.0, TMath::kRoundToNearest, 33.0},

    {-33.0, 1.0, TMath::kChop, -33.0}, 
    {-33.0, 1.0, TMath::kRoundUp, -33.0}, 
    {-33.0, 1.0, TMath::kRoundDown, -33.0}, 
    {-33.0, 1.0, TMath::kRoundToNearest, -33.0},

    {33.5, 1.0, TMath::kChop, 33.0}, 
    {33.5, 1.0, TMath::kRoundUp, 34.0}, 
    {33.5, 1.0, TMath::kRoundDown, 33.0}, 
    {33.5, 1.0, TMath::kRoundToNearest, 34.0},

    {-33.5, 1.0, TMath::kChop, -33.0}, 
    {-33.5, 1.0, TMath::kRoundUp, -33.0}, 
    {-33.5, 1.0, TMath::kRoundDown, -34.0}, 
    {-33.5, 1.0, TMath::kRoundToNearest, -34.0},

    {17.25, 0.5, TMath::kChop, 17.0}, 
    {17.25, 0.5, TMath::kRoundUp, 17.5}, 
    {17.25, 0.5, TMath::kRoundDown, 17.0}, 
    {17.25, 0.5, TMath::kRoundToNearest, 17.5},

    {-17.25, 0.5, TMath::kChop, -17.0}, 
    {-17.25, 0.5, TMath::kRoundUp, -17.0}, 
    {-17.25, 0.5, TMath::kRoundDown, -17.5}, 
    {-17.25, 0.5, TMath::kRoundToNearest, -17.5}
  };

  for (size_t i = 0; i < MCountOf(sQuantizeTests); ++i)
  {
    const TQuantizeModeTestRecord& TestRecord = sQuantizeTests[i];

    double Value = TestRecord.mValue;
    TMath::Quantize(Value, TestRecord.mQuantum, TestRecord.mMode);
    BOOST_CHECK_EQUAL(Value, TestRecord.mResult);

    float FloatValue = (float)TestRecord.mValue;
    TMath::Quantize(FloatValue, (float)TestRecord.mQuantum, TestRecord.mMode);
    BOOST_CHECK_EQUAL(FloatValue, (float)TestRecord.mResult);

    if ((double)(int)(TestRecord.mValue) == TestRecord.mValue && 
        (double)(int)(TestRecord.mQuantum) == TestRecord.mQuantum)
    {
      int IntValue = (int)TestRecord.mValue;
      TMath::Quantize(IntValue, (int)TestRecord.mQuantum, TestRecord.mMode);
      BOOST_CHECK_EQUAL(IntValue, (int)TestRecord.mResult);
    }
  }
}

