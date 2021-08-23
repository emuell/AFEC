#pragma once

#ifndef _SampleDescriptorPool_h_
#define _SampleDescriptorPool_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Directory.h"
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
  TSampleDescriptors::TDescriptorSet DescriptorSet()const;
  //@}


  //@{ ... Pool File Location & Base path

  // Pool version: increase to force to recreate the pool when running crawler 
  // on an existing pool db/file.
  enum { kCurrentVersion = 1 };

  // check if the pool needs to be upgraded. 
  // !! NB: open the pool in read-only mode before checking, to avoid that the pool 
  // structures (e.g. tables) automatically get wiped clean in \function Open. !!
  bool NeedsUpgrade()const;

  // Open an existing or new database. When \param ReadOnly is true, tables will not 
  // be created, in case they do not exist in the database.
  // !! NB: when opening in "Write" mode and the database needs to be upgraded, all 
  // existing tables will get dropped without any further checks or warnings !!
  bool Open(const TString& FileName, bool ReadOnly = false);

  // when set, make all sample filenames relative to the given directory
  TDirectory BasePath()const;
  void SetBasePath(const TDirectory& BasePath);
  //@}


  //@{ ... Access existing sample descriptors

  // check if there is any asset present (failed or succeeded)
  bool IsEmpty() const;

  // fetch samples (suceeded ones only)
  int NumberOfSamples() const;
  TOwnerPtr<TSampleDescriptors> Sample(int Index) const;

  // fetch abs path and modification stat time of all stored samples
  TList< TPair<TString, int> > SampleModificationDates() const;
  //@}


  //@{ ... Insert/replace/remove sample descriptors

  void InsertSample(
    const TString&            FileName,
    const TSampleDescriptors& Results);

  void InsertFailedSample(
    const TString& FileName,
    const TString& Reason);

  void RemoveSample(const TString& FileName);
  void RemoveSamples(const TList<TString>& FileNames);
  //@}


  //@{ ... Insert/replace class descriptors

  //! Set list classifier names (e.g. 'OneShot-Categories')
  //! Available in high level high level descriptor dbs only.
  void InsertClassifier(
    const TString&        ClassifierName,
    const TList<TString>& Classes);
  //@}

protected:
  TSampleDescriptorPool(TSampleDescriptors::TDescriptorSet DescriptorSet);
  virtual ~TSampleDescriptorPool();


  //@{ ... Base path helpers

  // normalize and convert abs to relative path, using 'BasePath'
  TString RelativeFilenamePath(const TString& SampleFileName) const;
  // normalize and convert relative to abs path, using 'BasePath'
  TString AbsFilenamePath(const TString& SampleFileName) const;
  //@}


  //@{ ... Pool Impl

  virtual bool OnNeedsUpgrade()const = 0;
  virtual bool OnOpen(const TString& FileName, bool ReadOnly) = 0;

  virtual bool OnIsEmpty() const = 0;

  virtual int OnNumberOfSamples() const = 0;
  virtual TOwnerPtr<TSampleDescriptors> OnSample(int Index) const = 0;

  virtual TList< TPair<TString, int> > OnSampleModificationDates() const = 0;

  virtual void OnInsertSample(
    const TString&            FileName,
    const TSampleDescriptors& Results) = 0;
  virtual void OnInsertFailedSample(
    const TString& FileName,
    const TString& Reason) = 0;

  virtual void OnRemoveSample(const TString& FileName) = 0;
  virtual void OnRemoveSamples(const TList<TString>& FileNames) = 0;

  virtual void OnInsertClassifier(
    const TString&        ClassifierName,
    const TList<TString>& Classes) = 0;
  //@}

  TString mFileName;
  TDirectory mBasePath;
  const TSampleDescriptors::TDescriptorSet mDescriptorSet;
};


#endif // _SampleDescriptorPool_h_
