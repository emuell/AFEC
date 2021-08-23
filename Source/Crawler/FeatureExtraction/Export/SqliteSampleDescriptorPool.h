#pragma once

#ifndef _SqliteSampleDescriptorPool_h_
#define _SqliteSampleDescriptorPool_h_

// =================================================================================================

#include "CoreFileFormats/Export/Database.h"
#include "FeatureExtraction/Export/SampleDescriptorPool.h"

// =================================================================================================

/*!
 * Writes TSampleDescriptor results into a SQlite database.
!*/

class TSqliteSampleDescriptorPool : public TSampleDescriptorPool
{
public:
  TSqliteSampleDescriptorPool(TSampleDescriptors::TDescriptorSet DescriptorSet);
  virtual ~TSqliteSampleDescriptorPool();

private:
  void InitializeDatabase();
  void ShutdownDatabase();

  //@{ ... TSampleDescriptorPool impl

  virtual bool OnNeedsUpgrade()const override;
  virtual bool OnOpen(const TString& FileName, bool ReadOnly = false) override;

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

  mutable TDatabase mDatabase;
};

#endif // _SqliteSampleDescriptorPool_h_
