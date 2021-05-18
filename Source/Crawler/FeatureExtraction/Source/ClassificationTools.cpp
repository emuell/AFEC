#include "FeatureExtraction/Source/ClassificationTools.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TList<double> TClassificationTools::CategoryStrengths(
  const TList<double>& CategoriesWeights,
  double               MinWeight)
{
  TList<double> Ret;
  Ret.PreallocateSpace(CategoriesWeights.Size());

  // relative to the sum of other non zero weights
  double Sum = 0.0;
  for (int c = 0; c < CategoriesWeights.Size(); ++c)
  {
    if (CategoriesWeights[c] >= MinWeight) 
    {
      Sum += CategoriesWeights[c];
    }
  }

  for (int c = 0; c < CategoriesWeights.Size(); ++c)
  {
    if (CategoriesWeights[c] >= MinWeight && Sum > 0.0)
    {
      // strength = relative weight
      const double Weight = CategoriesWeights[c] / Sum;
      Ret.Append(Weight);
    }
    else
    {
      Ret.Append(0.0);
    }
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

// collect class name list from relative strength and filter the passed strengths
// which do not match any of the "strong" classes

TList<TString> TClassificationTools::PickAllStrongCategories(
  const std::vector<TString>& CategoryNames,
  TList<double>&              CategoryStrengths, // NB: in & out!
  double                      MinDefaultWeight,
  double                      MinFallbackWeight)
{
  MAssert((int)CategoryNames.size() == CategoryStrengths.Size(),
    "Unexpected category lists");

  // filter "strong" categories from weights
  TList<int> StrongCategories;

  for (int i = 0; i < CategoryStrengths.Size(); ++i)
  {
    double BestSoFar = 0.0;
    int BestSoFarIndex = -1;
    for (int j = 0; j < CategoryStrengths.Size(); ++j)
    {
      const double Weight = CategoryStrengths[j];
      if (Weight > MinDefaultWeight && Weight >= BestSoFar && !StrongCategories.Contains(j))
      {
        BestSoFar = Weight;
        BestSoFarIndex = j;
      }
    }

    if (BestSoFarIndex != -1)
    {
      StrongCategories.Append(BestSoFarIndex);
    }
    else
    {
      break;
    }
  }

  // include single strongest peak as fallback
  if (StrongCategories.IsEmpty())
  {
    auto Iter = std::max_element(CategoryStrengths.Begin(), CategoryStrengths.End());
    if (Iter.IsValid() && *Iter > MinFallbackWeight)
    {
      StrongCategories.Append(Iter.Index());
    }
  }

  // convert to category string list
  TList<TString> Categories;
  for (int i = 0; i < StrongCategories.Size(); ++i)
  {
    Categories.Append(CategoryNames[StrongCategories[i]]);
  }

  // when the strongest category is the special "None" category, ignore it and 
  // reset all remaining ones...
  if (! Categories.IsEmpty() && 
      gStringsEqualIgnoreCase(Categories.First(), "None"))
  {
    Categories.Empty();
    StrongCategories.Empty();
  }
  // remove secondary "None" categories as well
  for (int i = 0; i < Categories.Size(); ++i)
  {
    if (gStringsEqualIgnoreCase(Categories[i], "None")) 
    {
      StrongCategories.Delete(i);
      Categories.Delete(i);
      --i;
    }
  }

  // reset strengths for all not included classes, so they match the category names
  for (int i = 0; i < CategoryStrengths.Size(); ++i)
  {
    if (! StrongCategories.Contains(i))
    {
      CategoryStrengths[i] = 0.0;
    }
  }

  return Categories;
}

// -------------------------------------------------------------------------------------------------

TList<TString> TClassificationTools::PickStrongestCategory(
  const std::vector<TString>& CategoryNames,
  TList<double>&              CategoryStrengths) // NB: in & out!
{
  auto MaxIter = std::max_element(
    CategoryStrengths.Begin(),
    CategoryStrengths.End());

  // ignore/reset special "None" category
  if (MaxIter.IsValid() && 
      gStringsEqualIgnoreCase(CategoryNames[MaxIter.Index()], "None"))
  {
    MaxIter = CategoryStrengths.End();
  }

  // pick single strongest class
  if (MaxIter.IsValid())
  {
    // reset strengths for all not included classes, so they match the category name(s)
    for (int i = 0; i < CategoryStrengths.Size(); ++i)
    {
      if (i != MaxIter.Index())
      {
        CategoryStrengths[i] = 0.0;
      }
    }

    return MakeList<TString>(CategoryNames[MaxIter.Index()]);
  }
  else 
  {
    CategoryStrengths.Init(0.0);
    return TList<TString>();
  }
}

