#pragma once

#ifndef _SvmModel_h_
#define _SvmModel_h_

// =================================================================================================

#include "Classification/Export/ClassificationModel.h"

// for shark::AbstractKernelFunction
#include "../../3rdParty/Shark/Export/SharkSVM.h"

// =================================================================================================

/*!
 * Shark SVM model with an RBF kernel for classification
!*/

class TSvmClassificationModel :
  public TSharkClassificationModel<unsigned int>
{
public:
  TSvmClassificationModel();
  virtual ~TSvmClassificationModel();

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

  TOwnerPtr< shark::AbstractKernelFunction<shark::RealVector> > mpKernel;
};


#endif // _SvmModel_h_

