#include "Classification/Export/Models/NaiveBayes.h"

#include "../../3rdParty/Shark/Export/SharkNB.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TNaiveBayesClassificationModel::TNaiveBayesClassificationModel()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

TString TNaiveBayesClassificationModel::OnName() const
{
  return "NaiveBayes";
}

// -------------------------------------------------------------------------------------------------

TNaiveBayesClassificationModel::TSharkModelType*
  TNaiveBayesClassificationModel::OnCreateNewModel(
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses)
{
  return new shark::NBClassifier<>(
    NumberOfClasses, InputFeaturesSize.mX * InputFeaturesSize.mY);
}

// -------------------------------------------------------------------------------------------------

TNaiveBayesClassificationModel::TSharkModelType*
  TNaiveBayesClassificationModel::OnCreateTrainedModel(
    const shark::ClassificationDataset& TrainData,
    const shark::ClassificationDataset& TestData,
    const TString&                      DataSetFileName,
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses)
{
  TOwnerPtr< shark::NBClassifier<> > pModel(new shark::NBClassifier<>(
    NumberOfClasses, InputFeaturesSize.mX * InputFeaturesSize.mY));

  shark::NBClassifierTrainer<> trainer;
  trainer.train(*pModel, TrainData);

  shark::ZeroOneLoss<unsigned int> loss;

  shark::Data<unsigned int> prediction = (*pModel)(TrainData.inputs());
  double input_error_rate = loss(TrainData.labels(), prediction);

  prediction = (*pModel)(TestData.inputs());
  double test_error_rate = loss(TestData.labels(), prediction);

  gTraceVar("  Train: %.2f Test: %.2f", input_error_rate, test_error_rate);

  return pModel.ReleaseOwnership();
}

