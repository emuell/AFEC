#include "Classification/Export/Models/GBDT.h"

#include "CoreTypes/Export/Exception.h"
#include "CoreTypes/Export/File.h"
#include "CoreFileFormats/Export/ZipFile.h"

#include "../../3rdParty/LightGBM/Export/LightGBM.h"

#include <boost/serialization/binary_object.hpp>

#include <memory>
#include <cstring>
#include <string>
#include <map>
#include <vector>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TGbdtClassificationModel::TGbdtClassificationModel()
{ 
}

// -------------------------------------------------------------------------------------------------

TGbdtClassificationModel::~TGbdtClassificationModel()
{
  // avoid getting inlined
}

// -------------------------------------------------------------------------------------------------

TString TGbdtClassificationModel::OnName() const
{
  return "GBDT";
}

// -------------------------------------------------------------------------------------------------

TClassificationTestResults TGbdtClassificationModel::OnTrain(
  const TString&                      DatabaseFileName,
  const shark::ClassificationDataset& TrainData,
  const TList<TString>&               TrainDataSampleNames,
  const shark::ClassificationDataset& TestData,
  const TList<TString>                TestDataSampleNames,
  const TPoint&                       InputFeaturesSize,
  int                                 NumberOfClasses)
{
  // ... configure LightGBM
  
  const TString TrainDataFileName = gGenerateTempFileName(gTempDir(), ".data");
  const TString TestDataFileName = gGenerateTempFileName(gTempDir(), ".data");

  const int NumberOfTrainSamples = (int)TrainData.numberOfElements();
  const int NumberOfFeatures = InputFeaturesSize.mX * InputFeaturesSize.mY;

  std::unordered_map<std::string, std::string> params;

  // general setup
  params.emplace("task", "train");
  params.emplace("tree_learner", "data");
  params.emplace("objective", (NumberOfClasses > 2) ? "multiclass_ova" : "multiclass");
  params.emplace("metric", "multi_error");
  params.emplace("boosting", "gbdt");
  params.emplace("verbose", "1"); // suppress debug messages

  // training data and report metrics
  params.emplace("num_class", std::to_string(NumberOfClasses));
  params.emplace("data", std::string(TrainDataFileName.CString()));
  params.emplace("valid_data", std::string(TestDataFileName.CString()));
  params.emplace("metric_freq", "1");
  params.emplace("train_metric", "true");
  params.emplace("deterministic", "true");
  params.emplace("force_col_wise", "true");

  // multi-class bin & leaves setup (tweaked for "OneShot-Categories")
  if (NumberOfClasses > 2)
  {
    // make sure the train data fits into all bins
    const int MaxBin = 32;
    const int MinDataInBin = MMin(12, MMax(1, NumberOfTrainSamples / MaxBin));
    const int MinDataInLeaf = MMin(32, MMax(1, NumberOfTrainSamples / MaxBin));
    // params.emplace("extra_trees", "true");
    params.emplace("max_bin", std::to_string(MaxBin));
    params.emplace("min_data_in_bin", std::to_string(MinDataInBin));
    params.emplace("min_data_in_leaf", std::to_string(MinDataInLeaf));
    params.emplace("max_depth", "6");
    params.emplace("num_leaves", "6");
  }
  // dual-class bin & leaves setup (tweaked for "OneShot-vs-Loops")
  else
  {
    // make sure the train data fits into all bins
    const int MaxBin = 32;
    const int MinDataInBin = MMin(12, MMax(1, NumberOfTrainSamples / MaxBin));
    const int MinDataInLeaf = MMin(32, MMax(1, NumberOfTrainSamples / MaxBin));
    params.emplace("max_bin", std::to_string(MaxBin));
    params.emplace("min_data_in_bin", std::to_string(MinDataInBin));
    params.emplace("min_data_in_leaf", std::to_string(MinDataInLeaf));
    params.emplace("max_depth", "8");
    params.emplace("num_leaves", "3");
  }

  // enable bagging when there are enough samples (especially disable for unit tests)
  if (NumberOfTrainSamples > 1000)
  {
    params.emplace("bagging_freq", "1");
    params.emplace("bagging_fraction", "0.5");
    params.emplace("feature_fraction", "0.25");
  }

  // common iterations and early-stopping setup 
  params.emplace("num_iterations", "800");
  params.emplace("early_stopping", "100");
  params.emplace("learning_rate", "0.06");

  // check for aliases
  LightGBM::ParameterAlias::KeyAliasTransform(&params);

  // assign parameters to config
  mpConfig = std::unique_ptr<LightGBM::Config>(new LightGBM::Config());
  mpConfig->Set(params);
  

  // ... Convert datasets

  // TODO: add label, feature names to header

  #if defined(MCompiler_VisualCPP)
    std::ofstream TrainDataFileStream(TrainDataFileName.Chars());
    std::ofstream TestDataFileStream(TestDataFileName.Chars());
  #elif defined(MCompiler_GCC)
    std::ofstream TrainDataFileStream(TrainDataFileName.CString(TString::kFileSystemEncoding));
    std::ofstream TestDataFileStream(TestDataFileName.CString(TString::kFileSystemEncoding));
  #else
    #error "Unknown compiler"
  #endif

  shark::detail::exportCSV_labeled(
    TrainData.inputs().elements(),
    TrainData.labels().elements(),
    TrainDataFileStream,
    shark::FIRST_COLUMN,
    ','
  );

  shark::detail::exportCSV_labeled(
    TestData.inputs().elements(),
    TestData.labels().elements(),
    TestDataFileStream,
    shark::FIRST_COLUMN,
    ','
  );

  TrainDataFileStream.close();
  TestDataFileStream.close();


  // ... Initialize datasets

  if (mpConfig->is_parallel)
  {
    throw TReadableException("is_parallel (network) options not supported");
  }
  else if (mpConfig->save_binary)
  {
    throw TReadableException("save_binary option not expected");
  }

  // reset previous sets and metrics
  mTestDatasets.clear();
  mTrainMetrics.clear();
  mTestMetrics.clear();

  LightGBM::DatasetLoader DatasetLoader(
    *mpConfig, nullptr, mpConfig->num_class, mpConfig->data.c_str());
  
  // load data for single machine
  const int Rank = 0;
  const int NumberOfMachines = 1;
  mpTrainDataset.reset(DatasetLoader.LoadFromFile(
    mpConfig->data.c_str(), Rank, NumberOfMachines));

  // create training metric
  if (mpConfig->is_provide_training_metric)
  {
    for (auto MetricType : mpConfig->metric)
    {
      if (auto pMetric = std::unique_ptr<LightGBM::Metric>(
            LightGBM::Metric::CreateMetric(MetricType, *mpConfig)))
      {
        pMetric->Init(mpTrainDataset->metadata(), mpTrainDataset->num_data());
        mTrainMetrics.push_back(std::move(pMetric));
      }
    }
  }
  mTrainMetrics.shrink_to_fit();

  // only when we have metrics then need to construct validation data
  if (!mpConfig->metric.empty())
  {
    // Add validation data, if it exists
    for (size_t i = 0; i < mpConfig->valid.size(); ++i)
    {
      // HACK: instantiate a new loader: reusing the "DatasetLoader" randomly
      // crashes in Linux builds - probably due to a memory overflow...
      LightGBM::DatasetLoader TrainDatasetLoader(
        *mpConfig, LightGBM::PredictFunction(), mpConfig->num_class, mpConfig->data.c_str());
  
      // add new dataset
      auto pNewDataset = std::unique_ptr<LightGBM::Dataset>(
        TrainDatasetLoader.LoadFromFileAlignWithOtherDataset(
          mpConfig->valid[i].c_str(),
          mpTrainDataset.get())
        );

      mTestDatasets.push_back(std::move(pNewDataset));
      
      // add metric for validation data
      mTestMetrics.emplace_back();
      for (auto MetricType : mpConfig->metric)
      {
        if (auto pMetric = std::unique_ptr<LightGBM::Metric>(
              LightGBM::Metric::CreateMetric(MetricType, *mpConfig)))
        {
          pMetric->Init(mTestDatasets.back()->metadata(),
            mTestDatasets.back()->num_data());
          mTestMetrics.back().push_back(std::move(pMetric));
        }
      }
      mTestMetrics.back().shrink_to_fit();
    }
    mTestDatasets.shrink_to_fit();
    mTestMetrics.shrink_to_fit();
  }

  if (TFile(TrainDataFileName).Exists())
  {
    TFile(TrainDataFileName).Unlink();
  }

  if (TFile(TestDataFileName).Exists())
  {
    TFile(TestDataFileName).Unlink();
  }


  // ... Create Boosting Model

  MAssert(mpConfig->boosting == "gbdt", "Unexpected type");

  // create boosting
  mpBoosting.reset(LightGBM::Boosting::CreateBoosting("gbdt", nullptr));

  // create objective function
  mpObjectiveFunction.reset(LightGBM::ObjectiveFunction::CreateObjectiveFunction(
    mpConfig->objective, *mpConfig));
  
  // initialize the objective function
  mpObjectiveFunction->Init(mpTrainDataset->metadata(), mpTrainDataset->num_data());
  
  // initialize the boosting
  mpBoosting->Init(mpConfig.get(), mpTrainDataset.get(), mpObjectiveFunction.get(),
    LightGBM::Common::ConstPtrInVectorWrapper<LightGBM::Metric>(mTrainMetrics));
  
  // add validation data into boosting
  for (size_t i = 0; i < mTestDatasets.size(); ++i)
  {
    mpBoosting->AddValidDataset(mTestDatasets[i].get(),
      LightGBM::Common::ConstPtrInVectorWrapper<LightGBM::Metric>(mTestMetrics[i]));
  }

  
  // ... Train Model
  
  mpBoosting->Train(mpConfig->snapshot_freq, mpConfig->output_model);
  
 
  // ... Evaluate Results

  // preallocate predictions data
  shark::Data<shark::RealVector> Predictions(TestData.numberOfElements(),
    shark::RealVector(NumberOfClasses));

  // make sure predictions are partititioned just like the test data that
  // we'll apply the prediction on
  Predictions.repartition(TestData.getPartitioning());

  LightGBM::PredictionEarlyStopConfig EarlyStopConfig;
  EarlyStopConfig.margin_threshold = mpConfig->pred_early_stop_margin;
  EarlyStopConfig.round_period = mpConfig->pred_early_stop_freq;

  LightGBM::PredictionEarlyStopInstance EarlyStop = 
    LightGBM::CreatePredictionEarlyStopInstance("multiclass", EarlyStopConfig);

  const int StartIteration = 0;
  const int NumberOfIterations = -1;
  const bool PredictFeatureContribution = false;
  mpBoosting->InitPredict(StartIteration, NumberOfIterations, PredictFeatureContribution);

  // evaluate and convert LightGBM predictions to shark::Data<shark::RealVector>
  int CurrentRow = 0;
  for (auto Prediction : Predictions.elements())
  {
    auto InputRow = TestData.element(CurrentRow++);

    std::vector<double> InputFeatures(NumberOfFeatures);
    std::copy(InputRow.input.begin(), InputRow.input.end(), InputFeatures.begin());

    std::vector<double> Outputs(NumberOfClasses);
    mpBoosting->Predict(InputFeatures.data(), Outputs.data(), &EarlyStop);
    for (size_t c = 0; c < (size_t)NumberOfClasses; ++c)
    {
      Prediction[c] = Outputs[c];
    }
  }
  
  return TClassificationTestResults(TestData, TestDataSampleNames, Predictions);
}

