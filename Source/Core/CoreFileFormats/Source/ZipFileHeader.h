#pragma once

#ifndef ZipFileHeader_H
#define ZipFileHeader_H

#include "CoreTypes/Export/DateAndTime.h"

// =================================================================================================

class TZipFileHeader  
{
public:  
  TZipFileHeader();
  
  //! return the total size of the structure as stored in the central directory  
  size_t Size()const;
  
  //! return the filename size in characters (without NULL);  
  int GetFileNameSize()const {return (int)mArchiveFileName.Size();}
  //! return the comment size in characters (without NULL);    
  int GetCommentSize()const {return (int)mComment.Size();}
  //! return the extra field size in characters (without NULL);    
  int GetExtraFieldSize()const {return (int)mExtraField.Size();}

  TString GetFileName()const;
  void SetFileName(const TString& FileName);

  TString GetComment()const;
  void SetComment(const TString& Comment);

  TDate GetDate()const;
  void SetDate(const TDate& Date);

  TDayTime GetDayTime()const;
  void SetDayTime(const TDayTime& DayTime);
  
  //! return true if the data descriptor is present  
  bool HasDataDescr()const;
  //! return true when the filenames are utf8 converted
  bool HasUtf8FileNameAndComment()const;

  // central file header signature   4 bytes  (0x02014b50)
  TInt8 mSignature[4];
  
  // version made by                 2 bytes
  TInt16 mVersionMadeBy;

  // version needed to extract       2 bytes
  TInt16 mVersionNeeded;

  // general purpose bit flag        2 bytes
  TInt16 mFlags;

  // compression method              2 bytes
  TInt16 mCompressionMethod;

  // last mod file time              2 bytes
  TInt16 mModTime;

  // last mod file date              2 bytes
  TInt16 mModDate;

  // crc-32                          4 bytes
  TUInt32 mCrc32;

  // compressed size                 4 bytes
  TUInt32 mCompressedSize;

  // uncompressed size               4 bytes
  TUInt32 mUncompressedSize;

  // filename length                 2 bytes
  // TInt16 mFileNameSize;

  // extra field length              2 bytes
  // TInt16 mExtraFieldSize;

  // file comment length             2 bytes
  // TInt16 mCommentSize;

  // disk number start               2 bytes
  TInt16 mDiskStart;

  // internal file attributes        2 bytes
  TInt16 mInternalAttr;

  // external file attributes        4 bytes
  TUInt32 mExternalAttr;

  // relative offset of local header 4 bytes
  TUInt32 mOffset;  

  // extra field (variable size)
  TArray<TInt8> mExtraField;

  static const TZipArchive::THeaderSignature sGZSignature;
  static const TZipArchive::THeaderSignature sPkZipSignature;

private:
  friend class TZipArchiveRoot;
  friend class TZipArchive;
  
  bool CheckCrcAndSizes(TFile* pStorage, bool ExpectNull)const;
  void WriteCrcAndSizes(TFile* pStorage);
  
  bool PrepareData(int CompressionLevel, bool ExtraHeader, bool Encrypted);
  
  bool Read(TFile* pStorage);
  size_t Write(TFile* pStorage);

  bool CheckLocal(
    const TZipArchive::THeaderSignature& HeaderSignature,
    TFile*                               pStorage, 
    TInt16&                              LocExtrFieldSize)const;
    
  void WriteLocal(
    const TZipArchive::THeaderSignature& HeaderSignature,
    TFile*                               pStorage);
  
  // change slash to backslash or vice-versa  
  void ChangeSlash();
  
  // filename (variable size)
  TArray<char> mArchiveFileName;  

  // file comment (variable size)
  TArray<char> mComment;
};


#endif // !defined(ZipFileHeader_H)

