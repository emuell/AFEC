#pragma once

#ifndef _ClassificationModel_h_
#define _ClassificationModel_h_

// =================================================================================================

#include "CoreTypes/Export/Array.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Directory.h"
#include "CoreTypes/Export/ReferenceCountable.h"

#include "Classification/Export/ClassificationTestDataSet.h"
#include "Classification/Export/ClassificationTestDataItem.h"
#include "Classification/Export/ClassificationTestResults.h"

// for shark::AbstractModel, Normalizer and serialization
#include "../../3rdParty/Shark/Export/SharkDataSet.h"

// =================================================================================================

/*!
 * Interface for training and (re)using classification models.
!*/

class TClassificationModel : public TReferenceCountable
{
public:
  TClassificationModel();
  virtual ~TClassificationModel();


  //@{ ... General info

  //! Short name of the Model
  TString Name() const;

  //! Number of input features
  TPoint InputFeaturesSize() const;

  //! Number of output features
  int NumberOfOutputClasses() const;
  //! Name of output classes
  std::vector<TString> OutputClasses() const;

  //! Normalizer we got trained with
  const shark::Normalizer<shark::RealVector>* Normalizer() const;
  shark::Normalizer<shark::RealVector>* Normalizer();

  //! Outlier limits which we've applied to our train data
  const shark::RealVector& OutlierLimits()const;
  //@}


  //@{ ... Training & Evaluation

  //! Train a new model with the given dataset and return the test result
  //! Default ratio: use 0.8 of the dataset for training, 0.2 for testing
  TClassificationTestResults Train(
    const TClassificationTestDataSet& DataSet,
    double                            TestSizeFraction = 0.2);
  //! Train a new model with the given already splitted datasets and return 
  //! the validation test results. Passed dataSet is only used as reference.
  TClassificationTestResults Train(
    const TClassificationTestDataSet&   DataSet,
    const shark::ClassificationDataset& TrainData,
    const TList<TString>&               TrainDataSampleNames,
    const shark::ClassificationDataset& TestData,
    const TList<TString>                TestDataSampleNames,
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses);

  //! Get class predictions for a single sample/item.
  TList<float> Evaluate(
    const TClassificationTestDataItem& Item) const;
  //@}


  //@{ ... Loading & Saving

  //! Load a pretrained model. @throws shark::Exception on errors.
  void Load(const TString& FileName);
  void Load(eos::portable_iarchive& Archive);
  //! Save a pretrained model. @throws shark::Exception on errors.
  void Save(const TString& FileName) const;
  void Save(eos::portable_oarchive& Archive) const;
  //@}


  //@{ ... Tools

  //! "Model/MODEL_NAME" directory, created when it does not exist
  TDirectory ModelDir() const;
  //! "Results/MODEL_NAME" directory, created when it does not exist
  TDirectory ResultsDir() const;
  //@}

protected:

  //@{ ... Impl 

  //! Short name of the model (ANN, CNN, ...)
  virtual TString OnName() const = 0;

  //! Dataset splitting.
  //! By default splits the DataSet for cross validation and then calls OnTrain
  virtual TClassificationTestResults OnSplitDataAndTrain(
    const TClassificationTestDataSet& DataSet,
    double                            TestSizeFraction);

  //! Train the model.
  virtual TClassificationTestResults OnTrain(
    const TString&                      DatabaseFileName,
    const shark::ClassificationDataset& TrainData,
    const TList<TString>&               TrainSampleNames,
    const shark::ClassificationDataset& TestData,
    const TList<TString>                TestSampleNames,
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses) = 0;

  //! Evaluate the pretrained model.
  virtual TList<float> OnEvaluate(
    const TClassificationTestDataItem& Item) const = 0;

  //! Load the pretrained model.
  virtual void OnLoadModel(
    eos::portable_iarchive& Archive) = 0;
  //! Save the pretrained model.
  virtual void OnSaveModel(
    eos::portable_oarchive& Archive) const = 0;
  //@}

private:
  TPoint mInputFeaturesSize;
  std::vector<TString> mOutputClasses;

  TOwnerPtr< shark::Normalizer<shark::RealVector> > mpNormalizer;
  shark::RealVector mOutlierLimits;
};

// =================================================================================================

/*!
 * TClassificationModel, which uses/trains either
 * a weighted <shark::RealVector, shark::RealVector> model (for example an ANN) or
 * an unweighted <shark::RealVector, unsigned int> model (for example a SVM).
!*/

template <typename TModelOutputType = shark::RealVector>
class TSharkClassificationModel : public TClassificationModel
{
public:
  typedef shark::AbstractModel<
    shark::RealVector, TModelOutputType> TSharkModelType;

  //! Access to the trained shark model
  const TSharkModelType* TrainedModel() const;
  TSharkModelType* TrainedModel();

protected:

  //@{ ... TClassificationModel impl 

  //! Train, evaluate model 
  virtual TClassificationTestResults OnTrain(
    const TString&                      DatabaseFileName,
    const shark::ClassificationDataset& TrainData,
    const TList<TString>&               TrainDataSampleNames,
    const shark::ClassificationDataset& TestData,
    const TList<TString>                TestDataSampleNames,
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses) override;

  //! Get class predictions for a single sample/item.
  virtual TList<float> OnEvaluate(
    const TClassificationTestDataItem& Item) const override;

