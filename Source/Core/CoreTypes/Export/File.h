#pragma once

#ifndef _File_h_
#define _File_h_

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/ByteOrder.h"
#include "CoreTypes/Export/Array.h"
#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Exception.h"
#include "CoreTypes/Export/Memory.h"
#include "CoreTypes/Export/DateAndTime.h"

#include <cstdio> // FILE

// HACK
template <typename T> class TObservableBaseType;
template <typename T> class TObservableList;
template <typename T> class TObservableEnum;

// =================================================================================================

class TFile
{
public:
  TFile(
    const TString&          FileName = TString(), 
    TByteOrder::TByteOrder  ByteOrder = TByteOrder::kSystemByteOrder);

  ~TFile();

  enum TAccessMode
  {
    kInvalidAccessMode = -1,

    kRead,
    
    kWrite,
    kWriteAppend,
    
    kReadWrite,
    kReadWriteAppend,

    kNumberOfAccessModes
  };
  
  //! @return FileName if set
  TString FileName()const;
  //! set a filename !before! opening it
  void SetFileName(const TString& FileName);

  //! @return true if the File exists
  bool Exists()const;
  //! sample as Exists(), but wont assert if the filename case does 
  //! not match. This is needed for Windows as a workaround, where we 
  //! can't guarantee that the filenames match exactly.
  bool ExistsIgnoreCase()const;

  //! current access mode. kInvalidAccessMode when closed.
  TAccessMode AccessMode()const;

  //! @return true if the File is open
  bool IsOpen()const;
  //! @return true if the File is open in read or read/write mode
  bool IsOpenForReading()const;
  //! @return true if the File is open in write or read/write mode
  bool IsOpenForWriting()const;

  //! open a File for read or write
  //! @return success
  bool Open(TAccessMode Mode);
  
  TByteOrder::TByteOrder ByteOrder()const;
  //! Default is the system's byteorder. When a different one is set, bytes 
  //! are automatically swapped after reading or before writing to the file.
  //! See also @class TSetAndRestoreFileByteOrder to temporary change the order.
  void SetByteOrder(TByteOrder::TByteOrder ByteOrder);
  
  //! close the open File (also called in the destructor)
  void Close();

  //! Set a size that the file should use for buffering before it actually
  //! writes to disk. Set a size of 0 to disable buffering
  void SetBufferSize(size_t Size);

  //! try unlink (delete) a File and return success. The File has to be closed!
  bool Unlink();

  //! return the size of this file. File has no (but can be) open
  size_t SizeInBytes()const;
  
  //! @{ File has to be open !
  size_t Position()const;
  //! @return success
  bool SetPosition(size_t NewPos);
  
  //! @return success
  bool Skip(size_t NumOfBytes);
  
  //! flush the system buffers only
  //! @throw TReadableException
  void Flush();
  //! flush the system buffers and write all the bytes directly to the HD (do a fsync)
  //! @throw TReadableException
  void FlushToDisk();
  
  //! return the date, where the file was modified
  //! file must exist
  TDate ModificationDate()const;
  //! return the time, where the file was modified
  //! file must exist
  TDayTime ModificationDayTime()const;
  
  //! return the time, where the file was modified, as used in stat
  int ModificationStatTime()const;
  //@}
  
  //@{ Generic base type reading/writing. 
  //   Will assert on non base types to avoid problems with endian safeness 
  //   @throw TReadableException
  template <typename T> void Read(T& Value);
  template <typename T> void Read(T* pBuffer, size_t Count);
  template <typename T> void Write(const T& Value);
  template <typename T> void Write(const T* pBuffer, size_t Count);
  //@}
  
  //@{ Read/Write our own base types
  //   @throw TReadableException
  
  //! Read/write/unget a single byte. Returns success instead of throwing an error
  bool ReadByte(char& Char);
  bool WriteByte(const char& Char);
  bool RewindByte(const char& Char);

  //! Read/write a byte buffer, not throwing exceptions, but returning if there was 
  //! an error and setting param pBytesToRead or pBytesToWrite to the amount of read/written bytes
  bool ReadBytes(char* pBuffer, size_t* pBytesToRead);
  bool WriteBytes(const char* pBuffer, size_t* pBytesToWrite);

  //! Read a c_string with the specified encoding.
  void Read(
    TString&                  Value, 
    TString::TCStringEncoding Encoding = TString::kUtf8);
   //! Write a c_string with the specified encoding.
  void Write(const TString&   Value, 
    TString::TCStringEncoding Encoding = TString::kUtf8);

