#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Version.h"

#include <windows.h>
#include <shlobj.h>
#include <tlhelp32.h>

// =================================================================================================

// Defined in the windows driver SDK only. Redefined here for SReadLink.

typedef struct _REPARSE_DATA_BUFFER {
  ULONG  ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  union {
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG  Flags;
      WCHAR  PathBuffer[1];
    } SymbolicLinkReparseBuffer;
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR  PathBuffer[1];
    } MountPointReparseBuffer;
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

// =================================================================================================

// -------------------------------------------------------------------------------------------------

// shamelessly stolen from node's 'fs__readlink_handle' implementation:
// https://github.com/nodejs/node/blob/5ebfaa88917ebcef1dd69e31e40014ce237c60e2/deps/uv/src/win/fs.c#L301
// IShellLink::GetPath seems bogus and we actually want the same behavior as node here...

static TString SReadLink(const TString& AbsPath) 
{
  // Open file handle
  HANDLE handle = ::CreateFile(
    AbsPath.Chars(),
    0,
    0,
    NULL,
    OPEN_EXISTING,
    FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
    NULL);

  if (handle == INVALID_HANDLE_VALUE) 
  {
    return TString();
  }

  // Get reparse point
  char buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
  DWORD bytes;

  if (!::DeviceIoControl(handle,
        FSCTL_GET_REPARSE_POINT,
        NULL,
        0,
        buffer,
        sizeof buffer,
        &bytes,
        NULL)) 
  {
    ::CloseHandle(handle);
    return TString();
  }

  // parse reparse data content
  REPARSE_DATA_BUFFER* reparse_data = (REPARSE_DATA_BUFFER*) buffer;
  
  WCHAR* w_target = NULL;
  DWORD w_target_len = 0;
  
  if (reparse_data->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
    // Real symlink
    w_target = reparse_data->SymbolicLinkReparseBuffer.PathBuffer +
        (reparse_data->SymbolicLinkReparseBuffer.SubstituteNameOffset /
        sizeof(WCHAR));
    w_target_len =
        reparse_data->SymbolicLinkReparseBuffer.SubstituteNameLength /
        sizeof(WCHAR);

    // Real symlinks can contain pretty much everything, but the only thing
    // we really care about is undoing the implicit conversion to an NT
    // namespaced path that CreateSymbolicLink will perform on absolute
    // paths. If the path is win32-namespaced then the user must have
    // explicitly made it so, and we better just return the unmodified
    // reparse data.
    if (w_target_len >= 4 &&
        w_target[0] == L'\\' &&
        w_target[1] == L'?' &&
        w_target[2] == L'?' &&
        w_target[3] == L'\\') {
      // Starts with \??\*
      if (w_target_len >= 6 &&
          ((w_target[4] >= L'A' && w_target[4] <= L'Z') ||
           (w_target[4] >= L'a' && w_target[4] <= L'z')) &&
          w_target[5] == L':' &&
          (w_target_len == 6 || w_target[6] == L'\\')) {
        // \??\<drive>:\*
        w_target += 4;
        w_target_len -= 4;

      } else if (w_target_len >= 8 &&
                 (w_target[4] == L'U' || w_target[4] == L'u') &&
                 (w_target[5] == L'N' || w_target[5] == L'n') &&
                 (w_target[6] == L'C' || w_target[6] == L'c') &&
                 w_target[7] == L'\\') {
        // \??\UNC\<server>\<share>\ - make sure the final path looks like
        // \\<server>\<share>\*
        w_target += 6;
        w_target[0] = L'\\';
        w_target_len -= 6;
      }
    }

  } else if (reparse_data->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
    // Junction.
    w_target = reparse_data->MountPointReparseBuffer.PathBuffer +
        (reparse_data->MountPointReparseBuffer.SubstituteNameOffset /
        sizeof(WCHAR));
    w_target_len = reparse_data->MountPointReparseBuffer.SubstituteNameLength /
        sizeof(WCHAR);

    // Only treat junctions that look like \??\<drive>:\ as symlink.
    // Junctions can also be used as mount points, like \??\Volume{<guid>},
    // but that's confusing for programs since they wouldn't be able to
    // actually understand such a path when returned by uv_readlink().
    // UNC paths are never valid for junctions so we don't care about them.
    if (!(w_target_len >= 6 &&
          w_target[0] == L'\\' &&
          w_target[1] == L'?' &&
          w_target[2] == L'?' &&
          w_target[3] == L'\\' &&
          ((w_target[4] >= L'A' && w_target[4] <= L'Z') ||
           (w_target[4] >= L'a' && w_target[4] <= L'z')) &&
          w_target[5] == L':' &&
          (w_target_len == 6 || w_target[6] == L'\\'))) {
      SetLastError(ERROR_SYMLINK_NOT_SUPPORTED);
      ::CloseHandle(handle);
      return TString();
    }

    // Remove leading \??\*
    w_target += 4;
    w_target_len -= 4;

  } else {
    // Reparse tag does not indicate a symlink.
    SetLastError(ERROR_SYMLINK_NOT_SUPPORTED);
    ::CloseHandle(handle);
    return TString();
  }

  // convert w_target to TString
  TArray<WCHAR> Array(w_target_len);
  TMemory::Copy(Array.FirstWrite(), w_target, Array.Size() * sizeof(WCHAR));
  return TString().SetFromCharArray(Array);
}

