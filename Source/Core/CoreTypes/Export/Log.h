#pragma once

#ifndef _Log_h_
#define _Log_h_

// =================================================================================================

#include "CoreTypes/Export/ReferenceCountable.h"
#include "CoreTypes/Export/Pointer.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Directory.h"
#include "CoreTypes/Export/File.h"
#include "CoreTypes/Export/Timer.h"

// =================================================================================================

class TLog : public TReferenceCountable
{
public:
  //! returns the instance of a global log, that should be used to log/show
  //! all global/application related messages.
  static TLog* SLog();
  
  //! To be called in CoreTypesInit to create the global log.
  static void SCreate();
  static void SDestroy();

  enum TFileMode
  {
    kAppend,  //! always appends the new entries to an existing log
    kReplace  //! flushes (deletes) the old file when created
  };

  TLog(
    const TDirectory& Directory, 
    const TString&    Name, 
    TFileMode         FileMode = kReplace, 
    bool              TraceLogContents = true);

  ~TLog();

  //! return the log's file name
  TString FileNameAndPath()const;

  //! Get/override if we should print a header with sytem info and dividers
  //! as the first log content. By default enabled. Must be disabled before
  //! the first entry gets logged.
  bool DumpLogHeader()const;
  void SetDumpLogHeader(bool DumpHeader);

  //! Get/override if we should print the log to the std out.
  //! as set in the constructor
  bool TraceLogContents()const;
  void SetTraceLogContents(bool Trace);
  
  //! Add a line to the log with the given category and line
  //! both arguments must be valid (non NULL).
  //! @throw(): Never throws exceptions. 
  #if defined(MCompiler_GCC)
    void AddLine(const char* pCategory, const char* pString, ...)
      __attribute__((format(printf, 3, 4)));
  #else
    void AddLine(const char* pCategory, const char* pString, ...);
  #endif
  
  //! Non var_arg version, which may be needed if you want to use var_arg 
  //! placeholders in \param pString without evaluating them.
  //! @throw(): Never throws exceptions. 
  void AddLineNoVarArgs(const char* pCategory, const char* pString);
  void AddLineNoVarArgs(const char* pCategory, const TString& String);
  
private:
  void CreateLogFile();
  void Dump(const char* pCategory, const char* pContent);
  
  TDirectory mDirectory;
  TString mName;
  
  TString mLastCategory;
  TStamp mLastLineStamp;

  TFileMode mFileMode;
  bool mTraceContents;
  bool mDumpHeader;

  TFile mLogFile;
  bool mAddingLogLine;

  static TPtr<TLog> spGlobalLog;
};


#endif // _Log_h_

