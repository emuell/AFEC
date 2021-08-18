#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Version.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

// =================================================================================================

#define MSeperator (60 * TString(L"="))

// =================================================================================================

//! NB: not in a TGlobalStorage, we really only want one instance.
TPtr<TLog> TLog::spGlobalLog;

// -------------------------------------------------------------------------------------------------

TLog* TLog::SLog()
{
  if (spGlobalLog == NULL) 
  {
    #if defined(MMac)
      const TString LogName(MProductString + ".log");
    #else
      const TString LogName("Log.txt");
    #endif

    spGlobalLog = new TLog(gUserLogDir(), LogName, TLog::kAppend);
  }

  return spGlobalLog;
}

// -------------------------------------------------------------------------------------------------

void TLog::SCreate()
{
  // nothing to do: log is created when accessed the first time
}

// -------------------------------------------------------------------------------------------------

void TLog::SDestroy()
{
  spGlobalLog.Delete();
}

// -------------------------------------------------------------------------------------------------

TLog::TLog(
  const TDirectory& Directory, 
  const TString&    Name, 
  TFileMode         FileMode,
  bool              TraceContents)
  : mDirectory(Directory),
    mName(Name),
    mFileMode(FileMode),
    mTraceContents(TraceContents),
    mDumpHeader(true),
    mAddingLogLine(false)
{
  mLogFile.SetFileName(mDirectory.Path() + mName);
}

// -------------------------------------------------------------------------------------------------

TLog::~TLog()
{
  if (mLogFile.IsOpen())
  {
    Dump("", "Closing log file...");
    mLogFile.Close();
  }
}

// -------------------------------------------------------------------------------------------------

TString TLog::FileNameAndPath()const
{
  return mLogFile.FileName();
}

// -------------------------------------------------------------------------------------------------

bool TLog::DumpLogHeader()const
{
  return mDumpHeader;
}

// -------------------------------------------------------------------------------------------------

void TLog::SetDumpLogHeader(bool DumpHeader)
{
  mDumpHeader = DumpHeader;
}

// -------------------------------------------------------------------------------------------------

bool TLog::TraceLogContents()const
{
  return mTraceContents;
}

// -------------------------------------------------------------------------------------------------

void TLog::SetTraceLogContents(bool Trace)
{
  mTraceContents = Trace;
}
  
// -------------------------------------------------------------------------------------------------

void TLog::CreateLogFile()
{
  try
  {
    MAssert(! mLogFile.IsOpen(), "Already open!");
    MAssert(! mName.IsEmpty(), "No name was set");

    // ... make sure the log wont grow beyond 4MB
 
    bool CreatingNewFile = !mLogFile.Exists();
  
    if (!CreatingNewFile && mFileMode == kAppend)
    {
      const size_t FileSizeInBytes = 
        TFile(mDirectory.Path() + mName).SizeInBytes();
    
      if (FileSizeInBytes > 4 * 1024*1024)
      {
        try
        {
          TArray<char> FileBuffer((int)FileSizeInBytes);
          TArray<char> SchrinkedFileBuffer(1024*1024);
      
          TFile TempFile(mDirectory.Path() + mName);
        
          if (!TempFile.Open(TFile::kRead))
          {
            throw TReadableException("File I/O Error");
          }
        
          TempFile.Read(FileBuffer.FirstWrite(), FileBuffer.Size());
        
          TMemory::Copy(SchrinkedFileBuffer.FirstWrite(), FileBuffer.FirstRead() +
            FileBuffer.Size() - SchrinkedFileBuffer.Size(), SchrinkedFileBuffer.Size());
          
          TempFile.Close();
        
          if (!TempFile.Open(TFile::kWrite))
          {
            throw TReadableException("File I/O Error");
          }
        
          TempFile.Write(SchrinkedFileBuffer.FirstWrite(), SchrinkedFileBuffer.Size());
        }
        catch (const TReadableException& Exception)
        {
          MInvalid(Exception.what()); MUnused(Exception);
        
          TFile(mDirectory.Path() + mName).Unlink();
          CreatingNewFile = true;
        }
      }
    }
  

    // ... open the file
 
    if (! mDirectory.Exists())
    {
      mDirectory.Create();
    }
  
    mLogFile.Open((mFileMode == kAppend) ? 
      TFile::kWriteAppend : TFile::kWrite);
  
    MAssert(mLogFile.IsOpen(), "Failed to open the log file...");

  
    // ... dump the header

    mLastLineStamp.Start();

    if (mDumpHeader)
    {
      if (mLogFile.IsOpen() && (mFileMode == kAppend) && !CreatingNewFile)
      {
        const std::string Spacing = (4 * TString::SNewLine()).StdCString();
        mLogFile.Write(Spacing.c_str(), Spacing.size());
      }

      Dump("", TString(MSeperator).StdCString().c_str());
      Dump("", TString("Version : " + MProductNameAndVersionString +
        MAlphaOrBetaVersionString + " (" + TString(__DATE__) + ")").StdCString().c_str());
      Dump("", TString("Date    : " + ToString(TDate::SCurrentDate())).StdCString().c_str());
      Dump("", TString("Time    : " + ToString(TDayTime::SCurrentDayTime())).StdCString().c_str());
      Dump("", TString("OS      : " + TSystem::GetOsAsString()).StdCString().c_str());
      Dump("", TString(MSeperator).StdCString().c_str());
    }
  }
  catch (const TReadableException& Exception)
  {
    // exceptions are logged. do not rethrow to safely recover from errors.
    MUnused(Exception);
  }
}

