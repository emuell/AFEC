#pragma once

#ifndef _RandomForestClassificationModel_h_
#define _RandomForestClassificationModel_h_

// =================================================================================================

#include "Classification/Export/ClassificationModel.h"

// =================================================================================================

/*!
 * Random Forest CART model for classification.
!*/

class TRandomForestClassificationModel :
  public TSharkClassificationModel<shark::RealVector>
{
public:
  TRandomForestClassificationModel();

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


#endif // _RandomForestClassificationModel_h_

