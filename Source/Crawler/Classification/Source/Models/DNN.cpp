#include "Classification/Export/Models/DNN.h"
#include "CoreTypes/Export/Cpu.h"

// =================================================================================================

// disabled by default and takes ages to compile. enable for local testing...
// #define MEnableDNN

// =================================================================================================

#if defined(MEnableDNN)
  #include "../../3rdParty/TinyDNN/Export/TinyDNN.h"
#endif

// =================================================================================================

// TODO:
//   - evaluating loaded models crashes
//   - add regularizers
//   - use OpenCV or some other GPU backend for training

// =================================================================================================

// -------------------------------------------------------------------------------------------------

// Convert shark classification data to tiny_dnn data

#if defined(MEnableDNN)

static void SConvertData(
  const shark::ClassificationDataset& Dataset,
  std::vector<tiny_dnn::vec_t>&       DestData,
  std::vector<tiny_dnn::label_t>&     DestLabels)
{
  // convert data
  shark::DataView< shark::Data<shark::RealVector> > DatasetInputs(
    const_cast<shark::ClassificationDataset&>(Dataset).inputs());

  DestData.clear();
  DestData.reserve(DatasetInputs.size());

  for (auto Element : DatasetInputs)
  {
    tiny_dnn::vec_t DestElement;
    Element.reserve(Element.size());

    for (auto Value : Element)
    {
      DestElement.push_back((tiny_dnn::float_t)Value);
    }

    DestData.push_back(DestElement);
  }

  // convert labels
  shark::DataView< shark::Data<unsigned int> > DatasetLabels(
    const_cast<shark::ClassificationDataset&>(Dataset).labels());

  DestLabels.clear();
  DestLabels.reserve(DatasetLabels.size());

  for (auto Label : DatasetLabels)
  {
    DestLabels.push_back(Label);
  }
}

#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TDnnClassificationModel::TDnnClassificationModel()
{ 
}

// -------------------------------------------------------------------------------------------------

TDnnClassificationModel::~TDnnClassificationModel()
{
  // avoid getting inlined
}

// -------------------------------------------------------------------------------------------------

TString TDnnClassificationModel::OnName() const
{
  return "DNN";
}

// -------------------------------------------------------------------------------------------------

