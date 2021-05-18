#pragma once

#ifndef _DefaultClassificationModel_h_
#define _DefaultClassificationModel_h_

// =================================================================================================

#include "Classification/Export/Models/Bagging.h"
#include "Classification/Export/Models/GBDT.h"

// =================================================================================================

//! typedef of the classification model that we are currently using for 
//! classification. If it's changed here, it's used in all other tools.

// the bagging "source model" 
typedef TGbdtClassificationModel TDefaultClassificationModel;

// the final model ensemble
typedef TBaggingClassificationModel<TDefaultClassificationModel, 5> 
  TDefaultBaggingClassificationModel;


#endif // _DefaultClassificationModel_h_

