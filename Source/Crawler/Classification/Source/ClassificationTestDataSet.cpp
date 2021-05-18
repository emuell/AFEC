#include "Classification/Export/ClassificationTestDataSet.h"
#include "Classification/Export/ClassificationModel.h"

#include "FeatureExtraction/Export/SampleClassificationDescriptors.h"

#include "CoreTypes/Export/File.h"
#include "CoreTypes/Export/Directory.h"

#include <shark/Data/CVDatasetTools.h>

#include <algorithm> // TList::Sort

// =================================================================================================

namespace boost {
  namespace serialization {

  // -----------------------------------------------------------------------------------------------

    template<class Archive>
    void serialize(Archive & ar, TPoint & Point, const unsigned int version)
    {
      ar & Point.mX;
      ar & Point.mY;
    }

    // ---------------------------------------------------------------------------------------------

    template<class Archive>
    void serialize(Archive & ar, TString & String, const unsigned int version)
    {
      if (Archive::is_loading::value)
      {
        std::string CString;
        ar & CString;
        String = TString(CString.c_str(), TString::kUtf8);
      }
      else
      {
        std::string CString(String.CString(TString::kUtf8));
        ar & CString;
      }
    }

    // ---------------------------------------------------------------------------------------------

    template<class Archive>
    void serialize(Archive & ar, TList<TString> & StringList, const unsigned int version)
    {
      if (Archive::is_loading::value)
      {
        int size = 0;
        ar & size;

        StringList.Empty();
        for (int i = 0; i < size; ++i)
        {
          TString String;
          ar & String;
          StringList.Append(String);
        }
      }
      else
      {
        int size = StringList.Size();
        ar & size;
        for (int i = 0; i < size; ++i)
        {
          ar & StringList[i];
        }
      }
    }

} } // namespace boost::serialization

// =================================================================================================

// -------------------------------------------------------------------------------------------------

//! Extract class paths from a list of filenames (unique base paths)
static TList<TString> SClassPaths(const TList<TString>& FileNames)
{
  TList<TString> ClassPaths;

  for (int i = 0; i < FileNames.Size(); ++i)
  {
    TDirectory FileBaseDirectory = gExtractPath(FileNames[i]);

    int Found = false;
    for (int c = 0; c < ClassPaths.Size(); ++c)
    {
      if (gStringsEqualIgnoreCase(FileBaseDirectory.Path(), ClassPaths[c]))
      {
        Found = true;
        break;
      }
    }

    if (!Found)
    {
      ClassPaths.Append(FileBaseDirectory.Path());
    }
  }

  // sort classes by name
  ClassPaths.Sort();

  return ClassPaths;
}

// -------------------------------------------------------------------------------------------------

//! Convert a class path to a pretty class name
static TList<TString> SClassNames(const TList<TString>& ClassPaths)
{
  TList<TString> ClassNames;
  ClassNames.PreallocateSpace(ClassPaths.Size());

  for (int i = 0; i < ClassPaths.Size(); ++i)
  {
    // assumes we're using nested folders for classes but the last name is unique
    ClassNames.Append(TDirectory(ClassPaths[i]).SplitPathComponents().Last());
  }

  return ClassNames;
}

// -------------------------------------------------------------------------------------------------

//! Get class index from a file name
static unsigned int SClassIndex(
  const TString&        FileName,
  const TList<TString>& ClassPaths)
{
  TDirectory FileBaseDirectory = gExtractPath(FileName);

  int ClassIndex = -1;
  for (int i = 0; i < ClassPaths.Size(); ++i)
  {
    if (gStringsEqualIgnoreCase(FileBaseDirectory.Path(), ClassPaths[i]))
    {
      ClassIndex = i;
      break;
    }
  }

  MAssert(ClassIndex != -1, "unknown class");
  return ClassIndex;
}

// -------------------------------------------------------------------------------------------------

//! Randomize elements in the given dataset and sync changes with the given 
//! attached SampleNames list