TClassificationTestResults TDnnClassificationModel::OnTrain(
  const TString&                      DatabaseFileName,
  const shark::ClassificationDataset& SharkTrainData,
  const TList<TString>&               TrainDataSampleNames,
  const shark::ClassificationDataset& SharkTestData,
  const TList<TString>                TestDataSampleNames,
  const TPoint&                       InputFeaturesSize,
  int                                 NumberOfClasses)
{
#if !defined(MEnableDNN)
  gTraceVar("  DNN builds not enabled...");
  return TClassificationTestResults();

#else

  // ... convert datasets

  std::vector<tiny_dnn::label_t> TrainLabels, TestLabels;
  std::vector<tiny_dnn::vec_t> TrainData, TestData;

  SConvertData(SharkTrainData, TrainData, TrainLabels);
  SConvertData(SharkTestData, TestData, TestLabels);


  // ... create network

  mpNetwork.reset(new tiny_dnn::network<tiny_dnn::sequential>());

  tiny_dnn::network<tiny_dnn::sequential>& Network = *mpNetwork;
  tiny_dnn::core::backend_t backend_type = tiny_dnn::core::default_engine();

  const tiny_dnn::float_t DropoutRate = 0.0;
  const tiny_dnn::float_t DropoutRate2 = 0.5;

  const uint32_t NumInputs = (uint32_t)InputFeaturesSize.mX * InputFeaturesSize.mY;
  const uint32_t NumOutputs = NumberOfClasses;
  const uint32_t NumHidden = 3 * (int)::sqrt(NumInputs + NumOutputs) + NumOutputs;

#if 0 // CNN

  const int NumberOfLayers = 80;

  const bool AddBias = true;

  tiny_dnn::convolutional_layer<tiny_dnn::activation::tan_h>* pConvLayer =
    new tiny_dnn::convolutional_layer<tiny_dnn::activation::tan_h>(
      1, InputFeaturesSize.mY, // input size
      InputFeaturesSize.mX, InputFeaturesSize.mY, // kernel size
      1, NumberOfLayers,  // channel inputs & output (layers)
      tiny_dnn::padding::valid, // add padding as needed
      AddBias, // add bias weights
      1, 1, // stride size
      backend_type);
  Network << std::shared_ptr<tiny_dnn::layer>(pConvLayer);

  tiny_dnn::max_pooling_layer<tiny_dnn::activation::tan_h>* pPoolingLayer =
    new tiny_dnn::max_pooling_layer<tiny_dnn::activation::tan_h>(
      pConvLayer->out_shape()[0].width_, pConvLayer->out_shape()[0].height_, // input size 
      pConvLayer->out_shape()[0].depth_, // input channels (layers)
      1, // scale factor
      backend_type);
  Network << std::shared_ptr<tiny_dnn::layer>(pPoolingLayer);

  tiny_dnn::fully_connected_layer<tiny_dnn::activation::tan_h>* pFcLayer =
    new tiny_dnn::fully_connected_layer<tiny_dnn::activation::tan_h>(
      pPoolingLayer->out_data_size(), NumHidden, AddBias, backend_type);
  Network << std::shared_ptr<tiny_dnn::layer>(pFcLayer);

  if (DropoutRate > 0.0)
  {
    tiny_dnn::dropout_layer* pDropoutLayer =
      new tiny_dnn::dropout_layer(NumHidden, DropoutRate);
    Network << std::shared_ptr<tiny_dnn::layer>(pDropoutLayer);
  }

  tiny_dnn::fully_connected_layer<tiny_dnn::activation::softmax>* pOutputLayer =
    new tiny_dnn::fully_connected_layer<tiny_dnn::activation::softmax>(
      NumHidden, NumOutputs, AddBias, backend_type);
  Network << std::shared_ptr<tiny_dnn::layer>(pOutputLayer);

#else // simple MLP

  const bool AddBias = true;

  tiny_dnn::input_layer* pInputLayer =
    new tiny_dnn::input_layer(tiny_dnn::shape3d(InputFeaturesSize.mX, InputFeaturesSize.mY, 1));
  Network << std::shared_ptr<tiny_dnn::layer>(pInputLayer);

  if (DropoutRate > 0.0)
  {
    tiny_dnn::dropout_layer* pDropoutLayer =
      new tiny_dnn::dropout_layer(pInputLayer->out_size(), DropoutRate);
    Network << std::shared_ptr<tiny_dnn::layer>(pDropoutLayer);
  }

  tiny_dnn::fully_connected_layer<tiny_dnn::activation::tan_h>* pFcLayer1 =
    new tiny_dnn::fully_connected_layer<tiny_dnn::activation::tan_h>(
      pInputLayer->out_size(), NumHidden, AddBias, backend_type);
  Network << std::shared_ptr<tiny_dnn::layer>(pFcLayer1);

  if (DropoutRate2 > 0.0)
  {
    tiny_dnn::dropout_layer* pDropoutLayer2 =
      new tiny_dnn::dropout_layer(NumHidden, DropoutRate2);
    Network << std::shared_ptr<tiny_dnn::layer>(pDropoutLayer2);
  }

  tiny_dnn::fully_connected_layer<tiny_dnn::activation::softmax>* pOutputLayer =
    new tiny_dnn::fully_connected_layer<tiny_dnn::activation::softmax>(
      NumHidden, NumOutputs, AddBias, backend_type);
  Network << std::shared_ptr<tiny_dnn::layer>(pOutputLayer);

#endif

  // initialize layers (xavier)
  Network.weight_init(tiny_dnn::weight_init::xavier(1.0));
  Network.bias_init(tiny_dnn::weight_init::constant(0.0));


  // ... setup optimizer and train parameters

  const size_t BatchSize = MMin<size_t>(64, TestData.size()); // mini batch size 
  const size_t ReportStep = 5;

#if 1 // gradient_descent (with weight decay regularization)
  const size_t TrainingEpochs = 30;
  const tiny_dnn::float_t LearningRate = 0.1f;
  const tiny_dnn::float_t Regularization = 0.038f;

  tiny_dnn::momentum Optimizer;
  Optimizer.alpha = LearningRate * (tiny_dnn::float_t)(::sqrt(BatchSize) * LearningRate);
  Optimizer.lambda = Regularization;
  Optimizer.mu = 0.5;

#else // adam 
  const size_t TrainingEpochs = 40;
  const tiny_dnn::float_t LearningRate = 0.01f;

  tiny_dnn::adam Optimizer;
  Optimizer.alpha = LearningRate * (tiny_dnn::float_t)(::sqrt(BatchSize) * LearningRate);
#endif


  // ... train

  // batch callback
  auto on_enumerate_batch = []() {
    // do nothing 
  };

  // epoch callback
  int CurrentEpoch = 0;
  auto on_enumerate_epoch = [&]() {
    if ((++CurrentEpoch % ReportStep) == 0)
    {
      const tiny_dnn::result TrainResult = Network.test(TrainData, TrainLabels);

      const float TrainError = (float)(TrainResult.num_total - TrainResult.num_success) /
        (float)TrainResult.num_total;

      const tiny_dnn::result TestResult = Network.test(TestData, TestLabels);

      const float TestError = (float)(TestResult.num_total - TestResult.num_success) /
        (float)TestResult.num_total;

      gTraceVar("    Step: %d: Train %.2f - Test %.2f", CurrentEpoch, TrainError, TestError);

#if 0
      tiny_dnn::image<> img = conv.weight_to_image();
      img.write((ModelDir().Path() + "Conv_Layer_0.png").StdCString(TString::kFileSystemEncoding));

      img = pooling.output_to_image();
      img.write((ModelDir().Path() + "Pool_Layer_0.png").StdCString(TString::kFileSystemEncoding));
#endif
    }
  };

  // train
  const bool ResetWeigths = false;
  const int NumThreads = TCpu::NumberOfConcurrentThreads();

  Network.train<tiny_dnn::cross_entropy>(Optimizer,
    TrainData, TrainLabels,
    BatchSize, TrainingEpochs,
    on_enumerate_batch, on_enumerate_epoch,
    ResetWeigths,
    NumThreads);


  // ... evaluate results

  Network.set_netphase(tiny_dnn::net_phase::test);

  // preallocate predictions data
  shark::Data<shark::RealVector> Predictions(TestData.size(),
    shark::RealVector(NumberOfClasses));

  // make sure predictions are partititioned just like the test data that
  // we'll apply the prediction on
  Predictions.repartition(SharkTestData.getPartitioning());

  // convert tiny_dnn::vec_t predictions to shark::Data<shark::RealVector>
  int CurrentRow = 0;
  for (auto Prediction : Predictions.elements())
  {
    tiny_dnn::vec_t Outputs = Network.predict(TestData[CurrentRow]);
    for (size_t c = 0; c < (size_t)NumberOfClasses; ++c)
    {
      Prediction[c] = Outputs[c];
    }

    ++CurrentRow;
  }

  return TClassificationTestResults(
    SharkTestData, TestDataSampleNames, Predictions);
#endif
}

