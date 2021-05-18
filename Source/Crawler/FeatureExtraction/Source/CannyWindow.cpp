#include "FeatureExtraction/Export/Statistics.h"
#include "FeatureExtraction/Source/CannyWindow.h"

#include "CoreTypes/Export/Array.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TCannyWindow::TCannyWindow(int WindowLength, double WindowShape)
  : mCannyLength(WindowLength),
    mCannyShape(WindowShape)
{
  MAssert(WindowLength > 1, "Invalid window length");
  MAssert(WindowShape > 1.0, "Invalid window shape");

  // calculate and save CannyWindowValue window
  mWindow.SetSize(mCannyLength * 2 + 1);
  for (int i = mCannyLength * -1; i < mCannyLength + 1; i++)
  {
    mWindow[i + mCannyLength] = WindowValue(i);
  }
}

// -------------------------------------------------------------------------------------------------

void TCannyWindow::Apply(double* pX, int Length) const
{
  MAssert(Length > 0, "");

  TArray<double> Temp(Length);

  for (int i = 0; i < Length; i++)
  {
    // reset feature details
    double Sum = 0.0;

    // convolve the CannyWindowValue window with the envelope of that sub-band
    for (int shift = mCannyLength * -1; shift < mCannyLength; shift++)
    {
      if (i + shift >= 0 && i + shift < Length)
      {
        Sum += pX[i + shift] * mWindow[shift + mCannyLength];
      }
    }

    // save result
    Temp[i] = Sum;
  }

  // copy back from temp
  for (int i = 0; i < Length; i++)
  {
    pX[i] = Temp[i];
  }

  // normalise
  const double Mean = TStatistics::Mean(pX, Length);
  const double Variance = TStatistics::Variance(pX, Length, Mean);

  if (Variance > 0.0)
  {
    const double StdDev = ::sqrt(Variance);

    for (int i = 0; i < Length; i++)
    {
      pX[i] = MMax(0.0, (pX[i] - Mean) / StdDev);
    }
  }
}

// -------------------------------------------------------------------------------------------------

double TCannyWindow::WindowValue(int n) const
{
  const double SquareCannyShape = TMathT<double>::Square(mCannyShape);

  return (double)n / SquareCannyShape *
    ::exp(-1.0 * TMathT<int>::Square(n) / (2.0 * SquareCannyShape));
}

