#include "Classification/Export/Models/RBM.h"
#include "CoreTypes/Export/File.h"

#include "../../3rdParty/Shark/Export/SharkRBM.h"

// =================================================================================================

// supervised trained network structure
typedef shark::FFNet<shark::TanhNeuron, shark::LinearNeuron> TNetworkType;

// -------------------------------------------------------------------------------------------------

//! creates/trains and a single RBM layer
//! \param data: the data to train with (RBM input layer data)
//! \param numHidden: number of features in the AutoencoderModel
//! \param iterations: number of iterations to optimize
//! \param regularisation: strength of the regularisation
//! \param learningRate: learning rate of steepest descent 

static shark::BipolarRBM STrainRBM(
  const TDirectory&                               ModelDir,
  size_t                                          layer,
  const shark::UnlabeledData<shark::RealVector>&  data,
  size_t                                          dataWidth,
  size_t                                          dataHeight,
  size_t                                          numHidden,
  double                                          regularisation,
  double                                          learningRate,
  double                                          momentum,
  size_t                                          iterations)
{
  // create rbm with simple binary units using the global random number generator
  size_t inputs = shark::dataDimension(data);
  shark::BipolarRBM rbm(shark::Rng::globalRng);
  rbm.setStructure(inputs, numHidden);

  // initialize weights uniformly
  const double r = -0.1; // std::sqrt(6.0) / std::sqrt(numHidden + inputs + 1.0);
  shark::initRandomUniform(rbm, -r, r);

  // create derivative to optimize the rbm
  // we want a simple vanilla CD-1.
  shark::BipolarCD estimator(&rbm);
  // shark::BipolarPCD estimator( &rbm );
  shark::TwoNormRegularizer regularizer;

  // 0.0 is the regularization strength. 0.0 means no regularization. choose as >= 0.0
  estimator.setRegularizer(regularisation, &regularizer);
  estimator.setK(1);//number of sampling steps
  estimator.setData(data);//the data used for optimization
  estimator.init();

  // create and configure optimizer
  shark::SteepestDescent optimizer;
  optimizer.setLearningRate(learningRate);// learning rate of the algorithm
  optimizer.setMomentum(momentum);// momentum of the algorithm

  // now we train the rbm and evaluate the mean negative log-likelihood at the end
  size_t numIterations = iterations;// iterations for training
  optimizer.init(estimator);
  for (size_t Step = 0; Step != numIterations; ++Step)
  {
    optimizer.step(estimator);

    if ((Step % 10) == 0)
    {
      // export layer filter weights as PGM image for debugging
#if 1
      if (layer == 0)
      {
        const TString FeaturesFileName = ModelDir.Path() + "Features_Layer";

        shark::exportFiltersToPGMGrid(
          FeaturesFileName.CString(TString::kFileSystemEncoding),
          rbm.weightMatrix(), dataWidth, dataHeight);
      }
#endif

      gTraceVar("    Step: %d - Error: %.2f",
        (int)Step, (float)optimizer.solution().value);
    }
  }

  rbm.setParameterVector(optimizer.solution().point);

  return rbm;
}

// -------------------------------------------------------------------------------------------------

//! unsupervised pre training of a network with X hidden layers
//! \param data: input data
//! \param numHidden1: number of neurons in first hidden layer
//! \param numHidden2: number of neurons in second hidden layer
//! \param numOutputs: number of output neurons (classes)
//! \param regularisation: regularization factor for the RBM layers
//! \param iterations: iterations for the RBM layers
//! \param learningRate: learningRate for the RBM layers