// -------------------------------------------------------------------------------------------------

TList<float> TDnnClassificationModel::OnEvaluate(
  const TClassificationTestDataItem& Item) const
{
#if !defined(MEnableDNN)
  MInvalid("Should not be called");
  return TList<float>();

#else
  MAssert(mpNetwork, "Need to train or load a model first");

  const shark::Data<shark::RealVector>& SharkTestData = Item.TestData();

  // convert test data to tiny_dnn vector
  shark::DataView< shark::Data<shark::RealVector> > SharkTestDataView(
    const_cast<shark::Data<shark::RealVector>&>(SharkTestData));

  MAssert(SharkTestDataView.size() == 1, "Expecting one item only");

  std::vector<tiny_dnn::vec_t> DestData;
  DestData.reserve(SharkTestDataView.size());

  for (auto Element : SharkTestDataView)
  {
    tiny_dnn::vec_t DestElement;
    Element.reserve(Element.size());

    for (auto Value : Element)
    {
      DestElement.push_back((tiny_dnn::float_t)Value);
    }

    DestData.push_back(DestElement);
  }

  // evaluate
  mpNetwork->set_netphase(tiny_dnn::net_phase::test);
  tiny_dnn::vec_t Outputs = mpNetwork->predict(DestData.front());

  // convert to TList
  TList<float> Ret;
  Ret.PreallocateSpace((int)Outputs.size());

  for (auto Weight : Outputs)
  {
    Ret.Append(Weight);
  }

  return Ret;
#endif
}

// -------------------------------------------------------------------------------------------------

void TDnnClassificationModel::OnLoadModel(
  eos::portable_iarchive& Archive)
{
#if defined(MEnableDNN)
  mpNetwork.reset(new tiny_dnn::network<tiny_dnn::sequential>());

  std::string data;
  Archive & data;

  std::istringstream stringstream(data);
  cereal::BinaryInputArchive archive(stringstream);

  mpNetwork->from_archive(archive);
#endif
}

// -------------------------------------------------------------------------------------------------

void TDnnClassificationModel::OnSaveModel(
  eos::portable_oarchive& Archive) const
{
#if defined(MEnableDNN)
  MAssert(mpNetwork, "Need to train or load a model first");

  std::ostringstream stringstream;
  cereal::BinaryOutputArchive archive(stringstream);

  mpNetwork->to_archive(archive);

  std::string Data = stringstream.str();
  Archive & Data;
#endif
}

