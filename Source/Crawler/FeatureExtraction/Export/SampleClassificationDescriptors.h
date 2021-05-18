#pragma once

#ifndef _ClassificationFeatures_h_
#define _ClassificationFeatures_h_

// =================================================================================================

#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/Str.h"

#include <vector>
#include <string>

class TSampleDescriptors;

// =================================================================================================

/*!
 * Subset of TSampleDescriptors features, which is used as input data for
 * classificaion models.
!*/

class TSampleClassificationDescriptors
{
public:
  //! Number of time frames in feature set
  static int sNumberOfTimeFrames;
  //! Number of spectrum bands in the feature set
  static int sNumberOfSpectrumBands;

  //! Project init/exit function
  static void SInit();
  static void SExit();

  enum {
    // extract feature values 
    kExtractFeatureValues  = (1 << 0),
    // extract feature names
    kExtractFeatureNames   = (1 << 1)
  };
  typedef int TFeatureExtractionFlags;

  //! Default constructor - does nothing
  TSampleClassificationDescriptors();

  //! Create classification model features from the given TSampleDescriptors
  //! Extracts feature names and/or the features itself - see TFeatureExtractionFlags.
  TSampleClassificationDescriptors(
    const TSampleDescriptors& Descriptors, 
    TFeatureExtractionFlags   Flags = kExtractFeatureValues);

  //! resulting raw double vector representation of all inputs
  //! only valid when created with "kExtractFeatureValues"
  std::vector<double> mFeatures;
  
  //! vector representation of feature names
  //! only valid when created with "kExtractFeatureNames"
  std::vector<std::string> mFeatureNames;

  //! filename the features got extracted from
  TString mFileName;

private:
  static TOwnerPtr<TSampleDescriptors> spSilenceDescriptors;
};


#endif // _ClassificationModel_h_

