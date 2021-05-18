#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/Directory.h"

#include <cstdlib> // qsort
#include <cstring> // memcmp

#include <limits>

namespace TZLib {
  #include "../../3rdParty/ZLib/Export/ZLib.h"
}

#if defined(MWindows)
  #include <windows.h> // SetFileAttributes
  #undef DeleteFile
  #undef min
  #undef max
#endif

// =================================================================================================

#define MBufferSize 65535

#ifndef DEF_MEM_LEVEL
  #if MAX_MEM_LEVEL >= 8
    #define DEF_MEM_LEVEL 8
  #else
    #define DEF_MEM_LEVEL MAX_MEM_LEVEL
  #endif
#endif

// =================================================================================================

namespace {
  // -----------------------------------------------------------------------------------------------

  int CompareInts(const void* pArg1, const void* pArg2)
  {
    int w1 = *(int*)pArg1;
    int w2 = *(int*)pArg2;
    
    return (w1 == w2) ? 0 : (w1 < w2) ? - 1 : 1;
  }

  // -----------------------------------------------------------------------------------------------

  void* ZLibAlloc(void* opaque, TZLib::uInt items, TZLib::uInt size)
  {
    return TMemory::Alloc("#ZipArchive", items * size);
  }

  // -----------------------------------------------------------------------------------------------

  void ZLibFree(void* opaque, void* address)
  {
    TMemory::Free(address);
  }
  
  // -----------------------------------------------------------------------------------------------

  bool HasDirectoryAttribute(
    TUInt16         VersionMadeBy, 
    TUInt32         Attributes, 
    const TString&  EntryName)
  {
    const static int sWinDirectory_Attribute = 0x10; // FILE_ATTRIBUTE_DIRECTORY
    const static int sUnixDirectory_Attribute = 0x4000; // S_IFDIR
    
    // When the entry's name ends in a '/' we always have a dir. Some third-party 
    // Zip programs do not set the subdir bits at all
    if (EntryName.EndsWith("/") || EntryName.EndsWith("\\"))
    {
      return true;
    }

    const int AttributeFormat = (VersionMadeBy >> 8);
    const int UnixAttributes = (Attributes >> 16);

    switch (AttributeFormat)
    {
    default:
      MInvalid("Unknown format"); // assume windows

    case  0: // MS-DOS and OS/2
    case  7: // Macintosh
    case 11: // Windows NTFS
    case 14: // VFAT
      return !!(Attributes & sWinDirectory_Attribute);
    
    case  2: // OpenVMS
    case  3: // UNIX                      
    case  5: // Atari ST
    case 13: // Acorn Risc
    case 16: // BeOS
    case 17: // Tandem
    case 19: // OS/X (Darwin) 
    case 30: // Atheos
      return !!(UnixAttributes & sUnixDirectory_Attribute);

    case  1: // Amiga
    case  4: // VM/CMS
    case  6: // OS/2 H.P.F.S.
    case  8: // Z-System
    case  9: // CP/M
    case 10: // MVS (OS/390 - Z/OS)
    case 12: // VSE
    case 15: // alternate MVS
    case 18: // OS/400
      MInvalid("Unsupported zip host format"); // but assume unix
      return !!(UnixAttributes & sUnixDirectory_Attribute);
    }
  }
}

// =================================================================================================

class TGZipState
{
public:
  TGZipState();
  
  void Init();
  
  size_t mBufferSize;
  TZLib::z_stream mZStream;

  size_t mUncomprLeft;
  size_t mComprLeft;
  TZLib::uLong mCrc32;
  
