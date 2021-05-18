#include "CoreTypes/Export/Array.h"
#include "CoreTypes/Export/InlineMath.h"

#include "FeatureExtraction/Export/Statistics.h"

#include <limits> 

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TStatistics::Calc(
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
  double&       AbsDMean,
  double&       AbsDVariance,
  const double* pX,
  int           Length)
{
  if (Length > 1)
  {
    // min & max
    Min = TStatistics::Min(pX, Length);
    Max = TStatistics::Max(pX, Length);

    // mean & variance
    Median = TStatistics::Median(pX, Length);
    Mean = TStatistics::Mean(pX, Length);
    GeometricMean = TStatistics::GeometricMean(pX, Length);
    Variance = TStatistics::Variance(pX, Length, Mean);

    // centroid, spread & co
    Centroid = TStatistics::Centroid(pX, Length);
    Spread = TStatistics::Spread(pX, Length, Centroid);
    Skewness = TStatistics::Skewness(pX, Length, Centroid, Spread);
    Kurtosis = TStatistics::Kurtosis(pX, Length, Centroid, Spread);
    
    // flatness
    Flatness = TStatistics::Flatness(pX, Length, Mean, GeometricMean);

    // derived min, max, mean, var and sum
    if (Length > 2)
    {
      TArray<double> Derived(Length - 1);
      TArray<double> AbsDerived(Length - 1);
      for (int i = 0; i < Length - 1; ++i)
      {
        Derived[i] = pX[i + 1] - pX[i];
        AbsDerived[i] = TMathT<double>::Abs(Derived[i]);
      }

      AbsDMean = TStatistics::Mean( // Abs derivate
        AbsDerived.FirstRead(), AbsDerived.Size());
      AbsDVariance = TStatistics::Variance(
        AbsDerived.FirstRead(), AbsDerived.Size(), AbsDMean);
    }
    else
    {
      AbsDMean = 0.0;
      AbsDVariance = 0.0;
    }
  }
  else if (Length > 0)
  {
    Min = pX[0];
    Max = pX[0];
    Mean = pX[0];
    Variance = 0.0;
    AbsDMean = 0.0;
    AbsDVariance = 0.0;
  }
  else
  {
    Min = 0.0;
    Max = 0.0;
    Mean = 0.0;
    Variance = 0.0;
    AbsDMean = 0.0;
    AbsDVariance = 0.0;
  }
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Min(const double* pX, int Length)
{
  if (Length > 0)
  {
    double Min = pX[0];
    for (int i = 1; i < Length; ++i)
    {
      Min = MMin(Min, pX[i]);
    }

    return Min;
  }
  else
  {
    MInvalid("Should check for empty arrays "
      "and deal with them as appropriate for calling context");

    return 0.0;
  }
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Max(const double* pX, int Length)
{
  if (Length > 0)
  {
    double Max = pX[0];
    for (int i = 1; i < Length; ++i)
    {
      Max = MMax(Max, pX[i]);
    }

    return Max;
  }
  else
  {
    MInvalid("Should check for empty arrays "
      "and deal with them as appropriate for calling context");

    return 0.0;
  }
}

// -------------------------------------------------------------------------------------------------

TList< TPair<int, double> > TStatistics::Peaks(
  const double* pArray,
  int           ArraySize,
  double        Threshold)
{
  TList< TPair<int, double> > Peaks;

  // return empty handed when the array is too small
  if (ArraySize <= 2)
  {
    return Peaks;
  }

  int i = 0;

  // first check the boundaries
  if (i + 1 < ArraySize && pArray[i] > pArray[i + 1])
  {
    if (pArray[i] > Threshold)
    {
      Peaks.Append(MakePair(i, pArray[i]));
    }
  }

  // loop until all peaks got found
  for (MEver)
  {
    // going down
    while (i + 1 < ArraySize - 1 && pArray[i] >= pArray[i + 1])
    {
      i++;
    }

    // now we're climbing
    while (i + 1 < ArraySize - 1 && pArray[i] < pArray[i + 1])
    {
      i++;
    }

    // not anymore, go through the plateau
    int j = i;
    while (j + 1 < ArraySize - 1 && (pArray[j] == pArray[j + 1]))
    {
      j++;
    }

    // end of plateau, do we go up or down?
    if (j + 1 < ArraySize - 1 && pArray[j + 1] < pArray[j] && pArray[j] > Threshold)
    {
      // going down again
      int resultBin = 0;
      double resultVal = 0.0;

      if (j != i)
      { // plateau peak between i and j
        resultBin = (i + j) / 2;
        resultVal = pArray[i];
      }
      else
      { // interpolate peak at i-1, i and i+1
        resultBin = j;
        resultVal = pArray[j];
      }

      Peaks.Append(MakePair(resultBin, resultVal));
    }

    // nothing found, start loop again
    i = j;

    if (i + 1 >= ArraySize - 1)
    { // check the one just before the last position
      if (i == ArraySize - 2 && pArray[i - 1] < pArray[i] &&
        pArray[i + 1] < pArray[i] &&
        pArray[i] > Threshold)
      {
        Peaks.Append(MakePair(i, pArray[i]));
      }
      break;
    }
  }

  // check last value
  if (pArray[ArraySize - 1] > pArray[ArraySize - 2])
  {
    if (pArray[ArraySize - 1] > Threshold)
    {
      Peaks.Append(MakePair(ArraySize - 1, pArray[ArraySize - 1]));
    }
  }

  return Peaks;
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Sum(const double* pX, int Length)
{
  double Sum = 0.0;
  for (int i = 0; i < Length; ++i)
  {
    Sum += pX[i];
  }

  return Sum;
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Mean(const double* pX, int Length)
{
  if (Length >= 2)
  {
    return Sum(pX, Length) / (double)Length;
  }
  else if (Length == 1)
  {
    return pX[0];
  }
  else // Lenght == 0
  {
    MInvalid("Should check for empty arrays "
      "and deal with them as appropriate for calling context");

    return 0.0;
  }
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Variance(const double* pX, int Length)
{
  return Variance(pX, Length, Mean(pX, Length));
}

double TStatistics::Variance(const double* pX, int Length, double Mean)
{
  if (Length >= 2)
  {
    double Result = 0.0;
    for (int i = 0; i < Length; ++i)
    {
      Result += TMathT<double>::Square(pX[i] - Mean);
    }

    Result = Result / (Length);

    return Result;
  }
  else if (Length == 1)
  {
    return 0.0;
  }
  else  // ( Lenght == 0 )
  {
    MInvalid("Should check for empty arrays "
      "and deal with them as appropriate for calling context");

    return 0.0;
  }
}

// -------------------------------------------------------------------------------------------------

double TStatistics::StandardDeviation(const double* pX, int Length)
{
  return ::sqrt(TStatistics::Variance(pX, Length));
}

double TStatistics::StandardDeviation(const double* pX, int Length, double Mean)
{ 
  return ::sqrt(TStatistics::Variance(pX, Length, Mean));
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Median(const double* pX, int Length)
{
  if (Length >= 2)
  {
    // This routine is based on the algorithm described in "Numerical recipes in C", 
    // Second Edition, http://ndevilla.free.fr/Median/Median/index.html

    TArray<double> TempArray(Length);
    TMemory::Copy(TempArray.FirstWrite(), pX, Length * sizeof(double));

    int Low = 0;
    int High = Length - 1;
    int Median = (Low + High) / 2;

    for (;;)
    {
      // One element only
      if (High <= Low)
      {
        return TempArray[Median];
      }

      // Two elements only
      if (High == Low + 1)
      {
        if (TempArray[Low] > TempArray[High])
        {
          TMathT<double>::Swap(TempArray[Low], TempArray[High]);
        }

        return TempArray[Median];
      }

      // Find Median of Low, Middle and High items; swap into position Low
      int Middle = (Low + High) / 2;

      if (TempArray[Middle] > TempArray[High])
      {
        TMathT<double>::Swap(TempArray[Middle], TempArray[High]);
      }

      if (TempArray[Low] > TempArray[High])
      {
        TMathT<double>::Swap(TempArray[Low], TempArray[High]);
      }

      if (TempArray[Middle] > TempArray[Low])
      {
        TMathT<double>::Swap(TempArray[Middle], TempArray[Low]);
      }

      // Swap Low item (now in position Middle) into position (Low+1)
      TMathT<double>::Swap(TempArray[Middle], TempArray[Low + 1]);

      // Nibble from each end towards Middle, swapping items when stuck
      int ll = Low + 1;
      int hh = High;

      for (;; )
      {
        do { ll++; } while (TempArray[Low] > TempArray[ll]);
        do { hh--; } while (TempArray[hh] > TempArray[Low]);

        if (hh < ll)
        {
          break;
        }

        TMathT<double>::Swap(TempArray[ll], TempArray[hh]);
      }

      // Swap Middle item (in position Low) back into correct position
      TMathT<double>::Swap(TempArray[Low], TempArray[hh]);

      // Re-set active partition 
      if (hh <= Median)
      {
        Low = ll;
      }

      if (hh >= Median)
      {
        High = hh - 1;
      }
    }
  }
  else if (Length == 1)
  {
    return pX[0];
  }
  else  // ( Lenght == 0 )
  {
    MInvalid("Should check for empty arrays "
      "and deal with them as appropriate for calling context");

    return 0.0;
  }
}

// -------------------------------------------------------------------------------------------------

double TStatistics::GeometricMean(const double* pX, int Length)
{
  if (Length >= 2)
  {
    // prevent under and overflows
    const double too_large = 1.e64;
    const double too_small = 1.e-64;

    double SumLog = 0.0;
    double Product = 1.0;

    for (int i = 0; i < Length; ++i)
    {
      // MAssert( pX[i] >= 0.0, "Expecting positive values only" );
      const double x = TMathT<double>::Abs(pX[i]);

      Product *= (x + 1e-20); // avoid taking log(0.0)

      if (Product > too_large || Product < too_small)
      {
        SumLog += ::log(Product);
        Product = 1.0;
      }
    }

    return ::exp((SumLog + ::log(Product)) / (double)Length);
  }
  else if (Length == 1)
  {
    return pX[0];
  }
  else // ( Lenght == 0 )
  {
    MInvalid("Should check for empty arrays "
      "and deal with them as appropriate for calling context");

    return 0;
  }
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Centroid(const double* pX, int Length)
{
  const double Sum = TStatistics::Sum(pX, Length);

  if (Sum == 0.0)
  {
    return 0.0;
  }
  else
  {
    double Sc = 0.0;
    for (int j = 0; j < Length; j++)
    {
      Sc += (double)j * pX[j];
    }

    return Sc / Sum;
  }
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Spread(const double* pX, int Length)
{
  return Spread(pX, Length, Centroid(pX, Length));
}

double TStatistics::Spread(const double* pX, int Length, double Centroid)
{
  const double Sum = TStatistics::Sum(pX, Length);

  if (Sum == 0.0)
  {
    return 0.0;
  }
  else
  {
    double Sc = 0.0;
    for (int j = 0; j < Length; j++)
    {
      const double Temp = j - Centroid;
      Sc += Temp * Temp * pX[j];
      // Sc += ::pow( j - Centroid, (double)Order ) * pX[j];
    }

    return Sc / Sum;
  }
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Skewness(const double *pX, int Length, double Centroid, double Spread)
{
  if (!Length || TMathT<double>::Abs(Spread) <= MEpsilon)
  {
    return 0.0;
  }

  double result = 0.0;

  int n = Length;
  while (n--)
  {
    double temp = (pX[n] - Centroid) / Spread;
    // result += pow( temp, 3 );
    result += temp * temp * temp;
  }

  return result / Length;
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Kurtosis(const double* pX, int Length, double Centroid, double Spread)
{
  if (!Length || TMathT<double>::Abs(Spread) <= MEpsilon)
  {
    return 0.0;
  }

  double result = 0.0;

  int n = Length;
  while (n--)
  {
    double temp = (pX[n] - Centroid) / Spread;
    double tempTemp = temp * temp;
    result += tempTemp * tempTemp;
    // result += pow( temp, 4 );
  }

  result /= Length;
  result -= 3.0;

  return result;
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Flatness(const double* pX, int Length)
{
  const double ArithmeticMean = TStatistics::Mean(pX, Length);
  const double GeometricMean = TStatistics::GeometricMean(pX, Length);
  return TStatistics::Flatness(pX, Length, ArithmeticMean, GeometricMean);
}

double TStatistics::Flatness(const double* pX, int Length, 
  double ArithmeticMean, double GeometricMean)
{
  if (ArithmeticMean == 0.0)
  {
    return 0.0; // silence
  }

  return GeometricMean / ArithmeticMean;
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Flux(const double* pX1, const double* pX2, int Length)
{
  if (!Length)
  {
    return 0.0;
  }

#if 1 // correlation

  return TStatistics::Correlation(pX1, pX2, Length);

#else // l2 norm 

  double Flux = 0.0;
  for (int i = 0; i < Length; ++i)
  {
    Flux += TMathT<double>::Square(pX1[i] - pX2[i]);
  }

  return ::sqrt(Flux) / Length;

#endif
}

// -------------------------------------------------------------------------------------------------

double TStatistics::Correlation(const double* pX1, const double* pX2, int Length)
{
  if (!Length)
  {
    return 0.0;
  }


  double ss1 = 0, ss2 = 0, ss11 = 0, ss12 = 0, ss22 = 0;

  for (int i = 0; i < Length; ++i)
  {
    double a = pX1[i];
    double b = pX2[i];

    ss12 = ss12 + a*b;
    ss1 = ss1 + a;
    ss11 = ss11 + a*a;
    ss2 = ss2 + b;
    ss22 = ss22 + b*b;
  }

  ss1 = ss1 / Length;
  ss2 = ss2 / Length;

  double denom2 = (ss11 - ss1*ss1*Length)*(ss22 - ss2*ss2*Length);
  double num = ss12 - (ss1*ss2*Length);

  if (TMathT<double>::Abs(denom2) > MEpsilon)
  {
    return num / ::sqrt(denom2);
  }

  return 0.0;
}

