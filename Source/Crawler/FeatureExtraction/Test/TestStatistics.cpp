#include "FeatureExtraction/Test/TestStatistics.h"

#include "FeatureExtraction/Export/Statistics.h"
#include "CoreTypes/Export/TestHelpers.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TFeatureExtractionTest::Statistics()
{
  BOOST_TEST_MESSAGE("  Testing Statistics...");

  // ... test min/max & peaks
  {
    static double sTestSequence[] = {
      1, 2, 2, 2, 0, 5, 6
    };

    static const int sTestSequenceLength =
      sizeof(sTestSequence) / sizeof(double);

    BOOST_CHECK_EQUAL(TStatistics::Min(sTestSequence, sTestSequenceLength), 0);
    BOOST_CHECK_EQUAL(TStatistics::Max(sTestSequence, sTestSequenceLength), 6);

    const TList<TPair<int, double>> Peaks1 =
      TStatistics::Peaks(sTestSequence, sTestSequenceLength, 0); // no threshold
    const TList<TPair<int, double>> Peaks2 =
      TStatistics::Peaks(sTestSequence, sTestSequenceLength, 2);

    BOOST_CHECK_EQUAL(Peaks1, MakeList(MakePair(2, 2.0), MakePair(6, 6.0)));
    BOOST_CHECK_EQUAL(Peaks2, MakeList(MakePair(6, 6.0)));
  }

  // ... test sum, variance, stddev, mean and median
  {
    static double sTestSequence[][6] = {
      { 1, 2, 3, 4, 5, 6 },
      { 6, 5, 4, 3, 2, 1 }, // reverse
      { 3, 2, 4, 6, 5, 1 }, // shuffled
      { 4, 3, 6, 5, 2, 1 }  // shuffled
    };

    static const int sNumberOfTestSequences = (int)MCountOf(sTestSequence);
    static const int sTestSequenceLength = MCountOf(sTestSequence[0]);

    for (int i = 0; i < sNumberOfTestSequences; ++i)
    {
      BOOST_CHECK_EQUAL(TStatistics::Sum(sTestSequence[i], sTestSequenceLength), 21);
      
      BOOST_CHECK_EQUAL_EPSILON(TStatistics::Variance(sTestSequence[i], sTestSequenceLength),
        2.9, 0.1);
      BOOST_CHECK_EQUAL_EPSILON(TStatistics::StandardDeviation(sTestSequence[i], sTestSequenceLength),
        1.7, 0.1);

      BOOST_CHECK_EQUAL(TStatistics::Median(sTestSequence[i], sTestSequenceLength), 3);
      BOOST_CHECK_EQUAL_EPSILON(TStatistics::Mean(sTestSequence[i], sTestSequenceLength),
        21.0 / (double)sTestSequenceLength, 1e-16);

      BOOST_CHECK_EQUAL_EPSILON(TStatistics::GeometricMean(sTestSequence[i], sTestSequenceLength),
        TStatistics::Median(sTestSequence[i], sTestSequenceLength), 1.0); // roughly
    }
  }


  // ... test centroid
  {
    static double sTestSequence[][6] = {
      { 1, 2, 3, 4, 5, 6 },
      { 1, 1, 1, 1, 6, 8 },
      { 1, 20, 4, 6, 5, 1 },
      { 1, 1, 1, 1, 1, 1 }
    };

    static const int sTestSequenceLength = (int)MCountOf(sTestSequence[0]);

    BOOST_CHECK_EQUAL_EPSILON(TStatistics::Centroid(sTestSequence[0], sTestSequenceLength),
      3.0, 1.0); // roughly in the middle
    BOOST_CHECK_EQUAL_EPSILON(TStatistics::Centroid(sTestSequence[1], sTestSequenceLength),
      4.0, 1.0); // roughly right
    BOOST_CHECK_EQUAL_EPSILON(TStatistics::Centroid(sTestSequence[2], sTestSequenceLength),
      2.0, 1.0); // roughly left
    BOOST_CHECK_EQUAL_EPSILON(TStatistics::Centroid(sTestSequence[3], sTestSequenceLength),
      2.5, 0.001); // exactly
  }


  // ... test results for single items

  {
    static double sTestValue = (double)TMath::Rand();

    BOOST_CHECK_EQUAL_EPSILON(TStatistics::Sum(&sTestValue, 1), sTestValue, 1e-12);
    BOOST_CHECK_EQUAL_EPSILON(TStatistics::Median(&sTestValue, 1), sTestValue, 1e-12);
    BOOST_CHECK_EQUAL_EPSILON(TStatistics::Variance(&sTestValue, 1), 0, 1e-12);
    BOOST_CHECK_EQUAL_EPSILON(TStatistics::StandardDeviation(&sTestValue, 1), 0, 1e-12);
    BOOST_CHECK_EQUAL_EPSILON(TStatistics::Mean(&sTestValue, 1), sTestValue, 1e-12);
    BOOST_CHECK_EQUAL_EPSILON(TStatistics::GeometricMean(&sTestValue, 1), sTestValue, 1e-12);
    BOOST_CHECK_EQUAL_EPSILON(TStatistics::Centroid(&sTestValue, 1), 0, 1e-12);
    BOOST_CHECK_EQUAL_EPSILON(TStatistics::Spread(&sTestValue, 1), 0, 1e-12);
  }


  // ... test results for empty sequences

  BOOST_CHECK_EQUAL(TStatistics::Sum(NULL, 0), 0);
  BOOST_CHECK_EQUAL(TStatistics::Median(NULL, 0), 0);
  BOOST_CHECK_EQUAL(TStatistics::Variance(NULL, 0), 0);
  BOOST_CHECK_EQUAL(TStatistics::StandardDeviation(NULL, 0), 0);
  BOOST_CHECK_EQUAL(TStatistics::Mean(NULL, 0), 0);
  BOOST_CHECK_EQUAL(TStatistics::GeometricMean(NULL, 0), 0);
  BOOST_CHECK_EQUAL(TStatistics::Centroid(NULL, 0), 0);
  BOOST_CHECK_EQUAL(TStatistics::Spread(NULL, 0), 0);
}

