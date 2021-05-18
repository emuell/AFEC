#pragma once

#ifndef _Directory_h_
#define _Directory_h_

// =================================================================================================

#include "CoreTypes/Export/Pointer.h"
#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Pair.h"
#include "CoreTypes/Export/DateAndTime.h"

#include <ctime> // time_t
#include <map>
#include <functional>

// =================================================================================================
  
struct TFileProperties
{
  enum TAttributes
  {
    kReadOnly   = 1 << 0,
    kSystem     = 1 << 1,
    kHidden     = 1 << 2
  };

  typedef int TAttributeFlags;
  
  TFileProperties() 
    : mSizeInBytes(0), mAttributes(0) {}

  TString mName;
  TDate mWriteDate;
  TDayTime mWriteTime;
  long long mSizeInBytes;
  TAttributeFlags mAttributes;
};

// =================================================================================================

class TDirectory
{
public:

  //! '/' on the Mac, '\' on Windows...
  static TUnicodeChar SPathSeparatorChar();
  static TString SPathSeparator();
  
  //! the separator char that is NOT used by the current system
  static TUnicodeChar SWrongPathSeparatorChar();
  static TString SWrongPathSeparator();
  

  TDirectory();
  
  TDirectory(const TString& Other);
  TDirectory& operator= (const TString& String);
  // copy constructor and default assignment operators are fine
  
  
  //@{ ... Basic const properties
  
  //! @return false if a path was set (must not exist)
  bool IsEmpty()const;
  
  //! @return true if this is a complete, absolute path
  bool IsAbsolute()const;
  //! @return true if this is a relative path
  bool IsRelative()const;
  
  //! @return true if the file exist
  bool Exists()const;
  //! same as Exists(), but will ignore case mismatch assertions in debug builds
  bool ExistsIgnoreCase()const;

  //! @return true if we (the current process) is allowed to write in this directory
  //! The directory must exist.
  bool CanWrite()const;

  //! when true, the folder may have subdirs or not, when returning false
  //! it really doesn't have one. Used to speed up dir-browsing on network paths...
  bool MayHaveSubDirs()const;
  //! @return true if the current Directory has subdirs
  bool HasSubDirs()const;
    
  //! @return how many parents this dir has. Will be 0 for the 
  //! root (or drive paths like C:\\ on Windows)
  int ParentDirCount()const;
  
  //! @return true when other dir is part of this dir
  bool IsParentDirOf(const TDirectory& Other)const;
  //! @return true when this dir is part of other dir
  bool IsSubDirOf(const TDirectory& Other)const;
  //! @return true when this dir is part of other dir or matches other
  bool IsSameOrSubDirOf(const TDirectory& Other)const;

  //! @return const access to the path string
  TString Path()const;
  
  //! returns a list of all subcomponents (folders), keeping the root path 
  //! (like "/Volumes/SomePath/") intact if specified. 
  //! The root path must be valid when specified!
  TList<TString> SplitPathComponents(
    const TDirectory& RootDir = TDirectory())const;
  
  //! @return the gmtime date, where the folder was modified. 
  //! When param \CheckMostRecentSubFolders is enabled, the most recent time 
  //! of all sub folders is returned, which can be useful to check if a 
  //! directory contains newer content than some another.
  TDate ModificationDate(bool CheckMostRecentSubFolders = false)const;
  //! @return the gmtime time, where the folder was modified
  TDayTime ModificationDayTime(bool CheckMostRecentSubFolders = false)const;
  //! @return the time in stat's format, where the folder was modified
  time_t ModificationStatTime(bool CheckMostRecentSubFolders = false)const;
  //@}
  
  
  //@{ ... Find Files/Dirs
  
  // Helper class to detect recursive visits of symlinks in 'FindSubDir[Names]'.
  // When recursively walking folders, instanciate TSymLinkRecursionTest outside of 
  // the initial 'FindSubDir' and pass this instance to all following FindSubDir calls.
  // When a recursion is detected, 'FindSubDir' will not include the folder anymore.
  class TSymLinkRecursionTest
  {
  public:
    // checks if the given LinkSrc already got visited. folder should then not be 
    // followed anymore in recursive 'FindSubDir' calls
    bool IsRecursedLink(
      const TDirectory& LinkSrc, 
      const TDirectory& LinkTarget);
  
