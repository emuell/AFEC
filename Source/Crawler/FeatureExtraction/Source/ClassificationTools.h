#pragma once

#ifndef _ClassificationTools_h_
#define _ClassificationTools_h_

#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/Str.h"

#include <vector>

// =================================================================================================

/*!
 * Helper functions for classifier evaluation
 */

namespace TClassificationTools
{
  //! Convert raw, absolute classification output values into relative ones
  TList<double> CategoryStrengths(
    const TList<double>& CategoriesWeights,
    double               MinWeight = 0.05);

  //! Collect class name list from relative strength and filter the passed strengths
  //! which do not match any of the "strong" classes
  TList<TString> PickAllStrongCategories(
    const std::vector<TString>& CategoryNames,
    TList<double>&              CategoryStrengths, // NB: in & out!
    double                      MinDefaultWeight = 0.2,
    double                      MinFallbackWeight = 0.1);

  //! Collect class name list from relative strengths and filter out all passed 
  //! strengths except the strongest one
  TList<TString> PickStrongestCategory(
    const std::vector<TString>& CategoryNames,
    TList<double>&              CategoryStrengths); // NB: in & out!
};


#endif // _ClassificationTools_h_