static TNetworkType* SCreatePretrainedDeepAutoEncoderNetwork(
  const TString&                                  CvsDataFileName,
  const TDirectory&                               ModelDir,
  const shark::UnlabeledData<shark::RealVector>&  data,
  size_t                                          dataWidth,
  size_t                                          dataHeight,
  const std::vector<size_t>&                      hiddenLayers,
  size_t                                          numOutputs,
  double                                          regularisation,
  double                                          learningRate,
  double                                          momentum,
  size_t                                          iterations)
{
  const TString ModelFileName = ModelDir.Path() + "Pretrained.net";


  // ... load a previously trained model

  // model file must exist and be newer than data...
  if (gFileExists(ModelFileName) &&
    TFile(ModelFileName).ModificationStatTime() >=
    TFile(CvsDataFileName).ModificationStatTime())
  {
    // save the model to a file

    const std::ios_base::openmode StreamFlags = std::ifstream::binary;

#if defined(MCompiler_VisualCPP)
  // Visual CPP has a w_char open impl, needed to properly open unicode filenames
  std::ifstream ifs(ModelFileName.Chars(), StreamFlags);
#elif defined(MCompiler_GCC)
  // kFileSystemEncoding should be utf-8 on linux/OSX, so this should be fine too
  std::ifstream ifs(ModelFileName.CString(TString::kFileSystemEncoding), StreamFlags);
#else
  #error "Unknown compiler"
#endif

    eos::portable_iarchive ia(ifs);

    TOwnerPtr<TNetworkType> pNetwork(new TNetworkType());
    pNetwork->load(ia, 0);
    ifs.close();

    return pNetwork.ReleaseOwnership();
  }


  // ... create a new model

  else
  {
    // train all hidden layers

    const size_t numHiddenLayers = hiddenLayers.size();

    std::list<shark::BipolarRBM> trainedHiddenLayers;
    shark::UnlabeledData<shark::RealVector> intermediateData = data;

    for (size_t l = 0; l != numHiddenLayers; ++l)
    {
      gTraceVar("  Training layer %d (%d neurons)...", (int)l, (int)hiddenLayers[l]);

      trainedHiddenLayers.push_back(STrainRBM(
        ModelDir,
        l,
        intermediateData,
        dataWidth, dataHeight,
        hiddenLayers[l],
        regularisation,
        learningRate,
        momentum,
        iterations
      ));

      // compute the mapping onto features of the first hidden layer:
      // we compute the direction visible->hidden and want the features and no samples
      trainedHiddenLayers.back().evaluationType(true, true);
      intermediateData = trainedHiddenLayers.back()(data);
    }

    // create the final network structure
    TOwnerPtr<TNetworkType> pFinalNetwork(new TNetworkType());

    std::vector<size_t> finalLayerSizes(numHiddenLayers + 2);
    finalLayerSizes[0] = shark::dataDimension(data);
    for (size_t l = 0; l < numHiddenLayers; ++l)
    {
      finalLayerSizes[l + 1] = hiddenLayers[l];
    }
    finalLayerSizes[finalLayerSizes.size() - 1] = numOutputs;

    pFinalNetwork->setStructure(finalLayerSizes, shark::FFNetStructures::Normal);

    // xavier init for nun hidden layers
#if 0
    shark::RealVector parameterVector = pFinalNetwork->parameterVector();
    const std::vector<shark::RealMatrix>& layerMatrices = pFinalNetwork->layerMatrices();

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

    pFinalNetwork->setParameterVector(parameterVector);

#else
    // random uniform
    shark::initRandomUniform(*pFinalNetwork, -0.01, 0.01);
#endif

    for (size_t l = 0; l < numHiddenLayers; ++l)
    {
      std::list<shark::BipolarRBM>::iterator layerIter =
        std::next(trainedHiddenLayers.begin(), l);

      pFinalNetwork->setLayer(l, layerIter->weightMatrix(),
        layerIter->hiddenNeurons().bias());
    }

    // save the model 

    const std::ios_base::openmode StreamFlags = std::ofstream::binary;

#if defined(MCompiler_VisualCPP)
    std::ofstream ofs(ModelFileName.Chars(), StreamFlags); // see Load above...
#elif defined(MCompiler_GCC)
    std::ofstream ofs(ModelFileName.CString(TString::kFileSystemEncoding), StreamFlags);
#else
    #error "Unknown compiler"
#endif

    eos::portable_oarchive oa(ofs);

    pFinalNetwork->save(oa, 0);
    ofs.close();

    return pFinalNetwork.ReleaseOwnership();
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TRbmClassificationModel::TRbmClassificationModel()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

TString TRbmClassificationModel::OnName() const
{
  return "RBM";
}

// -------------------------------------------------------------------------------------------------

TRbmClassificationModel::TSharkModelType*
  TRbmClassificationModel::OnCreateNewModel(
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses)
{
  return new TNetworkType();
}

// -------------------------------------------------------------------------------------------------

TRbmClassificationModel::TSharkModelType*
  TRbmClassificationModel::OnCreateTrainedModel(
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

  // layers
  const size_t numInputs = (size_t)InputFeaturesSize.mX * InputFeaturesSize.mY;
  const size_t numOutputs = NumberOfClasses;
  const size_t numHidden = 3 * (size_t)::sqrt(numInputs + numOutputs) + numOutputs;

  std::vector<size_t> numHiddens = {
    numHidden,
    // numHidden / 3,
    // numHidden
  };

  // unsupervised hyper parameters
  double unsupLearningRate = 0.1;
  double unsupMomentum = 0.0;
  double unsupRegularisation = 0.001;
  size_t unsupIterations = 200;

  // supervised hyper parameters
  const unsigned numberOfSteps = 120;
  const double regularisation = 0.001;
  const double stopAtTrainError = -1.0; // don't stop

    // load or create unsupervised pre training
  TOwnerPtr<TNetworkType> pNetwork(SCreatePretrainedDeepAutoEncoderNetwork(
    DataSetFileName, ModelDir(), TrainData.inputs(),
    InputFeaturesSize.mX, InputFeaturesSize.mY,
    numHiddens, numOutputs,
    unsupRegularisation, unsupLearningRate, unsupMomentum, unsupIterations
  ));


    // ... network optimization

  gTraceVar("  Training supervised model...");

  // create the supervised problem. Cross Entropy loss with regularisation
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

  // set weights to best weights from optimizer
  pNetwork->setParameterVector(bestSolution.point);

  return pNetwork.ReleaseOwnership();
}

