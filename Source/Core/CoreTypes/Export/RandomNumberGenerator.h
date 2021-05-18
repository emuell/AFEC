#pragma once

#ifndef _RandomNumberGenerator_h_
#define _RandomNumberGenerator_h_

// =================================================================================================

#include "CoreTypes/Export/List.h"

// =================================================================================================

/*
  Implementation of the Park Miller (1988) "minimal standard" linear 
  congruential pseudo-random number generator.

  For a full explanation visit: http://www.firstpr.com.au/dsp/rand31/

  The generator uses a modulus constant (m) of 2^31 - 1 which is a
  Mersenne Prime number and a full-period-multiplier of 16807.
  Output is a 31 bit unsigned integer. The range of values output is
  1 to 2,147,483,646 (2^31 - 1) and the seed must be in this range too.
*/

class TParkMillerPRNG : public TReferenceCountable
{
public:
  TParkMillerPRNG();
  void SetSeed(unsigned int Seed);

  //! get the next pseudo-random number as an int
  unsigned int NextInt();

  //! get the next pseudo-random number as an int within the given range
  unsigned int NextIntRange(unsigned int Min, unsigned int Max);

  //! get the next pseudo-random number as a double
  double NextDouble();

  //! get the next pseudo-random number as a double within the given range
  double NextDoubleRange(double Min, double Max);
  
protected:
  //! generates the next random unsigned int
  unsigned int Generate();

  //! seed should be between 1 and 2^31-1
  unsigned int mSeed;
};

// =================================================================================================

class TWeightedPRNG : public TParkMillerPRNG
{
public:

  struct TWeightedItem
  {
    int mIndex;
    int mValue;
    int mWeight;
    double mThreshold;
  };

  TWeightedPRNG();

  //! Add a new item with the given value and weight. Return its index.
  int AddValue(int Value, int Weight);

  //! Return the sum of all weights.
  int SumOfWeights()const;
  //! Set item weight by its index.
  void SetWeightByIndex(int Index, int Weight);
  //! Set item weight for the item with the given value. 
  //! Assumes all values are therefore unique, and only sets the weight of the
  //! first matching item in the list.
  void SetWeightByValue(int Value, int Weight);

  //! Randomly choose an item from the list and return its index.
  int NextIndex();
  //! Randomly choose an item from the list and return its value.
  int NextValue();

private:
  //! Called by NextValue() if any weights have changed.
  void Update();

  TList<TWeightedItem> mItems;
  TList<int> mIndexes;
  bool mNeedUpdate;
  int mSumOfWeights;
};


#endif // _RandomNumberGenerator_h_

