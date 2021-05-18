#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Version.h"

#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fstream>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static int SRawFileCopy(const char* pFromfile, const char* pTofile)
{
  FILE* pInputFile;
  if ((pInputFile = ::fopen(pFromfile, "rb")) == NULL)
  {
    return -1;
  }

  FILE* pOutputFile;
  if ((pOutputFile = ::fopen(pTofile, "wb")) == NULL)
  {
    ::fclose(pInputFile);

    return -1;
  }

  size_t BytesRead;
  char TempBuffer[64 * 1024];

  while ((BytesRead = ::fread(TempBuffer, 1, sizeof(TempBuffer), pInputFile)) > 0)
  {
    if (::fwrite(TempBuffer, 1, BytesRead, pOutputFile) != BytesRead)
    {
      ::fclose(pInputFile);
      ::fclose(pOutputFile);

      return -1;
    }
  }

  ::fclose(pInputFile);
  ::fclose(pOutputFile);

  return 0;
}

// -------------------------------------------------------------------------------------------------

static bool SResolveLink(char* linkbuf, size_t linkbufsize, const char* fullpath)
{
  const ssize_t len = ::readlink(fullpath, linkbuf, linkbufsize);

  if (len > 0)
  {
    linkbuf[len] = 0;
    
    struct stat dirInfo;
    if (::stat(linkbuf, &dirInfo) == 0 && S_ISLNK(dirInfo.st_mode))
    {
      char newfullpath[PATH_MAX + NAME_MAX];
      ::strcpy(newfullpath, linkbuf);
      
      return SResolveLink(linkbuf, linkbufsize, newfullpath);
    }
    else
    {
      return true;
    }
  }
  
  
  linkbuf[0] = 0;
  return false;
}

// -------------------------------------------------------------------------------------------------

static bool SIsRegularFile(const TString& Directory, const dirent* dp)
{
  if (dp->d_type != DT_UNKNOWN)
  {
    return (dp->d_type == DT_REG);
  }
  else
  {
    char fullpath[PATH_MAX + NAME_MAX];
    fullpath[0] = 0;

    const char* pDirectory = Directory.CString(TString::kFileSystemEncoding);
    MAssert(pDirectory[::strlen(pDirectory) - 1] == '/', "Expected a trailing /");
 
    ::strcpy(fullpath, pDirectory);
    ::strcat(fullpath, dp->d_name);

    struct stat fileInfo;

    if (::stat(fullpath, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode))
    {
      return true;
    }

    return false;
  }
}

// -------------------------------------------------------------------------------------------------

static bool SIsDirectory(const TString& Directory, const dirent* dp)
{
  if (dp->d_type != DT_UNKNOWN)
  {
    return (dp->d_type == DT_DIR);
  }
  else
  {
    char fullpath[PATH_MAX + NAME_MAX];
    fullpath[0] = 0;

    const char* pDirectory = Directory.CString(TString::kFileSystemEncoding);
    MAssert(pDirectory[::strlen(pDirectory) - 1] == '/', "Expected a trailing /");
 
    ::strcpy(fullpath, pDirectory);
    ::strcat(fullpath, dp->d_name);

    struct stat dirInfo;

    if (::stat(fullpath, &dirInfo) == 0 && S_ISDIR(dirInfo.st_mode))
    {
      return true;
    }

    return false;
  }
}

// -------------------------------------------------------------------------------------------------

