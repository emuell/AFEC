#include "Classification/Export/Models/RandomForest.h"

#include "../../3rdParty/Shark/Export/SharkRF.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TRandomForestClassificationModel::TRandomForestClassificationModel()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

TString TRandomForestClassificationModel::OnName() const
{
  return "RandomForest";
}

// -------------------------------------------------------------------------------------------------

TRandomForestClassificationModel::TSharkModelType*
  TRandomForestClassificationModel::OnCreateNewModel(
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses)
{
  return new shark::RFClassifier();
}

// -------------------------------------------------------------------------------------------------

TRandomForestClassificationModel::TSharkModelType*
  TRandomForestClassificationModel::OnCreateTrainedModel(
    const shark::ClassificationDataset& TrainData,
    const shark::ClassificationDataset& TestData,
    const TString&                      DataSetFileName,
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses)
{
  TOwnerPtr<shark::RFClassifier> pModel(new shark::RFClassifier());

  bool computeFeatureImportances = true;
  shark::RFTrainer trainer(computeFeatureImportances);
  trainer.setMTry(shark::dataDimension(TrainData.inputs()));
  trainer.setNTrees(100);

  trainer.train(*pModel, TrainData);

  shark::ZeroOneLoss<unsigned int, shark::RealVector> loss;

  shark::Data<shark::RealVector> prediction = (*pModel)(TrainData.inputs());
  double input_error_rate = loss(TrainData.labels(), prediction);

  prediction = (*pModel)(TestData.inputs());
  double test_error_rate = loss(TestData.labels(), prediction);

  gTraceVar("  Train:%.2f Test:%.2f", input_error_rate, test_error_rate);

#if 0
  model.computeFeatureImportances();
  shark::RealVector featureImportances = model.featureImportances();

  int Feature = 0;
  for (auto iter = featureImportances.begin();
  iter != featureImportances.end(); ++iter)
  {
    gTraceVar("  Feature: %d Importance: %.4f", Feature++, *iter);
  }

  MUnused(featureImportances);
#endif

  return pModel.ReleaseOwnership();
}

