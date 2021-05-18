#include "CoreTypesPrecompiledHeader.h"

#include <algorithm>  // TList::Sort

// =================================================================================================

namespace 
{
  struct TSortByWeight 
  {
    bool operator()(
      const TWeightedPRNG::TWeightedItem& pNode1, 
      const TWeightedPRNG::TWeightedItem& pNode2)const
    {
      return pNode1.mWeight < pNode2.mWeight;
    }
  };
}

// =================================================================================================

#define MParkMillerPRNGMax 2147483647 // 2^31 - 1

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TParkMillerPRNG::TParkMillerPRNG()
  : mSeed(1)
{
}

// -------------------------------------------------------------------------------------------------

void TParkMillerPRNG::SetSeed(unsigned int Seed)
{
  mSeed = Seed;
}

// -------------------------------------------------------------------------------------------------

unsigned int TParkMillerPRNG::NextInt()
{
  return Generate();
}

// -------------------------------------------------------------------------------------------------

unsigned int TParkMillerPRNG::NextIntRange(unsigned int Min, unsigned int Max)
{
  MAssert(Max > Min, "Invalid range");

  return Min + (NextInt() % (Max - Min));
}

// -------------------------------------------------------------------------------------------------

double TParkMillerPRNG::NextDouble()
{
  return double(Generate()) / MParkMillerPRNGMax;
}

// -------------------------------------------------------------------------------------------------

double TParkMillerPRNG::NextDoubleRange(double Min, double Max)
{
  MAssert(Max > Min, "Invalid range");

  return Min + ((Max - Min) * NextDouble());
}

// -------------------------------------------------------------------------------------------------

unsigned int TParkMillerPRNG::Generate()
{
  // new-value = (old-value * 16807) mod (2^31 - 1)
  mSeed = (mSeed * 16807) % MParkMillerPRNGMax;
  return mSeed;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TWeightedPRNG::TWeightedPRNG()
  : mNeedUpdate(false),
    mSumOfWeights(0)
{
  // nothing special to do
}

// -------------------------------------------------------------------------------------------------

int TWeightedPRNG::AddValue(int Value, int Weight)
{
  const int Index = mItems.Size();

  TWeightedItem pItem;

  pItem.mIndex = Index;
  pItem.mValue = Value;
  pItem.mWeight = Weight;
  pItem.mThreshold = -1.0;

  mItems.Append(pItem);

  mIndexes.Append(Index);

  mNeedUpdate = true;

  return Index;
}

// -------------------------------------------------------------------------------------------------

void TWeightedPRNG::SetWeightByIndex(int Index, int Weight)
{
  MAssert(Index >= 0 && Index < mItems.Size(), "");

  mItems[mIndexes[Index]].mWeight = Weight;

  mNeedUpdate = true;
}

// -------------------------------------------------------------------------------------------------

void TWeightedPRNG::SetWeightByValue(int Value, int Weight)
{
  for (int i = 0; i < mItems.Size(); ++i)
  {
    if (mItems[i].mValue == Value)
    {
      mItems[i].mWeight = Weight;

      mNeedUpdate = true;

      return;
    }
  }  
}

// -------------------------------------------------------------------------------------------------

int TWeightedPRNG::NextIndex()
{
  if (mNeedUpdate)
  {
    Update();
  }

  if (mSumOfWeights > 0)
  {
    const double r = NextDouble();

    const int LastIndex = mItems.Size() - 1;

    for (int i = LastIndex; i >= 0; --i)
    {
      if (r >= mItems[i].mThreshold)
      {
        return mItems[i].mIndex;
      }
    }
  }

  return -1;
}

// -------------------------------------------------------------------------------------------------

int TWeightedPRNG::NextValue()
{
  const int Index = NextIndex();

  if (Index != -1)
  {
    MAssert(Index >= 0 && Index < mItems.Size(), "");

    return mItems[mIndexes[Index]].mValue;
  }

  return -1;
}

// -------------------------------------------------------------------------------------------------

int TWeightedPRNG::SumOfWeights()const
{
  int Sum = 0;

  const int NumItems = mItems.Size();
  for (int i = 0; i < NumItems; ++i)
  {
    Sum += mItems[i].mWeight;
  }

  return Sum;
}

// -------------------------------------------------------------------------------------------------

void TWeightedPRNG::Update()
{
  // Recalculate Weights
  mSumOfWeights = SumOfWeights();

  // Sort values
  if (mSumOfWeights > 0)
  {
    mItems.Sort(TSortByWeight());
  }

  // Rebuild indexes and calculate thresholds
  const int NumItems = mItems.Size();

  const double Scaling = (mSumOfWeights > 0) ?  1.0 / mSumOfWeights : 0.0;

  double Threshold = 0.0;

  for (int i = 0; i < NumItems; ++i)
  {
    mIndexes[mItems[i].mIndex] = i;

    if (mItems[i].mWeight > 0)
    {
      mItems[i].mThreshold = Threshold;

      Threshold += (mItems[i].mWeight * Scaling);
    }
    else
    {
      mItems[i].mThreshold = -1.0;
    }
  }

  mNeedUpdate = false;
}