static bool SLinksToRegularFile(const TString& Directory, const dirent* dp)
{
  if (dp->d_type == DT_LNK)
  {
    const char* pDirectory = Directory.CString(TString::kFileSystemEncoding);
    MAssert(pDirectory[::strlen(pDirectory) - 1] == '/', "Expected a trailing /");

    const char* pFileName = dp->d_name;

    if (::strlen(pDirectory) + ::strlen(pFileName) >= PATH_MAX + NAME_MAX)
    {
      MInvalid("Unexpected path size...");
      return false;
    }

    char fullpath[PATH_MAX + NAME_MAX];
    fullpath[0] = 0;
   
    ::strcpy(fullpath, pDirectory);
    ::strcat(fullpath, pFileName);

    char linkbuf[PATH_MAX + NAME_MAX];
    if (SResolveLink(linkbuf, sizeof(linkbuf), fullpath))
    {
      struct stat dirInfo;
      if (::stat(linkbuf, &dirInfo) == 0 && S_ISREG(dirInfo.st_mode))
      {
        return true;
      }
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

static bool SLinksToDirectory(
  const TString&                     Directory, 
  const dirent*                      dp,
  TDirectory::TSymLinkRecursionTest* pSymlinkChecker = NULL)
{
  if (dp->d_type == DT_LNK)
  {
    // combine directory and filename to full src path
    const char* pDirectory = Directory.CString(TString::kFileSystemEncoding);
    MAssert(pDirectory[::strlen(pDirectory) - 1] == '/', "Expected a trailing /");
    
    const char* pFileName = dp->d_name;
    if (::strlen(pDirectory) + ::strlen(pFileName) >= PATH_MAX + NAME_MAX)
    {
      MInvalid("Unexpected path size...");
      return false;
    }
    
    char fullpath[PATH_MAX + NAME_MAX] = { 0 };
    ::strcpy(fullpath, pDirectory);
    ::strcat(fullpath, pFileName);

    // resolve the link
    char linkbuf[PATH_MAX + NAME_MAX];
    if (SResolveLink(linkbuf, sizeof(linkbuf), fullpath))
    {
      // check link's type 
      struct stat dirInfo;
      if (::stat(linkbuf, &dirInfo) == 0 && S_ISDIR(dirInfo.st_mode))
      {
        // check for recursive links, when pSymlinkChecker is present
        if (pSymlinkChecker != NULL) 
        {
          const TDirectory SrcDir = TDirectory(
            TString(fullpath, TString::kFileSystemEncoding));
          const TDirectory TargetDir = TDirectory(
            TString(linkbuf, TString::kFileSystemEncoding));
            
          if (pSymlinkChecker->IsRecursedLink(SrcDir, TargetDir)) 
          {
            // symlinks recursed - ignore this directory
            return false;
          }
        }
        
        // link points to a directory
        return true;
      }
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

static void SSetFileProperties(
  TFileProperties*    pProperties,
  const TDirectory&   Path,
  const TString&      Name)
{
  pProperties->mName = Name;

  if (Name.StartsWithChar('.'))
  {
    pProperties->mAttributes = TFileProperties::kHidden;
  }
  else
  {
    pProperties->mAttributes = 0;
  }
  
  const TString PathAndName = Path.Path() + Name;
  
  struct stat sb;
  if (::stat(PathAndName.CString(TString::kFileSystemEncoding), &sb) == 0)
  {
    pProperties->mWriteDate = TDate::SCreateFromStatTime(sb.st_mtime);
    pProperties->mWriteTime = TDayTime::SCreateFromStatTime(sb.st_mtime);
    pProperties->mSizeInBytes = sb.st_size;
  } 
  else 
  {
    MInvalid("stat failed");
    
    pProperties->mWriteDate = TDate::SCurrentDate();
    pProperties->mWriteTime = TDayTime::SCurrentDayTime();
    pProperties->mSizeInBytes = 0;
  }
}

// -------------------------------------------------------------------------------------------------

static NSMutableArray* SCreateExtensionListNSArray(const TList<TString>& FileExtensions)
{
  NSMutableArray* pFileTypes;
  
  // allow all extensions ?
  if (FileExtensions.Size() == 1 && FileExtensions[0] == "*.*")
  {
    pFileTypes = nil;
  }
  else
  {
    pFileTypes = [NSMutableArray arrayWithCapacity: FileExtensions.Size()];
    
    for (int i = 0; i < FileExtensions.Size(); ++i)
    {
      if (FileExtensions[i] != "*.*")
      {
        const TString Ext = TString(FileExtensions[i]).RemoveFirst("*.");
        
        [pFileTypes addObject:gCreateNSString(Ext)];
      }
    }
  }
  
  return pFileTypes;
}

// -------------------------------------------------------------------------------------------------

static TString SEscapeFileName(const TString& FileName)
{
  return TString("'") + TString(FileName).Replace("'", "'\\''") + TString("'");
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool TDirectory::Exists()const
{
  if (ExistsIgnoreCase())
  {
    // be case sensitive (OSX or other Unix platforms can be pitty here)
    #if defined(MDebug)
      if (ParentDirCount() > 1)
      {
        const TDirectory ParentPath = TDirectory(*this).Ascend();
        
        const TString RequestedName = TString(Path()).RemoveLast(
          SPathSeparator()).SplitAt(SPathSeparatorChar()).Last();
        
        const TList<TString> SubDirs = ParentPath.FindSubDirNames("*");
                                    
        MAssert(!SubDirs.IsEmpty(), "Then stat should not have worked!");
                        
        for (int i = 0; i < SubDirs.Size(); ++i)
        {
          if (gStringsEqualIgnoreCase(SubDirs[i], RequestedName))
          {
            MAssert(SubDirs[i] == RequestedName, 
              "Some filesystems will be case sensitive!");
            
            return true;
          }
        }
        
        MInvalid("Should never reach this");
      }
    #endif
    
    return true;
  }
  
  return false;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::ExistsIgnoreCase()const
{
  if (!IsEmpty())
  {
    struct stat dirInfo;
    
    if (::stat(Path().CString(TString::kFileSystemEncoding), &dirInfo) == 0)
    {
      if (S_ISDIR(dirInfo.st_mode)) // is a dir
      {
        return true;
      }
      else if (S_ISLNK(dirInfo.st_mode)) // or links to a dir
      {
        char linkbuf[PATH_MAX + NAME_MAX];
        if (SResolveLink(linkbuf, sizeof(linkbuf), Path().CString(TString::kFileSystemEncoding)))
        {
          struct stat dirInfo;
          if (::stat(linkbuf, &dirInfo) == 0 && S_ISDIR(dirInfo.st_mode))
          {
            return true;
          }
        }
      }
    }
  }
  
  return false; 
}

// -------------------------------------------------------------------------------------------------

time_t TDirectory::ModificationStatTime(bool CheckMostRecentSubFolders)const
{
  MAssert(ExistsIgnoreCase(), "Querying stat time for a non existing directory");
  
  time_t Ret = 0;
  
  struct stat sb;
  if (::stat(Path().CString(TString::kFileSystemEncoding), &sb) == 0)
  {
    Ret = sb.st_mtime;
    
    if (CheckMostRecentSubFolders)
    {
      const TList<TString> SubDirNames = FindSubDirNames();
      for (int i = 0; i < SubDirNames.Size(); ++i)
      {
        const TDirectory SubDir = 
          TDirectory(*this).Descend(SubDirNames[i]);

        const time_t StatTime = 
          SubDir.ModificationStatTime(CheckMostRecentSubFolders);
        
        if (StatTime > Ret)
        {
          Ret = StatTime;
        }
      }
    }
  }
  else
  {
    MAssert(!Exists(), "Stat failed (unknown reason)");
  }

  return Ret;
}
  
// -------------------------------------------------------------------------------------------------

bool TDirectory::CanWrite()const
{
  if (ExistsIgnoreCase())
  {
    TFile Temp(gGenerateTempFileName(*this));
  
    if (Temp.Open(TFile::kWrite))
    {
      Temp.Close();
      Temp.Unlink();
      return true;
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

TList<TString> TDirectory::FindFileNames(const TList<TString>& Extensions)const
{
  MAssert(Exists(), "Querying for files in a non existing directory");
  
  TList<TString> FileNameList;

  // open directory
  DIR* dir = ::opendir(Path().CString(TString::kFileSystemEncoding));

  if (!dir)
  {
    // opendir failed (may not have sufficient permissions)
    return FileNameList;
  }

  dirent* dp;

  // loop while there are still entries
  while ((dp = ::readdir(dir)) != NULL)
  {
    if (dp->d_name[0] != 0 && ::strcmp(dp->d_name, ".") != 0 && ::strcmp(dp->d_name, "..") != 0)
    { 
      if (SIsRegularFile(Path(), dp) || SLinksToRegularFile(Path(), dp)) 
      {
        const TString Name = TString(dp->d_name, TString::kFileSystemEncoding);
    
        if (gFileMatchesExtension(Name, Extensions))
        {
          FileNameList.Append(Name);
        }
      }  
    }
  }
  
  // close directory when we're done
  ::closedir(dir);

  return FileNameList;
}

// -------------------------------------------------------------------------------------------------

TList<TFileProperties> TDirectory::FindFiles(const TList<TString>& Extensions)const
{
  MAssert(Exists(), "Querying for files in a non existing directory");
  
  TList<TFileProperties> FileList;

  // open directory
  DIR* dir = ::opendir(Path().CString(TString::kFileSystemEncoding));

  if (!dir)
  {
    // opendir failed (may not have sufficient permissions)
    return FileList;
  }

  dirent* dp;

  // loop while there are still entries
  while ((dp = ::readdir(dir)) != NULL)
  {
    if (dp->d_name[0] != 0 && ::strcmp(dp->d_name, ".") != 0 && ::strcmp(dp->d_name, "..") != 0)
    { 
      if (SIsRegularFile(Path(), dp) || SLinksToRegularFile(Path(), dp)) 
      {
        const TString Name = TString(dp->d_name, TString::kFileSystemEncoding);
    
        if (gFileMatchesExtension(Name, Extensions))
        {
          TFileProperties Properties;
          SSetFileProperties(&Properties, *this, Name);
          FileList.Append(Properties);
        }
      }  
    }
  }
  
  // close directory when we're done
  ::closedir(dir);

  return FileList;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::MayHaveSubDirs()const
{
  // not implemented on the mac
  return HasSubDirs();
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::HasSubDirs()const
{
  MAssert(Exists(), "Querying for sub directories in a non existing directory");
  
  // open directory
  DIR* dir = ::opendir(Path().CString(TString::kFileSystemEncoding));

  if (!dir)
  {
    // opendir failed (may not have sufficient permissions)
    return false;
  }

  dirent* dp;

  // loop while there are still entries
  while ((dp = ::readdir(dir)) != NULL)
  {
    if (dp->d_name[0] != 0 && ::strcmp(dp->d_name, ".") != 0 && ::strcmp(dp->d_name, "..") != 0)
    { 
      if (SIsDirectory(Path(), dp) || SLinksToDirectory(Path(), dp)) 
      {
        // close directory when we're done
        ::closedir(dir);
        
        return true;
      }
    }
  }

  // close directory when we're done
  ::closedir(dir);
  
  return false;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::IsParentDirOf(const TDirectory& Other)const
{
  // case sensitive (see comparison operators)
  return Other.Path().StartsWith(this->Path()) && *this != Other;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::IsSubDirOf(const TDirectory& Other)const
{
  // case sensitive (see comparison operators)
  return this->Path().StartsWith(Other.Path()) && *this != Other;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::IsSameOrSubDirOf(const TDirectory& Other)const
{
  // case sensitive (see comparison operators)
  return this->Path().StartsWith(Other.Path());
}

// -------------------------------------------------------------------------------------------------

TList<TString> TDirectory::FindSubDirNames(
  const TString&          Searchmask,
  TSymLinkRecursionTest*  pSymlinkChecker)const
{
  MAssert(Exists(), "Querying for sub directories in a non existing directory");
  
  TList<TString> SubDirNameList;

  // open directory
  DIR* dir = ::opendir(Path().CString(TString::kFileSystemEncoding));

  if (!dir)
  {
    // opendir failed (may not have sufficient permissions)
    return SubDirNameList;
  }

  dirent* dp;

  const TList<TString> Extensions = MakeList<TString>(Searchmask);
  
  // loop while there are still entries
  while ((dp = ::readdir(dir)) != NULL)
  {
    if (dp->d_name[0] != 0 && ::strcmp(dp->d_name, ".") != 0 && ::strcmp(dp->d_name, "..") != 0)
    { 
      if (SIsDirectory(Path(), dp) || SLinksToDirectory(Path(), dp, pSymlinkChecker)) 
      {
        const TString Name = TString(dp->d_name, TString::kFileSystemEncoding);
    
        if (gFileMatchesExtension(Name, Extensions))
        {
          SubDirNameList.Append(Name);
        }
      }  
    }
  }
  
  // close directory when we're done
  ::closedir(dir);

  return SubDirNameList;
}

// -------------------------------------------------------------------------------------------------

TList<TFileProperties> TDirectory::FindSubDirs(
  const TString&          Searchmask,
  TSymLinkRecursionTest*  pSymlinkChecker)const
{
  MAssert(Exists(), "Querying for sub directories in a non existing directory");
  
  TList<TFileProperties> SubDirList;

  // open directory
  DIR* dir = ::opendir(Path().CString(TString::kFileSystemEncoding));

  if (!dir)
  {
    // opendir failed (may not have sufficient permissions)
    return SubDirList;
  }

  dirent* dp;

  const TList<TString> Extensions = MakeList<TString>(Searchmask);
  
  // loop while there are still entries
  while ((dp = ::readdir(dir)) != NULL)
  {
    if (dp->d_name[0] != 0 && ::strcmp(dp->d_name, ".") != 0 && ::strcmp(dp->d_name, "..") != 0)
    { 
      if (SIsDirectory(Path(), dp) || SLinksToDirectory(Path(), dp, pSymlinkChecker)) 
      {
        const TString Name = TString(dp->d_name, TString::kFileSystemEncoding);
    
        if (gFileMatchesExtension(Name, Extensions))
        {
          TFileProperties Properties;
          SSetFileProperties(&Properties, *this, Name);
          SubDirList.Append(Properties);
        }
      }  
    }
  }
  
  // close directory when we're done
  ::closedir(dir);

  return SubDirList;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::Create(bool CreateParentDirs)const
{
  if (Exists())
  {
    return true;
  }

  if (CreateParentDirs && ParentDirCount() > 1)
  {
    const TDirectory ParentDir = TDirectory(*this).Ascend();

    if (! ParentDir.Exists() && ! ParentDir.Create(CreateParentDirs))
    {
      return false;
    }    
  }

  // This should maybe be discussed? But 0755 should be a reasonable default permission
  ::mkdir(Path().CString(TString::kFileSystemEncoding), 0755); 

  const bool Success = Exists();
  return Success;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::Unlink()const
{
  if (ParentDirCount() == 0 || (mDir.StartsWith("/Volumes/") && ParentDirCount() <= 2))
  {
    MInvalid("Forget it! Won't delete everything or a whole drive");
    return false;
  }


  // ... try deleting an empty folder

  if (::rmdir(Path().CString(TString::kFileSystemEncoding)) == 0)
  {
    return true;
  }                    


  // ... recursively remove sub folders and files.

  if (DIR* dir = ::opendir(Path().CString(TString::kFileSystemEncoding)))
  {
    dirent* dp;
    while ((dp = ::readdir(dir)) != NULL)
    {
      const TString Name = TString(dp->d_name, TString::kFileSystemEncoding);
    
      if (Name != "." && Name != "..")
      {
        if (SIsDirectory(Path(), dp) || SLinksToDirectory(Path(), dp))
        {
          if (!TDirectory(*this).Descend(Name).Unlink())
          {
            return false;
          }
        }
        else
        {
          if (!TFile(Path() + Name).Unlink())
          {
            return false;
          }
        }
      }
    }

    // close directory when we're done
    ::closedir(dir);
  }
  
  // directory should be empty here...
  const bool Success = ::rmdir(Path().CString(TString::kFileSystemEncoding)) == 0;
  return Success;
}

// =================================================================================================

//! NB: global comparison operators for TDirectory are case-sensitive on OSX
//! because file-systems !can! be (well, but usually will not be)

bool operator== (const TDirectory& First, const TDirectory& Second)
{
  return First.Path() == Second.Path();
}

bool operator!= (const TDirectory& First, const TDirectory& Second)
{
  return First.Path() != Second.Path();
}

bool operator< (const TDirectory& First, const TDirectory& Second)
{
  return First.Path() < Second.Path();
}

bool operator> (const TDirectory& First, const TDirectory& Second)
{
  return First.Path() > Second.Path();
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TDirectory gTempDir()
{
  // cached: often used
  static std::string sCachedTempDir;
  
  TString TempPath(sCachedTempDir.c_str(), TString::kUtf8);

  // also recreate when something (or even we) trashed the cached dir
  if (TempPath.IsEmpty() || ! TDirectory(TempPath).Exists())
  {
    // try NSTemporaryDirectory first...
    TDirectory TempDir(
      gCreateStringFromCFString((CFStringRef)NSTemporaryDirectory()));

    if (!TempDir.Exists())
    {
      TempDir = TString("/tmp/");
    
      if (!TempDir.Exists())
      {
        // fall back to "/private/tmp" if "/tmp" doesn't exists
        TempDir = TDirectory("/private/tmp/");
      }
    }
    
    // NSTemporaryDirectory may return /tmp on some old OSX releases, so
    // make sure our temp dir is in all cases unique...
  
    TempDir.Descend(gTempDirName());
  
    if (!TempDir.Exists())
    {
      TempDir.Create();
    }

    if (!TempDir.Exists())
    {
      MInvalid("Creating a dir in the tmp root dir failed!");

      TempDir.Ascend();
    }

    sCachedTempDir = TempDir.Path().CString(TString::kUtf8);
    return TempDir;
  }
  else
  {
    return TDirectory(TempPath);
  }
}

// -------------------------------------------------------------------------------------------------

TDirectory gApplicationDir()
{
  NSBundle* pBundle = NULL;

  // use NSBundle with the dladdr exe to resolve shared libraries paths
  Dl_info dl_info; 
  ::memset(&dl_info, 0, sizeof(Dl_info));
  
  if (::dladdr((void*)gApplicationDir, &dl_info) && 
      dl_info.dli_fname != NULL)
  {
    TDirectory BundleDir = gExtractPath(
      TString(dl_info.dli_fname, TString::kFileSystemEncoding));
        
    if (BundleDir.Path().EndsWithIgnoreCase("MacOS/"))
    {
      BundleDir.Ascend().Ascend(); // ascend to the bundle root
    }
        
    pBundle = [NSBundle bundleWithPath:
      gCreateNSString(BundleDir.Path())];
  }

  // and the main bundle as fallback
  if (!pBundle)
  {
    pBundle = [NSBundle mainBundle];
  }
    
  if (pBundle == NULL)
  {
    MInvalid("Failed to access the main bundle");
    return TDirectory("/Applications");
  }
  else
  {
    TDirectory ApplicationDir = gCreateStringFromCFString(
      (CFStringRef)[pBundle bundlePath]);
    
    const bool IsBundled = TFile(
      ApplicationDir.Path() + "Contents/Info.plist").Exists();
      
    if (IsBundled) 
    { 
      // ignore the "XXX.app" or "XXX.bundle" dir itself
      ApplicationDir.Ascend();
    }
    
    return ApplicationDir;
  } 
}

// -------------------------------------------------------------------------------------------------

TDirectory gApplicationBundleResourceDir()
{
  NSBundle* pBundle = NULL;

  // use NSBundle with the dladdr exe to resolve shared libraries paths
  Dl_info dl_info; 
  ::memset(&dl_info, 0, sizeof(Dl_info));
  
  if (::dladdr((void*)gApplicationBundleResourceDir, &dl_info) && 
      dl_info.dli_fname != NULL)
  {
    TDirectory BundleDir = gExtractPath(
      TString(dl_info.dli_fname, TString::kFileSystemEncoding));
        
    if (BundleDir.Path().EndsWithIgnoreCase("MacOS/"))
    {
      BundleDir.Ascend().Ascend(); // ascend to the bundle root
    }
        
    pBundle = [NSBundle bundleWithPath:
      gCreateNSString(BundleDir.Path())];
  }
  
  // and the main bundle as fallback (for applications)
  if (!pBundle)
  {
    pBundle = [NSBundle mainBundle];
  }

  if (pBundle == NULL)
  {
    MInvalid("Failed to access the main bundle");
    
    return TDirectory("/Applications/" + MProductString + 
      ".app/Contents/Resources/");
  }
  else
  {
    TDirectory BundleDir = gCreateStringFromCFString(
      (CFStringRef)[pBundle resourcePath]);

    return BundleDir;
  }
}

// -------------------------------------------------------------------------------------------------

TDirectory gApplicationSupportResourceDir()
{
  TDirectory Dir = gUserHomeDir().Descend("Library/Application Support/").
    Descend(MProductString).Descend(TString(MVersionString).RemoveFirst("V"));
    
  if (! Dir.Exists())
  {
    Dir.Create();
  }
    
  return Dir;
}

// -------------------------------------------------------------------------------------------------

TDirectory gApplicationResourceDir()
{
  // cached: often used
  static std::string sResourcesBaseDir;
  
  if (sResourcesBaseDir.empty())
  {
    const bool IsBundledApp = TSystem::ApplicationPathAndFileName().EndsWithIgnoreCase(".app");
    
    const TString AppBaseName = gCutExtensionAndPath(
      TSystem::ApplicationPathAndFileName());
    
    const TDirectory ResourceBundleDir = 
      gApplicationDir().Descend(AppBaseName + ".res");

    // use "APP_NAME.res" for plugins, when present
    if (!IsBundledApp && ResourceBundleDir.ExistsIgnoreCase())
    {
      sResourcesBaseDir = ResourceBundleDir.Path().CString(TString::kUtf8);
    }
    else
    {
      // only use the dev paths when running in the debugger, to check/test
      // if the resources in the bundle are copied correctly...
      if ((!IsBundledApp || TSystem::RunningInDebugger()) && gRunningInDeveloperEnvironment())
      {
        sResourcesBaseDir = gDeveloperProjectsDir().Descend(
          MProductProjectsLocation).Descend("Resources").Path().CString(
            TString::kUtf8);
      }
      else
      {
        sResourcesBaseDir = gApplicationBundleResourceDir().Path().CString(TString::kUtf8);
      }
    }

    TLog::SLog()->AddLine("System", "Using '%s' as resource base directory...", 
      TString(sResourcesBaseDir.c_str()).CString());
  }
  
  return TString(sResourcesBaseDir.c_str(), TString::kUtf8);
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserConfigsDir()
{
  TDirectory Dir = gUserHomeDir().Descend("Library/Preferences");
  
  Dir.Descend(MProductString);
    
  if (!Dir.Exists())
  {
    Dir.Create();
  }
  
  Dir.Descend(TString(MVersionString).RemoveFirst("V"));
  
  if (!Dir.Exists())
  {
    Dir.Create();
  }
  
  return Dir;
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserLogDir()
{
  return gUserHomeDir().Descend("Library/Logs");
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserHomeDir()
{
  // cached: often used
  static std::string sUserHomeDir;
   
  if (sUserHomeDir.empty())
  {
    // Try a few methods to find the correct directory
    TDirectory HomeDir(gCreateStringFromNSString(NSHomeDirectory()));
    
    if (HomeDir.Exists())
    {
      sUserHomeDir = HomeDir.Path().CString(TString::kUtf8);
      
      return HomeDir;
    }
    
    // home directory from environment variable
    char* pBsdHomeDir = ::getenv("HOME");
    
    if (pBsdHomeDir)
    {
      HomeDir = TString(pBsdHomeDir, TString::kFileSystemEncoding);
      sUserHomeDir = HomeDir.Path().CString(TString::kUtf8);
      
      return HomeDir;
    }
    
    // homedirectory based on user name
    char* pUserName = ::getlogin();
    
    if (pUserName)
    {
      HomeDir = TDirectory(TString("/Users/")).Descend(TString(pUserName));
      sUserHomeDir = HomeDir.Path().CString(TString::kUtf8);
      
      return HomeDir;
    }

    HomeDir = TString("~/");
    sUserHomeDir = HomeDir.Path().CString(TString::kUtf8);
    return HomeDir;
  }
  else
  {
    return TString(sUserHomeDir.c_str(), TString::kUtf8);
  }
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserDesktopDir()
{
  return gUserHomeDir().Descend("Desktop");
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserDocumentsDir()
{
  return gUserHomeDir().Descend("Documents");
}

// -------------------------------------------------------------------------------------------------

bool gRunningInDeveloperEnvironment()
{
  static int sResult = -1;

  if (sResult == -1)
  {
    sResult = 0;

    // check if Product.Exe/../../../Source/ProductName exists and
    // is under version control

    const TDirectory GitRoot = (gApplicationDir().ParentDirCount() >= 2) ?
      gApplicationDir().Ascend().Ascend() : TDirectory();
    const TDirectory SvnRoot = (gApplicationDir().ParentDirCount() >= 3) ?
      gApplicationDir().Ascend().Ascend().Ascend() : TDirectory();

    if ((GitRoot.ExistsIgnoreCase() &&
          TDirectory( GitRoot ).Descend( ".git" ).ExistsIgnoreCase()) ||
        (SvnRoot.ExistsIgnoreCase() &&
          TDirectory(SvnRoot).Descend(".svn").ExistsIgnoreCase()))
    {
      const TDirectory ProjectsRoot = gApplicationDir().Ascend().Ascend();
      const TDirectory ProjectsDir = TDirectory(ProjectsRoot).Descend("Source");

      if (ProjectsDir.ExistsIgnoreCase())
      {
        const TDirectory ProductDir = 
          TDirectory(ProjectsDir).Descend(MProductProjectsLocation);

        if (ProductDir.ExistsIgnoreCase())
        {
          TLog::SLog()->AddLine("System", "Running in dev environment...");
          sResult = 1;
        }
      }
    }
  }

  return (sResult != 0);
}

// -------------------------------------------------------------------------------------------------

TDirectory gDeveloperProjectsDir()
{
  MAssert(gRunningInDeveloperEnvironment(), "Invalid internal dev dir access");
  return gApplicationDir().Ascend().Ascend().Descend("Source");
}

// -------------------------------------------------------------------------------------------------

TList<TDirectory> gReadOnlyApplicationResourceDirs()
{
  TDirectory AppDirRoot = gUserHomeDir().Descend("Library/Application Support").Descend(
    MProductString);
  
  return MakeList<TDirectory>(AppDirRoot, gApplicationBundleResourceDir());
}

// -------------------------------------------------------------------------------------------------

TList<TDirectory> gAvailableDrives()
{
  TList<TDirectory> Dirs;
  Dirs.Append(TDirectory("/"));

  return Dirs;
}

// -------------------------------------------------------------------------------------------------

TDirectory gCurrentWorkingDir()
{
  char WorkingDirChars[PATH_MAX] = {0};
  const char* pResult = ::getcwd(WorkingDirChars, sizeof(WorkingDirChars));
  MAssert(pResult != NULL, "getcwd failed"); MUnused(pResult);

  return TDirectory(TString(WorkingDirChars, TString::kFileSystemEncoding));
}

// -------------------------------------------------------------------------------------------------

void gSetCurrentWorkingDir(const TDirectory& Directory)
{
  const int Result = ::chdir(Directory.Path().CString(TString::kFileSystemEncoding));
  MAssert(Result == 0, "chdir failed"); MUnused(Result);
}

// -------------------------------------------------------------------------------------------------

void gCleanUpTempDir()
{
  // the system takes care of this on the mac...
}

// -------------------------------------------------------------------------------------------------

bool gCopyFile(const TString& From, const TString& To, bool PreserveSymlinks)
{
  // try a raw file copy for files and unlinked content
  if (! PreserveSymlinks && TFile(From).Exists() && ! TFile(To).Exists())
  {
    if (::SRawFileCopy(From.CString(TString::kFileSystemEncoding),
            To.CString(TString::kFileSystemEncoding)) == 0)
    {                 
      return true;
    }
  }
  
  // then try NSFileManager if it's OK to preserve symlinks
  else if (PreserveSymlinks)
  {
    NSFileManager* pFileManager = [NSFileManager defaultManager];

    NSString* pFrom = gCreateNSString(From);
    NSString* pTo = gCreateNSString(To);
  
    if ([pFileManager copyPath:pFrom toPath:pTo handler:nil])
    {
      return true;
    }
  }
  
  // the system's "cp" as fallback
  const TString CopyFlags = (PreserveSymlinks) ? "-Rf" : "-rf"; 

  const TString CmdLine = TString("if cp " + CopyFlags + " " + 
    SEscapeFileName(From) + " " + SEscapeFileName(To) + 
    "; then exit 1; fi");

  const int SysRet = ::system(CmdLine.CString(TString::kFileSystemEncoding));

  if (WIFEXITED(SysRet))
  {
    const bool Success = (WEXITSTATUS(SysRet) == 1);
    return Success;
  }
  else
  {
    return false;
  }
}

// -------------------------------------------------------------------------------------------------

bool gMoveFileToTrash(const TString &FileName)
{
  NSString* pSrcPath = gCreateNSString(gExtractPath(FileName).Path());
  NSString* pSrcFileOrPath = gCreateNSString(gCutPath(FileName));

  const bool Success = [
    [NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation
    source:pSrcPath 
    destination:@"" 
    files:[NSArray arrayWithObject:pSrcFileOrPath] 
    tag:NULL
  ];

  return Success;
}

// -------------------------------------------------------------------------------------------------

bool gRenameDirOrFile(const TString& OldName, const TString& NewName)
{
  if (OldName == NewName)
  {
    // don't bother with this...
    return true;
  }
  
  NSFileManager* pFileManager = [NSFileManager defaultManager];

  NSString* pOldName = gCreateNSString(OldName);
  NSString* pNewName = gCreateNSString(NewName);
  
  // try handling case sensitive renames. NSFileManager won't...
  if (gStringsEqualIgnoreCase(OldName, NewName))
  {
    if (TDirectory(OldName).Exists())
    {
      const TString TempPath = gGenerateTempFilePath(
        TDirectory(OldName).Ascend()).Path();

      // gGenerateTempFilePath will create a new dir...
      TDirectory(TempPath).Unlink();

      return gRenameDirOrFile(OldName, TempPath) && 
        gRenameDirOrFile(TempPath, NewName);
    }
    else if (TFile(OldName).Exists())
    {
      const TString TempFileName = gGenerateTempFileName(
        gExtractPath(OldName));
      
      return gRenameDirOrFile(OldName, TempFileName) && 
        gRenameDirOrFile(TempFileName, NewName);
    }
    else
    {
      MInvalid("Expected a file or dir!");
      return false;
    }
  }
  else
  {
    // remove dest file first or movePath will fail...
    if ([pFileManager fileExistsAtPath:pOldName] &&  
        [pFileManager fileExistsAtPath:pNewName])
    {
      if (! [pFileManager removeFileAtPath:pNewName handler:nil])
      {
        return false;
      }
    }
    
    const BOOL Success = 
      [pFileManager movePath:pOldName toPath:pNewName handler:nil];

    return Success;
  }
}

// -------------------------------------------------------------------------------------------------

bool gMakeFileWritable(const TString& FilePath)
{
  MAssert(gFileExists(FilePath), "Can only change flags for an existing file!");

  struct stat StatBuffer;

  if (::stat(FilePath.CString(TString::kFileSystemEncoding), &StatBuffer) != -1) 
  {
    if (!S_ISLNK(StatBuffer.st_mode))
    {
      const unsigned short NewMode = (StatBuffer.st_mode | 0200);

      if (NewMode != StatBuffer.st_mode)
      {
        const bool Success =
          ::chmod(FilePath.CString(TString::kFileSystemEncoding), NewMode) == 0;

        return Success;
      }
      else
      {
        return true;
      }
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

bool gMakeFileReadOnly(const TString& FilePath)
{
  MAssert(gFileExists(FilePath), "Can only change flags for an existing file!");

  struct stat StatBuffer;
  
  if (::stat(FilePath.CString(TString::kFileSystemEncoding), &StatBuffer) != -1) 
  {
    if (!S_ISLNK(StatBuffer.st_mode))
    {
      const unsigned short NewMode = (StatBuffer.st_mode & 0777555);
      
      if (NewMode != StatBuffer.st_mode)
      {
        const bool Success =
          ::chmod(FilePath.CString(TString::kFileSystemEncoding), NewMode) == 0;

        return Success;
      }
      else
      {
        return true;
      }
    }
  }
  
  return false;
}
