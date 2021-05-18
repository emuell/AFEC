#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Version.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <pwd.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <string>

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
  const size_t len = ::readlink(fullpath, linkbuf, linkbufsize);

  if (len > 0)
  {
    linkbuf[len] = 0;
    
    struct stat dirInfo;
    if (::lstat(linkbuf, &dirInfo) == 0 && S_ISLNK(dirInfo.st_mode))
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

    if (::lstat(fullpath, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode))
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

    if (::lstat(fullpath, &dirInfo) == 0 && S_ISDIR(dirInfo.st_mode))
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
      if (::lstat(linkbuf, &dirInfo) == 0 && S_ISREG(dirInfo.st_mode))
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
      if (::lstat(linkbuf, &dirInfo) == 0 && S_ISDIR(dirInfo.st_mode))
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

  // NB: use stat and not lstat here: we want to show link target file's properties 
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

static TString SEscapeFileName(const TString& FileName)
{
  return TString("'") + TString(FileName).Replace("'", "'\\''") + TString("'");
}

// -------------------------------------------------------------------------------------------------

enum TInstallationPath
{
  kInvalidInstallationPath = -1,

  kDevEnvironment,
  kUser,
  kUserLocal,
  kInstallationFolder,

  kNumberOfInstallationPaths
};

static TInstallationPath SInstallationPath()
{
  static TInstallationPath sInstallPath = kInvalidInstallationPath;

  if (sInstallPath == kInvalidInstallationPath)
  {
    // check if Product.Exe/../Source/ProductName exists and 
    // is under version control, for dev environments
  
    const TDirectory GitRoot = (gApplicationDir().ParentDirCount() >= 2) ?
      gApplicationDir().Ascend().Ascend() : TDirectory();
    const TDirectory SvnRoot = (gApplicationDir().ParentDirCount() >= 3) ?
      gApplicationDir().Ascend().Ascend().Ascend() : TDirectory();

    if ((GitRoot.Exists() && TDirectory(GitRoot).Descend(".git").Exists()) ||
        (SvnRoot.Exists() && TDirectory(SvnRoot).Descend(".svn").Exists()))
    {
      const TDirectory ProjectsRoot = gApplicationDir().Ascend().Ascend();
      const TDirectory ProjectsDir = TDirectory(ProjectsRoot).Descend("Source");

      if (ProjectsDir.Exists())
      {
        const TDirectory ProductDir = 
          TDirectory(ProjectsDir).Descend(MProductProjectsLocation);

        if (ProductDir.ExistsIgnoreCase())
        {
          TLog::SLog()->AddLine("System", "Running in dev environment...");

          sInstallPath = kDevEnvironment;
        }
      }
    }
  
    if (sInstallPath == kInvalidInstallationPath)
    {
      if (gApplicationDir() == TDirectory("/usr/bin/"))
      {
        TLog::SLog()->AddLine("System", "Running from '/usr/bin/'...");

        sInstallPath = kUser;
      }
      else if (gApplicationDir() == TDirectory("/usr/local/bin/"))
      {
        TLog::SLog()->AddLine("System", "Running from '/usr/local/bin'...");

        sInstallPath = kUserLocal;
      }
      else
      {
        TLog::SLog()->AddLine("System", "Running from directory '%s'...",
          gApplicationDir().Path().CString(TString::kPlatformEncoding));

        sInstallPath = kInstallationFolder; // "."
      }
    }
  }

  return sInstallPath;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool TDirectory::Exists()const
{
  struct stat dirInfo;
 
  if (::lstat(Path().CString(TString::kFileSystemEncoding), &dirInfo) == 0)
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
        if (::lstat(linkbuf, &dirInfo) == 0 && S_ISDIR(dirInfo.st_mode))
        {
          return true;
        }
      }
    }
  }
  
  return false;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::ExistsIgnoreCase()const
{
  return Exists(); // always be case sensitive on Linux
}

// -------------------------------------------------------------------------------------------------