// -------------------------------------------------------------------------------------------------

void TLog::AddLine(const char* pCategory, const char* pString, ...)
{
  char TempChars[4096];

  va_list ArgList;
  va_start(ArgList, pString);
  vsnprintf(TempChars, sizeof(TempChars), pString, ArgList);
  va_end(ArgList);              

  AddLineNoVarArgs(pCategory, TempChars);
}

// -------------------------------------------------------------------------------------------------

void TLog::AddLineNoVarArgs(const char* pCategory, const char* pContent)
{
  try
  {
    // Ignore logging on logging errors while trying to open the 
    // log file or allocating memory from within AddLine.
    if (! mAddingLogLine)
    {
      const TSetUnsetValue<bool> SetUnsetAddingLogLine(mAddingLogLine, true);

      if (! mLogFile.IsOpen())
      {
        CreateLogFile();
      }

      Dump(pCategory, pContent);
    }
  }
  catch (const TReadableException& Exception)
  {
    // exceptions are logged. do not rethrow to safely recover from errors.
    MUnused(Exception);
  }
}

void TLog::AddLineNoVarArgs(const char* pCategory, const TString& Content)
{
  try
  {
    if (! mAddingLogLine)
    {
      const TSetUnsetValue<bool> SetUnsetAddingLogLine(mAddingLogLine, true);

      if (! mLogFile.IsOpen())
      {
        CreateLogFile();
      }

      TArray<char> CStringChars;
      Content.CreateCStringArray(CStringChars, TString::kPlatformEncoding);
      CStringChars.Grow(CStringChars.Size() + 1);
      CStringChars[CStringChars.Size() - 1] = '\0';
    
      Dump(pCategory, CStringChars.FirstRead());
    }
  }
  catch (const TReadableException& Exception)
  {
    // exceptions are logged. do not rethrow to safely recover from errors.
    MUnused(Exception);
  }  
}

// -------------------------------------------------------------------------------------------------

void TLog::Dump(const char* pCategory, const char* pContent)
{
  const TString UnicodeContent(pContent, TString::kPlatformEncoding);
  const TString UnicodeCategory(pCategory, TString::kPlatformEncoding);
  
  if (mLogFile.IsOpen())  // open might have failed...
  {
    TString Out;

    if (mLastCategory != pCategory || 
        mLastLineStamp.DiffInMs() > 2000.0f)
    {
      Out = UnicodeCategory.IsNotEmpty() ? 
        TString::SNewLine() + UnicodeCategory + ": " + UnicodeContent + TString::SNewLine() : 
        TString::SNewLine() + UnicodeContent + TString::SNewLine();
    }
    else
    {
      Out = UnicodeCategory.IsNotEmpty() ? 
        UnicodeCategory + ": " + UnicodeContent + TString::SNewLine() : 
        UnicodeContent + TString::SNewLine();
    }
    
    TArray<char> OutChars;
    Out.CreateCStringArray(OutChars, TString::kPlatformEncoding);
    mLogFile.Write(OutChars.FirstRead(), OutChars.Size());

    mLogFile.Flush();

    mLastCategory = pCategory;
    mLastLineStamp.Start();
  }

  if (mTraceContents)
  {
    if (UnicodeCategory.IsNotEmpty())
    {
      gTrace((MProductString + TString(" LOG> ") + 
        UnicodeCategory + ": " + UnicodeContent)); 
    }
    else
    {
      gTrace((MProductString + TString(" LOG> ") + 
        UnicodeContent));
    }
  }
}

