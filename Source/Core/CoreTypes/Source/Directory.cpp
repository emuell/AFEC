#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Version.h"

#include <cstring>
#include <cwctype> 
 
// =================================================================================================

#if defined(MWindows)
  #define MPathSeperator '\\'
  #define MOtherPathSeperator '/'

#elif defined(MMac) || defined(MLinux)
  #define MPathSeperator '/'
  #define MOtherPathSeperator '\\'

#else
  #error unknown platform
#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static bool SIsHexChar(TUnicodeChar c) 
{
  return 
   (((c >= '0') && (c <= '9')) ||
    ((c >= 'a') && (c <= 'f')) ||
    ((c >= 'A') && (c <= 'F')));
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool TDirectory::TSymLinkRecursionTest::IsRecursedLink(
  const TDirectory& LinkSrcDirectory, 
  const TDirectory& LinkTargetDirectory)
{
  // subpath of resolved path? 
  if (LinkSrcDirectory.IsSubDirOf(LinkTargetDirectory)) 
  {
    return true;
  }

  // subpath of a symlinked folder?
  std::map<TString, TList<TString> >::iterator Iter = 
    mVisitedDirectories.find(LinkTargetDirectory.Path());
  
  if (Iter != mVisitedDirectories.end()) 
  {
    for (int i = 0; i < Iter->second.Size(); ++i)  
    {
      if (LinkSrcDirectory.IsSubDirOf(Iter->second[i])) 
      {
        return true;
      }
    }

    // memorize visited symlink target
    Iter->second.Append(LinkSrcDirectory.Path());
  }
  else // Iter == mVisitedDirectories.end()
  {
    // memorize visited symlink target
    mVisitedDirectories.insert(std::make_pair(LinkTargetDirectory.Path(), 
      MakeList<TString>(LinkSrcDirectory.Path())));
  }

  return false;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TUnicodeChar TDirectory::SPathSeparatorChar()
{
  return MPathSeperator;
}

// -------------------------------------------------------------------------------------------------

TString TDirectory::SPathSeparator()
{
  const TUnicodeChar Chars[2] = {MPathSeperator, 0};
  return TString(Chars);
}

// -------------------------------------------------------------------------------------------------

TUnicodeChar TDirectory::SWrongPathSeparatorChar()
{
  return MOtherPathSeperator;
}

// -------------------------------------------------------------------------------------------------

TString TDirectory::SWrongPathSeparator()
{
  const TUnicodeChar Chars[2] = {MOtherPathSeperator, 0};
  return TString(Chars);
}
  
// -------------------------------------------------------------------------------------------------

TDirectory::TDirectory()
{
}

TDirectory::TDirectory(const TString &Other)
{
  operator=(Other);
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::IsEmpty()const
{
  return mDir.IsEmpty();
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::IsAbsolute()const
{
  return gFilePathIsAbsolute(mDir);
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::IsRelative()const
{
  return !gFilePathIsAbsolute(mDir);
}

// -------------------------------------------------------------------------------------------------

TDate TDirectory::ModificationDate(bool CheckMostRecentSubFolders)const
{
  return TDate::SCreateFromStatTime(
    ModificationStatTime(CheckMostRecentSubFolders));
}

// -------------------------------------------------------------------------------------------------

TDayTime TDirectory::ModificationDayTime(bool CheckMostRecentSubFolders)const
{
  return TDayTime::SCreateFromStatTime(
    ModificationStatTime(CheckMostRecentSubFolders));
}

// -------------------------------------------------------------------------------------------------

TString TDirectory::Path()const
{
  return mDir;
}

// -------------------------------------------------------------------------------------------------
  
TList<TString> TDirectory::SplitPathComponents(const TDirectory& RootDir)const
{
  if (RootDir.IsEmpty())
  {
    if (Path().IsEmpty())
    {
      return TList<TString>();
    }
    else
    {
      return TDirectory(*this).Path().RemoveLast(SPathSeparator()).SplitAt(
        TDirectory::SPathSeparator());
    }
  }
  else
  {
    MAssert(IsSubDirOf(RootDir), "Invalid Root!");
    
    TString RootSplit = TDirectory(*this).Path().RemoveFirst(RootDir.Path());
    
    return MakeList<TString>(TString(RootDir.Path()).RemoveLast(TDirectory::SPathSeparator())) + 
      RootSplit.RemoveLast(SPathSeparator()).SplitAt(TDirectory::SPathSeparator());
  }
}
  
// -------------------------------------------------------------------------------------------------
  
TDirectory& TDirectory::operator= (const TString& String)  
{
  mDir = String;
  
  // fix wrong separators
  mDir.ReplaceChar(MOtherPathSeperator, MPathSeperator);
  
  // add a MPathSeperator at the end if none is present
  if (mDir.Size() && mDir[mDir.Size() - 1] != MPathSeperator)
  {
    mDir.AppendChar(MPathSeperator);
  }

  // fix doubled MPathSeperators
  while (mDir.Find(2*SPathSeparator()) > 0)
  {
    mDir.Replace(2*SPathSeparator(), SPathSeparator());
  }

  // normalize . and ..'s
  const TString UnnormalizedDir = mDir;

  if (mDir.Find(SPathSeparator() + "." + SPathSeparator()) > 0)
  {
    mDir.Replace(SPathSeparator() + "." + SPathSeparator(), SPathSeparator());
  }

  while (mDir.Find(SPathSeparator() + "..") != -1)
  {
    const int AscendStart = mDir.Find(SPathSeparator() + "..");
    const int PreviousDirStart = mDir.Find(SPathSeparator(), AscendStart - 1, true);

    if (PreviousDirStart != -1)
    {
      mDir = mDir.SubString(0, PreviousDirStart) + mDir.SubString(AscendStart + 3);
    }
    else
    {
      MInvalid("Invalid path!");

      // roll back (bogus path?)...
      mDir = UnnormalizedDir;
      break;
    }
  }
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

int TDirectory::ParentDirCount()const
{
  return mDir.CountChar(MPathSeperator) - 1;
}

// -------------------------------------------------------------------------------------------------

TDirectory& TDirectory::Ascend()
{
  MAssert(ParentDirCount() > 0, "Cannot ascent the root");
  
  // security hack: might not be clear that this is not allowed...
  if (ParentDirCount() > 0)
  {
    int Pos = mDir.Size() - 2; // last char is always MPathSeperator
    MAssert(Pos > 0, "");

    // Decent
    while (Pos > 0 && mDir[Pos] != MPathSeperator)
    {
      --Pos;
    }

    mDir.SetChar(Pos + 1, '\0');
  }

  return *this;
}

// -------------------------------------------------------------------------------------------------

TDirectory& TDirectory::Descend(const TString& NewSubDir)
{
  if (NewSubDir.Size())
  {
    // operator= will normalize (ascend) ".."s if needed...
    return operator=(Path() + NewSubDir);
  }
  else
  {
    return *this;
  }
}
  
// -------------------------------------------------------------------------------------------------

void TDirectory::DeleteAllFiles()const
{
  if (Exists())
  {
    const TList<TString> FileList = FindFileNames(MakeList<TString>("*.*"));

    for (int i = 0; i < FileList.Size(); ++i)
    {
      TFile File((Path() + FileList[i]));
      File.Unlink();
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TDirectory::DeleteAllSubDirs()const
{
  if (Exists())
  {
    const TList<TString> DirList = FindSubDirNames();

    for (int i = 0; i < DirList.Size(); ++i)
    {
      TDirectory Dir((Path() + DirList[i]));
      Dir.Unlink();
    }
  }
}

// =================================================================================================

// ==, !=, <, > operators are defined in the platform impls 
// (Unix/OSX may be case-sensitive, Windows not...)

// -------------------------------------------------------------------------------------------------

bool StringToValue(TDirectory& Value, const TString& String)
{
  Value = String;
  return true;
}

// -------------------------------------------------------------------------------------------------

TString ToString(const TDirectory& Other)
{
  return Other.Path();
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TString gTempDirName()
{
  static const int sProcessId = TSystem::CurrentProcessId();
  return MProductString + "-" + ToString(sProcessId);
}

// -------------------------------------------------------------------------------------------------

TString gGenerateTempFileName(const TDirectory &Directory, const TString& Extension)
{
  MAssert(Extension.StartsWith("."), "Invalid extension");
  
  TString TempName;

  do 
  {
    static int sTempCount = 0, sTempCount2 = 0;

    TempName = TString(MProductString + "_TmpFile-") + 
      ToString(sTempCount) + "-" + ToString(sTempCount2) + Extension;

    if (++sTempCount2 == 0)
    {
      ++sTempCount;
    }
  } 
  while (TFile(Directory.Path() + TempName).Exists() || 
         TDirectory(Directory.Path() + TempName).Exists());

  return Directory.Path() + TempName;
}

// -------------------------------------------------------------------------------------------------

TDirectory gGenerateTempFilePath(const TDirectory &Directory)
{
  TString TempName;

  do 
  {
    static int sTempCount = 0, sTempCount2 = 0;

    TempName = TString(MProductString) + "_TmpPath-" + 
      ToString(sTempCount) + "-" + ToString(sTempCount2);

    if (++sTempCount2 == 0)
    {
      ++sTempCount;
    }
  } 
  while (TDirectory(Directory.Path() + TempName).Exists() || 
         TFile(Directory.Path() + TempName).Exists());

  TDirectory Result(Directory.Path() + TempName);
  Result.Create();

  return Result;
}

// -------------------------------------------------------------------------------------------------

bool gIsPathSeperatorChar(const TUnicodeChar& Char)
{
  return Char == '\\' || Char == '/';
}

// -------------------------------------------------------------------------------------------------

bool gIsInvalidFileNameChar(const TUnicodeChar& Char)
{
  return ((Char >= 0x01 && Char <= 0x1F) || // NTFS, FAT32 limitation
    Char == '\\' || Char == '/'  || Char == ':' || 
    Char == '?'  || Char == '\"' || Char == '*' ||
    Char == '<'  || Char == '>'  || Char == '|');
}

// -------------------------------------------------------------------------------------------------

TString gCreateValidFileName(const TString& FileName)
{
  TString Ret = FileName;

  for (int Char = 0; Char < Ret.Size(); ++Char)
  {
    if (gIsInvalidFileNameChar(Ret[Char]))
    {
      Ret.SetChar(Char, '_');
    }
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------
  
bool gIsValidFileName(const TString &FileName)
{
  // empty filenames are never allowed
  if (FileName.IsEmpty())
  {
    return false;
  }

  // gIsInvalidFileNameChar chars are also never allowed
  const int StrSize = FileName.Size();
  
  for (int i = 0; i < StrSize; ++i)
  {
    if (::gIsInvalidFileNameChar(FileName[i]))
    {
      return false;
    }
  }

  // finally make sure its really valid by trying to open a dummy file:
  // filename rules are far to complex to be safely applied here.
  TFile TestFile(gTempDir().Path() + FileName);

  if (TestFile.Open(TFile::kWrite))
  {
    TestFile.Close();
    TestFile.Unlink();
    
    return true;
  }
  else
  {
    // failed to open, so its not valid...
    return false;
  }
}

// -------------------------------------------------------------------------------------------------
  
bool gFileExists(const TString& FileNameAndPath)
{
  return TFile(FileNameAndPath).Exists();
}

// -------------------------------------------------------------------------------------------------

bool gFilePathIsAbsolute(const TString& Path)
{
  #if defined(MWindows)
    // represents a Windows UNC network path?
    if (Path.Size() > 2)
    {
      if (Path[0] == '\\' && Path[1] == '\\')
      {
        return true;
      }
    }

    // has a drive letter prefix?
    if (Path.Size() >= 3)
    {
      if ((::iswalpha(Path[0])) && 
          (Path[1] == ':') && 
          (Path[2] == '/' || Path[2] == '\\'))
      {
        return true;
      }
    }

    return false;

  #else // !Windows -> POSIX
    return Path.StartsWithChar('/');
  #endif
}

// -------------------------------------------------------------------------------------------------

TString gCutPath(const TString &FileName)
{
  TString Ret(FileName);
  
  const int OriginalSize = Ret.Size() - 1;
  int LastBackSlashPos = OriginalSize;
  
  while (--LastBackSlashPos >= 0)
  {
    if (::gIsPathSeperatorChar(Ret[LastBackSlashPos]))
    {
      TArray<TUnicodeChar> TempChars(OriginalSize - LastBackSlashPos + 1);
      
      TMemory::Copy(TempChars.FirstWrite(), Ret.Chars() + 
        LastBackSlashPos + 1, (OriginalSize - LastBackSlashPos) * sizeof(TUnicodeChar));
      
      TempChars[TempChars.Size() - 1] = 0;
      
      return TempChars.FirstRead();
    }
  }

  // no path present
  return Ret;
}

// -------------------------------------------------------------------------------------------------

TDirectory gExtractPath(const TString &FileNameandPath)
{
  const TString Name = gCutPath(FileNameandPath);
  
  return TDirectory(TString(FileNameandPath).RemoveLast(Name));
}

// -------------------------------------------------------------------------------------------------

TString gExtractFileExtension(const TString &Filename)
{
  int i = Filename.Size();

  for (;i > 0; i--)
  {
    if (Filename[i] == '.')
    {
      i++;
      break;
    }
    else if (::gIsPathSeperatorChar(Filename[i]))
    {
      i = 0;
      break;
    }
  }

  if (i == 0)
  {
    // no extension found
    return TString();
  }

  return TString(&Filename.Chars()[i]);
}
  
// -------------------------------------------------------------------------------------------------

bool gFileMatchesExtension(const TString& FileName, const TList<TString>& ExtensionList)
{
  for (int e = 0; e < ExtensionList.Size(); ++e)
  {
    const TString SearchString = ExtensionList[e];
    
    MAssert(SearchString.StartsWith("*.") || SearchString == L"*", "Invalid Extension");

    if (SearchString == L"*.*" || SearchString == L"*")
    {
      return true;
    }
    else
    {
      bool Matches = true;
      
      for (int i = SearchString.Size() - 1, j = FileName.Size() - 1; i >= 0 && j >= 0; --i, --j)
      {
        if (SearchString[i] != '*' && ::towlower(SearchString[i]) != ::towlower(FileName[j]))
        {
          Matches = false;
          break;
        }
      }
      
      if (Matches)
      {
        // don't allow partial matches (else "ext" or "xt" would match "*.ext")
        return (FileName.Size() >= SearchString.Size() - 1);
      }
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

TString gCutExtension(const TString &FileName)
{
  TString Ret(FileName);

  // Cut Extension
  int FileNameSize = FileName.Size();
  
  for (int i = FileNameSize; i > 0; i--)
  {
    if (Ret[i] == '.')
    {
      Ret.SetChar(i, '\0');
      break;
    }
    else if (::gIsPathSeperatorChar(Ret[i]))
    {
      break;
    }
  }

  return Ret;
}

TString gCutExtension(const TString& Filename, const TList<TString>& ExtensionList)
{
  TString Ret = Filename;

  for (int i = 0; i < ExtensionList.Size(); ++i)
  {
    MAssert(ExtensionList[i].StartsWith("*."), "Invalid Extension");

    if (ExtensionList[i] != "*.*" && 
        Ret.EndsWithIgnoreCase(TString(ExtensionList[i]).RemoveFirst("*")))
    {
      Ret.RemoveLast(TString(ExtensionList[i]).RemoveFirst("*"));
      break;
    }
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TString gCutExtensionAndPath(const TString &Filename)
{
  return gCutExtension(gCutPath(Filename));
}

TString gCutExtensionAndPath(const TString &Filename, const TList<TString>& ExtensionList)
{
  return gCutExtension(gCutPath(Filename), ExtensionList);
}

// -------------------------------------------------------------------------------------------------

TString gConvertUriToFileName(const TString& String) 
{
  TString Ret(String);

  if (Ret.StartsWithIgnoreCase("file://localhost"))
  {
    Ret.RemoveFirst("file://localhost");
  } 
  else if (Ret.StartsWithIgnoreCase("file://"))
  {
    Ret.RemoveFirst("file://");
  } 
  else if (Ret.StartsWithIgnoreCase("file:"))
  {
    Ret.RemoveFirst("file:");
  } 

  int StringSize = Ret.Size();
  int StartOffset = 0;
  
  bool ReplacedEscapedChars = false;
  
  while (StartOffset < StringSize)
  {
    const int Start = Ret.Find("%", StartOffset);

    if (Start != -1 && Start + 2 < Ret.Size())
    {
      StartOffset = Start + 1;
      
      if (SIsHexChar(Ret[Start + 1]) && SIsHexChar(Ret[Start + 2]))
      {
        ReplacedEscapedChars = true;
        
        const TString NumberString = Ret.SubString(Start + 1, Start + 3);
        
        int Number = 0;
        if (StringToValue(Number, NumberString, "%x"))
        {
          const TString SubString = Ret.SubString(Start, Start + 3);
          const TUnicodeChar ReplString[] = {(TUnicodeChar)Number, 0};
          Ret.Replace(SubString, ReplString);
          
          StringSize = Ret.Size();
        }
      }  
    }
    else
    {
      break;
    }
  }
  
  if (ReplacedEscapedChars)
  {
    // URIs use plain ASCII, so treat the resulted converted entities as UTF8 bytes
    TArray<char> Utf8Array(Ret.Size());
    
    for (int i = 0; i < Utf8Array.Size(); ++i)
    {
      Utf8Array[i] = (char)Ret[i];
    }
      
    return TString().SetFromCStringArray(Utf8Array, TString::kUtf8);
  }
  else
  { 
    return Ret;
  }  
}

