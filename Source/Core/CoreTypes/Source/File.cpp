#include "CoreTypesPrecompiledHeader.h"

#include <cstring>
#include <cstdlib>

#include <cerrno>

#include <sys/stat.h>

#if defined(MWindows)
  #include <io.h>     // ::_commit

  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  
#elif defined(MMac)
  #include <fcntl.h>  // ::fcntl
  #include <unistd.h> // unlink

#elif defined(MLinux)
  #include <unistd.h> // ::fsync

#endif

// =================================================================================================

enum TEnum{ kEnumTest = 0 };
MStaticAssert(sizeof(bool) == 1 || sizeof(bool) == 4); // expected to be unreliable
MStaticAssert(sizeof(char) == 1);
MStaticAssert(sizeof(short) == 2);
MStaticAssert(sizeof(TEnum) == sizeof(int));
MStaticAssert(sizeof(int) == 4);
MStaticAssert(sizeof(long) == 4 || sizeof(long) == 8); // expected to be unreliable
MStaticAssert(sizeof(long long) == 8);
MStaticAssert(sizeof(float) == 4);
MStaticAssert(sizeof(double) == 8);
  
// =================================================================================================

#if defined(MWindows)
  FILE* ufopen(const TUnicodeChar* pFileName, const wchar_t* pFlags)
  {
    // Suppress error dialogs from the OS when there is no media in the drive
    ::SetErrorMode(SEM_FAILCRITICALERRORS); 
    
    return _wfopen(pFileName, pFlags);
  }
  
  FILE* ufreopen(const TUnicodeChar* pFileName, const wchar_t* pFlags, FILE* FileHandle)
  {
    // Suppress error dialogs from the OS when there is no media in the drive
    ::SetErrorMode(SEM_FAILCRITICALERRORS); 
    
    return _wfreopen(pFileName, pFlags, FileHandle);
  }

  int uunlink(const TUnicodeChar* pFileName)
  {
    // Suppress error dialogs from the OS when there is no media in the drive
    ::SetErrorMode(SEM_FAILCRITICALERRORS); 
    
    return _wunlink(pFileName);
  }

  #define fseek64 _fseeki64
  #define ftell64 _ftelli64

   bool ufexists(const TUnicodeChar* pFileName)
  {
    // Suppress error dialogs from the OS when there is no media in the drive
    ::SetErrorMode(SEM_FAILCRITICALERRORS);
    
    const DWORD FileAttributes = ::GetFileAttributes(pFileName);

    if (FileAttributes != INVALID_FILE_ATTRIBUTES &&
         !(FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      return true;
    }

    return false;
  }

#elif defined(MMac) || defined(MLinux)
  FILE* ufopen(const TUnicodeChar* pFileName, const wchar_t* pFlags)
  {
    const std::string Utf8FileName = 
      TString(pFileName).StdCString(TString::kFileSystemEncoding);

    const std::string FlagsCString = TString(pFlags).StdCString();

    struct stat FileStatInfo;
    if (::stat(Utf8FileName.c_str(), &FileStatInfo) == 0 &&
        S_ISDIR(FileStatInfo.st_mode))
    {
      // fopen strangely does not fail for directories, so we must 
      // checks this first...
      return NULL;
    }
    
    return ::fopen(Utf8FileName.c_str(), FlagsCString.c_str());
  }
  
  FILE* ufreopen(const TUnicodeChar* pFileName, const wchar_t* pFlags, FILE* FileHandle)
  {
    const std::string Utf8FileName = 
      TString(pFileName).StdCString(TString::kFileSystemEncoding);

    const std::string FlagsCString = TString(pFlags).StdCString();
    
    return ::freopen(Utf8FileName.c_str(), FlagsCString.c_str(), FileHandle);
  }

  int uunlink(const TUnicodeChar* pFileName)
  {
    const std::string Utf8FileName = 
      TString(pFileName).StdCString(TString::kFileSystemEncoding);

    return ::unlink(Utf8FileName.c_str());
  }
  
  #define fseek64 fseeko
  #define ftell64 ftello

  bool ufexists(const TUnicodeChar* pFileName)
  {
    // NB: could use stat here too, but will then need to follow symbolic links

    if (FILE* pFile = ::ufopen(pFileName, L"rb"))
    {
      ::fclose(pFile);
      return true;
    }

    return false;
  }

#else
  #error Unknown Platform
#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TFile::TFile(
  const TString&          FileName, 
  TByteOrder::TByteOrder  ByteOrder)
  : mFileHandle(NULL), 
    mMode(kReadWrite),
    mByteOrder(ByteOrder)
{
  SetFileName(FileName);
}

