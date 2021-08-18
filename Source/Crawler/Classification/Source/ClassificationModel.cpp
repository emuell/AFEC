#include "Classification/Export/ClassificationModel.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

namespace boost
{
  namespace serialization
  {

    template<class Archive>
    void serialize(Archive& ar, TPoint& Point, const unsigned int version)
    {
      ar & Point.mX;
      ar & Point.mY;
    }

    template<class Archive>
    void serialize(Archive& ar, TString& String, const unsigned int version)
    {
      if (Archive::is_loading::value)
      {
        std::string CString;
        ar & CString;
        String = TString(CString.c_str(), TString::kUtf8);
      }
      else 
      {
        std::string CString(String.StdCString(TString::kUtf8).c_str());
        ar & CString;
      }
    }

  }
} // namespace boost::serialization

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TClassificationModel::TClassificationModel()
  : mInputFeaturesSize(),
    mOutputClasses(),
    mpNormalizer(new shark::Normalizer<shark::RealVector>()),
    mOutlierLimits()
{ }

// -------------------------------------------------------------------------------------------------

TClassificationModel::~TClassificationModel()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

TString TClassificationModel::Name() const
{
  return OnName();
}

// -------------------------------------------------------------------------------------------------

TPoint TClassificationModel::InputFeaturesSize() const
{
  return mInputFeaturesSize;
}

// -------------------------------------------------------------------------------------------------

int TClassificationModel::NumberOfOutputClasses() const
{
  return (int)mOutputClasses.size();
}

// -------------------------------------------------------------------------------------------------

std::vector<TString> TClassificationModel::OutputClasses() const
{
  return mOutputClasses;
}

// -------------------------------------------------------------------------------------------------

const shark::Normalizer<shark::RealVector>* TClassificationModel::Normalizer() const
{
  return mpNormalizer;
}

shark::Normalizer<shark::RealVector>* TClassificationModel::Normalizer()
{
  return mpNormalizer;
}

// -------------------------------------------------------------------------------------------------

const shark::RealVector& TClassificationModel::OutlierLimits()const
{
  return mOutlierLimits;
}

// -------------------------------------------------------------------------------------------------

TClassificationTestResults TClassificationModel::Train(
  const TClassificationTestDataSet& DataSet,
  double                            TestSizeFraction)
{
  // memorize training parameters
  mInputFeaturesSize = DataSet.InputFeaturesSize();

  mOutputClasses.clear();
  for (int i = 0; i < DataSet.NumberOfClasses(); ++i)
  {
    mOutputClasses.push_back(DataSet.ClassNames()[i]);
  }

  mpNormalizer = TOwnerPtr< shark::Normalizer<shark::RealVector> >(
    new shark::Normalizer<shark::RealVector>(*DataSet.Normalizer()));

  mOutlierLimits = DataSet.OutlierLimits();

  // split data into train and validation data, train the model and return results
  return OnSplitDataAndTrain(DataSet, TestSizeFraction);
}

TClassificationTestResults TClassificationModel::Train(
  const TClassificationTestDataSet&   DataSet,
  const shark::ClassificationDataset& TrainData,
  const TList<TString>&               TrainDataSampleNames,
  const shark::ClassificationDataset& TestData,
  const TList<TString>                TestDataSampleNames,
  const TPoint&                       InputFeaturesSize,
  int                                 NumberOfClasses)
{ 
  // memorize training parameters
  mInputFeaturesSize = DataSet.InputFeaturesSize();

  mOutputClasses.clear();
  for (int i = 0; i < DataSet.NumberOfClasses(); ++i)
  {
    mOutputClasses.push_back(DataSet.ClassNames()[i]);
  }

  mpNormalizer = TOwnerPtr< shark::Normalizer<shark::RealVector> >(
    new shark::Normalizer<shark::RealVector>(*DataSet.Normalizer()));

  mOutlierLimits = DataSet.OutlierLimits();

  // train already split data and return the results
  return OnTrain(
    DataSet.DatabaseFileName(),
    TrainData,
    TrainDataSampleNames,
    TestData,
    TestDataSampleNames,
    InputFeaturesSize,
    NumberOfClasses);
}