  private:
    std::map<TString, TList<TString> > mVisitedDirectories;
  };

  //! @return a list of file names (unsorted) of this directory
  //! using the specified wild carded extensions.
  //! A valid extension is something like "*.*" or "*.rni" or "*"
  //! The returned names will !not! contain the path of this Directory !
  TList<TString> FindFileNames(
    const TList<TString>& Extensions = MakeList<TString>("*.*"))const;
  
  //! Same as FindFileNames, but including file properties
  TList<TFileProperties> FindFiles(
    const TList<TString>& Extensions = MakeList<TString>("*.*"))const;

  //! @return a list of SubDirectories of this Directory that match to the given 
  //! search mask. The returned names will !not! contain the path of this directory!
  //! When param TSymLinkRecursionTest is set, check for recursive symlinks.
  TList<TString> FindSubDirNames(
    const TString&          Searchmask = TString("*"),
    TSymLinkRecursionTest*  pSymLinkRecursionTest = NULL)const;
  
  //! Same as FindSubDirNames, but including file properties
  TList<TFileProperties> FindSubDirs(
    const TString&          Searchmask = TString("*"),
    TSymLinkRecursionTest*  pSymLinkRecursionTest = NULL)const;
  //@}
  
  
  //@{ ... Create/Modify the directory in the system
  
  //! @return success. Will create missing parent directories as well when 
  //! \param CreateParentDirs is set to true
  bool Create(bool CreateParentDirs = true)const;
  //! unlink (delete) a directory. Will try to delete existing content of the 
  //! directory first, then the directory itself. Returns success.
  bool Unlink()const;

  //! deletes all Files that are part of this Directory. Subdirs are not deleted!
  void DeleteAllFiles()const;
  //! deletes all subdirs of this Directory. Files are not deleted!
  void DeleteAllSubDirs()const;  
  //@}


  //@{ ... Modify the Path
  
  //! ascend to the parent directory
  TDirectory& Ascend();

  //! descend into the given subdir
  TDirectory& Descend(const TString& NewDir);
  //@}
  
private:
   TString mDir;  
};

// =================================================================================================

bool operator== (const TDirectory& First, const TDirectory& Second);
bool operator!= (const TDirectory& First, const TDirectory& Second);

bool operator< (const TDirectory& First, const TDirectory& Second);
bool operator> (const TDirectory& First, const TDirectory& Second);

TString ToString(const TDirectory& Value);
bool StringToValue(TDirectory& Value, const TString& String);

// =================================================================================================

//! Unique temp dir for every running process and instance (in case there are 
//! multiple. Can be in a user local directory or the system's root filesystem.
//! This function will create the dir if it doesn't exist.
TDirectory gTempDir();

//! Returns the name of just the temp dir (without the path).
TString gTempDirName();

//! Returns the path of the application's executable dir on Linux and Windows,
//! the application's bundle dir on OSX. For shared libraries, this will NOT
//! resolve the application the library is loaded in, but the path the shared
//! library itself gets loaded from.
TDirectory gApplicationDir();

//! Root path of the application's resource files. Bundle resource path on 
//! OSX, else the apps default resource path. In dev environments this will 
//! be the project files resource root. 
TDirectory gApplicationResourceDir();

#if defined(MMac)
  //! Explicitly get the bundle resource folder on OSX. gApplicationResourceDir
  //! might not be used when for example running in dev environments.
  TDirectory gApplicationBundleResourceDir();
  //! The place where applications should save resources to which the user can
  //! "see", such as demo documents or manuals. NOT a user based path but unique
  //! for each product version...
  TDirectory gApplicationSupportResourceDir();
#endif


//! Path of the application user config files. Unique for every user and 
//! product version.
TDirectory gUserConfigsDir();
//! Path of the user log files. Unique for every user and product version.
TDirectory gUserLogDir();
//! Path of the user's home directory.
TDirectory gUserHomeDir();
//! The users desktop folder (may not exist on Linux)
TDirectory gUserDesktopDir();
//! The users "My Documents" folder (gUserHomeDir() on Linux)
TDirectory gUserDocumentsDir();