  //! Load a pretrained model.
  virtual void OnLoadModel(
    eos::portable_iarchive& Archive) override;

  //! Save the pretrained model.
  virtual void OnSaveModel(
    eos::portable_oarchive& Archive) const override;
  //@}


  //@{ ... New impl 

  //! Create a new, empty, untrained model. 
  virtual TSharkModelType* OnCreateNewModel(
    const TPoint& InputFeaturesSize,
    int           NumberOfClasses) = 0;

  //! Train a new model on the given data set and return the trained model. 
  virtual TSharkModelType* OnCreateTrainedModel(
    const shark::ClassificationDataset& TrainData,
    const shark::ClassificationDataset& TestData,
    const TString&                      DataSetFileName,
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses) = 0;
  //@}

private:
  //! The pretrained model
  TOwnerPtr<TSharkModelType> mpTrainedModel;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename TModelOutputType>
const typename TSharkClassificationModel<TModelOutputType>::TSharkModelType*
TSharkClassificationModel<TModelOutputType>::TrainedModel() const
{
  return mpTrainedModel;
}

template <typename TModelOutputType>
typename TSharkClassificationModel<TModelOutputType>::TSharkModelType*
TSharkClassificationModel<TModelOutputType>::TrainedModel()
{
  return mpTrainedModel;
}

// -------------------------------------------------------------------------------------------------

template <typename TModelOutputType>
TClassificationTestResults TSharkClassificationModel<TModelOutputType>::OnTrain(
  const TString&                      DatabaseFileName,
  const shark::ClassificationDataset& TrainData,
  const TList<TString>&               TrainDataSampleNames,
  const shark::ClassificationDataset& TestData,
  const TList<TString>                TestDataSampleNames,
  const TPoint&                       InputFeaturesSize,
  int                                 NumberOfClasses)
{
  // ... train

  mpTrainedModel.Delete();

  try
  {
    mpTrainedModel = TOwnerPtr<TSharkModelType>(
      OnCreateTrainedModel(
        TrainData, 
        TestData,
        DatabaseFileName,
        InputFeaturesSize,
        NumberOfClasses));

    if (!mpTrainedModel)
    {
      throw shark::Exception("Failed to train model");
    }

    // evaluate and return final solution
    return TClassificationTestResults(TestData, TestDataSampleNames,
      (*mpTrainedModel)(TestData.inputs()));
  }
  catch (const shark::Exception& Exception)
  {
    MInvalid(Exception.what()); MUnused(Exception);
    return TClassificationTestResults();
  }
}

// -------------------------------------------------------------------------------------------------

template <>
inline TList<float> TSharkClassificationModel<unsigned int>::OnEvaluate(
  const TClassificationTestDataItem& TestItem) const
{
  MAssert(mpTrainedModel, "Train or load a model first");

  const shark::Data<unsigned int> PredictionsData =
    (*mpTrainedModel)(TestItem.TestData());

  shark::DataView< shark::Data<unsigned int> > PredictionsDataView(
    const_cast<shark::Data<unsigned int>&>(PredictionsData));

  MAssert(PredictionsDataView.size() == 1, "");

  const int NumberOfClasses = (int)NumberOfOutputClasses();

  TList<float> PredictionWeights;
  PredictionWeights.PreallocateSpace(NumberOfClasses);

  for (int k = 0; k < NumberOfClasses; k++)
  {
    PredictionWeights.Append(((int)PredictionsDataView[0] == k) ?
      1.0f : 0.0f);
  }

  return PredictionWeights;
}

template <>
inline TList<float> TSharkClassificationModel<shark::RealVector>::OnEvaluate(
  const TClassificationTestDataItem& TestItem) const
{
  MAssert(mpTrainedModel, "Train or load a model first");

  const shark::Data<shark::RealVector> PredictionsData =
    (*mpTrainedModel)(TestItem.TestData());

  shark::DataView< shark::Data<shark::RealVector> > PredictionsDataView(
    const_cast<shark::Data<shark::RealVector>&>(PredictionsData));

  MAssert(PredictionsDataView.size() == 1, "");
  shark::RealVector Predictions = PredictionsDataView[0];

  const int NumberOfClasses = (int)Predictions.size();
  MAssert(NumberOfClasses == NumberOfOutputClasses(), "");

  TList<float> PredictionWeights;
  PredictionWeights.PreallocateSpace(NumberOfClasses);

  for (int k = 0; k < NumberOfClasses; k++)
  {
    PredictionWeights.Append((float)Predictions(k));
  }

  return PredictionWeights;
}

// -------------------------------------------------------------------------------------------------

template <typename TModelOutputType>
void TSharkClassificationModel<TModelOutputType>::OnLoadModel(
  eos::portable_iarchive& Archive)
{
  mpTrainedModel = TOwnerPtr<TSharkModelType>(
    OnCreateNewModel(InputFeaturesSize(), NumberOfOutputClasses()));

  Archive >> *mpTrainedModel;
}

// -------------------------------------------------------------------------------------------------

template <typename TModelOutputType>
void TSharkClassificationModel<TModelOutputType>::OnSaveModel(
  eos::portable_oarchive& Archive) const
{
  MAssert(mpTrainedModel, "Train or load a model first");
  Archive << *mpTrainedModel;
}


#endif // _ClassificationModel_h_

