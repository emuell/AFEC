#pragma once

#ifndef _GbdtClassificationModel_h_
#define _GbdtClassificationModel_h_

// =================================================================================================

#include "Classification/Export/ClassificationModel.h"

#include <memory>
#include <vector>

namespace LightGBM {
  struct Config;
  class Boosting;
  class ObjectiveFunction;
  class Dataset;
  class Metric;
}

// =================================================================================================

/*!
 * Gradient Boosting Descision Tree model, implemented with LightGBM.
!*/

class TGbdtClassificationModel : public TClassificationModel
{
public:
  TGbdtClassificationModel();
  ~TGbdtClassificationModel();

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

  // All configs 
  std::unique_ptr<LightGBM::Config> mpConfig;

  // Boosting object
  std::unique_ptr<LightGBM::Boosting> mpBoosting;
  // Training objective function 
  std::unique_ptr<LightGBM::ObjectiveFunction> mpObjectiveFunction;

  // Training data
  std::unique_ptr<LightGBM::Dataset> mpTrainDataset;
  // Validation data
  std::vector<std::unique_ptr<LightGBM::Dataset>> mTestDatasets;
  // Metric for training data
  std::vector<std::unique_ptr<LightGBM::Metric>> mTrainMetrics;
  // Metrics for validation data
  std::vector<std::vector<std::unique_ptr<LightGBM::Metric>>> mTestMetrics;
};


#endif // _GbdtClassificationModel_h_