//! Returns true, when running the exe in a local dev path environment.
bool gRunningInDeveloperEnvironment();
//! Only valid with gRunningInDeveloperEnvironment(). returns the root 
//! "Projects" source path. Should only be used for internal tools!
TDirectory gDeveloperProjectsDir();

//! Dirs where users should not save files to, as they are synced/deleted 
//! by the application when installing (gApplicationSupportResourceDir dir 
//! on OSX or app installation resource dirs on all systems).
TList<TDirectory> gReadOnlyApplicationResourceDirs();

// =================================================================================================

//! Return all currently available drives by path.
TList<TDirectory> gAvailableDrives();

//! Get/set the current working dir.
TDirectory gCurrentWorkingDir();
void gSetCurrentWorkingDir(const TDirectory& Directory);

//! Helper class to temporarily set a working dir to the given path, and
//! restoring it to the previous one.
class TSetAndRestoreWorkingDir
{
public:
  TSetAndRestoreWorkingDir(const TDirectory& Directory)
    : mOldWorkingDir(gCurrentWorkingDir())
  {
    gSetCurrentWorkingDir(Directory);
  }

  ~TSetAndRestoreWorkingDir()
  {
    gSetCurrentWorkingDir(mOldWorkingDir);
  }

private:
  TDirectory mOldWorkingDir;
};

// =================================================================================================

//! generate a Filename using the given directory to create a unique temp file
//! returns the full path and name!
TString gGenerateTempFileName(
  const TDirectory& Directory, 
  const TString&    Extension = TString(".tmp"));
  
//! generate a filename using the given directory to create a unique temp path
//! returns the full Path and Name !
TDirectory gGenerateTempFilePath(const TDirectory &Directory);

// clear old temp folders of previous runs (which might not be deleted
// when the app who created it crashed...)
void gCleanUpTempDir();

// =================================================================================================

//! copy a file from full path "From" to "To"
bool gCopyFile(
  const TString&  From, 
  const TString&  To, 
  bool            PreserveSymlinks = false);

//! move a file to the systems recycle bin
bool gMoveFileToTrash(const TString& FileName);

//! rename a file or directory by specifying the old and new full path and name
bool gRenameDirOrFile(const TString& OldName, const TString& NewName);

//! try making a file writable for everyone or the current user (in this order)
bool gMakeFileWritable(const TString& FilePath);
//! try making a file read-only for everyone or the current user (in this order)
bool gMakeFileReadOnly(const TString& FilePath);

// =================================================================================================

//! return true if the File with the given filename and path can be accessed
bool gFileExists(const TString &FileNameAndPath);

//! return true if the file or directory path is an absolute path
bool gFilePathIsAbsolute(const TString &FileNameAndPath);

//! replaces characters which can not be saved with dummies. Won't change/fix empty strings
TString gCreateValidFileName(const TString &FileName);

//! return true if the FileName (NOT PATH) can be used to save a file
bool gIsValidFileName(const TString &FileNameAndPath);
bool gIsPathSeperatorChar(const TUnicodeChar& Char);
bool gIsInvalidFileNameChar(const TUnicodeChar& Char);

//! returns the extension (if present) of a filename 
TString gExtractFileExtension(const TString &FileName);
//! returns true if any extension matches with one of the given extensions
bool gFileMatchesExtension(const TString& Filename, const TList<TString>& ExtensionList);

//! Cuts the Extension from a full path or filename
TString gCutExtension(const TString &FileName);
//! returns the filename without the extension, if it matches one of the given ones
TString gCutExtension(const TString& Filename, const TList<TString>& ExtensionList);

//! Cuts the extension and the path from the filename
TString gCutExtensionAndPath(const TString &FileName);
//! returns the filename without path and the extension, if it matches one of the given ones
TString gCutExtensionAndPath(const TString& Filename, const TList<TString>& ExtensionList);

//! Cuts the path from the filename and path
TString gCutPath(const TString &FileNameandPath);
//! Cuts the filename from the FileName and path
TDirectory gExtractPath(const TString &FileNameandPath);

//! Convert an file:// URI to a file system filename
TString gConvertUriToFileName(const TString& String);


#endif // _Directory_h_

