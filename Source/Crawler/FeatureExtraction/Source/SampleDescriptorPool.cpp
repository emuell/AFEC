#include "FeatureExtraction/Export/SampleDescriptorPool.h"

typedef TSampleDescriptors::TDescriptor TSampleDescriptor;

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TSampleDescriptorPool::TSampleDescriptorPool(
  TSampleDescriptors::TDescriptorSet DescriptorSet)
    : mDescriptorSet(DescriptorSet)
{
}

// -------------------------------------------------------------------------------------------------

TSampleDescriptorPool::~TSampleDescriptorPool()
{
  // just be virtual
}

// -------------------------------------------------------------------------------------------------

TSampleDescriptors::TDescriptorSet TSampleDescriptorPool::DescriptorSet()const 
{
  return mDescriptorSet;
}

// -------------------------------------------------------------------------------------------------

TDirectory TSampleDescriptorPool::BasePath()const
{
  return mBasePath;
}

// -------------------------------------------------------------------------------------------------

void TSampleDescriptorPool::SetBasePath(const TDirectory& BasePath)
{
  mBasePath = BasePath;
}

// -------------------------------------------------------------------------------------------------

TString TSampleDescriptorPool::RelativeFilenamePath(
  const TString &FileName) const
{
  TString Ret = FileName;

  if (! mBasePath.IsEmpty())
  {
    TString NormalizedFileName = FileName;
    NormalizedFileName.ReplaceChar(TDirectory::SWrongPathSeparatorChar(),
      TDirectory::SPathSeparatorChar());

    if (NormalizedFileName.StartsWith(mBasePath.Path()))
    {
      Ret = NormalizedFileName;
      Ret.RemoveFirst(mBasePath.Path());
    }
  }

  // always use '/' for relative paths to allow using the pool on Linux and OSX
  #if defined(MWindows)
    Ret.ReplaceChar('\\', '/');
  #endif

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TString TSampleDescriptorPool::AbsFilenamePath(
  const TString &FileName) const
{
  TString Ret = FileName;

  if (! mBasePath.IsEmpty())
  {
    TString NormalizedFileName = FileName;
    NormalizedFileName.ReplaceChar(TDirectory::SWrongPathSeparatorChar(),
      TDirectory::SPathSeparatorChar());

    if (!NormalizedFileName.StartsWith(mBasePath.Path()))
    {
      Ret = mBasePath.Path() + NormalizedFileName;
    }
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

bool TSampleDescriptorPool::NeedsUpgrade()const 
{
  MAssert(! mFileName.IsEmpty(), "Pool got not opened yet");
  return OnNeedsUpgrade();
}

// -------------------------------------------------------------------------------------------------

bool TSampleDescriptorPool::Open(const TString& FileName, bool ReadOnly) 
{
  if (OnOpen(FileName, ReadOnly)) 
  {
    mFileName = FileName;
    return true;
  }

  mFileName.Empty();
  return false;
}

// -------------------------------------------------------------------------------------------------

bool TSampleDescriptorPool::IsEmpty() const
{
  MAssert(! mFileName.IsEmpty(), "Pool got not opened yet");
  return OnIsEmpty(); 
}

// -------------------------------------------------------------------------------------------------

int TSampleDescriptorPool::NumberOfSamples() const
{
  MAssert(! mFileName.IsEmpty(), "Pool got not opened yet");
  return OnNumberOfSamples(); 
}

// -------------------------------------------------------------------------------------------------

TOwnerPtr<TSampleDescriptors> TSampleDescriptorPool::Sample(int Index) const
{
  MAssert(! mFileName.IsEmpty(), "Pool got not opened yet");
  return OnSample(Index); 
}

// -------------------------------------------------------------------------------------------------

TList< TPair<TString, int> > TSampleDescriptorPool::SampleModificationDates() const
{
  MAssert(! mFileName.IsEmpty(), "Pool got not opened yet");
  return OnSampleModificationDates(); 
}

// -------------------------------------------------------------------------------------------------

void TSampleDescriptorPool::InsertSample(
  const TString&            FileName,
  const TSampleDescriptors& Results)
{
  MAssert(! mFileName.IsEmpty(), "Pool got not opened yet");
  OnInsertSample(FileName, Results);
}

// -------------------------------------------------------------------------------------------------

void TSampleDescriptorPool::InsertFailedSample(
  const TString& FileName,
  const TString& Reason)
{
  MAssert(! mFileName.IsEmpty(), "Pool got not opened yet");
  OnInsertFailedSample(FileName, Reason);
}

// -------------------------------------------------------------------------------------------------

void TSampleDescriptorPool::RemoveSample(const TString& FileName)
{
  MAssert(! mFileName.IsEmpty(), "Pool got not opened yet");
  OnRemoveSample(FileName);
}

// -------------------------------------------------------------------------------------------------

void TSampleDescriptorPool::RemoveSamples(const TList<TString>& FileNames)
{
  MAssert(! mFileName.IsEmpty(), "Pool got not opened yet");
  OnRemoveSamples(FileNames);
}

// -------------------------------------------------------------------------------------------------

void TSampleDescriptorPool::InsertClassifier(
  const TString&        ClassifierName,
  const TList<TString>& Classes)
{
  MAssert(! mFileName.IsEmpty(), "Pool got not opened yet");
  OnInsertClassifier(ClassifierName, Classes);
}