  TArray<char> mpBuffer;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TGZipState::TGZipState()
  : mBufferSize(16384),
    mZStream(),
    mUncomprLeft(0),
    mComprLeft(0),
    mCrc32(0)
{
}

// -------------------------------------------------------------------------------------------------

void TGZipState::Init()
{
  mpBuffer.SetSize((int)mBufferSize);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool TZipArchive::SIsZipArchive(const TString& PathAndName)
{
  return SIsZipArchiveWithCustomHeader(PathAndName, 
    TZipFileHeader::sPkZipSignature);
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::SIsZipArchiveWithCustomHeader(
  const TString&        PathAndName, 
  const THeaderSignature&  Header)
{
  try
  {
    TFile File(PathAndName);
    
    if (File.Open(TFile::kRead))
    {
      char FileHeader[sizeof(Header)];
      File.Read(FileHeader, sizeof(Header));

      return ::memcmp(FileHeader, Header, sizeof(Header)) == 0;
    }
  }
  catch (const TReadableException&) 
  {
    // ignore... 
  }

  return false;
}
  
// -------------------------------------------------------------------------------------------------

TZipArchive::TZipArchive(const TString& PathAndName)
{
  MStaticAssert(sizeof(TZipFileHeader::sPkZipSignature) == 
    sizeof(mHeaderSignature));
  
  TMemory::Copy(mHeaderSignature, TZipFileHeader::sPkZipSignature,
    sizeof(TZipFileHeader::sPkZipSignature));
  
  mFile.SetFileName(PathAndName);

  mpArchiveRoot = new TZipArchiveRoot();
  mpArchiveRoot->mpFile= &mFile;
  
  mpGZipState = new TGZipState();
  mpGZipState->mZStream.zalloc = (TZLib::alloc_func)ZLibAlloc;
  mpGZipState->mZStream.zfree = (TZLib::free_func)ZLibFree;
  
  mFileOpenMode = kNotOpen;
}

// -------------------------------------------------------------------------------------------------

TZipArchive::~TZipArchive()
{
  if (mFileOpenMode == kCompress)
  {
    // In case of an exception, the archive should be removed, but
    // thats something clients have to take care about (we cant)
    MInvalid("Forgot to close a Zip archive.");
  }
  
  delete mpGZipState;
  delete mpArchiveRoot;
  
  // AFiles destructor will close the file, if still open
}

// -------------------------------------------------------------------------------------------------

TString TZipArchive::FileName()const
{
  return mFile.FileName();
}

// -------------------------------------------------------------------------------------------------

TString TZipArchive::ArchiveComment()const
{
  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed");
    return "";
  }

  return TString().SetFromCStringArray(mpArchiveRoot->mComment);
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::SetArchiveComment(const TString &Comment)
{
  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed");
    return;
  }
  
  Comment.CreateCStringArray(mpArchiveRoot->mComment);
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::SetCustomHeader(const THeaderSignature& Header)
{
  MAssert(! IsOpen(), "Too late!");
  TMemory::Copy(mHeaderSignature, Header, sizeof(Header));
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::IsOpen()const
{
  return mFile.IsOpen();
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::Open(TFile::TAccessMode Mode)
{
  if (IsOpen())
  {
    MInvalid("ZipArchive already opened");
    return false;
  }

  mFileOpenMode = kNotOpen;
  
  if (mFile.Open(Mode))
  {
    mFile.SetByteOrder(TByteOrder::kIntel);
  
    mpArchiveRoot->Init();

    if (Mode == TFile::kReadWrite || Mode == TFile::kRead)
    {
      mpArchiveRoot->Read();
    }

    return true;
  }
  else
  {
    return false;
  }
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::Close(bool AfterException)
{
  // if after an exception - the archive may be closed, but the file may be opened
  if (! IsOpen() && (! AfterException || ! IsOpen()))
  {
    MInvalid("ZipArchive is already closed");
    return;
  }
  
  try
  {
    // close currently open files
    if (mFileOpenMode == kCompress)
    {
      CloseNewFile();
    }
    else if (mFileOpenMode == kExtract)
    {
      CloseFile();
    }
    
    // write the central directory
    mpArchiveRoot->Write();
  }
  catch (const TReadableException&)
  {
    if (AfterException)
    {
      // ignore errors on errors, but reset our write state
      mpGZipState->mpBuffer.Empty();
      mFileOpenMode = kNotOpen;
    }
    else
    {
      throw;
    }
  }
  
  // reset the archive root
  mpArchiveRoot->Clear();
  
  // autoflush (commit pending bytes to disk):
  if (mFile.IsOpenForWriting())
  {
    try
    {
      mFile.FlushToDisk();
    }
    catch (const TReadableException&)
    {
      if (AfterException)
      {
        // ignore errors or errors
      }
      else
      {
        throw;
      }
    }
  }

  // and finally close the file (never throws)
  mFile.Close();
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::TestArchive()
{
  for (int i = 0; i < NumberOfEntries(); ++i)
  {
    TestEntry(i);
  }
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::TestEntry(int Index)
{
  if (! mpArchiveRoot->IsValidIndex(Index))
  {
    MInvalid("Invalid Index");
    return;
  }

  TZipFileHeader* pHeader = mpArchiveRoot->mHeaders[Index];
  
  if (IsFileDirectory(Index))
  {
    size_t Size = pHeader->mCompressedSize;
    
    if ((Size != 0 || Size != pHeader->mUncompressedSize) && 
        // different treating compressed directories
        (Size == 12 && !pHeader->mUncompressedSize))
    {
      throw TReadableException(
        MText("Invalid directory entry in Zip archive."));
    }
  }
  else
  {
    try
    {
      if (! OpenFile(Index))
      {
        throw TReadableException(MText("Failed to open a Zip archive entry."));
      }

      TArray<char> TempBuffer(MBufferSize);
      size_t Read = 0;
      
      do
      {  
        Read = ReadFile(TempBuffer.FirstWrite(), (size_t)TempBuffer.Size());
      }
      while (Read == (size_t)TempBuffer.Size());

      CloseFile();
    }
    catch(TReadableException&)
    {
      CloseFileAfterTestFailed();
      throw;
    }
  }
}

// -------------------------------------------------------------------------------------------------

int TZipArchive::NumberOfEntries()const
{
  return mpArchiveRoot->mHeaders.Size();
}

// -------------------------------------------------------------------------------------------------

int TZipArchive::FindFile(const TString& ArchiveFileName)const
{
  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed");
    return -1;
  }

  for (int i = 0; i < mpArchiveRoot->mHeaders.Size(); ++i)
  {
    TZipFileHeader Header = *(mpArchiveRoot->mHeaders[i]);
    
    const bool FromZip = true;
    mpArchiveRoot->ConvertFileName(FromZip, &Header);

    if (gStringsEqual(Header.GetFileName(), ArchiveFileName))
    {
      return i;
    }
  }

  return -1;
}

// -------------------------------------------------------------------------------------------------

int TZipArchive::FindFileIgnoreCase(const TString& ArchiveFileName)const
{
  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed");
    return -1;
  }

  for (int i = 0; i < mpArchiveRoot->mHeaders.Size(); ++i)
  {
    TZipFileHeader Header = *(mpArchiveRoot->mHeaders[i]);

    const bool FromZip = true;
    mpArchiveRoot->ConvertFileName(FromZip, &Header);

    if (gStringsEqualIgnoreCase(Header.GetFileName(), ArchiveFileName))
    {
      return i;
    }
  }

  return -1;
}

// -------------------------------------------------------------------------------------------------

TZipArchive::TEntry TZipArchive::Entry(int Index)const
{
  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed");
    return TEntry();
  }
  
  if (! mpArchiveRoot->IsValidIndex(Index))
  {
    MInvalid("Invalid Index");
    return TEntry();
  }
  
  TZipFileHeader Header = *(mpArchiveRoot->mHeaders[Index]);
  const bool FromZip = true;
  mpArchiveRoot->ConvertFileName(FromZip, &Header);

  TEntry Entry;
  Entry.mIsDirectory = HasDirectoryAttribute(
    Header.mVersionMadeBy, Header.mExternalAttr, Header.GetFileName());
  Entry.mDate = Header.GetDate();
  Entry.mTime = Header.GetDayTime();
  Entry.mCompressedSizeInBytes = Header.mCompressedSize;
  Entry.mUncompressedSizeInBytes = Header.mUncompressedSize;
  Entry.mArchiveLocalName = Header.GetFileName();
  Entry.mComment = Header.GetComment();
  Entry.mExtraField = Header.mExtraField;
  
  return Entry;
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::AddNewFile(
  const TString&  FileNameAndPath, 
  const TString&  FileNameInArchive, 
  int             CompressionLevel)
{
  MAssert(CompressionLevel >= -1 && CompressionLevel <= 9, 
    "Invalid Compression Level");

  MAssert(TFile(FileNameAndPath).Exists(), "File could not be found");

  TZipFileHeader Header;
  
  Header.SetFileName(FileNameInArchive.IsEmpty() ? 
    gCutPath(FileNameAndPath) : FileNameInArchive);
  
  if (! OpenNewFile(Header, CompressionLevel, FileNameAndPath))
  {
    return false;
  }
  
  if (! HasDirectoryAttribute(
          Header.mVersionMadeBy, Header.mExternalAttr, Header.GetFileName()))
  {
    TFile File(FileNameAndPath);
    
    if (! File.Open(TFile::kRead))
    {
      return false;
    }
    
    size_t FileLength = File.SizeInBytes();
    size_t BytesRead = 0;

    TArray<char> Buffer(MBufferSize);
      
    do
    {
      const size_t NumBytes = MMin<size_t>(FileLength - BytesRead, MBufferSize);
      File.Read(Buffer.FirstWrite(), NumBytes);
      WriteNewFile(Buffer.FirstRead(), NumBytes);

      BytesRead += NumBytes;
    }
    while(BytesRead < FileLength);
  }

  CloseNewFile();
  return true;
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::SetEntryComment(TString Comment, int Index)
{
  if (Index == -1)
  {
    Index = mpArchiveRoot->mHeaders.Size() - 1;
  }

  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed");
    return;
  }
  
  if (! mpArchiveRoot->IsValidIndex(Index))
  {
    MInvalid("Invalid Index");
    return;
  }

  mpArchiveRoot->mHeaders[Index]->SetComment(Comment);
  mpArchiveRoot->RemoveFromDisk();
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::SetEntryExtraField(const TArray<char>& Extra, int Index)
{
  if (Index == -1)
  {
    Index = mpArchiveRoot->mHeaders.Size() - 1;
  }

  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed");
    return;
  }

  if (! mpArchiveRoot->IsValidIndex(Index))
  {
    MInvalid("Invalid Index");
    return;
  }

  mpArchiveRoot->mHeaders[Index]->mExtraField = Extra;
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::DeleteAllFiles()
{
  while (NumberOfEntries())
  {
    DeleteFile(0);
  }
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::DeleteFile(int Index)
{
  if (mFileOpenMode)
  {
    MInvalid("You cannot delete files if there is a file opened");
    return;
  }
  
  if (! mpArchiveRoot->IsValidIndex(Index))
  {
    MInvalid("Invalid Index");
    return;
  }
  
  mpGZipState->Init();
  mpArchiveRoot->RemoveFromDisk();
  
  DeleteInternal(Index);
  mpGZipState->mpBuffer.Empty();
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::DeleteFiles(const TList<TString>& Names)
{
  TList<int> Indexes;
  
  for (int i = 0; i < NumberOfEntries(); i++)
  {
    TZipFileHeader fh;
    GetFileInfo(fh, i);

    TString FileName = fh.GetFileName();
    
    for (int j = 0; j < Names.Size(); j++)
    {
      if (Names[j] == FileName)
      {
        Indexes.Append(i);
        break;
      }
    }
  }
  
  DeleteFiles(Indexes);
}

void TZipArchive::DeleteFiles(const TList<int>& Indexes)
{
  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed.");
    return;
  }
  
  if (mFileOpenMode)
  {
    MInvalid("You cannot delete files if there is a file opened.");
    return;
  }
  
  // sorting the index table using qsort 
  if (Indexes.Size())
  {
    ::qsort((void*)&Indexes[0], Indexes.Size(), sizeof(Indexes[0]), CompareInts);
    
    mpArchiveRoot->RemoveFromDisk();
    mpGZipState->Init();
    
    // remove in a reverse order
    for (int i = Indexes.Size() - 1; i >= 0; i--)
    {
      DeleteInternal(Indexes[i]);
    }

    mpGZipState->mpBuffer.Empty();
  }
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::Extract(
  const TDirectory& DestPath, 
  bool              UseArchivePaths)
{
  // create the root dest folder first
  if (! DestPath.Exists() && ! DestPath.Create())
  {
    return false;
  }
  
  // then extract the entries
  bool Result = true;

  for (int i = 0; i < NumberOfEntries(); ++i)
  {
    if (! ExtractEntry(i, DestPath, UseArchivePaths))
    {
      Result = false;
      // but continue
    }
  }

  return Result;
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::ExtractEntry(
  int               Index, 
  const TDirectory& DestPath, 
  bool              UseArchivePaths)
{
  // create the root dest folder first
  if (! DestPath.Exists() && ! DestPath.Create())
  {
    return false;
  }
  
  // to ensure that slash and OEM conversions take place
  TZipFileHeader Header;
  GetFileInfo(Header, Index); 
  
  TString ArchiveFileName = Header.GetFileName();

  const TString EntryDestFileNameAndPath = (UseArchivePaths) ?  
    DestPath.Path() + ArchiveFileName : 
    DestPath.Path() + gCutPath(ArchiveFileName); 
  
  if (IsFileDirectory(Index))
  {
    TDirectory(EntryDestFileNameAndPath).Create();
    // ignore errors as long as no files are stored inside the directory
    
    #if defined(MWindows)
      ::SetFileAttributes(EntryDestFileNameAndPath.Chars(), Header.mExternalAttr);
    #elif defined(MMac) || defined(MLinux)
      // use system's default permissions
    #endif
    
    return true;
  }
  else
  {
    if (! OpenFile(Index))
    {
      return false;
    }
    
    const TDirectory EntryDestPath(gExtractPath(EntryDestFileNameAndPath));
    
    TFile File(EntryDestFileNameAndPath);

    try
    {
      if (! EntryDestPath.Exists() && ! EntryDestPath.Create())
      {
        throw TReadableException(
          MText("Failed to create the destination directory '%s'", 
            EntryDestPath.Path()));
      }

      if (! File.Open(TFile::kWrite))
      {
        throw TReadableException(
          MText("Failed to open the destination file for writing:\n'%s'", 
          EntryDestFileNameAndPath));
      }
      
      size_t Read = 0;
      TArray<char> TempBuffer(MBufferSize);

      do
      {
        Read = ReadFile(TempBuffer.FirstWrite(), MBufferSize);

        if (Read)
        {  
          File.Write(TempBuffer.FirstRead(), Read);
        }
      }
      while (Read == (size_t)TempBuffer.Size());

      return (CloseFile(File) == 1);
    }  
    catch(const TReadableException&)
    {
      CloseFile(File);
      throw;
    }
  }
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::IsFileDirectory(int Index)
{
  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed");
    return false;
  }
  
  if (! mpArchiveRoot->IsValidIndex(Index))
  {
    MInvalid("Invalid Index");
    return false;
  }
  
  TZipFileHeader* pHeader = mpArchiveRoot->mHeaders[Index];
  
  return HasDirectoryAttribute(
    pHeader->mVersionMadeBy, pHeader->mExternalAttr, pHeader->GetFileName());
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::OpenFile(int Index)
{
  if (! mpArchiveRoot->IsValidIndex(Index))
  {
    MInvalid("Invalid Index");
    return false;
  }
  
  if (mFileOpenMode)
  {
    MInvalid("A file already opened");
    return false;
  }
  
  mpGZipState->Init();
  mpArchiveRoot->OpenFile(mHeaderSignature, Index);
  
  const int Method = CurrentFile()->mCompressionMethod;
  
  if (Method != 0 && Method != Z_DEFLATED)
  {
    throw TReadableException(MText("Unknown compression method!"));
  }
      
  if (Method == Z_DEFLATED)
  {
    mpGZipState->mZStream.opaque =  0;
    
    // windowBits is passed < 0 to tell that there is no zlib Header.
    // Note that in this case inflate *requires* an extra "dummy" byte
    // after the compressed stream in order to complete decompression and
    // return Z_STREAM_END. 

    const int GZipError = TZLib::inflateInit2_(&mpGZipState->mZStream, 
      -MAX_WBITS, ZLIB_VERSION, sizeof(TZLib::z_stream));

    CheckGZipError(GZipError);
  }

  mpGZipState->mComprLeft = CurrentFile()->mCompressedSize;
  mpGZipState->mUncomprLeft = CurrentFile()->mUncompressedSize;
  mpGZipState->mCrc32 = 0;
  mpGZipState->mZStream.total_out = 0;
  mpGZipState->mZStream.avail_in = 0;
  
  mFileOpenMode = kExtract;

  return true;
}

// -------------------------------------------------------------------------------------------------

int TZipArchive::GetLocalExtraField(char *pBuf, int Size)
{
  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed.");
    return -1;
  }
  
  if (mFileOpenMode != kExtract)
  {
    MInvalid("A file must be opened to get the local extra field");
    return -1;
  }
  
  int ExtraFieldSize = mpArchiveRoot->mLocalExtraField.Size();

  if (! pBuf || ! ExtraFieldSize)
  {
    return ExtraFieldSize;
  }
  
  if (Size < ExtraFieldSize)
  {
    ExtraFieldSize = Size;
  }
  
  if (ExtraFieldSize)
  {
    TMemory::Copy(pBuf, mpArchiveRoot->mLocalExtraField.FirstRead(), ExtraFieldSize);
  }

  return ExtraFieldSize;
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::CheckGZipError(int Err)
{
  if (Err == Z_OK || Err == Z_NEED_DICT)
  {
    return;
  }
  
  throw TReadableException(MText("GZip error: %s", ToString(Err)));
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::GetFileInfo(TZipFileHeader& fhInfo, int Index)
{
  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed");
    return false;
  }
  
  if (! mpArchiveRoot->IsValidIndex(Index))
  {
    MInvalid("Invalid Index");
    return false;
  }
  
  fhInfo = *(mpArchiveRoot->mHeaders[Index]);

  const bool FromZip = true;
  mpArchiveRoot->ConvertFileName(FromZip, &fhInfo);

  return true;
}

// -------------------------------------------------------------------------------------------------

TZipFileHeader* TZipArchive::CurrentFile()
{
  return mpArchiveRoot->mpOpenedFile;
}

// -------------------------------------------------------------------------------------------------

size_t TZipArchive::ReadFile(void *pBuf, size_t Size)
{
  if (mFileOpenMode != kExtract)
  {
    MInvalid("Current file must be opened");
    return 0;
  }
  
  if (! pBuf || ! Size)
  {
    return 0;
  }
  
  mpGZipState->mZStream.next_out = (TZLib::Bytef*)pBuf;
  
  mpGZipState->mZStream.avail_out = (Size > mpGZipState->mUncomprLeft) ? 
    (TZLib::uInt)mpGZipState->mUncomprLeft : (TZLib::uInt)Size;
  
  size_t Read = 0;
  
  // may happen when the file is 0 sized
  const bool bForce = (mpGZipState->mZStream.avail_out == 0 && mpGZipState->mComprLeft > 0);

  while (mpGZipState->mZStream.avail_out > 0 || (bForce && mpGZipState->mComprLeft > 0))
  {
    if ((mpGZipState->mZStream.avail_in == 0) && (mpGZipState->mComprLeft > 0))
    {
      size_t ToRead = (size_t)mpGZipState->mpBuffer.Size();

      if (mpGZipState->mComprLeft < ToRead)
      {
        ToRead = mpGZipState->mComprLeft;
      }
      
      if (ToRead == 0)
      {
        return 0;
      }
      
      mFile.Read(mpGZipState->mpBuffer.FirstWrite(), ToRead);
      mpGZipState->mComprLeft -= ToRead;
      
      mpGZipState->mZStream.next_in = (unsigned char*)mpGZipState->mpBuffer.FirstWrite();
      mpGZipState->mZStream.avail_in = (TZLib::uInt)ToRead;
    }
    
    if (CurrentFile()->mCompressionMethod == 0)
    {
      size_t ToCopy = (mpGZipState->mZStream.avail_out < mpGZipState->mZStream.avail_in) ? 
        mpGZipState->mZStream.avail_out : mpGZipState->mZStream.avail_in;
      
      if (ToCopy == 0 && mpGZipState->mUncomprLeft > 0)
      {
        throw TReadableException(MText("Unexpected end of archive!"));
      }

      if (ToCopy)
      {
        TMemory::Copy(mpGZipState->mZStream.next_out, 
          mpGZipState->mZStream.next_in, ToCopy);
      
        mpGZipState->mCrc32 = TZLib::crc32(mpGZipState->mCrc32, 
          mpGZipState->mZStream.next_out, (TZLib::uInt)ToCopy);
      
        mpGZipState->mUncomprLeft -= ToCopy;
        mpGZipState->mZStream.avail_in -= (TZLib::uInt)ToCopy;
        mpGZipState->mZStream.avail_out -= (TZLib::uInt)ToCopy;
        mpGZipState->mZStream.next_out += (TZLib::uInt)ToCopy;
        mpGZipState->mZStream.next_in += (TZLib::uInt)ToCopy;
        mpGZipState->mZStream.total_out += (TZLib::uInt)ToCopy;
        Read += ToCopy;
      }
    }
    else
    {
      size_t Total = mpGZipState->mZStream.total_out;
      TZLib::Bytef* pOldBuf =  mpGZipState->mZStream.next_out;
      const int GZipError = inflate(&mpGZipState->mZStream, Z_SYNC_FLUSH);
      size_t ToCopy = mpGZipState->mZStream.total_out - Total;
      
      mpGZipState->mCrc32 = 
        TZLib::crc32(mpGZipState->mCrc32, pOldBuf, (TZLib::uInt)ToCopy);
      
      mpGZipState->mUncomprLeft -= ToCopy;
      Read += ToCopy;
            
      if (GZipError == Z_STREAM_END)
      {
        return Read;
      }
      
      CheckGZipError(GZipError);
    }
  }
  
  return Read;
}

// -------------------------------------------------------------------------------------------------

int TZipArchive::CloseFile(TFile& File)
{
  const TDirectory Temp = gExtractPath(File.FileName());
  
  if (File.IsOpen())
  {
    File.Close();
  }
 
  return CloseFile(Temp.Path(), false);
}

int TZipArchive::CloseFile(const TString& FilePath, bool AfterException)
{
  if (mFileOpenMode != kExtract)
  {
    MInvalid("No opened file");
    return false;
  }

  int Ret = 1;

  if (! AfterException)
  {
    if (mpGZipState->mUncomprLeft == 0)
    {
      if (mpGZipState->mCrc32 != CurrentFile()->mCrc32)
      {
        throw TReadableException(MText("CRC error!"));
      }
    }
    else
    {
      Ret = -1;
    }
            
    if (CurrentFile()->mCompressionMethod == Z_DEFLATED)
    {
      TZLib::inflateEnd(&mpGZipState->mZStream);
    }
    
    
    if (! FilePath.IsEmpty())
    {
      // TODO?
      // TFile(FilePath).SetDayTime(ADateAndTime::CurrentDayTime());
      // TFile(FilePath).SetDate(ADateAndTime::CurrentDate());
      
      #if defined(MWindows)
        if (::SetFileAttributes(FilePath.Chars(), CurrentFile()->mExternalAttr) == 0)
        {
          return -2;
        }
      #elif defined(MMac) || defined(MLinux)
        // use systems default permissions?
      #endif
    }
  }
  
  mpArchiveRoot->CloseFile();
  mFileOpenMode = kNotOpen;
  mpGZipState->mpBuffer.Empty();
  
  return Ret;
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::OpenNewFile(
  TZipFileHeader& Header, 
  int             CompressionLevel,
  const TString&  FilePath)
{
  if (! IsOpen())
  {
    MInvalid("ZipArchive is closed");
    return false;
  }
  
  if (mFileOpenMode)
  {
    MInvalid("A file already opened");
    return false;
  }
  
  if (NumberOfEntries() == (int)std::numeric_limits<TUInt16>::max())
  {
    throw TReadableException(
      MText("Maximum file count inside the Zip archive reached!"));
  }
      
  if (! FilePath.IsEmpty())
  {
    Header.SetDayTime(TDayTime::SCurrentDayTime());
    Header.SetDate(TDate::SCurrentDate());
      
    #if defined(MWindows)
      Header.mExternalAttr = ::GetFileAttributes(FilePath.Chars()); 
      
      if (Header.mExternalAttr == -1)
      {
        return false;
      }
    #else
      const static int sWinDirectoryAttributes = 0x10; // FILE_ATTRIBUTE_DIRECTORY

      Header.mExternalAttr = TDirectory(FilePath).Exists() ? 
        sWinDirectoryAttributes : 0; 
    #endif
  }

  mpGZipState->Init();
  mpArchiveRoot->AddNewFile(Header);

  TString FileName = CurrentFile()->GetFileName();
  
  if (FileName.IsEmpty())
  {
    FileName = TString("file") + ToString(NumberOfEntries());
    CurrentFile()->SetFileName(FileName);
  }

  // this ensures the conversion will take place anyway (must take because we are going 
  //   to write the local Header in a moment
  const bool FromZip = false;
  mpArchiveRoot->ConvertFileName(FromZip);
                  
  const bool FileHasDirectoryAttribute = HasDirectoryAttribute(
    CurrentFile()->mVersionMadeBy, 
    CurrentFile()->mExternalAttr, 
    CurrentFile()->GetFileName());

  const bool Encrypted = false;
  const bool ExtraHeader = false;
  
  if (! CurrentFile()->PrepareData(CompressionLevel, ExtraHeader, Encrypted))
  {
    throw TReadableException(MText("Error while preparing the Zip data!"));
  }

  CurrentFile()->WriteLocal(mHeaderSignature, &mFile);

  // we have written the local Header, but if we keep filenames not converted
  // in memory, we have to restore the non-converted value
  CurrentFile()->SetFileName(FileName);
  
  mpGZipState->mComprLeft = 0;
  mpGZipState->mZStream.avail_in = (TZLib::uInt)0;
  mpGZipState->mZStream.avail_out = (TZLib::uInt)mpGZipState->mpBuffer.Size();
  mpGZipState->mZStream.next_out = (unsigned char*)(char*)mpGZipState->mpBuffer.FirstWrite();
  mpGZipState->mZStream.total_in = 0;
  mpGZipState->mZStream.total_out = 0;
  
  if (FileHasDirectoryAttribute && (CurrentFile()->mCompressionMethod != 0))
  {
    CurrentFile()->mCompressionMethod = 0;
  }
  
  if (CurrentFile()->mCompressionMethod == Z_DEFLATED)
  {
    mpGZipState->mZStream.opaque = 0;

    const int GZipError = TZLib::deflateInit2_(&mpGZipState->mZStream, CompressionLevel,
      Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
      ZLIB_VERSION, sizeof(TZLib::z_stream));

    CheckGZipError(GZipError);
  }

  mFileOpenMode = kCompress;

  return true;
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::WriteNewFile(const void *pBuf, size_t Size)
{
  if (mFileOpenMode != kCompress)
  {
    MInvalid("A new file must be opened");
    return false;
  }
  
  mpGZipState->mZStream.next_in = (unsigned char*)pBuf;
  mpGZipState->mZStream.avail_in = (TZLib::uInt)Size;
  
  CurrentFile()->mCrc32 = (TUInt32)TZLib::crc32(
      CurrentFile()->mCrc32, (unsigned char*)pBuf, (TZLib::uInt)Size);

  while (mpGZipState->mZStream.avail_in > 0)
  {
    if (mpGZipState->mZStream.avail_out == 0)
    {
      mFile.Write(mpGZipState->mpBuffer.FirstRead(), mpGZipState->mComprLeft);
      mpGZipState->mComprLeft = 0;
      mpGZipState->mZStream.avail_out = mpGZipState->mpBuffer.Size();
      mpGZipState->mZStream.next_out = (unsigned char*)mpGZipState->mpBuffer.FirstWrite();
    }
  
    if (CurrentFile()->mCompressionMethod == Z_DEFLATED)
    {
      size_t Total = mpGZipState->mZStream.total_out;
      const int GZipError = deflate(&mpGZipState->mZStream,  Z_NO_FLUSH);
      CheckGZipError(GZipError);
      mpGZipState->mComprLeft += mpGZipState->mZStream.total_out - Total;
    }
    else
    {
      size_t ToCopy = (mpGZipState->mZStream.avail_in < mpGZipState->mZStream.avail_out) ? 
        mpGZipState->mZStream.avail_in : mpGZipState->mZStream.avail_out;
  
      if (ToCopy)
      {
        TMemory::Copy(mpGZipState->mZStream.next_out, 
          mpGZipState->mZStream.next_in, ToCopy);
  
        mpGZipState->mZStream.avail_in -= (TZLib::uInt)ToCopy;
        mpGZipState->mZStream.avail_out -= (TZLib::uInt)ToCopy;
        mpGZipState->mZStream.next_in += (TZLib::uInt)ToCopy;
        mpGZipState->mZStream.next_out += (TZLib::uInt)ToCopy;
        mpGZipState->mZStream.total_in += (TZLib::uInt)ToCopy;
        mpGZipState->mZStream.total_out += (TZLib::uInt)ToCopy;
        mpGZipState->mComprLeft += ToCopy;
      }
    }
  }

  return true;
}

// -------------------------------------------------------------------------------------------------

bool TZipArchive::CloseNewFile()
{
  if (mFileOpenMode != kCompress)
  {
    MInvalid("A new file must be opened");
    return false;
  }
  
  mpGZipState->mZStream.avail_in = 0;
    
  int GZipError = Z_OK;
  
  try
  {
    if (CurrentFile()->mCompressionMethod == Z_DEFLATED)
    {
      while (GZipError == Z_OK)
      {
        if (mpGZipState->mZStream.avail_out == 0)
        {
          mFile.Write(mpGZipState->mpBuffer.FirstRead(), mpGZipState->mComprLeft);
          mpGZipState->mComprLeft = 0;
          mpGZipState->mZStream.avail_out = mpGZipState->mpBuffer.Size();
          mpGZipState->mZStream.next_out = (unsigned char*)mpGZipState->mpBuffer.FirstWrite();
        }
        size_t Total = mpGZipState->mZStream.total_out;
        GZipError = TZLib::deflate(&mpGZipState->mZStream,  Z_FINISH);
        mpGZipState->mComprLeft += mpGZipState->mZStream.total_out - Total;
      }
    }
  
    if (GZipError == Z_STREAM_END)
    {
      GZipError = Z_OK;
    }
  
    CheckGZipError(GZipError);
    
    if (mpGZipState->mComprLeft > 0)
    {
      mFile.Write(mpGZipState->mpBuffer.FirstRead(), mpGZipState->mComprLeft);
    }
  }
  catch(TReadableException&)
  {
    if (CurrentFile()->mCompressionMethod == Z_DEFLATED)
    {
      TZLib::deflateEnd(&mpGZipState->mZStream);
    } 

    mpArchiveRoot->CloseNewFile();
    mFileOpenMode = kNotOpen;     
    throw;
  }
  
  if (CurrentFile()->mCompressionMethod == Z_DEFLATED)
  {
    GZipError = TZLib::deflateEnd(&mpGZipState->mZStream);
    CheckGZipError(GZipError);
  }
    
  // it may be increased by the encrypted Header size
  CurrentFile()->mCompressedSize += (TUInt32)mpGZipState->mZStream.total_out;
  CurrentFile()->mUncompressedSize = (TUInt32)mpGZipState->mZStream.total_in;
  
  mpArchiveRoot->CloseNewFile();
  mFileOpenMode = kNotOpen;
  mpGZipState->mpBuffer.Empty();

  
  return true;
}

// -------------------------------------------------------------------------------------------------

int TZipArchive::RemovePackedFile(size_t StartOffset, size_t EndOffset)
{
  StartOffset += mpArchiveRoot->mBytesBeforeZip;
  EndOffset += mpArchiveRoot->mBytesBeforeZip;
  
  size_t BytesToCopy = mFile.SizeInBytes() - EndOffset;
  size_t TotalToWrite = BytesToCopy;
  
  char* TempBuffer = (char*)mpGZipState->mpBuffer.FirstWrite();
  
  if (BytesToCopy > (size_t)mpGZipState->mpBuffer.Size()) 
  {
    BytesToCopy = mpGZipState->mpBuffer.Size();
  }
  
  size_t TotalWritten = 0;
  
  mFile.SetPosition(EndOffset + TotalWritten);
  mFile.Read(TempBuffer, BytesToCopy);
  
  mFile.SetPosition(StartOffset + TotalWritten);
  mFile.Write(TempBuffer, BytesToCopy);

  TotalWritten += BytesToCopy;

  if (TotalToWrite != TotalWritten)
  {
    throw TReadableException(
      MText("File I/O error: Please make sure that enough space "
        "is available on the device!"));
  }

  const int Removed = (int)EndOffset - (int)StartOffset;
  return Removed;
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::DeleteInternal(int Index)
{
  TZipFileHeader* pfh = mpArchiveRoot->mHeaders[Index];
  int OtherOffsetChanged = 0;
  
  if (Index == NumberOfEntries() - 1) // last entry or the only one entry
  {
    ;//mFile.SetLength(pfh->mOffset + mpArchiveRoot->mBytesBeforeZip);            
  }
  else
  {
    OtherOffsetChanged = RemovePackedFile((size_t)pfh->mOffset, 
      (size_t)mpArchiveRoot->mHeaders[Index + 1]->mOffset);
  }
      
  mpArchiveRoot->RemoveFile(Index);
  
  // (update offsets in file headers in the central dir)
  if (OtherOffsetChanged)
  {
    for (int i = Index; i < NumberOfEntries(); i++)
    {
      mpArchiveRoot->mHeaders[i]->mOffset = 
        mpArchiveRoot->mHeaders[i]->mOffset - OtherOffsetChanged;
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TZipArchive::CloseFileAfterTestFailed()
{
  if (mFileOpenMode != kExtract)
  {
    MInvalid("No file opened");
    return;
  }

  mpGZipState->mpBuffer.Empty();
  mpArchiveRoot->Clear(false);
  mFileOpenMode = kNotOpen;
}