// -------------------------------------------------------------------------------------------------

TClassificationTestResults TClassificationModel::OnSplitDataAndTrain(
  const TClassificationTestDataSet& DataSet,
  double                            TestSizeFraction)
{ 
  // split train and validation data

  TList<TString> TrainDataSampleNames, TestDataSampleNames;
  shark::ClassificationDataset TrainData, TestData;

  DataSet.CreateCrossValidationSet(&TrainData, &TrainDataSampleNames,
    &TestData, &TestDataSampleNames, TestSizeFraction);

  // train model
  return OnTrain(
    DataSet.DatabaseFileName(),
    TrainData,
    TrainDataSampleNames,
    TestData,
    TestDataSampleNames,
    DataSet.InputFeaturesSize(),
    DataSet.NumberOfClasses());
}

// -------------------------------------------------------------------------------------------------

TList<float> TClassificationModel::Evaluate(
  const TClassificationTestDataItem& Item) const
{
  if (Item.DescriptorSize() != mInputFeaturesSize.mX * mInputFeaturesSize.mY)
  {
    throw shark::Exception(
      "Test item and model descriptor count do not match. Model out of date?");
  }

  return OnEvaluate(Item);
}

// -------------------------------------------------------------------------------------------------

void TClassificationModel::Load(const TString& FileName)
{
  const std::ios_base::openmode Flags = std::ifstream::binary;

#if defined(MCompiler_VisualCPP)
  // Visual CPP has a w_char open impl, needed to properly open unicode filenames
  std::ifstream ifs(FileName.Chars(), Flags);
#elif defined(MCompiler_GCC)
  // kFileSystemEncoding should be utf-8 on linux/OSX, so this should be fine too
  std::ifstream ifs(FileName.StdCString(TString::kFileSystemEncoding), Flags);
#else
  #error "Unknown compiler"
#endif

  eos::portable_iarchive ia(ifs);

  Load(ia);

  ifs.close();
}

void TClassificationModel::Load(eos::portable_iarchive& Archive)
{
  mpNormalizer = TOwnerPtr< shark::Normalizer<shark::RealVector> >(
    new shark::Normalizer<shark::RealVector>());

  // features and normalizer
  Archive >> mInputFeaturesSize;
  Archive >> mOutputClasses;
  Archive >> *mpNormalizer;
  Archive >> mOutlierLimits;

  // model
  OnLoadModel(Archive);
}

// -------------------------------------------------------------------------------------------------

void TClassificationModel::Save(const TString& FileName) const
{
  const std::ios_base::openmode Flags = std::ofstream::binary;

#if defined(MCompiler_VisualCPP)
  std::ofstream ofs(FileName.Chars(), Flags); // see TClassificationModel::Load...
#elif defined(MCompiler_GCC)
  std::ofstream ofs(FileName.StdCString(TString::kFileSystemEncoding), Flags);
#else
  #error "Unknown compiler"
#endif

  eos::portable_oarchive oa(ofs);

  Save(oa);

  ofs.close();
}

void TClassificationModel::Save(eos::portable_oarchive& Archive) const
{
  // features and normalizer
  Archive << mInputFeaturesSize;
  Archive << mOutputClasses;
  Archive << *mpNormalizer;
  Archive << mOutlierLimits;

  // model
  OnSaveModel(Archive);
}

// -------------------------------------------------------------------------------------------------

TDirectory TClassificationModel::ModelDir() const
{
  TDirectory ModelDir = gApplicationResourceDir().Descend("Models");

  if (!ModelDir.Exists())
  {
    ModelDir.Create();
  }

  ModelDir.Descend(Name());
  if (!ModelDir.Exists())
  {
    ModelDir.Create();
  }

  return ModelDir;
}

// -------------------------------------------------------------------------------------------------

TDirectory TClassificationModel::ResultsDir() const
{
  TDirectory ModelDir = gApplicationResourceDir().Descend("Results");

  if (!ModelDir.Exists())
  {
    ModelDir.Create();
  }

  ModelDir.Descend(Name());
  if (!ModelDir.Exists())
  {
    ModelDir.Create();
  }

  return ModelDir;
}

