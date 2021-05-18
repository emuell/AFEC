#pragma once

#ifndef _ClassificationTestDataItem_h_
#define _ClassificationTestDataItem_h_

// =================================================================================================

#include "CoreTypes/Export/List.h"

#include "../../3rdParty/Shark/Export/SharkDataSet.h"

namespace shark {
  template<class DataTypeT>
  class Normalizer;
}

// =================================================================================================

class TSampleClassificationDescriptors;

template<class DataTypeT>
class TSharkClassificationModel;

// =================================================================================================

/*!
 * Single classification test data item, used to calc predictions for a
 * single sample on a trained shark model.
!*/

class TClassificationTestDataItem
{
public:
  //! Construct a test item from the given single TClassificationFeatures.
  //! Will convert the given descriptors to a shark data set and apply
  //! the given normalizer and outlierlimits
  TClassificationTestDataItem(
    const TSampleClassificationDescriptors&     Descriptors,
    const shark::Normalizer<shark::RealVector>& Normalizer,
    const shark::RealVector&                    OutlierLimits);

  //! number of input features in the set
  int DescriptorSize()const;

  //! const access to the (shark) test data. data will have one item only.
  const shark::Data<shark::RealVector>& TestData()const;

private:
  shark::Data<shark::RealVector> mTestData;
};


#endif // _TestDataSet_h_

