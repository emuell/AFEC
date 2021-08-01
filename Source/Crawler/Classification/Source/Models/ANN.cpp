#include "Classification/Export/Models/ANN.h"

#include "../../3rdParty/Shark/Export/SharkANN.h"

// =================================================================================================

typedef shark::FFNet<shark::TanhNeuron, shark::LinearNeuron> TNetworkType;

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TAnnClassificationModel::TAnnClassificationModel()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

TString TAnnClassificationModel::OnName() const
{
  return "ANN";
}

// -------------------------------------------------------------------------------------------------

TAnnClassificationModel::TSharkModelType*
TAnnClassificationModel::OnCreateNewModel(
  const TPoint&                       InputFeaturesSize,
  int                                 NumberOfClasses)
{
  return new TNetworkType();
}

// -------------------------------------------------------------------------------------------------

TAnnClassificationModel::TSharkModelType*
TAnnClassificationModel::OnCreateTrainedModel(
  const shark::ClassificationDataset& TrainData,
  const shark::ClassificationDataset& TestData,
  const TString&                      DataSetFileName,
  const TPoint&                       InputFeaturesSize,
  int                                 NumberOfClasses)
{
  // ... export input data as PGM grid

#if 1
  const TString SourceLayerFileName = ModelDir().Path() + "Source_Layer";

  shark::exportFiltersToPGMGrid(
    SourceLayerFileName.CString(TString::kFileSystemEncoding),
    TrainData.inputs(), InputFeaturesSize.mX, InputFeaturesSize.mY);
#endif


  // ... network setup 

  // configuration
  const unsigned numberOfSteps = 120;
  const double regularisation = 0.002;
  const double stopAtTrainError = -1.0; // don't stop

  const size_t numInputs = (size_t)InputFeaturesSize.mX * InputFeaturesSize.mY;
  const size_t numOutputs = NumberOfClasses;
  const size_t numHidden = 2 * (size_t)::sqrt(numInputs + numOutputs) + numOutputs;

  const std::vector<size_t> layers = {
    numInputs,
    numHidden,
    // numHidden / 2,
    numOutputs
  };

  // create network and initialize weights random uniform
  TOwnerPtr<TNetworkType> pNetwork(new TNetworkType());
  pNetwork->setStructure(layers);

  // xavier init for layers
  shark::RealVector parameterVector = pNetwork->parameterVector();
  const std::vector<shark::RealMatrix>& layerMatrices = pNetwork->layerMatrices();

  // NB: this relies on layer matrices being the first parameters and biases last
  size_t initMatrixOffset = 0;
  for (size_t l = 0; l < layerMatrices.size(); ++l)
  {
    const shark::RealMatrix& layerMatrix = layerMatrices[l];

    const size_t layerInputs = layerMatrix.size2();
    const size_t layerOutputs = layerMatrix.size1();
    const size_t layerMatrixSize = layerInputs*layerOutputs;

    const double r = ::sqrt(1.0 / layerInputs);
    shark::Uniform<> uni(shark::Rng::globalRng, -r, r);

    std::generate(parameterVector.begin() + initMatrixOffset,
      parameterVector.begin() + initMatrixOffset + layerMatrixSize, uni);

    initMatrixOffset += layerMatrixSize;
  }

  // set remaining bias amounts to zero
  std::fill(parameterVector.begin() + initMatrixOffset,
    parameterVector.end(), 0.0);

  pNetwork->setParameterVector(parameterVector);

  // create error function
  shark::CrossEntropy loss;
  shark::ErrorFunction error(TrainData, pNetwork.Object(), &loss);

  shark::TwoNormRegularizer regularizer(error.numberOfVariables());
  error.setRegularizer(regularisation, &regularizer);
  error.init();

  // initialize steepest descent
  shark::IRpropPlusFull optimizer;
  optimizer.init(error, pNetwork->parameterVector());


  // ... network evaluation

  shark::ZeroOneLoss<unsigned int, shark::RealVector> loss01;

  shark::IRpropPlusFull::SolutionType bestSolution = optimizer.solution();
  double best_error = std::numeric_limits<double>::max();

  for (size_t step = 0; step != numberOfSteps; ++step)
  {
    optimizer.step(error);

    // evaluate current solution 
    if (step > 0 && (step % 10) == 0)
    {
      const double input_error = loss01(
        TrainData.labels(), (*pNetwork)(TrainData.inputs()));

      const double test_error = loss01(
        TestData.labels(), (*pNetwork)(TestData.inputs()));

      if (test_error < best_error)
      {
        best_error = test_error;
        bestSolution = optimizer.solution();
      }

      gTraceVar("  Step:%d - Train:%.2f Test:%.2f",
        (int)step, (float)input_error, (float)test_error);

      if (input_error <= stopAtTrainError)
      {
        break;
      }
    }
  }


  // ... collect results

  // set weights to best weights 
  pNetwork->setParameterVector(bestSolution.point);

  return pNetwork.ReleaseOwnership();
}

