#pragma once

#ifndef ZipCentralDir_H
#define ZipCentralDir_H

// =================================================================================================

class TZipFileHeader;
class TFile;

// =================================================================================================

class TZipArchiveRoot
{
public:
  TZipArchiveRoot();
  ~TZipArchiveRoot();
  
  void Init();
  
  bool IsValidIndex(int Index)const;
  unsigned int GetSize(bool Whole = false)const;
  
  //! Convert the filename of the TZipFileHeader.
  //! \param FromZip: convert from archive format
  //! \param pHeader: the header to have filename converted; if NULL, convert 
  //! the currently opened file
  void ConvertFileName(bool FromZip, TZipFileHeader* pHeader = NULL);
  void ConvertAll();

  void Clear(bool Everything = true);
  
  void CloseFile();
  void OpenFile(const TZipArchive::THeaderSignature& Header, int Index);
  
  void Read();
  void Write();
  
  void RemoveFile(int Index);
  void AddNewFile(TZipFileHeader & header);
  void CloseNewFile();
  
  void RemoveFromDisk();
  
  // end of central dir signature    4 bytes  (0x06054b50)
  TInt8 mSignature[4];

  // number of this disk             2 bytes
  TInt16 mThisDiskNumber;

  // number of the disk with the
  // start of the central directory  2 bytes
  TInt16 mDiskWithCD;

  // total number of entries in
  // the central dir on this disk    2 bytes
  TInt16 mNumberOfDiskEntries;

  // total number of entries in
  // the central dir                 2 bytes
  TInt16 mNumberOfEntries;

  // size of the central directory   4 bytes
  TUInt32 mSize;

  // offset of start of central
  // directory with respect to
  // the starting disk number        4 bytes
  TUInt32 mOffset;

  // zipfile comment length          2 bytes
  // TInt16 mCommentSize;
  // zipfile comment (variable size)
  TArray<TInt8> mComment;

  TZipFileHeader* mpOpenedFile;
  
  TFile* mpFile;
  size_t mCentrDirPos;
  size_t mBytesBeforeZip;
  
  bool mOnDisk;
  
  TList<TZipFileHeader*> mHeaders;
  TArray<TInt8> mLocalExtraField;

  static const char sGZSignature[4];
  
protected:
  size_t Locate();  
  size_t WriteCentralEnd();
  
  void ReadHeaders();
  void WriteHeaders();
  void RemoveHeaders();
};


#endif // !defined(ZipCentralDir_H)