// -------------------------------------------------------------------------------------------------

TList<float> TGbdtClassificationModel::OnEvaluate(
  const TClassificationTestDataItem& Item) const
{
  MAssert(mpBoosting.get(), "Need to train or load a model first");

  const shark::Data<shark::RealVector>& SharkTestData = Item.TestData();

  const shark::DataView< shark::Data<shark::RealVector> > SharkTestDataView(
    const_cast<shark::Data<shark::RealVector>&>(SharkTestData));

  MAssert(SharkTestDataView.size() == 1, "Expecting one item only");
  auto Element = SharkTestDataView[0];
  
  std::vector<double> InputFeatures;
  InputFeatures.reserve(Element.size());
  for (auto Value : Element)
  {
    InputFeatures.push_back(Value);
  }

  LightGBM::Config DefaultConfig;
  LightGBM::Config* pConfig = mpConfig ? mpConfig.get() : &DefaultConfig;

  LightGBM::PredictionEarlyStopConfig EarlyStopConfig;
  EarlyStopConfig.margin_threshold = pConfig->pred_early_stop_margin;
  EarlyStopConfig.round_period = pConfig->pred_early_stop_freq;

  LightGBM::PredictionEarlyStopInstance EarlyStop =
    LightGBM::CreatePredictionEarlyStopInstance("multiclass", EarlyStopConfig);

  const int StartIteration = 0;
  const int NumberOfIterations = -1;
  const bool PredictFeatureContribution = false;
  mpBoosting->InitPredict(StartIteration , NumberOfIterations, PredictFeatureContribution);

  const int NumberOfClasses = NumberOfOutputClasses();
  std::vector<double> Outputs(NumberOfClasses);
  mpBoosting->Predict(InputFeatures.data(), Outputs.data(), &EarlyStop);

  TList<float> Ret;
  Ret.PreallocateSpace(NumberOfClasses);
  for (size_t c = 0; c < (size_t)NumberOfClasses; ++c)
  {
    Ret.Append((float)Outputs[c]);
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

void TGbdtClassificationModel::OnLoadModel(
  eos::portable_iarchive& Archive)
{
  // load compressed model byte array
  int CompressedModelByteArraySize = 0;
  Archive & CompressedModelByteArraySize;

  TArray<TInt8> CompressedModelByteArray(CompressedModelByteArraySize);
  Archive & boost::serialization::make_binary_object(
    CompressedModelByteArray.FirstWrite(),
    CompressedModelByteArray.Size());

  // uncompress byte array into model string
  TArray<TInt8> ModelByteArray;
  TZipFile::SUnGZipData(CompressedModelByteArray, ModelByteArray);

  // load the boosting model from the string and assign it
  std::unique_ptr<LightGBM::Boosting> pNewBoosting(
    LightGBM::Boosting::CreateBoosting("gbdt", nullptr));

  pNewBoosting->LoadModelFromString(
    ModelByteArray.FirstRead(), ModelByteArray.Size());
  
  mpBoosting.reset(pNewBoosting.release());
}

// -------------------------------------------------------------------------------------------------

void TGbdtClassificationModel::OnSaveModel(
  eos::portable_oarchive& Archive) const
{
  MAssert(mpBoosting, "Need to train or load a model first");

  // serialize the boosting model to a string
  const int StartIteration = 0;
  const int NumberOfIterations = -1;
  const int FeatureImportance = 0; // 0: split, 1: gain
  const std::string ModelString = mpBoosting->SaveModelToString(
    StartIteration, NumberOfIterations, FeatureImportance);
  
  TArray<TInt8> ModelByteArray((int)ModelString.size());
  TMemory::Copy(ModelByteArray.FirstWrite(), 
    ModelString.c_str(), (int)ModelString.size());

  // gzip compress model string
  const int CompressionLevel = 6;
  TArray<TInt8> CompressedModelByteArray;
  TZipFile::SGZipData(ModelByteArray, CompressedModelByteArray, CompressionLevel);

  // save compressed model byte array
  Archive & (int)CompressedModelByteArray.Size();
  Archive & boost::serialization::make_binary_object(
    CompressedModelByteArray.FirstWrite(),
    CompressedModelByteArray.Size());
}

