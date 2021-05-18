#pragma once

#ifndef _Statistics_h_
#define _Statistics_h_

// =================================================================================================

#include "CoreTypes/Export/Pair.h"

// =================================================================================================

/*!
 * Simple, basic statistics for double arrays, as used by the feature extractors.
!*/

namespace TStatistics
{
  // batch calculate a statistic set of the given array
  void Calc(
    double&       Min,
    double&       Max,
    double&       Median,
    double&       Mean,
    double&       GeometricMean,
    double&       Variance,
    double&       Centroid,
    double&       Spread,
    double&       Skewness,
    double&       Kurtosis,
    double&       Flatness,
    double&       DMean,
    double&       DVariance,
    const double* pX,
    int           ValuesSize);

  // calculate min/max of the given array
  double Min(const double* pX, int Length);
  double Max(const double* pX, int Length);

  // find and return all local peaks in the given array which are > threshold.
  // returned pairs "First" is the peak index, "Second" the peak value.
  TList< TPair<int, double> > Peaks(
    const double* pArray,
    int           ArraySize,
    double        Threshold);

  // calculate sum of all elements in the given array
  double Sum(const double* pX, int Length);

  // calculate median and means of the given array
  double Median(const double* pX, int Length);
  double Mean(const double* pX, int Length);
  double GeometricMean(const double* pX, int Length);

  // calculate variance of the given array
  double Variance(const double* pX, int Length); // calculates mean
  double Variance(const double* pX, int Length, double Mean);
  // calculate standard deviation of the given array
  double StandardDeviation(const double* pX, int Length); // calculates mean
  double StandardDeviation(const double* pX, int Length, double Mean);

  // calculate central moments of the given array
  double Centroid(const double* pX, int Length);
  double Spread(const double* pX, int Length); // calculates centroid
  double Spread(const double* pX, int Length, double Centroid);

  // calculate skewness and spread with the given centroid and spread
  double Skewness(const double* pX, int Length, double Centroid, double Spread);
  double Kurtosis(const double* pX, int Length, double Centroid, double Spread);

  // calculate flatness factor for given array
  double Flatness(const double* pX, int Length);  // calculates means
  double Flatness(const double* pX, int Length, double ArithmeticMean, double GeometricMean);

  // calculate flux of two given arrays
  double Flux(const double* pX1, const double* pX2, int Length);

  // pearson correlation value of two given arrays [-1 - 1]
  double Correlation(const double* pX1, const double* pX2, int Length);
}

#endif // _Statistics_h_

