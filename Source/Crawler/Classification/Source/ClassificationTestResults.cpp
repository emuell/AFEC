#include "Classification/Export/ClassificationTestResults.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static TList<TClassificationTestResults::TPredictionError> SCalcPredictionErrors(
  const shark::ClassificationDataset&   TestData,
  const TList<TString>&                 TestDataSampleNames,
  const shark::Data<unsigned int>&      Predictions)
{
  MAssert((int)TestData.numberOfElements() == TestDataSampleNames.Size(), "");

  // calc matrix 
  shark::DataView< shark::Data<unsigned int> > TestDataLabelsView(
    const_cast<shark::ClassificationDataset&>(TestData).labels());

  shark::DataView< shark::Data<unsigned int> > PredictionView(
    const_cast<shark::Data<unsigned int>&>(Predictions));

  MAssert(TestDataLabelsView.size() == PredictionView.size(), "");

  TList<TClassificationTestResults::TPredictionError> Ret;

  for (int i = 0; i < (int)TestDataLabelsView.size(); ++i)
  {
    const unsigned int RealClass = TestDataLabelsView[i];
    const unsigned int PredictedClass = PredictionView[i];

    if (RealClass != PredictedClass)
    {
      TClassificationTestResults::TPredictionError Error;
      Error.mName = TestDataSampleNames[i];
      Error.mClass = RealClass;
      Error.mPredictedClass = PredictedClass;
      Error.mSecondaryPredictedClass = PredictedClass;

      Ret.Append(Error);
    }
  }

  return Ret;
}

