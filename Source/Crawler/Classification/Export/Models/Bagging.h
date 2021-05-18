#pragma once

#ifndef _BaggingClassificationModel_h_
#define _BaggingClassificationModel_h_

// =================================================================================================

#include "Classification/Export/ClassificationModel.h"

// =================================================================================================

/*!
 * An ensemble model which contains/trains multiple other models and evaluates
 * the mean of the trained models when predicting classes.
 *
 * Usually this is good for removing variance, especially on small train data set
 * but also makes models bigger and training a lot slower.
!*/

template <typename TModelType, size_t sNumberOfModels = 5>
class TBaggingClassificationModel : public TClassificationModel
{
public:
  TBaggingClassificationModel();

private:

  //@{ ... TClassificationModel Impl 

  virtual TString OnName() const override;

  virtual TClassificationTestResults OnSplitDataAndTrain(
    const TClassificationTestDataSet& DataSet,
    double                            TestSizeFraction) override;

  virtual TClassificationTestResults OnTrain(
    const TString&                      DatabaseFileName,
    const shark::ClassificationDataset& TrainData,
    const TList<TString>&               TrainDataSampleNames,
    const shark::ClassificationDataset& TestData,
    const TList<TString>                TestDataSampleNames,
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses) override;

  virtual TList<float> OnEvaluate(
    const TClassificationTestDataItem& Item) const override;

  virtual void OnLoadModel(
    eos::portable_iarchive& Archive) override;
  virtual void OnSaveModel(
    eos::portable_oarchive& Archive) const override;
  //@}

  TStaticArray< TPtr<TClassificationModel>, sNumberOfModels > mModels;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename TModelType, size_t sNumberOfModels>
TBaggingClassificationModel<TModelType, sNumberOfModels>::TBaggingClassificationModel()
{
  for (int i = 0; i < (int)sNumberOfModels; ++i)
  {
    mModels[i] = new TModelType();
  }
}

// -------------------------------------------------------------------------------------------------

template <typename TModelType, size_t sNumberOfModels>
TString TBaggingClassificationModel<TModelType, sNumberOfModels>::OnName() const
{
  return mModels.First()->Name() + "_Ensemble";
}

// -------------------------------------------------------------------------------------------------

template <typename TModelType, size_t sNumberOfModels>
TClassificationTestResults
TBaggingClassificationModel<TModelType, sNumberOfModels>::OnSplitDataAndTrain(
  const TClassificationTestDataSet& DataSet,
  double                            TestSizeFraction)
{
  MUnused(TestSizeFraction); // use 1.0 / sNumberOfModels instead

  TList<TClassificationTestResults> Results;
  TList< TPtr<TModelType> > Models;

  TList<shark::ClassificationDataset> TrainDataFolds;
  TList<TList<TString>> TrainDataSampleNameFolds;
  TList<shark::ClassificationDataset> TestDataFolds;
  TList<TList<TString>> TestDataSampleNameFolds;

  DataSet.CreateStackedCrossValidationSets(
    (int)sNumberOfModels,
    &TrainDataFolds,
    &TrainDataSampleNameFolds,
    &TestDataFolds,
    &TestDataSampleNameFolds
  );

  for (int i = 0; i < (int)sNumberOfModels; ++i)
  {
    gTraceVar("  Training ensemble model split %d/%d:", i + 1, (int)sNumberOfModels);

    // create a new model
    TPtr<TModelType> pNewModel = new TModelType();

    // train model
    const TClassificationTestResults Result =
      pNewModel->Train(
        DataSet, 
        TrainDataFolds[i], 
        TrainDataSampleNameFolds[i],
        TestDataFolds[i],
        TestDataSampleNameFolds[i],
        DataSet.InputFeaturesSize(),
        DataSet.NumberOfClasses());

    if (Result.mFinalError < 0.0f) // failed?
    {
      // bail out when models failed to train
      return TClassificationTestResults();
    }

    Results.Append(Result);
    Models.Append(pNewModel);
  }

  MAssert(Models.Size() == (int)sNumberOfModels, "");
  MAssert(Results.Size() == (int)sNumberOfModels, "");

  // memorize models
  for (int i = 0; i < (int)sNumberOfModels; ++i)
  {
    mModels[i] = Models[i];
  }

  // summarize results (mean of all trained sub models)
  TClassificationTestResults FinalResult = Results[0];

  for (int i = 1; i < (int)sNumberOfModels; ++i)
  {
    FinalResult.mFinalError += Results[i].mFinalError;

    if (Results[i].mFinalSecondaryError >= 0.0f)
    {
      FinalResult.mFinalSecondaryError += Results[i].mFinalSecondaryError;
    }

    FinalResult.mPredictionErrors += Results[i].mPredictionErrors;

    for (int cx = 0; cx < Results[i].mConfusionMatrix.Size(); ++cx)
    {
      for (int cy = 0; cy < Results[i].mConfusionMatrix[cx].Size(); ++cy)
      {
        FinalResult.mConfusionMatrix[cx][cy] += Results[i].mConfusionMatrix[cx][cy];
      }
    }
  }

  FinalResult.mFinalError /= (float)sNumberOfModels;

  if (FinalResult.mFinalSecondaryError >= 0.0f)
  {
    FinalResult.mFinalSecondaryError /= (float)sNumberOfModels;
  }

  return FinalResult;
}

// -------------------------------------------------------------------------------------------------

template <typename TModelType, size_t sNumberOfModels>
TClassificationTestResults TBaggingClassificationModel<TModelType, sNumberOfModels>::OnTrain(
  const TString&                      DatabaseFileName,
  const shark::ClassificationDataset& TrainData,
  const TList<TString>&               TrainSampleNames,
  const shark::ClassificationDataset& TestData,
  const TList<TString>                TestSampleNames,
  const TPoint&                       InputFeaturesSize,
  int                                 NumberOfClasses)
{
  throw TReadableException("OnTrain impl in TBaggingClassificationModel should not be used");
}

// -------------------------------------------------------------------------------------------------

template <typename TModelType, size_t sNumberOfModels>
TList<float> TBaggingClassificationModel<TModelType, sNumberOfModels>::OnEvaluate(
  const TClassificationTestDataItem& Item) const
{
  TStaticArray<TList<float>, sNumberOfModels> Results;

  for (int i = 0; i < (int)sNumberOfModels; ++i)
  {
    Results[i] = mModels[i]->Evaluate(Item);
  }

  TList<float> Ret;

  for (int c = 0; c < NumberOfOutputClasses(); ++c)
  {
    float Mean = 0.0;
    for (int i = 0; i < (int)sNumberOfModels; ++i)
    {
      Mean += Results[i][c];
    }
    Mean /= (float)sNumberOfModels;

    Ret.Append(Mean);
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

template <typename TModelType, size_t sNumberOfModels>
void TBaggingClassificationModel<TModelType, sNumberOfModels>::OnLoadModel(
  eos::portable_iarchive& Archive)
{
  for (int i = 0; i < (int)sNumberOfModels; ++i)
  {
    mModels[i]->Load(Archive);
  }
}

// -------------------------------------------------------------------------------------------------

template <typename TModelType, size_t sNumberOfModels>
void TBaggingClassificationModel<TModelType, sNumberOfModels>::OnSaveModel(
  eos::portable_oarchive& Archive) const
{
  for (int i = 0; i < (int)sNumberOfModels; ++i)
  {
    mModels[i]->Save(Archive);
  }
}

#endif // _BaggingClassificationModel_h_

