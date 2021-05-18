#pragma once

#ifndef _NaiveBayesClassificationModel_h_
#define _NaiveBayesClassificationModel_h_

// =================================================================================================

#include "Classification/Export/ClassificationModel.h"

// =================================================================================================

/*!
 * Naive Bayes classification model.
!*/

class TNaiveBayesClassificationModel :
  public TSharkClassificationModel<unsigned int>
{
public:
  TNaiveBayesClassificationModel();

private:
  //! TSharkClassificationModel implementation 
  virtual TString OnName() const override;

  virtual TSharkModelType* OnCreateNewModel(
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses) override;

  virtual TSharkModelType* OnCreateTrainedModel(
    const shark::ClassificationDataset& TrainData,
    const shark::ClassificationDataset& TestData,
    const TString&                      DataSetFileName,
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses) override;
};


#endif // _NaiveBayesClassificationModel_h_

