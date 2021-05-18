#pragma once

#ifndef _AnnClassificationModel_h_
#define _AnnClassificationModel_h_

// =================================================================================================

#include "Classification/Export/ClassificationModel.h"

// =================================================================================================

/*!
 * Simple shark ANN model with one hidden layer.
!*/

class TAnnClassificationModel :
  public TSharkClassificationModel<shark::RealVector>
{
public:
  TAnnClassificationModel();

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


#endif // _AnnClassificationModel_h_