// -------------------------------------------------------------------------------------------------

TFile::~TFile()
{
  if (IsOpen())
  {
    Close();
  }
}

// -------------------------------------------------------------------------------------------------

TFile::TAccessMode TFile::AccessMode()const 
{ 
  return mMode; 
}

// -------------------------------------------------------------------------------------------------

bool TFile::IsOpen()const
{
  return mFileHandle != NULL;
}

// -------------------------------------------------------------------------------------------------

bool TFile::IsOpenForReading()const
{
  return (mFileHandle != NULL) && (mMode == kRead || 
    mMode == kReadWrite || mMode == kReadWriteAppend);
}

// -------------------------------------------------------------------------------------------------

bool TFile::IsOpenForWriting()const
{
  return (mFileHandle != NULL) && (mMode == kWrite || mMode == kWriteAppend || 
    mMode == kReadWrite || mMode == kReadWriteAppend);
}

// -------------------------------------------------------------------------------------------------

TString TFile::FileName()const
{
  return mFileName;
}

// -------------------------------------------------------------------------------------------------

void TFile::SetFileName(const TString& FileName)
{
  MAssert(!IsOpen(), "Cannot change the name while the file is open !");
  mFileName = FileName;

  mFileName.ReplaceChar(TDirectory::SWrongPathSeparatorChar(), 
    TDirectory::SPathSeparatorChar());
}

// -------------------------------------------------------------------------------------------------

bool TFile::Exists()const
{
  if (ExistsIgnoreCase())
  {
    // be case sensitive (OSX or other Unix platforms can be pitty here)
    #if defined(MDebug)
      const TDirectory Path = ::gExtractPath(mFileName);

      if (! Path.IsEmpty()) // can not check filenames relative to the CWD
      {
        const TString RequestedName = ::gCutPath(mFileName);

        const TList<TString> Files = Path.FindFileNames(MakeList<TString>("*.*"));
                                    
        MAssert(!Files.IsEmpty(), "Then fopen should not have worked!");
                        
        for (int i = 0; i < Files.Size(); ++i)
        {
          const TString FileName = Files[i];

          if (gStringsEqualIgnoreCase(FileName, RequestedName))
          {
            MAssert(FileName == RequestedName, 
              "Some filesystems will be case sensitive!");
            
            return true;
          }
        }
        
        #if defined(MMac) || defined(MLinux)
          // Hack: this can happen when using unicode filenames. Why?
          MAssert(! RequestedName.IsPureAscii(), "Should never reach this");
        #else
          MInvalid("Should never reach this");
        #endif  
      }
    #endif
    
    return true;
  }
  
  return false;
}

// -------------------------------------------------------------------------------------------------