  //! Read a c_string text file with the specified iconv encoding.
  //! Note that if a BOM marker is found, the encoding is overridden 
  //! and the BOM encoding is automatically used instead. This way
  //! UTF-16 and 32 is supported as well... 
  //! Specifiying "auto" IconvEncoding encoding will try to guess the 
  //! encoding. This is not really reliable though and only UTF8 will 
  //! be detected right now.
  void ReadText(
    TList<TString>& Text, 
    const TString&  IconvEncoding = "UTF-8");
  void ReadText(
    TObservableList< TObservableBaseType<TString> >&  Text, 
    const TString&                                    IconvEncoding = "UTF-8");
  
  //! Write a c_string with the specified encoding. 
  //! Note that no BOM marker will be written for UTF-8 encodings.
  //! An empty IconvEncoding ("") will use the platforms default encoding 
  void WriteText(
    const TList<TString>& Text, 
    const TString&        IconvEncoding = "UTF-8",
    const TString&        NewLine = "\n");
  void WriteText(
    const TObservableList< TObservableBaseType<TString> >&  Text, 
    const TString&                                          IconvEncoding = "UTF-8",
    const TString&                                          NewLine = "\n");

  template <typename T> void Read(TList<T>& List);
  template <typename T> void Write(const TList<T>& List);

  template <typename T> void Read(TArray<T>& Array);
  template <typename T> void Write(const TArray<T>& Array);

  template <typename T> void Read(TObservableBaseType<T>& Value);
  template <typename T> void Write(const TObservableBaseType<T>& Value);

  template <typename T> void Read(TObservableEnum<T>& Value);
  template <typename T> void Write(const TObservableEnum<T>& Value);

  template <typename T> void Read(TObservableList<T>& List);
  template <typename T> void Write(const TObservableList<T>& List);
  //@}
  
  //@{ Special version for bools and T24 

  // We force the loading of bools to chars here to avoid document incompatibilities 
  // with different compiler builds.
  // @throw TReadableException
  void Read(bool& Value);
  void Read(bool* pBuffer, size_t Count);
  void Write(const bool& Value);
  void Write(const bool* pBuffer, size_t Count);
  
  void Read(T24& Value);
  void Read(T24* pBuffer, size_t Count);
  void Write(const T24& Value);
  void Write(const T24* pBuffer, size_t Count);
  //@}
  
private:
  //! private and not implemented: size of long and thus reading/writing 
  //! long is not portable. Use TInt32 or TInt64 instead...
  void Read(long& Value);
  void Write(const long& Value);
  
  FILE* mFileHandle;
  TString mFileName;
  
  TAccessMode mMode;
  TByteOrder::TByteOrder mByteOrder;
};           

// =================================================================================================

/*!
 * Small helper class to track how much bytes got read or written by a file
!*/

class TFileStamp
{
public:
  //! will stamp immediately
  TFileStamp(TFile* pFile);
  
  //! start a new measure
  void Stamp();
  
  //! return the diff in bytes till the last stamp
  size_t DiffInBytes()const;

private:
  size_t mPositionInBytes;
  TFile* mpFile;  
};

// =================================================================================================

/*!
 * Small helper class to temporary change the byte order of a file
!*/

