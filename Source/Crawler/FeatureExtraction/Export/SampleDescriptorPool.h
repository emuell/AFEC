#pragma once

#ifndef _SampleDescriptorPool_h_
#define _SampleDescriptorPool_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Pointer.h"

#include "FeatureExtraction/Export/SampleDescriptors.h"

// =================================================================================================

/*!
 * Abstract interface for a descriptor pool. Implementations may dump/store
 * features into different file formats or db's or anything else.
!*/

class TSampleDescriptorPool
{
public:

  //@{ ... Descriptor types

  // the sample descriptor (sub)set the sample pool stores
  TSampleDescriptors::TDescriptorSet DescriptorSet()const 
  { 
    return mDescriptorSet; 
  }
  //@}


  //@{ ... Access existing sample descriptors

  // check if there is any asset present (failed or succeeded)
  virtual bool IsEmpty() const = 0;

  // fetch samples (suceeded ones only)
  virtual int NumberOfSamples() const = 0;
  virtual TOwnerPtr<TSampleDescriptors> Sample(int Index) const = 0;

  // fetch abs path and modification stat time of all stored samples
  virtual TList< TPair<TString, int> > SampleModificationDates() const = 0;
  //@}


  //@{ ... Insert/replace/remove sample descriptors

  virtual void InsertSample(
    const TString&            FileName,
    const TSampleDescriptors& Results) = 0;

  virtual void InsertFailedSample(
    const TString& FileName,
    const TString& Reason) = 0;

  virtual void RemoveSample(const TString& FileName) = 0;
  virtual void RemoveSamples(const TList<TString>& FileNames) = 0;
  //@}


  //@{ ... Insert/replace class descriptors

  //! Set list classifier names (e.g. 'OneShot-Categories')
  //! Available in high level high level descriptor dbs only.
  virtual void InsertClassifier(
    const TString&        ClassifierName,
    const TList<TString>& Classes) = 0;
  //@}

protected:
  TSampleDescriptorPool(TSampleDescriptors::TDescriptorSet DescriptorSet)
    : mDescriptorSet(DescriptorSet)
  { }

  virtual ~TSampleDescriptorPool()
  { }

  const TSampleDescriptors::TDescriptorSet mDescriptorSet;
};


#endif // _SampleDescriptorPool_h_

