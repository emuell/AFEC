#include "Classification/Export/Models/KNN.h"

#include "../../3rdParty/Shark/Export/SharkNN.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TKnnClassificationModel::TKnnClassificationModel()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

TString TKnnClassificationModel::OnName() const
{
  return "KNN";
}

// -------------------------------------------------------------------------------------------------

TKnnClassificationModel::TSharkModelType*
  TKnnClassificationModel::OnCreateNewModel(
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses)
{
  return NULL; // TODO
}

// -------------------------------------------------------------------------------------------------

TKnnClassificationModel::TSharkModelType*
  TKnnClassificationModel::OnCreateTrainedModel(
    const shark::ClassificationDataset& TrainData,
    const shark::ClassificationDataset& TestData,
    const TString&                      DataSetFileName,
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses)
{
  unsigned int k = 1;  // number of neighbors

#if 0 // linear
  shark::KDTree<shark::RealVector> tree(TrainData.inputs());

  shark::TreeNearestNeighbors<shark::RealVector, unsigned int> algorithm(
    TrainData, &tree);

  TOwnerPtr< shark::NearestNeighborClassifier<shark::RealVector> > pModel(
    new shark::NearestNeighborClassifier<shark::RealVector>(&algorithm, k));

#else // kernel based
  shark::GaussianRbfKernel<shark::RealVector> kernel(0.001);
  // shark::MonomialKernel<shark::RealVector> kernel(1);
  // shark::LinearKernel<shark::RealVector> kernel;

  shark::DataView<shark::Data<shark::RealVector> > trainingInputs(
    const_cast<shark::ClassificationDataset&>(TrainData).inputs());

  shark::KHCTree<shark::DataView<shark::Data<shark::RealVector> > > khctree(
    trainingInputs, &kernel);

  shark::TreeNearestNeighbors<shark::RealVector, unsigned int> algorithmKHC(
    TrainData, &khctree);

  TOwnerPtr< shark::NearestNeighborClassifier<shark::RealVector> > pModel(
    new shark::NearestNeighborClassifier<shark::RealVector>(&algorithmKHC, k));
#endif

  shark::ZeroOneLoss<unsigned int> loss;

  shark::Data<unsigned int> prediction = (*pModel)(TrainData.inputs());
  double input_error_rate = loss(TrainData.labels(), prediction);

  prediction = (*pModel)(TestData.inputs());
  double test_error_rate = loss(TestData.labels(), prediction);

  gTraceVar("  Train: %.2f Test: %.2f", input_error_rate, test_error_rate);

  return pModel.ReleaseOwnership();
}