class TFileSetAndRestoreByteOrder
{
public:
  TFileSetAndRestoreByteOrder(TFile& File, TByteOrder::TByteOrder ByteOrder);
  ~TFileSetAndRestoreByteOrder();

private:
  TFile& mFile;
  TByteOrder::TByteOrder mPreviousByteOrder;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <typename T>
void TFile::Read(T& Value)
{
  MAssert(IsOpenForReading(), "File is not open for reading!");
  
  const size_t ReadCount = ::fread(&Value, sizeof(T), 1, mFileHandle);

  if (ReadCount != 1)
  {
    throw TReadableException(MText("Unexpected end of file!"));
  }

  if (MSystemByteOrder != mByteOrder)
  {
    TByteOrder::Swap<T>(Value);
  }
}

// -------------------------------------------------------------------------------------------------

template <typename T>
void TFile::Read(T* pBuffer, size_t Count)
{
  MAssert(IsOpenForReading(), "File is not open for reading!");
  
  const size_t ReadCount = ::fread(pBuffer, sizeof(T), Count, mFileHandle);

  if (ReadCount != Count)
  {
    throw TReadableException(MText("Unexpected end of file!"));
  }

  if (MSystemByteOrder != mByteOrder)
  {
    TByteOrder::Swap<T>(pBuffer, Count);
  }
}

template <> 
inline void TFile::Read(char* pBuffer, size_t Count)
{
  // avoid the swap overhead in char buffers

  MAssert(IsOpenForReading(), "File is not open for reading!");
  
  const size_t ReadCount = ::fread(pBuffer, 1, Count, mFileHandle);

  if (ReadCount != Count)
  {
    throw TReadableException(MText("Unexpected end of file!"));
  }
}

template <> 
inline void TFile::Read(unsigned char* pBuffer, size_t Count)
{
  // avoid the swap overhead in char buffers

  MAssert(IsOpenForReading(), "File is not open for reading!");
  
  const size_t ReadCount = ::fread(pBuffer, 1, Count, mFileHandle);

  if (ReadCount != Count)
  {
    throw TReadableException(MText("Unexpected end of file!"));
  }
}

// -------------------------------------------------------------------------------------------------

template <typename T>
void TFile::Write(const T& Value)
{
  MAssert(IsOpenForWriting(), "File is not open for writing!");
  
  T TempValue = Value;

  if (MSystemByteOrder != mByteOrder)
  {
    TByteOrder::Swap<T>(TempValue);
  }

  const size_t WriteCount = ::fwrite(&TempValue, sizeof(T), 1, mFileHandle);

  if (WriteCount != 1)
  {
    throw TReadableException(MText(
      "File I/O error: Please make sure that there is enough space "
      "available on the device!"));
  }
}

// -------------------------------------------------------------------------------------------------

template <typename T>
void TFile::Write(const T* pBuffer, size_t Count)
{
  MAssert(IsOpenForWriting(), "File is not open for writing!");

  TArray<T> TempArray((int)Count);
  TMemory::Copy(TempArray.FirstWrite(), pBuffer, Count*sizeof(T));

  if (MSystemByteOrder != mByteOrder)
  {
    TByteOrder::Swap<T>(TempArray.FirstWrite(), Count);
  }

  const size_t WriteCount = ::fwrite((void*)TempArray.FirstRead(), 
    sizeof(T), Count, mFileHandle);

  if (WriteCount != Count)
  {
    throw TReadableException(MText(
      "File I/O error: Please make sure that there is enough space "
      "available on the device!"));
  }
}

template <> 
inline void TFile::Write(const char* pBuffer, size_t Count)
{
  // avoid the swap overhead in char buffers

  MAssert(IsOpenForWriting(), "File is not open for writing!");

  const size_t WriteCount = ::fwrite(pBuffer, 1, Count, mFileHandle);

  if (WriteCount != Count)
  {
    throw TReadableException(MText(
      "File I/O error: Please make sure that there is enough space "
      "available on the device!"));
  }
}

template <> 
inline void TFile::Write(const unsigned char* pBuffer, size_t Count)
{
  // avoid the swap overhead in char buffers

  MAssert(IsOpenForWriting(), "File is not open for writing!");

  const size_t WriteCount = ::fwrite(pBuffer, 1, Count, mFileHandle);

  if (WriteCount != Count)
  {
    throw TReadableException(MText(
      "File I/O error: Please make sure that there is enough space "
      "available on the device!"));
  }
}

// -------------------------------------------------------------------------------------------------

template <typename T>
void TFile::Read(TList<T>& List)
{
  TInt32 Size;
  Read(Size);
  List.Empty();
  List.PreallocateSpace((int)Size);

  for (int i = 0; i < (int)Size; ++i)
  {
    // read element by element in case of nested lists
    T Value;
    Read(Value);
    List.Append(Value);
  }
}

// -------------------------------------------------------------------------------------------------

template <typename T>
void TFile::Write(const TList<T>& List)
{
  TInt32 Size = (TInt32)List.Size();
  Write(Size);

  for (int i = 0; i < (int)Size; ++i)
  {
    // write element by element in case of nested lists
    Write(List[i]);
  }
}

// -------------------------------------------------------------------------------------------------

template <typename T>
void TFile::Read(TArray<T>& Array)
{
  TInt32 Size;
  Read(Size);
  Array.SetSize((int)Size);

  for (int i = 0; i < (int)Size; ++i)
  {
    // read element by element in case of nested arrays
    T Value;
    Read(Value);
    Array[i] = Value;
  }
}

template <> 
inline void TFile::Read(TArray<char>& Array)
{
  TInt32 Size;
  Read(Size);
  Array.SetSize((int)Size);

  Read(Array.FirstWrite(), Size);
}

template <> 
inline void TFile::Read(TArray<unsigned char>& Array)
{
  TInt32 Size;
  Read(Size);
  Array.SetSize((int)Size);

  Read(Array.FirstWrite(), Size);
}

// -------------------------------------------------------------------------------------------------

template <typename T>
void TFile::Write(const TArray<T>& Array)
{
  TInt32 Size = (TInt32)Array.Size();
  Write(Size);

  for (int i = 0; i < (int)Size; ++i)
  {
    // write element by element in case of nested arrays
    Write(Array[i]);
  }
}

template <> 
inline void TFile::Write(const TArray<char>& Array)
{
  Write((TInt32)Array.Size());
  Write(Array.FirstRead(), Array.Size());
}

template <> 
inline void TFile::Write(const TArray<unsigned char>& Array)
{
  Write((TInt32)Array.Size());
  Write(Array.FirstRead(), Array.Size());
}


#endif // _File_h_

