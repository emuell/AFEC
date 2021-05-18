#include "FeatureExtraction/Export/FeatureExtractionInit.h"
#include "FeatureExtraction/Export/SampleClassificationDescriptors.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void FeatureExtractionInit()
{
  TSampleClassificationDescriptors::SInit();
}

// -------------------------------------------------------------------------------------------------

void FeatureExtractionExit()
{
  TSampleClassificationDescriptors::SExit();
}

