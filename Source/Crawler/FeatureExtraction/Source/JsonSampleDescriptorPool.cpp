#include "FeatureExtraction/Export/JsonSampleDescriptorPool.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TJsonSampleDescriptorPool::TJsonSampleDescriptorPool(
  TSampleDescriptors::TDescriptorSet DescriptorSet)
  : TSampleDescriptorPool(DescriptorSet)
{ }

// -------------------------------------------------------------------------------------------------

TJsonSampleDescriptorPool::~TJsonSampleDescriptorPool()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

bool TJsonSampleDescriptorPool::OnNeedsUpgrade()const 
{
  MCodeMissing;
  return false;
}

// -------------------------------------------------------------------------------------------------

bool TJsonSampleDescriptorPool::OnOpen(const TString& FileName, bool ReadOnly)
{
  MCodeMissing;
  return false;
}
  
// -------------------------------------------------------------------------------------------------

bool TJsonSampleDescriptorPool::OnIsEmpty() const
{
  MCodeMissing;
  return true;
}

// -------------------------------------------------------------------------------------------------

int TJsonSampleDescriptorPool::OnNumberOfSamples() const
{
  MCodeMissing;
  return 0;
}

// -------------------------------------------------------------------------------------------------

TOwnerPtr<TSampleDescriptors> TJsonSampleDescriptorPool::OnSample(int Index) const
{
  MCodeMissing;
  return TOwnerPtr<TSampleDescriptors>();
}

// -------------------------------------------------------------------------------------------------

TList<TPair<TString, int>> TJsonSampleDescriptorPool::OnSampleModificationDates() const
{
  MCodeMissing;
  TList<TPair<TString, int>> Ret;
  return Ret;
}

// -------------------------------------------------------------------------------------------------

void TJsonSampleDescriptorPool::OnInsertSample(
  const TString&             FileName,
  const TSampleDescriptors&  Results)
{
  MCodeMissing;
}

// -------------------------------------------------------------------------------------------------

void TJsonSampleDescriptorPool::OnInsertFailedSample(
  const TString& FileName,
  const TString& Reason)
{
  MCodeMissing;
}

// -------------------------------------------------------------------------------------------------

void TJsonSampleDescriptorPool::OnRemoveSample(const TString& FileName)
{
  MCodeMissing;
}

// -------------------------------------------------------------------------------------------------

void TJsonSampleDescriptorPool::OnRemoveSamples(const TList<TString>& FileNames)
{
  MCodeMissing;
}

// -------------------------------------------------------------------------------------------------

void TJsonSampleDescriptorPool::OnInsertClassifier(
  const TString&        ClassifierName,
  const TList<TString>& Classes)
{
  // unused for JSON export
}
