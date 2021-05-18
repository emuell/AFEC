#pragma once

#ifndef _ClassificationTestDataSet_h_
#define _ClassificationTestDataSet_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Point.h"
#include "CoreTypes/Export/Pointer.h"
#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/Array.h"

#include "../../3rdParty/Shark/Export/SharkDataSet.h"

namespace shark {
  template<class InputTypeT, class OutputTypeT>
  class AbstractModel;

  template<class DataTypeT>
  class Normalizer;
}

// =================================================================================================

class TSampleClassificationDescriptors;

// =================================================================================================

/*!
 * Classification test data set to train a classification model.
!*/

class TClassificationTestDataSet
{
public:
  //! create empty test data
  TClassificationTestDataSet();

  //! create a test data from the given TSampleClassificationDescriptors. 
  //! If the first descriptor also contains feature names, we'll memorize and
  //! export those too. The following descriptors are not checked for feature names.
  //! Throws TReabaleException or shark::Exception on errors
  TClassificationTestDataSet(
    const TString&                                  DatabaseFilename,
    const TList<TSampleClassificationDescriptors>&  Descriptors);

  //! Name and path of the data set
  TString DatabaseFileName()const;

  //! descriptor x time size in dataset
  TPoint InputFeaturesSize()const;
  //! dataset feature names - may not always be available
  TList<TString> InputFeatureNames()const;

  //! number of classes in the data set.
  int NumberOfClasses()const;
  //! names of all available classes
  TList<TString> ClassNames()const;

  //! number of samples in the dataset
  int NumberOfSamples()const;

  //! The trained normalizer which we applied to the data. When applying the model
  //! to other data, the normalizer must be applied to the data too
  const shark::Normalizer<shark::RealVector>* Normalizer() const;
  shark::Normalizer<shark::RealVector>* Normalizer();
  //! The limits which we used to clamp outliners in the test data
  const shark::RealVector& OutlierLimits()const;

  //! Create a new stratified test and train data cross validation set from 
  //! the whole data set using the given ratio.
  void CreateCrossValidationSet(
    shark::ClassificationDataset* pTrainData,
    TList<TString>*               pTrainDataSampleNames,
    shark::ClassificationDataset* pTestData,
    TList<TString>*               pTestDataSampleNames,
    double                        TestSizeFraction = 0.2)const;

  //! Create a set of \param NumberOfFolds stratified stacked cross-validation 
  //! sets. When consuming all folds, in for example an ensemble model, we can 
  //! this way gurantee that all data from the dataset got consumed.
  void CreateStackedCrossValidationSets(
    int                                   NumberOfFolds,
    TList<shark::ClassificationDataset>*  pTrainDataFolds,
    TList<TList<TString>>*                pTrainDataSampleNameFolds,
    TList<shark::ClassificationDataset>*  pTestDataFolds,
    TList<TList<TString>>*                pTestDataSampleNameFolds)const;

  //! Load/save the test data set from/to a file.
  //! Throws TReabaleException or shark::Exception on errors
  void LoadFromFile(const TString& FileName);
  void SaveToFile(const TString& FileName) const;

  // export as standard classification CSV data set, using column 0 for the label
  void ExportToCsvFile(const TString& FileName) const;

private:
  // make sure data looks OK and throw TReadableException when not
  void CheckDataIntegrity();

  TString mDatabaseFileName;

  shark::ClassificationDataset mData;
  
  TPoint mInputFeaturesSize;
  TList<TString> mInputFeatureNames;
  
  TList<TString> mSampleNames;
  TList<TString> mClassNames;

  TOwnerPtr< shark::Normalizer<shark::RealVector> > mpNormalizer;
  shark::RealVector mOutlierLimits;
};


#endif // _ClassificationTestDataSet_h_

