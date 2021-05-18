#pragma once

#ifndef _TestResults_h_
#define _TestResults_h_

// =================================================================================================

#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Array.h"

#include "../../3rdParty/Shark/Export/SharkDataSet.h"

// =================================================================================================

/*!
 * Shark classification test results
!*/

class TClassificationTestResults
{
public:
  typedef TArray< TArray<unsigned int> > TConfusionMatrix;

  //! Default constructor intializes with an error.
  TClassificationTestResults();

  //! Evaluate results from real vector -> vector models (e.g. ANN)
  TClassificationTestResults(
    const shark::ClassificationDataset&   TestData,
    const TList<TString>&                 TestDataNames,
    const shark::Data<shark::RealVector>& Predictions);

  //! Evaluate results from vector -> int models (e.g. SVM)
  TClassificationTestResults(
    const shark::ClassificationDataset&   TestData,
    const TList<TString>&                 TestDataNames,
    const shark::Data<unsigned int>&      Predictions);

  struct TPredictionError
  {
    TString mName;
    unsigned int mClass;
    unsigned int mPredictedClass;
    unsigned int mSecondaryPredictedClass;
    TArray<float> mPredictionWeights;
  };

  float mFinalError;
  float mFinalSecondaryError;
  TConfusionMatrix mConfusionMatrix;
  TList<TPredictionError> mPredictionErrors;
};


#endif // _TestResults_h_

