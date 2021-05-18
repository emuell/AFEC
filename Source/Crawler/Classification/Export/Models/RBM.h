#pragma once

#ifndef _RbmClassificationModel_h_
#define _RbmClassificationModel_h_

// =================================================================================================

#include "Classification/Export/ClassificationModel.h"

// =================================================================================================

/*!
 * Simple shark ANN model with one hidden layer, which is pretrained
 * with an RBM layer.
!*/

class TRbmClassificationModel :
  public TSharkClassificationModel<shark::RealVector>
{
public:
  TRbmClassificationModel();

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


#endif // _RbmClassificationModel_h_

