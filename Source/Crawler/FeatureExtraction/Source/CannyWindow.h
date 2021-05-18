#pragma once

#ifndef _CannyWindow_h_
#define _CannyWindow_h_

#include "CoreTypes/Export/Array.h"

// =================================================================================================

/*!
 * Canny edge filter, used to sharpen BPM histograms and fft spectrums for
 * peak analyzation.
!*/

class TCannyWindow
{
public:
  TCannyWindow(int WindowLength = 12, double WindowShape = 16.0);

  //! apply the window - in place
  void Apply(double* pX, int Length)const;

private:
  double WindowValue(int n) const;

  int mCannyLength;
  double mCannyShape;
  TArray<double> mWindow;
};


#endif // _CannyWindow_h_

