#pragma once

#ifndef _DnnClassificationModel_h_
#define _DnnClassificationModel_h_

// =================================================================================================

#include "Classification/Export/ClassificationModel.h"

#include <memory>

namespace tiny_dnn {
  template <typename NetType> class network;
  class sequential;
}

// =================================================================================================

/*!
 * DNN or CNN model, implemented with TinyDNN.
!*/

class TDnnClassificationModel : public TClassificationModel
{
public:
  TDnnClassificationModel();
  ~TDnnClassificationModel();

private:

  //! TClassificationModel implementation 
  virtual TString OnName() const override;

  virtual TClassificationTestResults OnTrain(
    const TString&                      DatabaseFileName,
    const shark::ClassificationDataset& TrainData,
    const TList<TString>&               TrainDataSampleNames,
    const shark::ClassificationDataset& TestData,
    const TList<TString>                TestDataSampleNames,
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses) override;

  virtual TList<float> OnEvaluate(
    const TClassificationTestDataItem& Item) const override;

  virtual void OnLoadModel(
    eos::portable_iarchive& Archive) override;
  virtual void OnSaveModel(
    eos::portable_oarchive& Archive) const override;

  // trained network
  std::shared_ptr< tiny_dnn::network<tiny_dnn::sequential> > mpNetwork;
};


#endif // _DnnClassificationModel_h_

