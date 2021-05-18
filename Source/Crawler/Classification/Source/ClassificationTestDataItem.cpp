#include "Classification/Export/ClassificationTestDataItem.h"
#include "Classification/Export/ClassificationModel.h"

#include "FeatureExtraction/Export/SampleClassificationDescriptors.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TClassificationTestDataItem::TClassificationTestDataItem(
  const TSampleClassificationDescriptors&     Descriptors,
  const shark::Normalizer<shark::RealVector>& Normalizer,
  const shark::RealVector&                    OutlierLimits)
{
  const std::size_t DataDimensions = Descriptors.mFeatures.size();

  // calculate batch sizes - well for one item only
  const std::vector<std::size_t> BatchSizes = shark::detail::optimalBatchSizes(
    1, shark::Data<shark::RealVector>::DefaultBatchSize);

  mTestData = shark::Data<shark::RealVector>(BatchSizes.size());

  // copy content into the batch
  MAssert(BatchSizes.size() == 1, "Expecting one batch for one element");

  shark::RealMatrix& DataInputBatch = mTestData.batch(0);
  DataInputBatch.resize(BatchSizes[0], DataDimensions);

  for (std::size_t j = 0; j < DataDimensions; ++j)
  {
    DataInputBatch(0, j) = Descriptors.mFeatures[j];
  }

  MAssert(mTestData.numberOfElements() == 1, "Expecting one element only");

  // normalize data
  mTestData = shark::transform(mTestData, Normalizer);

  // clamp outliers
  mTestData = shark::transform(mTestData,
    shark::Truncate(-OutlierLimits, OutlierLimits));
}

// -------------------------------------------------------------------------------------------------

int TClassificationTestDataItem::DescriptorSize()const
{
  return (int)shark::dataDimension(mTestData);
}

// -------------------------------------------------------------------------------------------------

const shark::Data<shark::RealVector>& TClassificationTestDataItem::TestData()const
{
  return mTestData;
}