// -------------------------------------------------------------------------------------------------

static void SSetFileProperties(
  TFileProperties*        pProperties, 
  const WIN32_FIND_DATA*  pFindFileData)
{
  // file to system time and system time to local time
  SYSTEMTIME SystemTime, LocalTime;
  if (::FileTimeToSystemTime(&pFindFileData->ftLastWriteTime, &SystemTime) && 
      ::SystemTimeToTzSpecificLocalTime(NULL, &SystemTime, &LocalTime))
  {
    pProperties->mWriteDate = 
      TDate(LocalTime.wDay, LocalTime.wMonth, LocalTime.wYear);
    
    pProperties->mWriteTime = 
      TDayTime(LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond);
  }
  else
  {
    pProperties->mWriteDate = TDate();
    pProperties->mWriteTime = TDayTime();
  }

  pProperties->mSizeInBytes = ((long long)pFindFileData->nFileSizeHigh) << 32 | 
    (unsigned long long)pFindFileData->nFileSizeLow;

  pProperties->mAttributes = 0;

  if ((pFindFileData->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
  {
    pProperties->mAttributes |= TFileProperties::kSystem;
  }
  if ((pFindFileData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
  {
    pProperties->mAttributes |= TFileProperties::kHidden;
  }
  if ((pFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY))
  {
    pProperties->mAttributes |= TFileProperties::kReadOnly;
  }
}

// -------------------------------------------------------------------------------------------------

//! creat a double paired extra zero terminated string for OPENFILENAME

static TArray<WCHAR> SConstructFileFilterBufferForReading(
  const TList<TString>& FileFilter)
{
  TList<TString> TempFileFilter(FileFilter);

  // add a "all specified files" entry
  if (FileFilter.Size() > 1)
  {
    TString AllSelectedFilesEntry;
    for (int i = 0; i < FileFilter.Size(); ++i)
    {
      AllSelectedFilesEntry += FileFilter[i];

      if (i != FileFilter.Size() - 1)
      {
        AllSelectedFilesEntry += TString("; ");
      }
    }

    TempFileFilter.Prepend(AllSelectedFilesEntry);
  }

  // add a "all files" entry
  TempFileFilter.Append("*.*");

  int NeededChars = 0;
  for (int i = 0; i < TempFileFilter.Size(); ++i)
  {
    NeededChars += (TempFileFilter[i].Size() * 2) + 2;
  }

  TArray<WCHAR> FileFilterBuffer(NeededChars + 1);
  int CharPos = 0;

  for (int i = 0; i < TempFileFilter.Size(); ++i)
  {
    MAssert(TempFileFilter[i].StartsWithChar('*'), "Invalid FileFilter Entry");

    ::wcscpy(FileFilterBuffer.FirstWrite() + CharPos, TempFileFilter[i].Chars());
    CharPos += TempFileFilter[i].Size();
    FileFilterBuffer[CharPos++] = '\0';

    ::wcscpy(FileFilterBuffer.FirstWrite() + CharPos, TempFileFilter[i].Chars());
    CharPos += TempFileFilter[i].Size();
    FileFilterBuffer[CharPos++] = '\0';
  }

  FileFilterBuffer[CharPos] = '\0';

  return FileFilterBuffer;
}

// -------------------------------------------------------------------------------------------------

static HWND SDefaultBrowseDialogOwnerWindow()
{
  // Hack: duplicated from: SDefaultDialogOwnerWindow() in WinSystem.cpp
  
  HWND WindowHandle = (HWND)TSystem::ApplicationHandle();

  if (!::IsWindow(WindowHandle) || !::IsWindowVisible(WindowHandle) || 
      (::GetForegroundWindow() != WindowHandle && 
       WindowHandle == ::GetWindow(::GetForegroundWindow(), GW_OWNER)))
  {
    WindowHandle = ::GetForegroundWindow();
  }
  
  return WindowHandle;
}  

// -------------------------------------------------------------------------------------------------

static TArray<WCHAR> SConstructFileFilterBufferForWriting(
  const TList<TString>& FileFilter)
{
  // add a "all files" entry
  TList<TString> TempFileFilter(FileFilter);
  TempFileFilter.Append("*.*");

  int NeededChars = 0;
  for (int i = 0; i < TempFileFilter.Size(); ++i)
  {
    NeededChars += (TempFileFilter[i].Size() * 2) + 2;
  }

  TArray<WCHAR> FileFilterBuffer(NeededChars + 1);
  int CharPos = 0;

  for (int i = 0; i < TempFileFilter.Size(); ++i)
  {
    MAssert(TempFileFilter[i].StartsWithChar('*'), "Invalid FileFilter Entry");

    ::wcscpy(FileFilterBuffer.FirstWrite() + CharPos, TempFileFilter[i].Chars());
    CharPos += TempFileFilter[i].Size();
    FileFilterBuffer[CharPos++] = '\0';

    ::wcscpy(FileFilterBuffer.FirstWrite() + CharPos, TempFileFilter[i].Chars());
    CharPos += TempFileFilter[i].Size();
    FileFilterBuffer[CharPos++] = '\0';
  }

  FileFilterBuffer[CharPos] = '\0';

  return FileFilterBuffer;
}

// -------------------------------------------------------------------------------------------------

static TArray<WCHAR> SCreateDoubleZeroedName(const TString& FileNameAndPath)
{
  TArray<WCHAR> Chars(FileNameAndPath.Size() + 2);
  Chars.Init(0);

  // copy the full path 
  ::wcscpy(Chars.FirstWrite(), FileNameAndPath.Chars());

  if (Chars.Size() > 2 && Chars[Chars.Size() - 3] == '\\')
  {
    // remove the last `\`
    Chars[Chars.Size() - 3] = 0;
  }

  return Chars;
}

// -------------------------------------------------------------------------------------------------

static int CALLBACK SBrowseCallbackProc(
  HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
  switch (uMsg) 
  {
  case BFFM_INITIALIZED:
    /* change the selected folder. */
    ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    break;

  case BFFM_VALIDATEFAILED:
    ::MessageBox(
      SDefaultBrowseDialogOwnerWindow(),
      L"The entered path is not valid.",
      L"Invalid path", 
      MB_OK | MB_ICONWARNING);
    return 1; // Return 1 = continue, 0 = EndDialog

  case BFFM_SELCHANGED:
    // nothing to do
    break;

  default:
    break;
  }

  return 0;
}

// -------------------------------------------------------------------------------------------------

static bool SProcessWithSameNameIsRunning()
{
  // Take a snapshot of all processes in the system.
  HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  
  if (hProcessSnap == INVALID_HANDLE_VALUE)
  {
    MInvalid("CreateToolhelp32Snapshot failed");
    return false;
  }

  PROCESSENTRY32 pe32;
  // Set the size of the structure before using it.
  pe32.dwSize = sizeof(PROCESSENTRY32);

  // our processes name
  WCHAR ModuleFileName[1024] = {0};
  ::GetModuleFileName(NULL, ModuleFileName, 
    sizeof(ModuleFileName) / sizeof(WCHAR));
  const TString CurrentExeName = gCutPath(ModuleFileName);

  if (CurrentExeName.IsEmpty())
  {
    MInvalid("GetModuleFileName failed");
    return false;
  }

  // our processes id
  DWORD OurProcessId = ::GetCurrentProcessId();

  if (OurProcessId == 0)
  {
    MInvalid("GetCurrentProcessId Failed");
    return false;
  }

  // Retrieve information about the first process,
  // and exit if unsuccessful
  if (!::Process32First(hProcessSnap, &pe32))
  {
    MInvalid("Process32First Failed");
    ::CloseHandle(hProcessSnap);
    return false;
  }

  do
  {
    if (pe32.th32ProcessID != OurProcessId && 
        TString(pe32.szExeFile) == CurrentExeName)
    {
      ::CloseHandle(hProcessSnap);
      return true;
    }
  } 
  while(::Process32Next(hProcessSnap, &pe32));

  ::CloseHandle(hProcessSnap);
  return false;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool TDirectory::Exists()const
{
  if (ExistsIgnoreCase())
  {
    // be case sensitive (OSX or other Unix platforms can be pity here)
    #if defined(MDebug)
      if (ParentDirCount() > 0)
      {
        const TDirectory ParentPath = TDirectory(*this).Ascend();
        
        const TString RequestedName = TString(Path()).RemoveLast(
          SPathSeparator()).SplitAt(SPathSeparatorChar()).Last();

        const TList<TString> SubDirs = ParentPath.FindSubDirNames("*");
                                    
        MAssert(!SubDirs.IsEmpty(), 
          "Then GetFileAttributes should not have worked!");
                        
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
  if (! IsEmpty())
  {
    // Suppress error dialogs from the OS when there is no media in the drive
    ::SetErrorMode(SEM_FAILCRITICALERRORS); 
   
    const DWORD FileAttributes = ::GetFileAttributes(mDir.Chars());

    if (FileAttributes != INVALID_FILE_ATTRIBUTES &&
        !!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      return true;
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::CanWrite()const
{
  // Suppress error dialogs from the OS when there is no media in the drive
  ::SetErrorMode(SEM_FAILCRITICALERRORS);

  const DWORD FileAttributes = ::GetFileAttributes(mDir.Chars());

  if (FileAttributes != INVALID_FILE_ATTRIBUTES &&
      !(FileAttributes & FILE_ATTRIBUTE_READONLY) &&
      !!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
  {
    // check if we can really write (FILE_ATTRIBUTE_READONLY is not enough)...
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

time_t TDirectory::ModificationStatTime(bool CheckMostRecentSubFolders)const
{
  MAssert(ExistsIgnoreCase(), "Querying stat time for a non existing directory");
  
  time_t Ret = 0;
  
  // _wstat needs a path without trailing slash
  MAssert(Path().EndsWith(SPathSeparator()), "");
  const TString FixedPath = Path().RemoveLast(SPathSeparator());

  // NB: avoid using stat here. It's broken in the WinXP runtime.
  WIN32_FILE_ATTRIBUTE_DATA Attributes;
  if (::GetFileAttributesEx(mDir.Chars(), GetFileExInfoStandard, &Attributes))
  {
    // convert Windows filetime_t into a UNIX time_t
    LARGE_INTEGER Time;
    Time.HighPart = Attributes.ftLastWriteTime.dwHighDateTime;
    Time.LowPart = Attributes.ftLastWriteTime.dwLowDateTime;

    // remove the diff between 1970 and 1601
    Time.QuadPart -= 11644473600000 * 10000;

    // convert back from 100-nanoseconds to seconds
    Ret = (int)(Time.QuadPart / 10000000);

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

bool TDirectory::MayHaveSubDirs()const
{
  MAssert(ExistsIgnoreCase(), "Querying for sub dirs in a non existing directory");

  SHFILEINFO FileInfo = {0};
  ::SHGetFileInfo(mDir.Chars(), 0,
    &FileInfo, sizeof(FileInfo), SHGFI_ATTRIBUTES);

  // fix odd bug where all bits get lit for control panel objects
  if (FileInfo.dwAttributes == 0xFFFFFFFF)
  {
    FileInfo.dwAttributes = 0;
  }

  return (FileInfo.dwAttributes & SFGAO_FOLDER) && 
    (FileInfo.dwAttributes & SFGAO_HASSUBFOLDER); 
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::HasSubDirs()const
{
  MAssert(ExistsIgnoreCase(), "Querying for sub dirs in a non existing directory");

  WIN32_FIND_DATA FindFileData;    

  HANDLE FileHandle = ::FindFirstFileEx((mDir + "*").Chars(), FindExInfoStandard, 
    &FindFileData, FindExSearchLimitToDirectories, NULL, 0);

  if (FileHandle != INVALID_HANDLE_VALUE)
  {
    if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {                                                                   
      if (::wcslen(FindFileData.cFileName) &&
          ::wcscmp(FindFileData.cFileName, L".") != 0 && 
          ::wcscmp(FindFileData.cFileName, L"..") != 0)
      {
         ::FindClose(FileHandle);
         return true;
      }
    }

    while (::FindNextFile(FileHandle, &FindFileData) != 0)
    {
      if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {
        if (::wcslen(FindFileData.cFileName) &&
            ::wcscmp(FindFileData.cFileName, L".") != 0 && 
            ::wcscmp(FindFileData.cFileName, L"..") != 0)
        {
          ::FindClose(FileHandle);
          return true;
        }
      }
    }

    ::FindClose(FileHandle);
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::IsParentDirOf(const TDirectory& Other)const
{
  // case insensitive (see comparison operators)
  return Other.Path().StartsWithIgnoreCase(this->Path()) && *this != Other;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::IsSubDirOf(const TDirectory& Other)const
{
  // case insensitive (see comparison operators)
  return this->Path().StartsWithIgnoreCase(Other.Path()) && *this != Other;
}
  
// -------------------------------------------------------------------------------------------------

bool TDirectory::IsSameOrSubDirOf(const TDirectory& Other)const
{
  // case insensitive (see comparison operators)
  return this->Path().StartsWithIgnoreCase(Other.Path());
}

// -------------------------------------------------------------------------------------------------

TList<TString> TDirectory::FindFileNames(
  const TList<TString>& ExtensionList)const
{
  MAssert(ExistsIgnoreCase(), "Querying for files in a non existing directory");

  TList<TString> Ret;
  WIN32_FIND_DATA FindFileData;    

  // remove/fixed doubled extensions
  TList<TString> UniqueExtensionList;
  UniqueExtensionList.PreallocateSpace(ExtensionList.Size());
  for (int i = 0; i < ExtensionList.Size(); ++i)
  {
    if (! UniqueExtensionList.Contains(ExtensionList[i]))
    {
      UniqueExtensionList.Append(ExtensionList[i]);
    }
  }

  for (int e = 0; e < UniqueExtensionList.Size(); ++e)
  {
    const TList<TString> CurrentExtensionAsList(
      MakeList<TString>(UniqueExtensionList[e]));

    HANDLE FileHandle = ::FindFirstFileEx((mDir + UniqueExtensionList[e]).Chars(), 
      FindExInfoStandard, &FindFileData, FindExSearchNameMatch, NULL, 0);

    if (FileHandle != INVALID_HANDLE_VALUE)
    {
      // Note: The extra gFileMatchesExtension is to avoid that for example 
      // *.fla and *.flac will both match "Bla.flac" (Windows extra 3 letter 
      // extension support)

      // The first file
      if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {
        const TString FileName = TString(FindFileData.cFileName);

        if (!FileName.IsEmpty() && 
            gFileMatchesExtension(FileName, CurrentExtensionAsList))
        {
          Ret.Append(FileName);
        }
      }

      // Find the rest of the files
      while (::FindNextFile(FileHandle, &FindFileData) != 0)
      {
        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
          const TString FileName = TString(FindFileData.cFileName);

          if (!FileName.IsEmpty() && 
              gFileMatchesExtension(FileName, CurrentExtensionAsList))
          {
            Ret.Append(FileName);
          }
        }
      }

      ::FindClose(FileHandle);
    }
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TList<TFileProperties> TDirectory::FindFiles(
  const TList<TString>& ExtensionList)const
{
  MAssert(ExistsIgnoreCase(), "Querying for files in a non existing directory");

  TList<TFileProperties> FileList;

  WIN32_FIND_DATA FindFileData;    

  // remove/fixed doubled extensions
  TList<TString> UniqueExtensionList;
  UniqueExtensionList.PreallocateSpace(ExtensionList.Size());
  for (int i = 0; i < ExtensionList.Size(); ++i)
  {
    if (! UniqueExtensionList.Contains(ExtensionList[i]))
    {
      UniqueExtensionList.Append(ExtensionList[i]);
    }
  }

  for (int e = 0; e < UniqueExtensionList.Size(); ++e)
  {
    const TList<TString> CurrentExtensionAsList(
      MakeList<TString>(UniqueExtensionList[e]));

    HANDLE FileHandle = ::FindFirstFileEx((mDir + UniqueExtensionList[e]).Chars(), 
      FindExInfoStandard, &FindFileData, FindExSearchNameMatch, NULL, 0);

    if (FileHandle != INVALID_HANDLE_VALUE)
    {
      // Note: The extra gFileMatchesExtension is to avoid that for example 
      // *.fla and *.flac will both match "Bla.flac" (Windows extra 3 letter 
      // extension support)

      // The first file
      if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {
        const TString FileName = TString(FindFileData.cFileName);

        if (!FileName.IsEmpty() && 
            gFileMatchesExtension(FileName, CurrentExtensionAsList))
        {
          TFileProperties Properties;
          Properties.mName = FileName;
          SSetFileProperties(&Properties, &FindFileData);
          
          FileList.Append(Properties);
        }
      }

      // Find the rest of the files
      while (::FindNextFile(FileHandle, &FindFileData) != 0)
      {
        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
          const TString FileName = TString(FindFileData.cFileName);
          
          if (!FileName.IsEmpty() && 
              gFileMatchesExtension(FileName, CurrentExtensionAsList))
          {
            TFileProperties Properties;
            Properties.mName = FileName;
            SSetFileProperties(&Properties, &FindFileData);
            
            FileList.Append(Properties);
          }
        }
      }

      ::FindClose(FileHandle);
    }
  }

  return FileList;
}

// -------------------------------------------------------------------------------------------------

TList<TString> TDirectory::FindSubDirNames(
  const TString&          Searchmask, 
  TSymLinkRecursionTest*  pSymlinkChecker)const
{
  MAssert(ExistsIgnoreCase(), "Querying for sub dirs in a non existing directory");

  TList<TString> Ret;
  
  WIN32_FIND_DATA FindFileData;    

  HANDLE FileHandle = ::FindFirstFileEx((mDir + Searchmask).Chars(), 
    FindExInfoStandard, &FindFileData, FindExSearchLimitToDirectories, NULL, 0);

  if (FileHandle != INVALID_HANDLE_VALUE) 
  {
    do
    {
      if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {                                                                   
        if (FindFileData.cFileName[0] != 0 &&
            ::wcscmp(FindFileData.cFileName, L".") != 0 && 
            ::wcscmp(FindFileData.cFileName, L"..") != 0)
        {
          const TString FileName = TString(FindFileData.cFileName);
          
          // check for recursive links, when pSymlinkChecker is present and this is a link
          if (pSymlinkChecker && !!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
          {
            const TDirectory SrcPath = TDirectory(*this).Descend(FileName);
            const TString TargetPath = SReadLink(SrcPath.Path());
            if (!TargetPath.IsEmpty() && pSymlinkChecker->IsRecursedLink(SrcPath, TargetPath)) 
            {
              // symlinks recursed - do not add this directory
              continue;
            }
          }

          Ret.Append(FileName);
        }
      }
    } while (::FindNextFile(FileHandle, &FindFileData) != 0);

    ::FindClose(FileHandle);
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TList<TFileProperties> TDirectory::FindSubDirs(
  const TString&          Searchmask, 
  TSymLinkRecursionTest*  pSymlinkChecker)const
{
  MAssert(ExistsIgnoreCase(), "Querying for sub dirs in a non existing directory");

  TList<TFileProperties> Ret;

  WIN32_FIND_DATA FindFileData;    

  HANDLE FileHandle = ::FindFirstFileEx((mDir + Searchmask).Chars(), 
    FindExInfoStandard, &FindFileData, FindExSearchLimitToDirectories, NULL, 0);

  if (FileHandle != INVALID_HANDLE_VALUE) 
  {
    do
    {
      if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {                                                                   
        if (FindFileData.cFileName[0] != 0 &&
            ::wcscmp(FindFileData.cFileName, L".") != 0 && 
            ::wcscmp(FindFileData.cFileName, L"..") != 0)
        {
          const TString FileName = TString(FindFileData.cFileName);

          // check for recursive links, when pSymlinkChecker is present and this is a link
          if (pSymlinkChecker && !!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
          {
            const TDirectory SrcPath = TDirectory(*this).Descend(FileName);
            const TString TargetPath = SReadLink(SrcPath.Path());
            if (!TargetPath.IsEmpty() && pSymlinkChecker->IsRecursedLink(SrcPath, TargetPath)) 
            {
              // symlinks recursed - do not add this directory
              continue;
            }
          }

          TFileProperties Properties;
          Properties.mName = FileName;
          SSetFileProperties(&Properties, &FindFileData);
        
          Ret.Append(Properties);
        }
      }
    } while (::FindNextFile(FileHandle, &FindFileData) != 0);

    ::FindClose(FileHandle);
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::Create(bool CreateParentDirs)const
{
  if (ExistsIgnoreCase())
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

  const bool Success = ::_wmkdir(mDir.Chars()) == 0;
  return Success;
}

// -------------------------------------------------------------------------------------------------

bool TDirectory::Unlink()const
{
  // ... security check

  if (! ExistsIgnoreCase())
  {
    MInvalid("Cannot delete a non existing Dir!");
    return false;
  }

  if (ParentDirCount() == 0)
  {
    MInvalid("Won't delete a whole drive!");
    return false;
  }

  // ... try deleting an empty folder

  if (::RemoveDirectory(mDir.Chars()))
  {
    return true;
  }                    

  // ... recursively remove sub folders and files.

  const TString SearchMask(mDir + "*.*");

  WIN32_FIND_DATA FileData = {0};
  HANDLE SearchHandle = ::FindFirstFile(SearchMask.Chars(), &FileData); 

  if (SearchHandle == INVALID_HANDLE_VALUE)
  {
    MInvalid("Failed to get dir content");
    return false;
  }

  bool Finished = false; 

  while (!Finished)
  {
    const TString CurrFilename(mDir + FileData.cFileName);

    if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      if (::wcscmp(FileData.cFileName, L".") != 0 && 
          ::wcscmp(FileData.cFileName, L"..") != 0)
      {
        // recursively delete
        if (!TDirectory(CurrFilename).Unlink())
        {
          ::FindClose(SearchHandle);
          return false;
        }
      }
    }
    else
    {
      if (FileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
      {
        ::SetFileAttributes(CurrFilename.Chars(), FILE_ATTRIBUTE_NORMAL);
      }

      if (!::DeleteFile(CurrFilename.Chars()))
      {
        ::FindClose(SearchHandle);
        return false;
      }
    }

    if (!::FindNextFile(SearchHandle, &FileData) && 
        ::GetLastError() == ERROR_NO_MORE_FILES) 
    {
      Finished = true;
    } 
  }

  ::FindClose(SearchHandle);

  // Here the directory should be empty
  const bool Success = !!::RemoveDirectory(mDir.Chars());
  return Success;
}

// =================================================================================================

//! NB: global comparison operators for TDirectory are case-insensitive on Windows
//! becuae file-systems are case-insensitive  too

// -------------------------------------------------------------------------------------------------

bool operator== (const TDirectory& First, const TDirectory& Second)
{
  return gStringsEqualIgnoreCase(First.Path(), Second.Path());
}

// -------------------------------------------------------------------------------------------------

bool operator!= (const TDirectory& First, const TDirectory& Second)
{
  return !gStringsEqualIgnoreCase(First.Path(), Second.Path());
}

// -------------------------------------------------------------------------------------------------

bool operator< (const TDirectory& First, const TDirectory& Second)
{
  return gSortStringCompareIgnoreCase(First.Path(), Second.Path()) < 0;
}

// -------------------------------------------------------------------------------------------------

bool operator> (const TDirectory& First, const TDirectory& Second)
{
  return gSortStringCompareIgnoreCase(First.Path(), Second.Path()) > 0;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TDirectory gTempDir()
{
  static std::wstring sCachedTempDir;

  TString TempPath(sCachedTempDir.c_str());

  // also recreate when something (or even we) trashed the cached dir
  if (TempPath.IsEmpty() || ! TDirectory(TempPath).ExistsIgnoreCase())
  {
    WCHAR Path[MAX_PATH] = {0};
    if (::GetTempPath(MAX_PATH, Path))
    {
      TDirectory TempDir(Path);
      TempDir.Descend(gTempDirName());

      if (! TempDir.ExistsIgnoreCase())
      {
        TempDir.Create();
      }

      if (TempDir.ExistsIgnoreCase())
      {
        sCachedTempDir = TempDir.Path().Chars();
        return TempDir;
      }
    }

    // create a folder Temp in the AppDir if all failed
    TDirectory AppTempDir = gApplicationDir().Descend("Temp");
    AppTempDir.Create();

    sCachedTempDir = AppTempDir.Path().Chars();

    return AppTempDir;
  }
  else
  {
    return TDirectory(TempPath);
  }
}

// -------------------------------------------------------------------------------------------------

TDirectory gApplicationDir()
{
  // cached: often used
  static std::wstring sApplicationDir;

  if (sApplicationDir.empty())
  {
    const TDirectory Directory = 
      gExtractPath(TSystem::ApplicationPathAndFileName());

    if (Directory.ExistsIgnoreCase())
    {
      sApplicationDir = Directory.Path().Chars();
      return Directory;
    }
    
    // get current WorkingDir as fall back...
    MInvalid("Failed to resolve the application dir");

    wchar_t Buffer[1024] = {0};
    ::_wgetcwd(Buffer, MCountOf(Buffer));
    
    sApplicationDir = Buffer;

    return TDirectory(Buffer);
  }
  else
  {
    return TDirectory(sApplicationDir.c_str());
  }
}

// -------------------------------------------------------------------------------------------------

TDirectory gApplicationResourceDir()
{
  static std::wstring sResourcesBaseDir;
  
  if (sResourcesBaseDir.empty())
  {
    if (gRunningInDeveloperEnvironment())
    {
      sResourcesBaseDir = gDeveloperProjectsDir().Descend(
        MProductProjectsLocation).Descend("Resources").Path().Chars();
    }
    else
    {
      const TString AppBaseName = gCutExtensionAndPath(
        TSystem::ApplicationPathAndFileName());
      
      const TDirectory ResourceBundleDir = 
        gApplicationDir().Descend(AppBaseName + ".res");

      if (ResourceBundleDir.ExistsIgnoreCase())
      {
        // use "APP_NAME.res" for plugins, when present 
        sResourcesBaseDir = ResourceBundleDir.Path().Chars();
      }
      else
      {
        const TDirectory ResourceSubDir = 
          gApplicationDir().Descend("Resources");

        // else APP_DIR/Resources when it exists
        if (ResourceSubDir.ExistsIgnoreCase())
        {
          sResourcesBaseDir = ResourceSubDir.Path().Chars();
        }
        else
        {
          // APP_DIR as fallback (NB: needed for exes in resource folders)
          sResourcesBaseDir = gApplicationDir().Path().Chars();
        }
      }
    }

    TLog::SLog()->AddLine("System", "Using '%s' as resource base directory...", 
      TString(sResourcesBaseDir.c_str()).StdCString().c_str());
  }
  
  return TString(sResourcesBaseDir.c_str());
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserConfigsDir()
{
  static std::wstring sConfigsDir;

  if (sConfigsDir.empty())
  {
    const DWORD Location = CSIDL_APPDATA | CSIDL_FLAG_CREATE;
    const DWORD Flags = SHGFP_TYPE_CURRENT;

    WCHAR Path[MAX_PATH] = {0};
    if (SUCCEEDED(::SHGetFolderPath(NULL, Location, NULL, Flags, Path)))
    {
      TDirectory Dir = TDirectory(Path);
      Dir.Descend(MProductString);

      if (!Dir.ExistsIgnoreCase())
      {
        Dir.Create();
      }

      Dir.Descend(TString(MVersionString).RemoveFirst("V"));

      if (!Dir.ExistsIgnoreCase())
      {
        Dir.Create();
      }

      if (Dir.ExistsIgnoreCase())
      {
        sConfigsDir = Dir.Path().Chars();
        return Dir;
      }
    }

    // use the application dir if everything failed
    sConfigsDir = gApplicationDir().Path().Chars();
    return gApplicationDir();
  }
  else
  {
    return TDirectory(sConfigsDir.c_str());
  }
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserLogDir()
{
  return gUserConfigsDir();
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserDesktopDir()
{
  static std::wstring sDesktopDir;

  if (sDesktopDir.empty())
  {
    const DWORD Location = CSIDL_DESKTOP;
    const DWORD Flags = SHGFP_TYPE_CURRENT;

    WCHAR Path[MAX_PATH] = {0};
    if (SUCCEEDED(::SHGetFolderPath(NULL, Location, NULL, Flags, Path)))
    {
      sDesktopDir = Path;
      return TDirectory(Path);
    }
    else
    {
      MInvalid("Failed to get the Desktop dir!");
      return TDirectory();
    }
  }
  else
  {
    return TDirectory(sDesktopDir.c_str());
  }
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserDocumentsDir()
{
  static std::wstring sDocumentsDir;

  if (sDocumentsDir.empty())
  {
    const DWORD Location = CSIDL_MYDOCUMENTS;
    const DWORD Flags = SHGFP_TYPE_CURRENT;

    WCHAR Path[MAX_PATH] = {0};
    if (SUCCEEDED(::SHGetFolderPath(NULL, Location, NULL, Flags, Path)))
    {
      sDocumentsDir = Path;
      return TDirectory(Path);
    }
    else
    {
      MInvalid("Failed to get the Desktop dir!");
      return TDirectory();
    }
  }
  else
  {
    return TDirectory(sDocumentsDir.c_str());
  }
}

// -------------------------------------------------------------------------------------------------

TDirectory gUserHomeDir()
{
  static std::wstring sUserHomeDir;

  if (sUserHomeDir.empty())
  {
    const DWORD Location = CSIDL_DESKTOPDIRECTORY;
    const DWORD Flags = SHGFP_TYPE_CURRENT;

    WCHAR Path[MAX_PATH] = {0};
    if (SUCCEEDED(::SHGetFolderPath(NULL, Location, NULL, Flags, Path)))
    {
      // hack: there is no CSIDL_USER
      const TDirectory HomeDir = TDirectory(Path).Ascend();

      sUserHomeDir = HomeDir.Path().Chars();
      return HomeDir;
    }
    else
    {
      MInvalid("Failed to get the User dir!");
      return TDirectory();
    }
  }
  else
  {
    return TDirectory(sUserHomeDir.c_str());
  }
}

// -------------------------------------------------------------------------------------------------

bool gRunningInDeveloperEnvironment()
{
  static int sResult = -1;

  if (sResult == -1)
  {
    sResult = 0;
    
    // check if Product.Exe/../../../Source/*/XProductName exists and 
    // is under version control
    
    const TDirectory GitRoot = (gApplicationDir().ParentDirCount() >= 2) ?
      gApplicationDir().Ascend().Ascend() : TDirectory();
    const TDirectory SvnRoot = (gApplicationDir().ParentDirCount() >= 3) ?
      gApplicationDir().Ascend().Ascend().Ascend() : TDirectory();

    if ((GitRoot.ExistsIgnoreCase() &&
          TDirectory(GitRoot).Descend(".git").ExistsIgnoreCase()) ||
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
  return MakeList<TDirectory>(gApplicationDir());
}

// -------------------------------------------------------------------------------------------------

void gCleanUpTempDir()
{
  if (!SProcessWithSameNameIsRunning())
  {
    // delete all subdirs with same app name that do NOT contain our PID

    TDirectory TempRootDir(gTempDir().Ascend());
    TList<TString> Subdirs = TempRootDir.FindSubDirNames(MProductString + "-*");

    static const int sProcessId = (int)::GetCurrentProcessId();

    for (int i = 0; i < Subdirs.Size(); ++i)
    {
      if (Subdirs[i].Find(ToString(sProcessId)) == -1)
      {
        TDirectory(TempRootDir).Descend(Subdirs[i]).Unlink();
      }
    }
  }
}  

// -------------------------------------------------------------------------------------------------

TList<TDirectory> gAvailableDrives()
{
  TList<TDirectory> Ret;

  static const int sMaxDrives = (int)('Z' - 'A' + 1);

  int AvailDrivesMask = ::GetLogicalDrives();

  for (int DriveIndex = 0; DriveIndex < sMaxDrives; ++DriveIndex)
  {
    if (AvailDrivesMask&1)
    {
      const TString DrivePath = ToString((int)(DriveIndex + 'A'), "%c:");
      Ret.Append( TDirectory(DrivePath) );
    }

    AvailDrivesMask >>= 1;
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TDirectory gCurrentWorkingDir()
{
  wchar_t WorkingDirChars[MAX_PATH] = {0};
  wchar_t* pResult = ::_wgetcwd(WorkingDirChars, MAX_PATH);
  MAssert(pResult != NULL, "wgetcwd failed"); MUnused(pResult);

  return TDirectory(WorkingDirChars);
}

// -------------------------------------------------------------------------------------------------

void gSetCurrentWorkingDir(const TDirectory& Directory)
{
  const int Result = ::_wchdir(Directory.Path().Chars());
  MAssert(Result == 0, "wchdir failed"); MUnused(Result);
}

// -------------------------------------------------------------------------------------------------

bool gCopyFile(const TString& From, const TString& To, bool PreserveSymLinks)
{
  MUnused(PreserveSymLinks);

  if (gFileExists(To))
  {
    // on vista we do need to trash the To file first, 
    // else FO_COPY will fail (sometimes even crash)...
    if (!gMoveFileToTrash(To))
    {
      return false;
    }
  }

  TArray<WCHAR> FromChars(SCreateDoubleZeroedName(From));
  TArray<WCHAR> ToChars(SCreateDoubleZeroedName(To));

  SHFILEOPSTRUCT ShFileHandle = {0};
  ShFileHandle.hwnd = (HWND)TSystem::ApplicationHandle();
  ShFileHandle.wFunc = FO_COPY; 
  ShFileHandle.pFrom = FromChars.FirstWrite();
  ShFileHandle.pTo = ToChars.FirstWrite();
  ShFileHandle.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;

  const bool Success = SHFileOperation(&ShFileHandle) == 0;
  return Success;
}

// -------------------------------------------------------------------------------------------------

bool gRenameDirOrFile(const TString &OldName, const TString &NewName)
{
  if (OldName == NewName)
  {
    // don't bother with this...
    return true;
  }

  // try handling case sensitive renames. FO_MOVE wont do so...
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
    TArray<WCHAR> FromChars(SCreateDoubleZeroedName(OldName));
    TArray<WCHAR> ToChars(SCreateDoubleZeroedName(NewName));

    SHFILEOPSTRUCT ShFileHandle = {0};
    ShFileHandle.hwnd = (HWND)TSystem::ApplicationHandle();
    ShFileHandle.wFunc = FO_MOVE; 
    ShFileHandle.pFrom = FromChars.FirstWrite();
    ShFileHandle.pTo = ToChars.FirstWrite();
    ShFileHandle.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;

    const bool Success = SHFileOperation(&ShFileHandle) == 0;
    return Success;
  }
}

// -------------------------------------------------------------------------------------------------

bool gMoveFileToTrash(const TString &FileNameAndPath)
{
  // make sure that it ends with double zeros
  TArray<WCHAR> Chars(SCreateDoubleZeroedName(FileNameAndPath));

  SHFILEOPSTRUCT ShFileHandle = {0};
  ShFileHandle.hwnd = (HWND)TSystem::ApplicationHandle();
  ShFileHandle.wFunc = FO_DELETE; 
  ShFileHandle.pFrom = Chars.FirstWrite();
  ShFileHandle.fFlags = FOF_NOCONFIRMATION | FOF_ALLOWUNDO | FOF_SILENT;

  const bool Success = SHFileOperation(&ShFileHandle) == 0;
  return Success;
}

// -------------------------------------------------------------------------------------------------

bool gMakeFileWritable(const TString& FilePath)
{
  MAssert(gFileExists(FilePath), "Can only change flags for an existing file!");

  DWORD CurrentAttributes = ::GetFileAttributes(FilePath.Chars());
  
  if (CurrentAttributes != INVALID_FILE_ATTRIBUTES)
  {
    if (CurrentAttributes & FILE_ATTRIBUTE_READONLY)
    {
      CurrentAttributes &= ~FILE_ATTRIBUTE_READONLY;
      const bool Success = !!::SetFileAttributes(FilePath.Chars(), CurrentAttributes);
      return Success;
    }
    
    return true;
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

bool gMakeFileReadOnly(const TString& FilePath)
{
  MAssert(gFileExists(FilePath), "Can only change flags for an existing file!");

  DWORD CurrentAttributes = ::GetFileAttributes(FilePath.Chars());

  if (CurrentAttributes != INVALID_FILE_ATTRIBUTES)
  {
    if (!(CurrentAttributes & FILE_ATTRIBUTE_READONLY))
    {
      CurrentAttributes |= FILE_ATTRIBUTE_READONLY;
      const bool Success = !!::SetFileAttributes(FilePath.Chars(), CurrentAttributes);
      return Success;
    }

    return true;
  }

  return false;
}