time_t TDirectory::ModificationStatTime(bool CheckMostRecentSubFolders)const
{
  MAssert(Exists(), "Querying stat time for a non existing directory");
  
  time_t Ret = 0;
  
  struct stat sb;
  if (::lstat(Path().CString(TString::kFileSystemEncoding), &sb) == 0)
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
  if (Exists())
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
  // not implemented on linux
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
  if (ParentDirCount() == 0 || (mDir.StartsWith("/mnt/") && ParentDirCount() <= 2))
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

//! NB: global comparison operators for TDirectory are case-sensitive on Linux
//! because file-systems on Linux are usually case-insensitive too

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
  static std::basic_string<TUnicodeChar> sCachedTempDir;

  TString TempPath(sCachedTempDir.c_str());

  // also recreate when something trashed the cached dir
  if (TempPath.IsEmpty() || ! TDirectory(TempPath).Exists())
  {
    // use temp dir from environment variable and P_tmpdir as fallback
    TDirectory TempDir;
    if (char* pTempEnvDir = ::getenv("TMPDIR")) 
    {
      TempDir = TDirectory(pTempEnvDir);
    }
    else 
    {
      TempDir = TDirectory(P_tmpdir); // usually /tmp
    }
    if (! TempDir.Exists())
    {
      TempDir.Create();
    }

    // add unique path for each running instance
    TempDir.Descend(gTempDirName());
    if (! TempDir.Exists())
    {
      TempDir.Create();
    }
    
    // make sure we got a valid tmp dir at the end...
    if (! TempDir.Exists())
    {
      MInvalid("Creating a dir in tmp failed");
      TempDir.Ascend();
    }

    sCachedTempDir = TempDir.Path().Chars();
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
  return gExtractPath(TSystem::ApplicationPathAndFileName());
}

// -------------------------------------------------------------------------------------------------

TDirectory gApplicationResourceDir()
{
  // cached: often used
  static std::string sResourcesBaseDir;
  
  if (sResourcesBaseDir.empty())
  {
    switch (SInstallationPath())
    {
    case kDevEnvironment:
      sResourcesBaseDir = gDeveloperProjectsDir().Descend(
        MProductProjectsLocation).Descend("Resources").Path().CString(
          TString::kFileSystemEncoding);
      break;
      
    case kUser:
      sResourcesBaseDir = TDirectory("/usr/share/" + 
        TString(MProductString).ToLower() + "-" +
        TString(MVersionString).RemoveFirst("V")).Path().CString(
          TString::kFileSystemEncoding);
      break;
      
    case kUserLocal:
      sResourcesBaseDir = TDirectory("/usr/local/share/" + 
        TString(MProductString).ToLower() + "-" +
        TString(MVersionString).RemoveFirst("V")).Path().CString(
          TString::kFileSystemEncoding);
      break;
      
    default:
      MInvalid("Unknown installation path!");

    case kInstallationFolder:
    {
      const TString AppBaseName = gCutExtensionAndPath(
        TSystem::ApplicationPathAndFileName());

      const TDirectory ResourceBundleDir = 
        gApplicationDir().Descend(AppBaseName + ".res");

      if (ResourceBundleDir.Exists())
      {
        // use "APP_NAME.res" for plugins, when present 
        sResourcesBaseDir = ResourceBundleDir.Path().CString(
          TString::kFileSystemEncoding);
      }
      else
      {
        const TDirectory ResourceSubDir = 
          gApplicationDir().Descend("Resources");
      
        // else APP_DIR/Resources when it exists
        if (ResourceSubDir.Exists())
        {
          sResourcesBaseDir = ResourceSubDir.Path().CString(
            TString::kFileSystemEncoding);
        }
        else
        {
          // APP_DIR as fallback (NB: needed for exes in resource folders)
          sResourcesBaseDir = gApplicationDir().Path().CString(
            TString::kFileSystemEncoding);
        }
      }

      TLog::SLog()->AddLine("System", "Using '%s' as local resource directory...", 
        sResourcesBaseDir.c_str());
        
    } break;

    }
  }
  
  return TDirectory(TString(sResourcesBaseDir.c_str(), 
    TString::kFileSystemEncoding));
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserHomeDir()
{
  // Home directory from environment variable
  char* pHomeEnvDir = ::getenv("HOME");

  // ignore '~'s HOME environment settings. want a real expanded path here.
  if (pHomeEnvDir && pHomeEnvDir[0] != '~')
  {
    return TDirectory(TString(pHomeEnvDir, TString::kFileSystemEncoding));
  }

  // Find home via getpwuid's pw_dir
  struct passwd* pPw = ::getpwuid(::getuid());

  if (pPw->pw_dir && pPw->pw_dir[0] != '~')
  {
    return TDirectory(TString(pPw->pw_dir, TString::kFileSystemEncoding));
  }

  // Find home based on user name
  char* pUserName = ::getlogin();

  if (pUserName)
  {
    return TDirectory("/home").Descend(pUserName);
  }

  // try resolving ~
  static char FullHomePath[PATH_MAX];
  const char* pResult = ::realpath("~", FullHomePath);
  
  if (pResult)
  {
    return TDirectory(pResult);
  }

  // the app dir as last fallback (should never get to here though)
  return gApplicationDir();
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserLogDir()
{
  return gUserConfigsDir(); //TDirectory("/var/log");
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserConfigsDir()
{
  static std::basic_string<TUnicodeChar> sUserConfigsDir;
  if (sUserConfigsDir.empty()) 
  {
    TDirectory ConfigDir;
 
    // use XDG_CONFIG_HOME as config base dir, when specified, else $HOME/.config
    if (char* pConfigEnvDir = ::getenv("XDG_CONFIG_HOME"))
    {
      ConfigDir = TDirectory(pConfigEnvDir);
    }
    else 
    {
      ConfigDir = TDirectory(gUserHomeDir()).Descend(".config");
    }
    if (! ConfigDir.Exists())
    {
      ConfigDir.Create();
    }

    // add product name
    ConfigDir.Descend(TString(MProductString));
    if (! ConfigDir.Exists())
    {
      ConfigDir.Create();
    }

    // add version number
    ConfigDir.Descend(MVersionString);
    if (! ConfigDir.Exists())
    {
      ConfigDir.Create();
    }

    sUserConfigsDir = ConfigDir.Path().Chars();
  }
  
  return TDirectory(sUserConfigsDir.c_str());
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserDesktopDir()
{
  return gUserHomeDir().Descend("Desktop");
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserDocumentsDir()
{
  return gUserHomeDir();
}

// -------------------------------------------------------------------------------------------------

bool gRunningInDeveloperEnvironment()
{
  return (SInstallationPath() == kDevEnvironment);
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
  return MakeList<TDirectory>(gApplicationDir(), gApplicationResourceDir());
}

// -------------------------------------------------------------------------------------------------

bool gCopyFile(const TString& From, const TString& To, bool PreserveSymlinks)
{
  if (!PreserveSymlinks && TFile(From).Exists() && ! TFile(To).Exists())
  {
    // try a raw file copy first
    if (::SRawFileCopy(From.CString(TString::kFileSystemEncoding),
            To.CString(TString::kFileSystemEncoding)) == 0)
    {
      return true;
    }
  }

  const TString CopyFlags = (PreserveSymlinks) ? "-rdf" : "-rf"; 

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

bool gRenameDirOrFile(const TString& OldName, const TString& NewName)
{
  // try 'rename' first (may fail when src, dest are on different devices)
  if (::rename(OldName.CString(TString::kFileSystemEncoding),
               NewName.CString(TString::kFileSystemEncoding)) == 0)
  {
    return true;
  }

  // then fork and try again with "mv":
  const TString CmdLine = TString("if mv -f " + SEscapeFileName(OldName) + " " +
    SEscapeFileName(NewName) + "; then exit 1; fi");

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

bool gMoveFileToTrash(const TString& FileName)
{
  // use gio trash or gvfs-trash: either should be available on all major Linux desktop systems
  const TString CmdLine = TString(
    "if hash gio 2> /dev/null; then gio trash " + SEscapeFileName(FileName) + " && exit 1; "
    "else gvfs-trash " + SEscapeFileName(FileName) + " && exit 1; fi");
    
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

bool gMakeFileWritable(const TString& FilePath)
{
  MAssert(gFileExists(FilePath), "Can only change flags for an existing file!");

  struct stat StatBuffer;

  if (::lstat(FilePath.CString(TString::kFileSystemEncoding), &StatBuffer) != -1) 
  {
    if (!S_ISLNK(StatBuffer.st_mode))
    {
      const unsigned short NewMode = (unsigned short)(StatBuffer.st_mode | 0200);

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
  
  if (::lstat(FilePath.CString(TString::kFileSystemEncoding), &StatBuffer) != -1) 
  {
    if (!S_ISLNK(StatBuffer.st_mode))
    {
      const unsigned short NewMode = (unsigned short)(StatBuffer.st_mode & 0777555);
      
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
  // the system takes care of this on linux...
}
