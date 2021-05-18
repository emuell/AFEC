#pragma once

#ifndef ZIPARCHIVE_H
#define ZIPARCHIVE_H

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/DateAndTime.h"
#include "CoreTypes/Export/File.h"

class TDirectory;
class TZipFileHeader;

// =================================================================================================

class TZipArchive
{
public:
  typedef char THeaderSignature[4];
  
  //! quickly check if the given file might be a ZipArchive 
  static bool SIsZipArchive(const TString& PathAndName);
  
  //! quickly check if the given file might be a ZipArchive with
  //! the given custom header sequence
  static bool SIsZipArchiveWithCustomHeader(
    const TString&           PathAndName, 
    const THeaderSignature&  Header);

  TZipArchive(const TString& PathAndName);
  ~TZipArchive();


  //@{ ... Archive properties

  //! return the file and path of the currently opended archive volume
  TString FileName()const;
  
  //! get the global comment
  TString ArchiveComment()const;
  //! Set the global comment in the archive
  //! must be open for writing!
  void SetArchiveComment(const TString& Comment);
  
  //! Setup 4 chars that we should use instead of the standard pkzip 
  //! header bytes (for reading and writing)
  void SetCustomHeader(const THeaderSignature& Header);
  //@}
  
  
  //@{ ... Open Close the archive

  //! test if the archive file is currently open (for read or write)
  bool IsOpen()const;

  //! Open a zip archive for reading or writing. returns success
  bool Open(TFile::TAccessMode Mode = TFile::kRead);

  //! close an open archive
  //! \param bool AfterException: set to true, if you want to close and reuse 
  //! TZipArchive after is has thrown an exception (it doesn't write any data
  //! to the file but only makes some cleaning then)
  void Close(bool AfterException = false);
  //@}


  //@{ ... Test the full archive or single entries

  //! Archive must be open in read or readwrite mode

  //! Test the complete archive 
  //! throws TReadableException on any problems
  void TestArchive();

  //! test a single entry
  void TestEntry(int Index);
  //@}

  

  //@{ ... Content Iteration
  
  //! Archive must be open in read or read write mode

  struct TEntry
  {
    bool mIsDirectory;

    TDate mDate;
    TDayTime mTime;

    int mCompressedSizeInBytes;
    int mUncompressedSizeInBytes;

    TString mArchiveLocalName;
    TString mComment;
    TArray<char> mExtraField;
  };

  //! get number of files in the archive
  int NumberOfEntries()const;

  //! find a file in the archive with the given name/path
  //! returns the index that is needed to access the file or -1
  int FindFile(const TString& ArchiveFileName)const;
  int FindFileIgnoreCase(const TString& ArchiveFileName)const;
  
  //! get the info of the file with the given index
  TEntry Entry(int Index)const;
  //@}


  //@{ ... Add files

  //! Archive must be open in write mode

  //! Add a new, existing file to the archive
  //! \param FileNameInArchive specifies the name, plus a possible local directory
  //! in the archive. If none is set, the filename will be extraced from the sourcefiles name
  //! \param CompressionLevel must be -1 for the default compression level, or a value
  //! between (including) 0 and 9  
  bool AddNewFile(
    const TString&  FileNameAndPath, 
    const TString&  FileNameInArchive = TString(), 
    int             CompressionLevel  = -1);

  //! Set a comment for the file with the given index
  //! if an index is specified, this entry at this index is used, else the last added
  void SetEntryComment(TString Comment, int Index = -1);
  
  //! set the extra field in the file header in the central directory
  //!  must be used after opening a new file in the archive, but before closing it
  //! if an index is specified, this entry at this index is used, else the last added
  void SetEntryExtraField(const TArray<char>& Extra, int Index = -1);
  //@}


  //@{ ... Remove files

  //! Archive must be open in write mode

  //! delete the whole content
  void DeleteAllFiles();

  //! delete the file with the given index
  void DeleteFile(int Index);

  //! delete several files from an opened archive
  void DeleteFiles(const TList<int>& Indexes);
  void DeleteFiles(const TList<TString>& Names);
  //@}


  //@{ ... Extract the archive

  //! Archive must be open in read or readwrite mode

  //! extract the whole archive content to the spcified directory
  bool Extract(
    const TDirectory& DestPath, 
    bool              UseArchivePaths = true);

  //! Extract a single file from the archive to the specified DestPath
  //! set UseArchivePaths to false if you dont want to use stored local 
  //! paths from the archive
  bool ExtractEntry(
    int               Index, 
    const TDirectory& DestPath, 
    bool              UseArchivePaths = true);
  //@}


private:
  //! prepare adding a new file to the zip archive
  bool OpenNewFile(
    TZipFileHeader& header, 
    int             CompressionLevel, 
    const TString&  FilePath);

  //! compress the contents of the buffer and write it to a new file
  //! Returns false if the new file hasn't been opened
  bool WriteNewFile(const void* pBuf, size_t Size);

  //! open the file with the given index in the archive for extracting
  bool OpenFile(int Index);

  //! decompress currently opened file to the bufor
  //! Returns number of bytes read      
  size_t ReadFile(void* pBuf, size_t Size);

  //! close current file and update date and attribute information of TFile
  int CloseFile(TFile& File);

  //! Close the file opened for extraction in the archive and copy its date and 
  //!   attributes to the file pointed by \e lpszFilePath
  //! \returns "1" = ok, "-1" = some bytes left to uncompress - probably due to a bad password,
  //! "-2" = setting extracted file date and attributes was not successful  
  int CloseFile(const TString& FilePath = TString(), bool AfterException = false);

  //! close the new file in the archive
  bool CloseNewFile();

  //! Perform the necessary cleanup after the exception 
  //! while testing the archive so that next files in the archive can be tested
  void CloseFileAfterTestFailed();

  //! delete entry with the index Index
  void DeleteInternal(int Index);
  
  //! returns bytes removed
  int RemovePackedFile(size_t StartOffset, size_t EndOffset);
  
  //! the currently open ZipFile (reading or writing)
  TZipFileHeader* CurrentFile();
  
  //! check if the file with the given index is a directory
  bool IsFileDirectory(int Index);

  //! get the info of the file with the given index
  bool GetFileInfo(TZipFileHeader& Info, int Index);

  //! throws an exception if the given error signals no success
  void CheckGZipError(int GZipErrCode);

  //! get the local extra filed of the currently opened 
  //!  for extraction file in the archive
  //! if pBuf == NULL return the size of the local extra field
  int GetLocalExtraField(char* pBuf, int Size);

  enum TArchiveFile
  {
    kExtract = -1, 
    kNotOpen, 
    kCompress
  };

  TArchiveFile mFileOpenMode;
  TFile mFile;

  THeaderSignature mHeaderSignature;
   
  class TZipArchiveRoot* mpArchiveRoot;
  class TGZipState* mpGZipState;
};


#endif // !defined(ZIPARCHIVE_H)

