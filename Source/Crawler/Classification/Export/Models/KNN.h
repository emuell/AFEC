#pragma once

#ifndef _KnnClassificationModel_h_
#define _KnnClassificationModel_h_

// =================================================================================================

#include "Classification/Export/ClassificationModel.h"

// =================================================================================================

/*!
 * K-Nearest-Neighbor model with an RBF kernel.
!*/

class TKnnClassificationModel :
  public TSharkClassificationModel<unsigned int>
{
public:
  TKnnClassificationModel();

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


#endif // _KnnClassificationModel_h_