bool TFile::ExistsIgnoreCase()const
{
  if (! mFileName.IsEmpty())
  {
    return ::ufexists(mFileName.Chars());
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

bool TFile::Open(TAccessMode Mode)
{
  if (mFileName.IsEmpty())
  {
    MInvalid("Set a filename first !");
    return false;
  }

  mMode = Mode;
  
  switch (Mode)
  {
  case kRead:
    mFileHandle = ::ufopen(mFileName.Chars(), L"rb");
    break;

  case kWrite:
    mFileHandle = ::ufopen(mFileName.Chars(), L"wb");
    break;

  case kWriteAppend:
    mFileHandle = ::ufopen(mFileName.Chars(), L"ab");
    break;
  
  case kReadWrite:
    mFileHandle = ::ufopen(mFileName.Chars(), L"wb+");
    break;

  case kReadWriteAppend:
    mFileHandle = ::ufopen(mFileName.Chars(), L"ab+");
    break;

  default:
    MInvalid("");
  }
    
  return mFileHandle != NULL;
}

// -------------------------------------------------------------------------------------------------

void TFile::Close()
{
  MAssert(IsOpen(), "File is not open.");
  
  ::fclose(mFileHandle);
  mFileHandle = NULL;
}

// -------------------------------------------------------------------------------------------------

void TFile::SetBufferSize(size_t Size)
{
  MAssert(IsOpen(), "File must be open");
  ::setvbuf(mFileHandle, NULL, _IOFBF, Size);  
}

// -------------------------------------------------------------------------------------------------

size_t TFile::Position()const
{
  MAssert(IsOpen(), "File is not open.");
  return (size_t)::ftell64(mFileHandle);
}

// -------------------------------------------------------------------------------------------------

bool TFile::SetPosition(size_t NewPos)
{
  MAssert(IsOpen(), "File is not open.");
  return (::fseek64(mFileHandle, (long long)NewPos, SEEK_SET) == 0);
}

// -------------------------------------------------------------------------------------------------

bool TFile::Skip(size_t NumOfBytes)
{
  return SetPosition(Position() + NumOfBytes);
}

// -------------------------------------------------------------------------------------------------

size_t TFile::SizeInBytes()const
{
  if (IsOpen())
  {
    const size_t OldPos = Position();
    const int Ret1 = ::fseek64(mFileHandle, 0, SEEK_END);
    MAssert(Ret1 == 0, "fseek failed"); MUnused(Ret1);
    
    const size_t Size = Position();
    const int Ret2 = ::fseek64(mFileHandle, OldPos, SEEK_SET);
    MAssert(Ret2 == 0, "fseek failed"); MUnused(Ret2);
    
    return Size;
  }
  else
  {
    TFile Temp(FileName());
    
    if (Temp.Open(kRead))
    {
      const int Ret = ::fseek64(Temp.mFileHandle, 0, SEEK_END);
      MAssert(Ret == 0, "fseek failed"); MUnused(Ret);
      
      return Temp.Position();
    }
    else
    {
      MInvalid("File is not accessible or doesnt exist");
      return 0;
    }
  }
}

// -------------------------------------------------------------------------------------------------

TByteOrder::TByteOrder TFile::ByteOrder()const
{
  return mByteOrder;
}

// -------------------------------------------------------------------------------------------------

void TFile::SetByteOrder(TByteOrder::TByteOrder ByteOrder)
{
  mByteOrder = ByteOrder;
}

// -------------------------------------------------------------------------------------------------

void TFile::Flush()
{
  MAssert(IsOpenForWriting(), "File is not open for writing!");
  
  if (::fflush(mFileHandle) != 0)
  {
    throw TReadableException(
      MText("Failed to flush the file to disk. Probably the disk is full."));
  }
}

// -------------------------------------------------------------------------------------------------

void TFile::FlushToDisk()
{
  MAssert(IsOpenForWriting(), "File is not open for writing!");
  
  if (::fflush(mFileHandle) != 0)
  {
    throw TReadableException(
      MText("Failed to flush the file to disk. Probably the disk is full."));
  }

  #if defined(MWindows)
    int FlushError = 0; MUnused(FlushError);
    FlushError = ::_commit(_fileno(mFileHandle));
    MAssert(FlushError == 0, "failed to flush the file!");
  
  #elif defined(MLinux)
    int FlushError = 0; MUnused(FlushError);
    FlushError = ::fsync(::fileno(mFileHandle));
    MAssert(FlushError == 0, "failed to flush the file!");
  
  #elif defined(MMac)
    int FlushError = 0; MUnused(FlushError);
    FlushError = ::fcntl(::fileno(mFileHandle), F_FULLFSYNC, NULL);
    MAssert(FlushError == 0, "failed to flush the file!");
  
  #else
    #error "unknown platform"
  #endif  
}

// -------------------------------------------------------------------------------------------------

TDate TFile::ModificationDate()const
{
  return TDate::SCreateFromStatTime(ModificationStatTime());
}

// -------------------------------------------------------------------------------------------------

TDayTime TFile::ModificationDayTime()const
{
  return TDayTime::SCreateFromStatTime(ModificationStatTime());
}

// -------------------------------------------------------------------------------------------------

int TFile::ModificationStatTime()const
{
  MAssert(ExistsIgnoreCase(), "Querying stat time for a non existing file");
  
  #if defined(MWindows)
    // NB: avoid using stat here. It's broken in the WinXP runtime.
    WIN32_FILE_ATTRIBUTE_DATA Attributes;
    if (::GetFileAttributesEx(mFileName.Chars(), GetFileExInfoStandard, &Attributes))
    {
      // convert Windows filetime_t into a UNIX time_t
      LARGE_INTEGER Time;
      Time.HighPart = Attributes.ftLastWriteTime.dwHighDateTime;
      Time.LowPart = Attributes.ftLastWriteTime.dwLowDateTime;

      // remove the diff between 1970 and 1601
      Time.QuadPart -= 11644473600000 * 10000;

      // convert back from 100-nanoseconds to seconds
      return (int)(Time.QuadPart / 10000000);
    }
    else
    {
      return 0;
    }
    
  #else
    const std::string Utf8FileName = 
      mFileName.StdCString(TString::kFileSystemEncoding);
  
    struct stat buf;
    int result = ::stat(Utf8FileName.c_str(), &buf);

    if (result != 0)
    {
      return 0;
    }
    else
    {
      return (int)buf.st_mtime;
    }
    
  #endif
}

// -------------------------------------------------------------------------------------------------

bool TFile::Unlink()
{
  MAssert(!IsOpen(), "Can only unlink closed Files !");

  const int Ret = ::uunlink(mFileName.Chars());
  return (Ret == 0);
}

// -------------------------------------------------------------------------------------------------

bool TFile::ReadByte(char& Char)
{
  MAssert(IsOpenForReading(), "File is not open for reading!");
  
  const size_t BytesRead = ::fread(&Char, 1, 1, mFileHandle);
  return (BytesRead == 1);
}

// -------------------------------------------------------------------------------------------------

bool TFile::WriteByte(const char& Char)
{
  MAssert(IsOpenForWriting(), "File is not open for writing!");

  const size_t BytesWritten = ::fwrite(&Char, 1, 1, mFileHandle);
  return (BytesWritten == 1);
}
  
// -------------------------------------------------------------------------------------------------

bool TFile::RewindByte(const char& Char)
{
  MAssert(IsOpenForReading(), "File is not open for reading!");

  const int Result = ::ungetc(Char, mFileHandle);
  return (Result != 0);
}

// -------------------------------------------------------------------------------------------------

bool TFile::ReadBytes(char* pBuffer, size_t* pBytesToRead)
{
  MAssert(IsOpenForReading(), "File is not open for reading!");

  *pBytesToRead = ::fread(pBuffer, 1, *pBytesToRead, mFileHandle);
  
  return (*pBytesToRead > 0);
}

// -------------------------------------------------------------------------------------------------

bool TFile::WriteBytes(const char* pBuffer, size_t* pBytesToWrite)
{
  MAssert(IsOpenForWriting(), "File is not open for writing!");

  *pBytesToWrite = ::fwrite(pBuffer, 1, *pBytesToWrite, mFileHandle);
  
  return (*pBytesToWrite > 0);
}

// -------------------------------------------------------------------------------------------------

void TFile::ReadText(
  TList<TString>& Text, 
  const TString&  IconvEncoding)
{
  Text.Empty();

  TArray<char> CharArray((int)SizeInBytes());
  Read(CharArray.FirstWrite(), SizeInBytes());

  int CharOffset = 0;
  
  TString SrcEncoding(IconvEncoding);
  
  
  // ... convert from UTF-16,32 or 8 automatically
  
  // UTF-8: EF BB BF
  if (CharArray.Size() > 3 && 
        (unsigned char)CharArray[0] == 0xEF && 
        (unsigned char)CharArray[1] == 0xBB &&
        (unsigned char)CharArray[2] == 0xBF)
  {
    SrcEncoding = "UTF-8";
    CharOffset = 3;
  }
  
  // UTF-16 Big Endian:  FE FF
  // UTF-16 Little Endian: FF FE
  else if (CharArray.Size() > 2 && 
           (((unsigned char)CharArray[0] == 0xFE && 
             (unsigned char)CharArray[1] == 0xFF) || 
            ((unsigned char)CharArray[0] == 0xFF && 
             (unsigned char)CharArray[1] == 0xFE)))
  {
    SrcEncoding = (unsigned char)CharArray[0] == 0xFE ? "UTF-16BE" : "UTF-16LE";
    CharOffset = 2;
  }
  
  // UTF-32 Big Endian:  00 00 FE FF
  // UTF-32 Little Endian: FF FE 00 00
  else if (CharArray.Size() > 4 && 
           (((unsigned char)CharArray[0] == 0x00 &&
             (unsigned char)CharArray[1] == 0x00 &&
             (unsigned char)CharArray[2] == 0xFE && 
             (unsigned char)CharArray[3] == 0xFF) || 
            ((unsigned char)CharArray[0] == 0xFF &&
             (unsigned char)CharArray[1] == 0xFE &&
             (unsigned char)CharArray[2] == 0x00 && 
             (unsigned char)CharArray[3] == 0x00)))
  {
    SrcEncoding = (unsigned char)CharArray[0] == 0x00 ? "UTF-32BE" : "UTF-32LE";
    CharOffset = 4;
  }
  

  // ... guess UTF-8 from the content if encoding is auto
  
  if (SrcEncoding == "auto")
  {
    SrcEncoding = ""; // platform encoding as fallback

    for (int i = 0; i < CharArray.Size(); ++i)
    {
      const unsigned char Character = 
        (unsigned char)CharArray.FirstRead()[i];
      
      if (Character >= 0x80) // non ascii
      {
        if ((Character & 0xE0) == 0xC0 ||
            (Character & 0xF0) == 0xE0 ||
            (Character & 0xF8) == 0xF0 ||
            (Character & 0xFC) == 0xF8 ||
            (Character & 0xFE) == 0xFC)
        {
          // looks like UTF-8
          SrcEncoding = "UTF-8";
          break;
        }
        else
        {
          // definitely not UTF8, assume platform encoding
          SrcEncoding = "";
          break;
        }
      }
    }
  }


  // ... convert to UTF-8 if needed
  
  TString::TCStringEncoding CStringEncoding;
  
  if (SrcEncoding.IsEmpty())
  {
    CStringEncoding = TString::kPlatformEncoding;
  }
  else if (SrcEncoding == "UTF-8")
  {
    CStringEncoding = TString::kUtf8;
  }
  else
  {
    // else convert char buffer to UTF8...
    CStringEncoding = TString::kUtf8;
    
    const std::string SrcEncodingCString = SrcEncoding.StdCString();
    TIconv IconvConverter(SrcEncodingCString.c_str(), "UTF-8");
      
    const char* pSourceChars = CharArray.FirstRead() + CharOffset;
    size_t SourceLen = CharArray.Size() - CharOffset;
      
    std::string TempString;
    
    while (SourceLen > 0)
    {
      // CharArray.Size() * 4: Try to encode them in one run if possible
      TArray<char> DestBuffer(MMax<int>(16, CharArray.Size() * 4 + 1));
      char* pDestChars = DestBuffer.FirstWrite();
    
      size_t DestLen = DestBuffer.Size() - 1; // -1: for the '\0'
    
      if (IconvConverter.Convert(&pSourceChars, &SourceLen, 
            &pDestChars, &DestLen) == size_t(-1))
      {
        switch (errno)
        {
        default:
          MInvalid("Unexpected iconv error!");
          SourceLen = 0;
          break;
          
        case E2BIG:
          MInvalid("Output buffer is too small!");
          // OK (handled), but should be avoided
          break;

        case EILSEQ:
          // Invalid multibyte sequence in the input
        case EINVAL:
          // Incomplete multibyte sequence in the input
          ++pSourceChars; *pDestChars++ = '?';
          --DestLen; --SourceLen; 
          break;
        }
      }
        
      *pDestChars = '\0';
      TempString += DestBuffer.FirstRead();
    }
    
    CharArray.SetSize((int)TempString.size());
    TMemory::Copy(CharArray.FirstWrite(), TempString.data(), TempString.size());
  }
    
  
  // ... split the UTF-8 char array
  
  const int Len = CharArray.Size();
   
  TList<char> CurrentLine;
  int i = CharOffset, LastFoundIndex = CharOffset;
  
  while (i < Len)
  {
    bool FoundNewLine = false;
    
    if (CharArray[i] == 0x0D)      // mac or win
    {
      if (i + 1 < Len && CharArray[i + 1] == 0x0A)  // win
      {
        i++;
      }
      
      FoundNewLine = true;
    }
    else if (CharArray[i] == 0x0A) // unix
    {
      FoundNewLine = true;
    }

    // create  a new paragraph 
    if (FoundNewLine)
    {
      CurrentLine.Append(0);
      Text.Append(TString(CurrentLine.FirstRead(), CStringEncoding));
      CurrentLine.Empty();

      LastFoundIndex = i + 1;
    }
    else
    {
      CurrentLine.Append(CharArray[i]);
    }

    ++i;
  }

  // don't miss the last paragraph if there isn't a newline at the end
  if (i > LastFoundIndex)
  {
    CurrentLine.Append(0);
    Text.Append(TString(CurrentLine.FirstRead(), CStringEncoding));
  }
}

// -------------------------------------------------------------------------------------------------

void TFile::WriteText(
  const TList<TString>& Text, 
  const TString&        IconvEncoding,
  const TString&        NewLine)
{
  TArray<char> NewLineChars;
  NewLine.CreateCStringArray(NewLineChars, TString::kAscii);
  
  if (IconvEncoding.IsEmpty())
  {
    for (int i = 0; i < Text.Size(); ++i)
    {
      const std::string LineCString = Text[i].StdCString(TString::kPlatformEncoding);
      Write(LineCString.c_str(), (TUInt32)LineCString.size());
      Write(NewLineChars.FirstRead(), NewLineChars.Size());
    }
  }
  else
  {
    const std::string IconvEncodingCString = IconvEncoding.StdCString();

    for (int i = 0; i < Text.Size(); ++i)
    {
      TArray<char> LineChars;
      Text[i].CreateCStringArray(LineChars, IconvEncodingCString.c_str());
    
      Write(LineChars.FirstRead(), LineChars.Size());
      Write(NewLineChars.FirstRead(), NewLineChars.Size());
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TFile::Read(TString& String, TString::TCStringEncoding Encoding)
{
  TInt32 Len = 0;
  Read(Len);

  if (Len < 0)
  {
    throw TReadableException(MText("Error while loading a string."));
  }
                 
  TArray<char> Chars((int)Len + 1);
  Read(Chars.FirstWrite(), Len);
                      
  // correct and ignore this one, as some old mods where 
  // saved this without null bytes
  Chars[(int)Len] = 0;

  String = TString(Chars.FirstRead(), Encoding);
}

void TFile::Read(bool& Value)
{
  // handle different bool sizes
   #if defined(MArch_PPC)
    MStaticAssert(sizeof(bool) == 4);
    MStaticAssert(MSystemByteOrder == MMotorolaByteOrder);
    
    Read( ((char*)(&Value))[3] );
    ((char*)(&Value))[2] = 0;
    ((char*)(&Value))[1] = 0;
    ((char*)(&Value))[0] = 0;
  
  #else
    MStaticAssert(sizeof(bool) == 1);
    
    Read((char&)Value);
  #endif
}

void TFile::Read(bool* pBuffer, size_t Count)
{
  for (size_t i = 0; i < Count; ++i)
  {
    Read(pBuffer[i]);
  }
}

void TFile::Read(T24 &Value)
{
  Read((char*)Value, sizeof(T24));  

  if (MSystemByteOrder != ByteOrder())
  {
    TByteOrder::Swap<T24>(Value);
  }
}

void TFile::Read(T24 *pBuffer, size_t Count)
{
  Read((char*)pBuffer, Count * sizeof(T24));  

  if (MSystemByteOrder != ByteOrder())
  {
    TByteOrder::Swap<T24>(pBuffer, Count);
  }
}

// -------------------------------------------------------------------------------------------------

void TFile::Write(const TString &String, TString::TCStringEncoding Encoding)
{ 
  const std::string CString = String.StdCString(Encoding).c_str();
  const TInt32 Length = (TInt32)CString.size() + 1;

  Write(Length);
  Write(CString.c_str(), Length);
}

void TFile::Write(const bool& Value)
{
  // handle different bool sizes
   #if defined(MArch_PPC)
    MStaticAssert(sizeof(bool) == 4);
    MStaticAssert(MSystemByteOrder == MMotorolaByteOrder);
    
    Write(((char*)(&Value))[3]);
    
  #else
    MStaticAssert(sizeof(bool) == 1);
    Write((char&)Value);

  #endif
}

void TFile::Write(const bool* pBuffer, size_t Count)
{
  for (size_t i = 0; i < Count; ++i)
  {
    Write(pBuffer[i]);
  }
}

void TFile::Write(const T24 &Value)
{
  T24 TempValue;
  TempValue[0] = Value[0];
  TempValue[1] = Value[1];
  TempValue[2] = Value[2];

  if (MSystemByteOrder != ByteOrder())
  {
    TByteOrder::Swap<T24>(TempValue);
  }

  Write((char*)TempValue, 3);  
}

void TFile::Write(const T24* pBuffer, size_t Count)
{
  for (size_t i = 0; i < Count; ++i)
  {
    Write(pBuffer[i]);
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TFileSetAndRestoreByteOrder::TFileSetAndRestoreByteOrder(
  TFile&                  File, 
  TByteOrder::TByteOrder  NewByteOrder)
  : mFile(File), mPreviousByteOrder(File.ByteOrder())
{ 
  mFile.SetByteOrder(NewByteOrder);
}

// -------------------------------------------------------------------------------------------------

TFileSetAndRestoreByteOrder::~TFileSetAndRestoreByteOrder()
{
  mFile.SetByteOrder(mPreviousByteOrder);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TFileStamp::TFileStamp(TFile* pFile)
  : mpFile(pFile)
{
  Stamp();
}

// -------------------------------------------------------------------------------------------------

void TFileStamp::Stamp()
{
  mPositionInBytes = mpFile->Position();
}

// -------------------------------------------------------------------------------------------------

size_t TFileStamp::DiffInBytes()const
{
  return (mpFile->Position() - mPositionInBytes);
}