static void SRandomizeDataset(
  shark::ClassificationDataset& Dataset,
  TList<TString>&               DatasetSampleNames)
{
  MAssert((int)Dataset.numberOfElements() == DatasetSampleNames.Size(), 
    "Expecting same data layouts");

  shark::DiscreteUniform<shark::Rng::rng_type> uni(shark::Rng::globalRng);
  for (int i = (int)Dataset.numberOfElements() - 1; i > 0; --i)
  {
    const int j = (int)uni(i);

    std::iter_swap(Dataset.elements().begin() + i,
      Dataset.elements().begin() + j);

    DatasetSampleNames.Swap(i, j);
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TClassificationTestDataSet::TClassificationTestDataSet()
{
  // nothing to do (all default constructed)
}

TClassificationTestDataSet::TClassificationTestDataSet(
  const TString&                                  DatabaseFileName,
  const TList<TSampleClassificationDescriptors>&  Descriptors)
  : mDatabaseFileName(DatabaseFileName)
{
  // ... Extract filenames from descriptors

  TList<TString> FileNames;
  FileNames.PreallocateSpace(Descriptors.Size());

  for (int i = 0; i < Descriptors.Size(); ++i)
  {
    FileNames.Append(Descriptors[i].mFileName);
  }

  // ... Extract classes from filenames & memorize sample names

  const TList<TString> ClassPaths = SClassPaths(FileNames);

  mClassNames = SClassNames(ClassPaths);
  mSampleNames = FileNames;


  // ... Extract featurenames from descriptors (in the first entry)

  std::vector<std::string> FeatureNames = Descriptors.First().mFeatureNames;
  if (FeatureNames.size())
  {
    mInputFeatureNames.PreallocateSpace((int)FeatureNames.size());
    for (size_t i = 0; i < FeatureNames.size(); ++i)
    {
      mInputFeatureNames.Append(FeatureNames[i].c_str());
    }
  }

  // ... Create shark classification data from classification descriptors

  shark::Data<shark::RealVector> DataInputs;
  shark::Data<unsigned int> ClassLabels;

  // calculate batch sizes
  const std::size_t DataDimensions = Descriptors.First().mFeatures.size();

  const std::vector<std::size_t> BatchSizes = shark::detail::optimalBatchSizes(
    Descriptors.Size(), shark::Data<shark::RealVector>::DefaultBatchSize);

  DataInputs = shark::Data<shark::RealVector>(BatchSizes.size());
  ClassLabels = shark::Data<unsigned int>(BatchSizes.size());

  // copy content into the batches
  std::size_t CurrentRow = 0;
  for (std::size_t b = 0; b < BatchSizes.size(); ++b)
  {
    shark::RealMatrix& DataInputBatch = DataInputs.batch(b);
    DataInputBatch.resize(BatchSizes[b], DataDimensions);

    shark::blas::vector<unsigned int>& ClassLabelBatch = ClassLabels.batch(b);
    ClassLabelBatch.resize(BatchSizes[b]);

    for (std::size_t i = 0; i < BatchSizes[b]; ++i, ++CurrentRow)
    {
      if (Descriptors[(int)CurrentRow].mFeatures.size() != DataDimensions)
        throw SHARKEXCEPTION("vectors are required to have same size");

      for (std::size_t j = 0; j < DataDimensions; ++j)
      {
        DataInputBatch(i, j) = Descriptors[(int)CurrentRow].mFeatures[j];
      }

      ClassLabelBatch[i] = SClassIndex(FileNames[(int)CurrentRow], ClassPaths);
    }
  }

  // convert into classification data
  mData = shark::ClassificationDataset(DataInputs, ClassLabels);

  mInputFeaturesSize.mX = TSampleClassificationDescriptors::sNumberOfTimeFrames;
  mInputFeaturesSize.mY = (int)DataDimensions / mInputFeaturesSize.mX;

  if ((DataDimensions % TSampleClassificationDescriptors::sNumberOfTimeFrames) != 0)
  {
    throw shark::Exception(
      "Data parser error: unexpected data element count");
  }


  // ... Validate data and meta descriptors

  CheckDataIntegrity();


  // ... Normalize Data

  // train normalizer
  mpNormalizer = TOwnerPtr< shark::Normalizer<shark::RealVector> >(
    new shark::Normalizer<shark::RealVector>());

  const bool RemoveMean = true;
  shark::NormalizeComponentsUnitVariance<> NormalizingTrainer(RemoveMean);
  NormalizingTrainer.train(*mpNormalizer, mData.inputs());
  
  // but don't normalize spectrum bands (reset trained weights)
  // so we can use them as input for CNNs or LSTMs
  auto Parameters = mpNormalizer->parameterVector();
  const int NumberOfUnnormalizedFeatures = 
    TSampleClassificationDescriptors::sNumberOfTimeFrames * 
    TSampleClassificationDescriptors::sNumberOfSpectrumBands;
  for (int i = 0; i < NumberOfUnnormalizedFeatures; ++i)
  {
    Parameters[i] = 1; // scaling
    Parameters[i + mpNormalizer->inputSize()] = 0; // offset
  }
  mpNormalizer->setParameterVector(Parameters);

  // apply normalizer
  mData = shark::transformInputs(mData, *mpNormalizer);

  // remove outliers outside of +/- 3 * standard deviation
  mOutlierLimits = 3.0 * shark::blas::sqrt(shark::variance(mData.inputs()));

  mData.inputs() = shark::transform(mData.inputs(),
    shark::Truncate(-mOutlierLimits, mOutlierLimits));
}

// -------------------------------------------------------------------------------------------------

TString TClassificationTestDataSet::DatabaseFileName()const
{
  return mDatabaseFileName;
}

// -------------------------------------------------------------------------------------------------

TPoint TClassificationTestDataSet::InputFeaturesSize()const
{
  return mInputFeaturesSize;
}

// -------------------------------------------------------------------------------------------------

TList<TString> TClassificationTestDataSet::InputFeatureNames()const 
{
  return mInputFeatureNames;
}

// -------------------------------------------------------------------------------------------------

int TClassificationTestDataSet::NumberOfClasses()const
{
  return mClassNames.Size();
}

// -------------------------------------------------------------------------------------------------

TList<TString> TClassificationTestDataSet::ClassNames()const
{
  return mClassNames;
}

// -------------------------------------------------------------------------------------------------

int TClassificationTestDataSet::NumberOfSamples()const
{
  return mSampleNames.Size();
}

// -------------------------------------------------------------------------------------------------

const shark::Normalizer<shark::RealVector>*
TClassificationTestDataSet::Normalizer() const
{
  return mpNormalizer;
}

shark::Normalizer<shark::RealVector>*
TClassificationTestDataSet::Normalizer()
{
  return mpNormalizer;
}

// -------------------------------------------------------------------------------------------------

const shark::RealVector& TClassificationTestDataSet::OutlierLimits()const
{
  return mOutlierLimits;
}

// -------------------------------------------------------------------------------------------------

void TClassificationTestDataSet::CreateCrossValidationSet(
  shark::ClassificationDataset* pTrainData,
  TList<TString>*               pTrainDataSampleNames,
  shark::ClassificationDataset* pTestData,
  TList<TString>*               pTestDataSampleNames,
  double                        TestSizeFraction)const
{
  MAssert(TestSizeFraction > 0.0f && TestSizeFraction < 1.0f, "Invalid ratio");

  MAssert(pTrainData && pTrainData->empty(), "");
  MAssert(pTrainDataSampleNames && pTrainDataSampleNames->IsEmpty(), "");

  MAssert(pTestData && pTestData->empty(), "");
  MAssert(pTestDataSampleNames && pTestDataSampleNames->IsEmpty(), "");

  // copy data (createCVSameSizeBalanced changes the layout)
  shark::ClassificationDataset Dataset = mData;
  Dataset.makeIndependent();

  // create data set for the sample names, using the same batch layout as the data
  shark::LabeledData<TString, unsigned int> SampleNameDataset;
  
  int ElementIndex = 0;
  for (auto Batch : Dataset.batches())
  {
    std::vector<TString> Inputs(Batch.size());
    for (size_t i = 0; i < Batch.size(); ++i)
    {
      Inputs[i] = mSampleNames[ElementIndex];
      ++ElementIndex;
    }
    SampleNameDataset.push_back(Inputs, Batch.label);
  }

  // create (balanced) data folds
  const int NumberOfFolds = TMath::d2iRound(1.0 / TestSizeFraction);
  MAssert(NumberOfFolds > 0, "");

  shark::RecreationIndices CvIndices;
  const shark::CVFolds<shark::ClassificationDataset> DatasetFolds =
    shark::createCVSameSizeBalanced(Dataset, NumberOfFolds,
      shark::ClassificationDataset::DefaultBatchSize, &CvIndices);

  // create fold of sample names using DatasetFolds as template (via CvIndices)
  const shark::CVFolds<shark::LabeledData<TString, unsigned int>> SampleNameDatasetFolds =
    shark::createCVFullyIndexed(SampleNameDataset, NumberOfFolds,
      CvIndices, shark::ClassificationDataset::DefaultBatchSize);

  // get data from DatasetFolds
  *pTrainData = DatasetFolds.training(0);
  *pTestData = DatasetFolds.validation(0);

  // get sample names from SampleNameDatasetFolds
  shark::LabeledData<TString, unsigned int> TrainDataNames =
    SampleNameDatasetFolds.training(0);
  shark::LabeledData<TString, unsigned int> TestDataNames = 
    SampleNameDatasetFolds.validation(0);

  pTrainDataSampleNames->Empty();
  pTrainDataSampleNames->PreallocateSpace((int)TrainDataNames.numberOfElements());
  for (auto SampleName : TrainDataNames.elements())
  {
    pTrainDataSampleNames->Append(SampleName.input);
  }

  pTestDataSampleNames->Empty();
  pTestDataSampleNames->PreallocateSpace((int)TestDataNames.numberOfElements());
  for (auto SampleName : TestDataNames.elements())
  {
    pTestDataSampleNames->Append(SampleName.input);
  }

  // Randomize train and test sets
  SRandomizeDataset(*pTrainData, *pTrainDataSampleNames);
  SRandomizeDataset(*pTestData, *pTestDataSampleNames);

  MAssert(pTrainDataSampleNames->Size() == (int)pTrainData->numberOfElements(), "");
  MAssert(pTestDataSampleNames->Size() == (int)pTestData->numberOfElements(), "");
}

// -------------------------------------------------------------------------------------------------

void TClassificationTestDataSet::CreateStackedCrossValidationSets(
  int                                   NumberOfFolds,
  TList<shark::ClassificationDataset>*  pTrainDataFolds,
  TList<TList<TString>>*                pTrainDataSampleNameFolds,
  TList<shark::ClassificationDataset>*  pTestDataFolds,
  TList<TList<TString>>*                pTestDataSampleNameFolds)const
{
  MAssert(NumberOfFolds > 0, "");

  // copy data (createCVSameSizeBalanced changes the layout)
  shark::ClassificationDataset Dataset = mData;
  Dataset.makeIndependent();

  // create data set for the sample names, using the same batch layout as the data
  shark::LabeledData<TString, unsigned int> SampleNameDataset;

  int ElementIndex = 0;
  for (auto Batch : Dataset.batches())
  {
    std::vector<TString> Inputs(Batch.size());
    for (size_t i = 0; i < Batch.size(); ++i)
    {
      Inputs[i] = mSampleNames[ElementIndex];
      ++ElementIndex;
    }
    SampleNameDataset.push_back(Inputs, Batch.label);
  }

  // create (balanced) data folds
  shark::RecreationIndices CvIndices;
  const shark::CVFolds<shark::ClassificationDataset> DatasetFolds =
    shark::createCVSameSizeBalanced(Dataset, NumberOfFolds,
      shark::ClassificationDataset::DefaultBatchSize, &CvIndices);

  // create fold of sample names using DatasetFolds as template (via CvIndices)
  const shark::CVFolds<shark::LabeledData<TString, unsigned int>> SampleNameDatasetFolds =
    shark::createCVFullyIndexed(SampleNameDataset, NumberOfFolds,
      CvIndices, shark::ClassificationDataset::DefaultBatchSize);

  // assign data from folds
  pTrainDataFolds->Empty();
  pTrainDataSampleNameFolds->Empty();
  pTestDataFolds->Empty();
  pTestDataSampleNameFolds->Empty();

  for (size_t FoldIndex = 0; FoldIndex < DatasetFolds.size(); ++FoldIndex)
  {
    // get data from DatasetFolds
    shark::ClassificationDataset TrainData = DatasetFolds.training(FoldIndex);
    shark::ClassificationDataset TestData = DatasetFolds.validation(FoldIndex);

    // get sample names from SampleNameDatasetFolds
    const shark::LabeledData<TString, unsigned int> TrainDataNames =
      SampleNameDatasetFolds.training(FoldIndex);
    const shark::LabeledData<TString, unsigned int> TestDataNames =
      SampleNameDatasetFolds.validation(FoldIndex);

    TList<TString> TrainDataSampleNames;
    TrainDataSampleNames.PreallocateSpace((int)TrainDataNames.numberOfElements());
    for (auto SampleName : TrainDataNames.elements())
    {
      TrainDataSampleNames.Append(SampleName.input);
    }

    TList<TString> TestDataSampleNames;
    TestDataSampleNames.PreallocateSpace((int)TestDataNames.numberOfElements());
    for (auto SampleName : TestDataNames.elements())
    {
      TestDataSampleNames.Append(SampleName.input);
    }
  
    // Randomize train and test sets
    SRandomizeDataset(TrainData, TrainDataSampleNames);
    SRandomizeDataset(TestData, TestDataSampleNames);

    // push to final folds
    MAssert(TrainDataSampleNames.Size() == (int)TrainData.numberOfElements(), "");
    MAssert(TestDataSampleNames.Size() == (int)TestData.numberOfElements(), "");

    pTrainDataFolds->Append(TrainData);
    pTrainDataSampleNameFolds->Append(TrainDataSampleNames);

    pTestDataFolds->Append(TestData);
    pTestDataSampleNameFolds->Append(TestDataSampleNames);
  }
}

// -------------------------------------------------------------------------------------------------

void TClassificationTestDataSet::ExportToCsvFile(const TString& Filename) const 
{
  #if defined(MCompiler_VisualCPP)
    std::ofstream FileStream(Filename.Chars());
  #elif defined(MCompiler_GCC)
    std::ofstream FileStream(Filename.CString(TString::kFileSystemEncoding));
  #else
    #error "Unknown compiler"
  #endif

  // write header
  if (mInputFeatureNames.Size() > 0) 
  {
    FileStream << "label,";
    FileStream << "id,";
    for (int i = 0; i < mInputFeatureNames.Size(); ++i)
    {
      FileStream << mInputFeatureNames[i].CString();
      if (i < mInputFeatureNames.Size() - 1)
      {
        FileStream << ",";
      }
    }
    FileStream << std::endl;
  }


  // write content
  auto IdIter = mSampleNames.Begin();
  auto DataIter = mData.inputs().elements().begin();
  auto LabelIter = mData.labels().elements().begin();

  if (DataIter->begin() == DataIter->end())
  {
    // no content present
    FileStream.close();
    return;
  }

  const shark::LabelPosition LabelPosition = shark::FIRST_COLUMN;
  const char Separator = ',';
  const std::streamsize Precision = 10;

  FileStream.setf(std::ios_base::scientific);
  FileStream.precision(Precision);

  for (; DataIter != mData.inputs().elements().end(); 
        ++IdIter, ++DataIter, ++LabelIter)
  {
    // label (on FIRST_COLUMN)
    if (LabelPosition == shark::FIRST_COLUMN)
    {
      FileStream << *LabelIter << Separator;
    }

    // id
    FileStream << "\"" << (*IdIter).CString(TString::kUtf8) << 
      "\"" << Separator;

    // feature values
    for (std::size_t i = 0; i<(*DataIter).size() - 1; i++)
    {
      FileStream << (*DataIter)(i) << Separator;
    }
    
    // label (on LAST_COLUMN)
    if (LabelPosition == shark::FIRST_COLUMN)
    {
      FileStream << (*DataIter)((*DataIter).size() - 1) << std::endl;
    }
    else
    {
      FileStream << (*DataIter)((*DataIter).size() - 1) << 
        Separator << *LabelIter << std::endl;
    }
  }

  FileStream.close();
}

// -------------------------------------------------------------------------------------------------

void TClassificationTestDataSet::LoadFromFile(const TString& FileName)
{
  const std::ios_base::openmode Flags = std::ifstream::binary;

  #if defined(MCompiler_VisualCPP)
    // Visual CPP has a w_char open impl, needed to properly open unicode filenames
    std::ifstream ifs(FileName.Chars(), Flags);
  #elif defined(MCompiler_GCC)
    // kFileSystemEncoding should be utf-8 on linux/OSX, so this should be fine too
    std::ifstream ifs(FileName.CString(TString::kFileSystemEncoding), Flags);
  #else
    #error "Unknown compiler"
  #endif

  eos::portable_iarchive ia(ifs);

  mpNormalizer = TOwnerPtr< shark::Normalizer<shark::RealVector> >(
    new shark::Normalizer<shark::RealVector>());

  ia >> mDatabaseFileName;
  ia >> mData;
  ia >> mInputFeaturesSize;
  ia >> mInputFeatureNames;
  ia >> mSampleNames;
  ia >> mClassNames;
  ia >> *mpNormalizer;
  ia >> mOutlierLimits;

  // Validate loaded data and meta descriptors
  CheckDataIntegrity();
}

// -------------------------------------------------------------------------------------------------

void TClassificationTestDataSet::SaveToFile(const TString& FileName) const
{
  const std::ios_base::openmode Flags = std::ofstream::binary;

  #if defined(MCompiler_VisualCPP)
    std::ofstream ofs(FileName.Chars(), Flags); // see LoadFromFile...
  #elif defined(MCompiler_GCC)
    std::ofstream ofs(FileName.CString(TString::kFileSystemEncoding), Flags);
  #else
    #error "Unknown compiler"
  #endif

  eos::portable_oarchive oa(ofs);

  oa << mDatabaseFileName;
  oa << mData;
  oa << mInputFeaturesSize;
  oa << mInputFeatureNames;
  oa << mSampleNames;
  oa << mClassNames;
  oa << *mpNormalizer;
  oa << mOutlierLimits;
}

// -------------------------------------------------------------------------------------------------

void TClassificationTestDataSet::CheckDataIntegrity()
{
  if ((int)mData.numberOfElements() != mSampleNames.Size())
  {
    throw shark::Exception(
      "Data parser error: data and sample names don't match");
  }

  if ((int)shark::inputDimension(mData) !=
    mInputFeaturesSize.mX * mInputFeaturesSize.mY)
  {
    throw shark::Exception(
      "Data parser error: data and data-time sizes don't match");
  }

  if ((int)shark::numberOfClasses(mData) != mClassNames.Size())
  {
    throw shark::Exception(
      "Data parser error: data and meta class names don't match");
  }
}

