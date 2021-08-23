#pragma once

#ifndef _JsonSampleDescriptorPool_h_
#define _JsonSampleDescriptorPool_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Directory.h"

#include "CoreFileFormats/Export/Database.h"

#include "FeatureExtraction/Export/SampleDescriptorPool.h"

// =================================================================================================

/*!
 * Writes TSampleDescriptors results into a SQlite database.
!*/

class TJsonSampleDescriptorPool : public TSampleDescriptorPool
{
public:
  TJsonSampleDescriptorPool(TSampleDescriptors::TDescriptorSet DescriptorSet);
  virtual ~TJsonSampleDescriptorPool();

private:

  //@{ ... TSampleDescriptorPool impl

  virtual bool OnNeedsUpgrade()const override;
  virtual bool OnOpen(const TString& FileName, bool ReadOnly) override;

  virtual bool OnIsEmpty() const override;

  virtual int OnNumberOfSamples() const override;
  virtual TOwnerPtr<TSampleDescriptors> OnSample(int Index) const override;

  virtual TList< TPair<TString, int> > OnSampleModificationDates() const override;

  virtual void OnInsertSample(
    const TString&             FileName,
    const TSampleDescriptors&  Results) override;

  virtual void OnInsertFailedSample(
    const TString& FileName,
    const TString& Reason) override;

  virtual void OnRemoveSample(const TString& FileNames) override;
  virtual void OnRemoveSamples(const TList<TString>& FileNames) override;

  virtual void OnInsertClassifier(
    const TString&        ClassifierName,
    const TList<TString>& Classes) override;
  //@}
};

#endif // _JsonSampleDescriptorPool_h_