static TList<TClassificationTestResults::TPredictionError> SCalcPredictionErrors(
  const shark::ClassificationDataset&   TestData,
  const TList<TString>&                 TestDataSampleNames,
  const shark::Data<shark::RealVector>& Predictions)
{
  MAssert((int)TestData.numberOfElements() == TestDataSampleNames.Size(), "");

  const std::size_t numberOfClasses = shark::numberOfClasses(TestData);

  shark::DataView< shark::Data<unsigned int> > TestDataLabelsView(
    const_cast<shark::ClassificationDataset&>(TestData).labels());

  shark::DataView< shark::Data<shark::RealVector> > PredictionView(
    const_cast<shark::Data<shark::RealVector>&>(Predictions));

  MAssert(TestDataLabelsView.size() == PredictionView.size(), "");

  TList<TClassificationTestResults::TPredictionError> Ret;

  for (int i = 0; i < (int)TestDataLabelsView.size(); ++i)
  {
    const int RealClass = (int)TestDataLabelsView[i];
    shark::RealVector Predictions = PredictionView[i];

    int PredictedClass = -1;
    double BestMatchValue = -1.0;
    for (std::size_t k = 0; k < numberOfClasses; k++)
    {
      if (Predictions(k) >= BestMatchValue)
      {
        BestMatchValue = Predictions(k);
        PredictedClass = (int)k;
      }
    }

    const int FirstPredictedClass = PredictedClass;
    int SecondaryPredictedClass = -1;
    double SecondaryBestMatchValue = -1.0;
    for (std::size_t k = 0; k < numberOfClasses; k++)
    {
      if ((int)k != FirstPredictedClass &&
        Predictions(k) >= SecondaryBestMatchValue)
      {
        SecondaryBestMatchValue = Predictions(k);
        SecondaryPredictedClass = (int)k;
      }
    }

    if (PredictedClass != RealClass)
    {
      TClassificationTestResults::TPredictionError Error;
      Error.mName = TestDataSampleNames[i];
      Error.mClass = RealClass;
      Error.mPredictedClass = PredictedClass;
      Error.mSecondaryPredictedClass = SecondaryPredictedClass;
      Error.mPredictionWeights.SetSize((int)numberOfClasses);
      for (std::size_t k = 0; k < numberOfClasses; k++)
      {
        Error.mPredictionWeights[(int)k] = (float)Predictions(k);
      }

      Ret.Append(Error);
    }
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TArray< TArray<unsigned int> > SCalcConfusionMatrix(
  const shark::ClassificationDataset&   TestData,
  const shark::Data<unsigned int>&      Predictions)
{
  // initialize
  const std::size_t numberOfClasses = shark::numberOfClasses(TestData);

  TArray< TArray<unsigned int> > ConfusionMatrix((int)numberOfClasses);
  for (int i = 0; i < (int)numberOfClasses; ++i)
  {
    ConfusionMatrix[i].SetSize((int)numberOfClasses);
    ConfusionMatrix[i].Init(0);
  }

  // calc matrix 
  shark::DataView< shark::Data<unsigned int> > TestDataLabelsView(
    const_cast<shark::ClassificationDataset&>(TestData).labels());

  shark::DataView< shark::Data<unsigned int> > PredictionView(
    const_cast<shark::Data<unsigned int>&>(Predictions));

  MAssert(TestDataLabelsView.size() == PredictionView.size(), "");

  for (int i = 0; i < (int)TestDataLabelsView.size(); ++i)
  {
    const int RealClass = (int)TestDataLabelsView[i];
    const int PredictedClass = (int)PredictionView[i];

    ++ConfusionMatrix[RealClass][PredictedClass];
  }

  return ConfusionMatrix;
}

TArray< TArray<unsigned int> > SCalcConfusionMatrix(
  const shark::ClassificationDataset&   TestData,
  const shark::Data<shark::RealVector>& Predictions)
{
  // initialize
  const std::size_t numberOfClasses = shark::numberOfClasses(TestData);

  TArray< TArray<unsigned int> > ConfusionMatrix((int)numberOfClasses);
  for (int i = 0; i < (int)numberOfClasses; ++i)
  {
    ConfusionMatrix[i].SetSize((int)numberOfClasses);
    ConfusionMatrix[i].Init(0);
  }

  // calc matrix 
  shark::DataView< shark::Data<unsigned int> > TestDataLabelsView(
    const_cast<shark::ClassificationDataset&>(TestData).labels());

  shark::DataView< shark::Data<shark::RealVector> > PredictionView(
    const_cast<shark::Data<shark::RealVector>&>(Predictions));

  MAssert(TestDataLabelsView.size() == PredictionView.size(), "");

  for (int i = 0; i < (int)TestDataLabelsView.size(); ++i)
  {
    const int RealClass = (int)TestDataLabelsView[i];
    shark::RealVector Predictions = PredictionView[i];

    int PredictedClass = -1;
    double BestMatchValue = -1.0;
    for (std::size_t k = 0; k < numberOfClasses; k++)
    {
      if (Predictions(k) >= BestMatchValue)
      {
        BestMatchValue = Predictions(k);
        PredictedClass = (int)k;
      }
    }

    ++ConfusionMatrix[RealClass][PredictedClass];
  }

  return ConfusionMatrix;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TClassificationTestResults::TClassificationTestResults()
  : mFinalError(-1.0f),
    mFinalSecondaryError(-1.0f)
{ 
}

// -------------------------------------------------------------------------------------------------

TClassificationTestResults::TClassificationTestResults(
  const shark::ClassificationDataset&   TestData,
  const TList<TString>&                 TestDataNames,
  const shark::Data<shark::RealVector>& Predictions)
{
  mConfusionMatrix = SCalcConfusionMatrix(TestData, Predictions);
  mPredictionErrors = SCalcPredictionErrors(TestData, TestDataNames, Predictions);

  shark::ZeroOneLoss<unsigned int, shark::RealVector> Loss;
  mFinalError = (float)Loss(TestData.labels(), Predictions);

  int SecondaryMisclassified = 0;
  for (int i = 0; i < mPredictionErrors.Size(); ++i)
  {
    if (mPredictionErrors[i].mPredictedClass != mPredictionErrors[i].mClass &&
      mPredictionErrors[i].mSecondaryPredictedClass != mPredictionErrors[i].mClass)
    {
      ++SecondaryMisclassified;
    }
  }

  mFinalSecondaryError = (float)SecondaryMisclassified /
    (float)TestData.numberOfElements();
}

TClassificationTestResults::TClassificationTestResults(
  const shark::ClassificationDataset&   TestData,
  const TList<TString>&                 TestDataNames,
  const shark::Data<unsigned int>&      Predictions)
{
  shark::ZeroOneLoss<unsigned int, unsigned int> Loss;
  mFinalError = (float)Loss(TestData.labels(), Predictions);
  mFinalSecondaryError = 1.0;
  mConfusionMatrix = SCalcConfusionMatrix(TestData, Predictions);
  mPredictionErrors = SCalcPredictionErrors(TestData, TestDataNames, Predictions);
}

