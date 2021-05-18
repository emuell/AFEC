#pragma once

#ifndef _SqliteSampleDescriptorPool_h_
#define _SqliteSampleDescriptorPool_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Directory.h"

#include "CoreFileFormats/Export/Database.h"

#include "FeatureExtraction/Export/SampleDescriptorPool.h"

// =================================================================================================

/*!
 * Writes TSampleDescriptors results into a SQlite database.
!*/

class TSqliteSampleDescriptorPool : public TSampleDescriptorPool
{
public:
  TSqliteSampleDescriptorPool(TSampleDescriptors::TDescriptorSet DescriptorSet);
  virtual ~TSqliteSampleDescriptorPool();


  //@{ ... TSampleDescriptorPool impl

  virtual bool IsEmpty() const override;

  virtual int NumberOfSamples() const override;
  virtual TOwnerPtr<TSampleDescriptors> Sample(int Index) const override;

  virtual TList< TPair<TString, int> > SampleModificationDates() const override;

  virtual void InsertSample(
    const TString&             FileName,
    const TSampleDescriptors&  Results) override;

  virtual void InsertFailedSample(
    const TString& FileName,
    const TString& Reason) override;

  virtual void RemoveSample(const TString& FileNames) override;
  virtual void RemoveSamples(const TList<TString>& FileNames) override;

  virtual void InsertClassifier(
    const TString&        ClassifierName,
    const TList<TString>& Classes) override;
  //@}


  //@{ ... New interface

  // db version: increase to force to recreate the database when running crawler on an 
  // existing db.
  enum { kCurrentVersion = 1 };

  // open databse. When \param ReadOnly is true, tables will not be created, in 
  // case they do not exist in the database.
  // !! NB: when opening in "Write" mode and the database needs to be upgraded, all 
  // existing tables will get dropped without any further checks or warnings !!
  bool Open(const TString& DatabaseName, bool ReadOnly = false);

  // when set, make all sample filenames relative to the given directory
  TDirectory BasePath()const;
  void SetBasePath(const TDirectory& BasePath);

  // check if the database needs to be upgraded. 
  // !! NB: open the database in read-only mode before checking, to
  // avoid that tables automatically get wiped clean in \function Open. !!
  bool DatabaseNeedsUpgrade()const;

  // normalize and convert abs to relative path, using 'BasePath'
  TString RelativeFilenamePath(const TString &SampleFileName) const;
  // normalize and convert relative to abs path, using 'BasePath'
  TString AbsFilenamePath(const TString &SampleFileName) const;
  //@}

private:
  void InitializeDatabase();
  void ShutdownDatabase();

  mutable TDatabase mDatabase;
  TDirectory mBasePath;
};


#endif // _SqliteSampleDescriptorPool_h_

