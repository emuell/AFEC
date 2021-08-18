#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/Array.h"
#include "CoreTypes/Export/Exception.h"

#include "FeatureExtraction/Export/SampleDescriptors.h"
#include "FeatureExtraction/Export/SqliteSampleDescriptorPool.h"

#include "../../../Msgpack/Export/Msgpack.h"

typedef TSampleDescriptors::TDescriptor TSampleDescriptor;

// =================================================================================================

// default table names
#define MAssetsTableName "assets"
#define MClassesTableName "classes"

// local log name prefix
#define MLogPrefix "SqliteExtractor"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

//! Operator function for MATCH in sqlite

static bool SFileNameMatchOperator(
  void*       pContext,
  const char* pFirst,
  const char* pSecond)
{
  // be case sensitive on Linux
  #if defined(MLinux)
    return gStringsEqual(
      TString(pFirst, TString::kUtf8), TString(pSecond, TString::kUtf8));
  #else
    return gStringsEqualIgnoreCase(
      TString(pFirst, TString::kUtf8), TString(pSecond, TString::kUtf8));
  #endif
}

// -------------------------------------------------------------------------------------------------

//! Javascript "join" alike helper to reduce a list of strings to a single one

static TString SJoinStrings(
  const TList<TString>& Strings,
  const TString&        Separator = ",")
{
  TString Ret;
  for (int i = 0; i < Strings.Size(); ++i)
  {
    Ret += Strings[i];
    if (i < Strings.Size() - 1)
    {
      Ret += Separator;
    }
  }
  return Ret;
}

// -------------------------------------------------------------------------------------------------

//! Raw character search to avoid TString's FindChar overhead when searching 
//! repedeately on the same string

static int SFindNextChar(
  const TUnicodeChar* pBegin,
  const TUnicodeChar* pEnd,
  TUnicodeChar        Char,
  int                 StartOffset)
{
  const TUnicodeChar* pPos = pBegin + StartOffset;

  while (pPos < pEnd && *pPos != Char)
  {
    ++pPos;
  }

  if (pPos < pEnd)
  {
    return (int)(intptr_t)(pPos - pBegin);
  }
  else
  {
    return -1;
  }
}

// -------------------------------------------------------------------------------------------------

static TString SToStringFormat(TSampleDescriptor::TExportFlags ExportFlags)
{
  return (ExportFlags & TSampleDescriptor::kAllowFloatingPointPrecisionStorage) ?
    TDefaultStringFormats<float>::SToString() : 
    TDefaultStringFormats<double>::SToString();
}

// -------------------------------------------------------------------------------------------------

static int SCharsPerNumberToPreallocate(TSampleDescriptor::TExportFlags ExportFlags)
{
  return (ExportFlags & TSampleDescriptor::kAllowFloatingPointPrecisionStorage) ?
    16 : 32;
}

// -------------------------------------------------------------------------------------------------

//! Unserialize a descriptor value, throwing a descriptive error when needed

template <typename T>
static T SUnserializeValue(const TString& ColumnName, const TString& Data)
{
  T Value;
  if (!StringToValue(Value, Data))
  {
    throw TReadableException("Invalid descriptor data in column '" + ColumnName +
      "': '" + Data + "' can not convert value from string data.");
  }

  return Value;
}

template <>
TString SUnserializeValue(const TString& ColumnName, const TString& Data)
{
  TString Value = TString(Data).Strip();

  // remove extra quotes, if any...
  if (Value.StartsWithChar('\"') && Value.EndsWithChar('\"'))
  {
    Value.RemoveFirst("\"").RemoveLast("\"");
  }
  else if (Value.StartsWithChar('\'') && Value.EndsWithChar('\''))
  {
    Value.RemoveFirst("\'").RemoveLast("\'");
  }

  return Value;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

//! Unserialize VR or VVR descriptor values from a JSON text

static void SFromJSON(
  const TString&        ColumnName,
  TList<double>&        Vector,
  const TUnicodeChar*   pDataBegin,
  const TUnicodeChar*   pDataEnd)
{
  const int DataSize = (int)(intptr_t)(pDataEnd - pDataBegin);

  Vector.Empty();

  if (pDataEnd > pDataBegin)
  {
    int Pos = SFindNextChar(pDataBegin, pDataEnd, '[', 0) + 1; // skip initial [
    int NextPos = -1;

    while ((NextPos = SFindNextChar(pDataBegin, pDataEnd, ',', Pos)) != -1)
    {
      const TString ValueString = TString(pDataBegin + Pos, pDataBegin + NextPos);
      Vector.Append(SUnserializeValue<double>(ColumnName, ValueString));
      Pos = NextPos + 1;
    }

    if (Pos < DataSize - 1) // skip last ]
    {
      const TString ValueString = TString(pDataBegin + Pos, pDataBegin + DataSize - 1);
      Vector.Append(SUnserializeValue<double>(ColumnName, ValueString));
    }
  }
}

static void SFromJSON(
  const TString&        ColumnName,
  TList<TString>&       Vector,
  const TUnicodeChar*   pDataBegin,
  const TUnicodeChar*   pDataEnd)
{
  const int DataSize = (int)(intptr_t)(pDataEnd - pDataBegin);

  Vector.Empty();

  if (pDataEnd > pDataBegin)
  {
    int Pos = SFindNextChar(pDataBegin, pDataEnd, '[', 0) + 1; // skip initial [
    int NextPos = -1;

    while ((NextPos = SFindNextChar(pDataBegin, pDataEnd, ',', Pos)) != -1)
    {
      const TString ValueString = TString(pDataBegin + Pos, pDataBegin + NextPos);
      Vector.Append(SUnserializeValue<TString>(ColumnName, ValueString));
      Pos = NextPos + 1;
    }

    if (Pos < DataSize - 1) // skip last ]
    {
      const TString ValueString = TString(pDataBegin + Pos, pDataBegin + DataSize - 1);
      Vector.Append(SUnserializeValue<TString>(ColumnName, ValueString));
    }
  }
}

static void SFromJSON(
  const TString&          ColumnName,
  TList< TList<double> >& VectorOfVectors,
  const TUnicodeChar*     pDataBegin,
  const TUnicodeChar*     pDataEnd)
{
  const int DataSize = (int)(intptr_t)(pDataEnd - pDataBegin);

  if (DataSize > 0)
  {
    int Pos = SFindNextChar(pDataBegin, pDataEnd, '[', 0) + 1; // skip initial [
    int NextPos = -1;
    int ItemCount = 1;

    while ((NextPos = SFindNextChar(pDataBegin, pDataEnd, ']', Pos)) != -1)
    {
      TList<double> Vector;
      SFromJSON(ColumnName, Vector, pDataBegin + Pos, pDataBegin + NextPos + 1);
      VectorOfVectors.Append(Vector);

      Pos = NextPos + 2; // skip ']' and ','
      ++ItemCount;
    }

    MAssert(Pos == DataSize, "should have skipped/consumed everything here");
  }
}

template <size_t sSize>
static void SFromJSON(
  const TString&                ColumnName,
  TStaticArray<double, sSize>&  Array,
  const TUnicodeChar*           pDataBegin,
  const TUnicodeChar*           pDataEnd)
{
  const int DataSize = (int)(intptr_t)(pDataEnd - pDataBegin);

  Array.Init(0.0);

  if (pDataEnd > pDataBegin)
  {
    int Pos = SFindNextChar(pDataBegin, pDataEnd, '[', 0) + 1; // skip initial [
    int NextPos = -1;
    int ItemCount = -1;

    while ((NextPos = SFindNextChar(pDataBegin, pDataEnd, ',', Pos)) != -1)
    {
      if (++ItemCount >= (int)sSize)
      {
        throw TReadableException("Invalid descriptor data in column '" +
          ColumnName + "': too many array entries in stream.");
      }

      const TString ValueString = TString(pDataBegin + Pos, pDataBegin + NextPos);
      Array[ItemCount] = SUnserializeValue<double>(ColumnName, ValueString);
      Pos = NextPos + 1;
    }

    if (Pos < DataSize - 1) // skip last ]
    {
      if (++ItemCount >= (int)sSize)
      {
        throw TReadableException("Invalid descriptor data in column '" +
          ColumnName + "': too many array entries in stream.");
      }

      const TString ValueString = TString(pDataBegin + Pos, pDataBegin + DataSize - 1);
      Array[ItemCount] = SUnserializeValue<double>(ColumnName, ValueString);
    }
  }
}

template <size_t sSize>
static void SFromJSON(
  const TString&                        ColumnName,
  TList< TStaticArray<double, sSize> >& VectorOfArrays,
  const TUnicodeChar*                   pDataBegin,
  const TUnicodeChar*                   pDataEnd)
{
  const int DataSize = (int)(intptr_t)(pDataEnd - pDataBegin);

  if (DataSize > 0)
  {
    int Pos = SFindNextChar(pDataBegin, pDataEnd, '[', 0) + 1; // skip initial [
    int NextPos = -1;
    int ItemCount = 1;

    while ((NextPos = SFindNextChar(pDataBegin, pDataEnd, ']', Pos)) != -1)
    {
      TStaticArray<double, sSize> Array;
      SFromJSON(ColumnName, Array, pDataBegin + Pos, pDataBegin + NextPos + 1);
      VectorOfArrays.Append(Array);

      Pos = NextPos + 2; // skip ']' and ','
      ++ItemCount;
    }

    MAssert(Pos == DataSize, "should have skipped/consumed everything here");
  }
}

// -------------------------------------------------------------------------------------------------

//! Serialize VR or VVR descriptor values to JSON

static TString SToJSON(
  const TList<double>&            Values, 
  TSampleDescriptor::TExportFlags ExportFlags)
{
  const TString ToStringFormat = SToStringFormat(ExportFlags);
  const int CharsPerNumberToPreallocate = SCharsPerNumberToPreallocate(ExportFlags);

  TString Ret;
  Ret.PreallocateSpace(2 + Values.Size() * CharsPerNumberToPreallocate);

  Ret += "[";
  if (Values.Size() > 0)
  {
    Ret += ToString(Values[0], ToStringFormat);
    for (int i = 1; i < Values.Size(); ++i)
    {
      Ret += "," + ToString(Values[i], ToStringFormat);
    }
  }
  Ret += "]";

  return Ret;
}

static TString SToJSON(
  const TList<TString>&           Values,
  TSampleDescriptor::TExportFlags ExportFlags)
{
  TString Ret;
  Ret.PreallocateSpace(2 + Values.Size() * 0x20);

  Ret += "[";
  if (Values.Size() > 0)
  {
    Ret += "\"" + Values[0] + "\"";
    for (int i = 1; i < Values.Size(); ++i)
    {
      Ret += ",\"" + Values[i] + "\"";
    }
  }
  Ret += "]";
  return Ret;
}

template <size_t sSize>
static TString SToJSON(
  const TStaticArray<double, sSize>&  Values,
  TSampleDescriptor::TExportFlags     ExportFlags)
{
  const TString ToStringFormat = SToStringFormat(ExportFlags);
  const int CharsPerNumberToPreallocate = SCharsPerNumberToPreallocate(ExportFlags);

  TString Ret;
  Ret.PreallocateSpace(2 + sSize * CharsPerNumberToPreallocate);

  Ret += "[";
  Ret += ToString(Values[0], ToStringFormat);
  for (int i = 1; i < (int)sSize; ++i)
  {
    Ret += "," + ToString(Values[i], ToStringFormat);
  }
  Ret += "]";
  return Ret;
}

template <size_t sSize>
static TString SToJSON(
  const TList< TStaticArray<double, sSize> >& Values,
  TSampleDescriptor::TExportFlags             ExportFlags)
{
  const TString ToStringFormat = SToStringFormat(ExportFlags);
  const int CharsPerNumberToPreallocate = SCharsPerNumberToPreallocate(ExportFlags);

  TString Ret;

  int NeededSize = 2;
  for (int i = 0; i < Values.Size(); ++i)
  {
    NeededSize += 2 + Values[i].Size() * CharsPerNumberToPreallocate;
  }
  Ret.PreallocateSpace(NeededSize);

  Ret += "[";
  if (Values.Size() > 0)
  {
    for (int i = 0; i < Values.Size(); ++i)
    {
      if (i > 0)
      {
        Ret += ",";
      }

      Ret += "[";
      Ret += ToString(Values[i][0], ToStringFormat);
      for (int j = 1; j < (int)sSize; ++j)
      {
        Ret += "," + ToString(Values[i][j], ToStringFormat);
      }
      Ret += "]";
    }
  }
  Ret += "]";
  return Ret;
}

static TString SToJSON(
  const TList< TList<double> >&   Values,
  TSampleDescriptor::TExportFlags ExportFlags)
{
  const TString ToStringFormat = SToStringFormat(ExportFlags);
  const int CharsPerNumberToPreallocate = SCharsPerNumberToPreallocate(ExportFlags);

  TString Ret;
  
  int NeededSize = 2;
  for (int i = 0; i < Values.Size(); ++i)
  {
    NeededSize += 2 + Values[i].Size() * CharsPerNumberToPreallocate;
  }
  Ret.PreallocateSpace(NeededSize);
  
  Ret += "[";
  if (Values.Size() > 0)
  {
    for (int i = 0; i < Values.Size(); ++i)
    {
      if (i > 0)
      {
        Ret += ",";
      }

      Ret += "[";
      Ret += ToString(Values[i][0], ToStringFormat);
      for (int j = 1; j < Values[i].Size(); ++j)
      {
        Ret += "," + ToString(Values[i][j], ToStringFormat);
      }
      Ret += "]";
    }
  }
  Ret += "]";
  return Ret;
}

// -------------------------------------------------------------------------------------------------

//! Unserialize VR or VVR descriptor values from raw msgpack data

static void SFromMsgpack(
  const TString&        ColumnName,
  TList<double>&        Vector,
  const void*           pData,
  int                   DataSize)
{
  try
  {
    msgpack::object_handle ObjectHandle =
      msgpack::unpack((const char*)pData, (size_t)DataSize);

    std::vector<double> StdVector;
    ObjectHandle.get().convert(StdVector);

    Vector.ClearEntries();
    for (int i = 0; i < (int)StdVector.size(); ++i)
    {
      Vector.Append(StdVector[i]);
    }
  }
  catch (const std::exception& exception)
  {
    throw TReadableException(
      MText("Invalid binary descriptor data in column '%s': %s",
        ColumnName, TString(exception.what())));
  }
}

static void SFromMsgpack(
  const TString&          ColumnName,
  TList< TList<double> >& VectorOfVectors,
  const void*           pData,
  int                   DataSize)
{ 
  try
  {
    msgpack::object_handle ObjectHandle =
      msgpack::unpack((const char*)pData, (size_t)DataSize);

    std::vector< std::vector<double> > StdVectorOfVectors;
    ObjectHandle.get().convert(StdVectorOfVectors);

    VectorOfVectors.ClearEntries();
    for (int i = 0; i < (int)StdVectorOfVectors.size(); ++i)
    {
      TList<double> Vector;
      for (int j = 0; j < (int)StdVectorOfVectors[i].size(); ++j)
      {
        Vector.Append(StdVectorOfVectors[i][j]);
      }
      VectorOfVectors.Append(Vector);
    }
  }
  catch (const std::exception& exception)
  {
    throw TReadableException(
      MText("Invalid binary descriptor data in column '%s': %s",
        ColumnName, TString(exception.what())));
  }
}

template <size_t sSize>
static void SFromMsgpack(
  const TString&                ColumnName,
  TStaticArray<double, sSize>&  Array,
  const void*                   pData,
  int                           DataSize)
{
  try
  {
    msgpack::object_handle ObjectHandle =
      msgpack::unpack((const char*)pData, (size_t)DataSize);

    std::vector<double> StdVector;
    ObjectHandle.get().convert(StdVector);

    if (StdVector.size() != sSize)
    {
      throw std::runtime_error("Static array size mismatch");
    }

    for (int i = 0; i < (int)StdVector.size(); ++i)
    {
      Array[i] = StdVector[i];
    }
  }
  catch (const std::exception& exception)
  {
    throw TReadableException(
      MText("Invalid binary descriptor data in column '%s': %s",
        ColumnName, TString(exception.what())));
  }
}

template <size_t sSize>
static void SFromMsgpack(
  const TString&                        ColumnName,
  TList< TStaticArray<double, sSize> >& VectorOfArrays,
  const void*                           pData,
  int                                   DataSize)
{
  try
  {
    msgpack::object_handle ObjectHandle =
      msgpack::unpack((const char*)pData, (size_t)DataSize);

    std::vector< std::vector<double> > StdVectorOfVectors;
    ObjectHandle.get().convert(StdVectorOfVectors);

    VectorOfArrays.ClearEntries();
    for (int i = 0; i < (int)StdVectorOfVectors.size(); ++i)
    {
      if (StdVectorOfVectors[i].size() != sSize)
      {
        throw std::runtime_error("Static nested array size mismatch");
      }

      TStaticArray<double, sSize> Array;
      for (int j = 0; j < (int)StdVectorOfVectors[i].size(); ++j)
      {
        Array[j] = StdVectorOfVectors[i][j];
      }
      VectorOfArrays.Append(Array);
    }
  }
  catch (const std::exception& exception)
  {
    throw TReadableException(
      MText("Invalid binary descriptor data in column '%s': %s",
        ColumnName, TString(exception.what())));
  }
}

// -------------------------------------------------------------------------------------------------

//! Serialize VR or VVR descriptor values to msgpack format

msgpack::sbuffer SToMsgpack(
  const TList<double>&            Value,
  TSampleDescriptor::TExportFlags ExportFlags)
{
  msgpack::sbuffer Buffer;

  msgpack::packer<msgpack::sbuffer> Packer(&Buffer);
  Packer.pack_array(Value.Size());
  if (ExportFlags & TSampleDescriptor::kAllowFloatingPointPrecisionStorage)
  {
    for (int i = 0; i < Value.Size(); ++i)
    {
      Packer.pack((float)Value[i]);
    }
  }
  else 
  {
    for (int i = 0; i < Value.Size(); ++i)
    {
      Packer.pack(Value[i]);
    }
  }

  return Buffer;
}

template <size_t sSize>
msgpack::sbuffer SToMsgpack(
  const TStaticArray<double, sSize>&  Value,
  TSampleDescriptor::TExportFlags     ExportFlags)
{
  msgpack::sbuffer Buffer;

  msgpack::packer<msgpack::sbuffer> Packer(&Buffer);
  Packer.pack_array(Value.Size());
  if (ExportFlags & TSampleDescriptor::kAllowFloatingPointPrecisionStorage)
  {
    for (int i = 0; i < Value.Size(); ++i)
    {
      Packer.pack((float)Value[i]);
    }
  }
  else 
  {
    for (int i = 0; i < Value.Size(); ++i)
    {
      Packer.pack(Value[i]);
    }
  }

  return Buffer;
}

msgpack::sbuffer SToMsgpack(
  const TList<TList<double>>&     Value,
  TSampleDescriptor::TExportFlags ExportFlags)
{
  msgpack::sbuffer Buffer;

  msgpack::packer<msgpack::sbuffer> Packer(&Buffer);
  Packer.pack_array(Value.Size());
  for (int i = 0; i < Value.Size(); ++i)
  {
    Packer.pack_array(Value[i].Size());
    if (ExportFlags & TSampleDescriptor::kAllowFloatingPointPrecisionStorage)
    {
      for (int j = 0; j < Value[i].Size(); ++j)
      {
        Packer.pack((float)Value[i][j]);
      }
    }
    else 
    {
      for (int j = 0; j < Value[i].Size(); ++j)
      {
        Packer.pack(Value[i][j]);
      }
    }
  }

  return Buffer;
}

template <size_t sSize>
msgpack::sbuffer SToMsgpack(
  const TList<TStaticArray<double, sSize>>& Value,
  TSampleDescriptor::TExportFlags           ExportFlags)
{
  msgpack::sbuffer Buffer;

  msgpack::packer<msgpack::sbuffer> Packer(&Buffer);
  Packer.pack_array(Value.Size());
  for (int i = 0; i < Value.Size(); ++i)
  {
    Packer.pack_array(Value[i].Size());
    if (ExportFlags & TSampleDescriptor::kAllowFloatingPointPrecisionStorage)
    {
      for (int j = 0; j < Value[i].Size(); ++j)
      {
        Packer.pack((float)Value[i][j]);
      }
    }
    else
    {
      for (int j = 0; j < Value[i].Size(); ++j)
      {
        Packer.pack(Value[i][j]);
      }
    }
  }

  return Buffer;
}

// =================================================================================================

/*!
 * Visitor for the TSampleDescriptors::TDescriptor::TValue variant:
 * Get a type alike postfix for the variants actual type.
 *
 * S = string (UTF8 encoded)
 * R = number (integer or real number)
 * VR = vector of real numbers
 * VS = vector of strings (UTF8 encoded)
 * VVR = vector of vector of real numbers
!*/

class TDescriptorValueNamePostfix : public boost::static_visitor<TString>
{
public:
  TDescriptorValueNamePostfix(const TSampleDescriptor& Descriptor)
    : mDescriptor(Descriptor)
  { }

  TString operator()(const int*) const
  {
    return "R";
  }
  TString operator()(const double*) const
  {
    return "R";
  }

  TString operator()(const TString*) const
  {
    return "S";
  }

  TString operator()(const TList<double>*) const
  {
    return "VR";
  }

  template <size_t sSize>
  TString operator()(const TStaticArray<double, sSize>*) const
  {
    return "VR";
  }

  TString operator()(const TList<TString>*) const
  {
    return "VS";
  }

  TString operator()(const TList<TList<double>>*) const
  {
    return "VVR";
  }

  template <size_t sSize>
  TString operator()(const TList<TStaticArray<double, sSize>>*) const
  {
    return "VVR";
  }

private:
  const TSampleDescriptor& mDescriptor;
};

// =================================================================================================

/*!
 * Visitor for the TSampleDescriptors::TDescriptor::TValue variant:
 * Get a sqlite column type for the variants actual type.
 * 
 * BLOBS are always encoded with msgpack, TEXT as JSON.
 * INTEGERS or REAL are raw integers or doubles.
!*/

class TDescriptorValueSqliteType : public boost::static_visitor<TString>
{
public:
  TDescriptorValueSqliteType(const TSampleDescriptor& Descriptor)
    : mDescriptor(Descriptor)
  { }

  TString operator()(const int*) const
  {
    return "INTEGER"; 
  }

  TString operator()(const double*) const
  {
    return "REAL";
  }

  TString operator()(const TString*) const
  {
    return "TEXT";
  }

  TString operator()(const TList<TString>*) const
  {
    // always store string lists in text formats
    return "TEXT";
  }

  TString operator()(const TList<double>*) const
  {
    return (mDescriptor.mExportFlags & TSampleDescriptor::kAllowBinaryStorage) ? 
      "BLOB" : "TEXT";
  }

  template <size_t sSize>
  TString operator()(const TStaticArray<double, sSize>*) const
  {
    return (mDescriptor.mExportFlags & TSampleDescriptor::kAllowBinaryStorage) ?
      "BLOB" : "TEXT";
  }

  TString operator()(const TList< TList<double> >*) const
  {
    return (mDescriptor.mExportFlags & TSampleDescriptor::kAllowBinaryStorage) ?
      "BLOB" : "TEXT";
  }

  template <size_t sSize>
  TString operator()(const TList<TStaticArray<double, sSize>>*) const
  {
    return (mDescriptor.mExportFlags & TSampleDescriptor::kAllowBinaryStorage) ?
      "BLOB" : "TEXT";
  }

private:
  const TSampleDescriptor& mDescriptor;
};

// =================================================================================================

/*!
 * Visitor for the TSampleDescriptors::TDescriptor::TValue variant:
 * Bind the variant's value to a column in the given statement.
 * 
 * See TDescriptorValueSqliteType for the expected column types.
!*/

class TBindDescriptorValueToStatement : public boost::static_visitor<>
{
public:
  TBindDescriptorValueToStatement(
    const TSampleDescriptor&  Descriptor,
    TDatabase::TStatement&    Statement,
    int                       ParameterIndex) 
    : mDescriptor(Descriptor),
      mStatement(Statement),
      mParameterIndex(ParameterIndex)
  { }
  
  void operator()(const int* pValue) const
  {
    mStatement.BindInt(mParameterIndex, *pValue); 
  }

  void operator()(const double* pValue) const
  {
    mStatement.BindDouble(mParameterIndex, *pValue);
  }

  void operator()(const TString* pValue) const
  {
    mStatement.BindText(mParameterIndex, *pValue);
  }

  void operator()(const TList<TString>* pValue) const
  {
    mStatement.BindText(mParameterIndex, 
      SToJSON(*pValue, mDescriptor.mExportFlags));
  }

  void operator()(const TList<double>* pValue) const
  {
    if (mDescriptor.mExportFlags & TSampleDescriptor::kAllowBinaryStorage)
    {
      const msgpack::sbuffer Buffer = 
        SToMsgpack(*pValue, mDescriptor.mExportFlags);
      
      mStatement.BindBlob(mParameterIndex, Buffer.data(), (int)Buffer.size());
    }
    else
    {
      mStatement.BindText(mParameterIndex, 
        SToJSON(*pValue, mDescriptor.mExportFlags));
    }
  }

  template <size_t sSize>
  void operator()(const TStaticArray<double, sSize>* pValue) const
  {
    if (mDescriptor.mExportFlags & TSampleDescriptor::kAllowBinaryStorage)
    {
      const msgpack::sbuffer Buffer = 
        SToMsgpack(*pValue, mDescriptor.mExportFlags);

      mStatement.BindBlob(mParameterIndex, Buffer.data(), (int)Buffer.size());
    }
    else
    {
      mStatement.BindText(mParameterIndex, 
        SToJSON(*pValue, mDescriptor.mExportFlags));
    }
  }

  void operator()(const TList< TList<double> >* pValue) const
  {
    if (mDescriptor.mExportFlags & TSampleDescriptor::kAllowBinaryStorage)
    {
      const msgpack::sbuffer Buffer = 
        SToMsgpack(*pValue, mDescriptor.mExportFlags);

      mStatement.BindBlob(mParameterIndex, Buffer.data(), (int)Buffer.size());
    }
    else
    {
      mStatement.BindText(mParameterIndex, 
        SToJSON(*pValue, mDescriptor.mExportFlags));
    }
  }

  template <size_t sSize>
  void operator()(const TList< TStaticArray<double, sSize> >* pValue) const
  {
    if (mDescriptor.mExportFlags & TSampleDescriptor::kAllowBinaryStorage)
    {
      const msgpack::sbuffer Buffer = 
        SToMsgpack(*pValue, mDescriptor.mExportFlags);
      
      mStatement.BindBlob(mParameterIndex, Buffer.data(), (int)Buffer.size());
    }
    else
    {
      mStatement.BindText(mParameterIndex, 
        SToJSON(*pValue, mDescriptor.mExportFlags));
    }
  }

private:
  const TSampleDescriptor& mDescriptor;
  TDatabase::TStatement& mStatement;
  const int mParameterIndex;
};

// =================================================================================================

/*!
 * Visitor for the TSampleDescriptors::TDescriptor::TValue variant:
 * Fetch the variant's value from a column in the given statement.
!*/

class TUnserializeDescriptorValueFromStatement : public boost::static_visitor<>
{
public:
  TUnserializeDescriptorValueFromStatement(
    const TSampleDescriptor&  Descriptor,
    TDatabase::TStatement&    Statement,
    int                       ColumnIndex,
    const TString&            ColumnName)
    : mDescriptor(Descriptor),
      mStatement(Statement),
      mColumnIndex(ColumnIndex)
  { }

  void operator()(int* pValue) const
  {
    *pValue = mStatement.ColumnInt(mColumnIndex);
  }

  void operator()(double* pValue) const
  {
    *pValue = mStatement.ColumnDouble(mColumnIndex);
  }

  void operator()(TString* pValue) const
  {
    *pValue = mStatement.ColumnText(mColumnIndex);
  }

  void operator()(TList<TString>* pValue) const
  {
    const TString ColumnText = mStatement.ColumnText(mColumnIndex);
    
    SFromJSON(mColumnName, *pValue,
      ColumnText.Chars(), ColumnText.Chars() + ColumnText.Size());
  }

  void operator()(TList<double>* pValue) const
  {
    if (mStatement.ColumnTypeString(mColumnIndex) == "BLOB") 
    {
      int RawColumnContentSize = 0;
      const void* pRawColumnContent = mStatement.ColumnBlob(
        mColumnIndex, RawColumnContentSize);

      SFromMsgpack(mColumnName, *pValue, 
        pRawColumnContent, RawColumnContentSize);
    }
    else
    {
      MAssert(mStatement.ColumnTypeString(mColumnIndex) == "TEXT", "Unexpected type");
      const TString ColumnText = mStatement.ColumnText(mColumnIndex);

      SFromJSON(mColumnName, *pValue,
        ColumnText.Chars(), ColumnText.Chars() + ColumnText.Size());
    }
  }

  template <size_t sSize>
  void operator()(TStaticArray<double, sSize>* pValue) const
  {
    if (mStatement.ColumnTypeString(mColumnIndex) == "BLOB")
    {
      int RawColumnContentSize = 0;
      const void* pRawColumnContent = mStatement.ColumnBlob(
        mColumnIndex, RawColumnContentSize);

      SFromMsgpack(mColumnName, *pValue,
        pRawColumnContent, RawColumnContentSize);
    }
    else
    {
      MAssert(mStatement.ColumnTypeString(mColumnIndex) == "TEXT", "Unexpected type");
      const TString ColumnText = mStatement.ColumnText(mColumnIndex);

      SFromJSON(mColumnName, *pValue,
        ColumnText.Chars(), ColumnText.Chars() + ColumnText.Size());
    }
  }

  void operator()(TList<TList<double>>* pValue) const
  {
    if (mStatement.ColumnTypeString(mColumnIndex) == "BLOB")
    {
      int RawColumnContentSize = 0;
      const void* pRawColumnContent = mStatement.ColumnBlob(
        mColumnIndex, RawColumnContentSize);

      SFromMsgpack(mColumnName, *pValue,
        pRawColumnContent, RawColumnContentSize);
    }
    else
    {
      MAssert(mStatement.ColumnTypeString(mColumnIndex) == "TEXT", "Unexpected type");
      const TString ColumnText = mStatement.ColumnText(mColumnIndex);

      SFromJSON(mColumnName, *pValue,
        ColumnText.Chars(), ColumnText.Chars() + ColumnText.Size());
    }
  }

  template <size_t sSize>
  void operator()(TList< TStaticArray<double, sSize> >* pValue) const
  {
    if (mStatement.ColumnTypeString(mColumnIndex) == "BLOB")
    {
      int RawColumnContentSize = 0;
      const void* pRawColumnContent = mStatement.ColumnBlob(
        mColumnIndex, RawColumnContentSize);

      SFromMsgpack(mColumnName, *pValue,
        pRawColumnContent, RawColumnContentSize);
    }
    else
    {
      MAssert(mStatement.ColumnTypeString(mColumnIndex) == "TEXT", "Unexpected type");
      const TString ColumnText = mStatement.ColumnText(mColumnIndex);

      SFromJSON(mColumnName, *pValue,
        ColumnText.Chars(), ColumnText.Chars() + ColumnText.Size());
    }
  }

private:
  const TSampleDescriptor& mDescriptor;
  TDatabase::TStatement& mStatement;
  const int mColumnIndex;
  const TString mColumnName;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TSqliteSampleDescriptorPool::TSqliteSampleDescriptorPool(
  TSampleDescriptors::TDescriptorSet DescriptorSet)
  : TSampleDescriptorPool(DescriptorSet)
{ }

// -------------------------------------------------------------------------------------------------

TSqliteSampleDescriptorPool::~TSqliteSampleDescriptorPool()
{
  ShutdownDatabase();
}

// -------------------------------------------------------------------------------------------------

bool TSqliteSampleDescriptorPool::Open(
  const TString& DatabaseName,
  bool           ReadOnly)
{
  if (!mDatabase.Open(DatabaseName))
  {
    TLog::SLog()->AddLine(MLogPrefix, "Failed to open database");
    return false;
  }
  else
  {
    // set match operator
    mDatabase.SetMatchOperator(this, SFileNameMatchOperator);

    // create or update tables
    if (!ReadOnly)
    {
      InitializeDatabase();
    }

    return true;
  }
}

// -------------------------------------------------------------------------------------------------

TDirectory TSqliteSampleDescriptorPool::BasePath()const
{
  return mBasePath;
}

// -------------------------------------------------------------------------------------------------

void TSqliteSampleDescriptorPool::SetBasePath(const TDirectory& BasePath)
{
  mBasePath = BasePath;
}

// -------------------------------------------------------------------------------------------------

bool TSqliteSampleDescriptorPool::DatabaseNeedsUpgrade()const
{
  const int Version = mDatabase.ExecuteScalarInt("PRAGMA user_version");
  return (kCurrentVersion > Version);
}

// -------------------------------------------------------------------------------------------------

TString TSqliteSampleDescriptorPool::RelativeFilenamePath(
  const TString &FileName) const
{
  TString Ret = FileName;

  if (!mBasePath.IsEmpty())
  {
    TString NormalizedFileName = FileName;
    NormalizedFileName.ReplaceChar(TDirectory::SWrongPathSeparatorChar(),
      TDirectory::SPathSeparatorChar());

    if (NormalizedFileName.StartsWith(mBasePath.Path()))
    {
      Ret = NormalizedFileName;
      Ret.RemoveFirst(mBasePath.Path());
    }
  }

  // always use '/' for relative paths to allow using the db on Linux and OSX
  #if defined(MWindows)
    Ret.ReplaceChar('\\', '/');
  #endif

  return Ret;
}

// -------------------------------------------------------------------------------------------------

TString TSqliteSampleDescriptorPool::AbsFilenamePath(
  const TString &FileName) const
{
  TString Ret = FileName;

  if (!mBasePath.IsEmpty())
  {
    TString NormalizedFileName = FileName;
    NormalizedFileName.ReplaceChar(TDirectory::SWrongPathSeparatorChar(),
      TDirectory::SPathSeparatorChar());

    if (!NormalizedFileName.StartsWith(mBasePath.Path()))
    {
      Ret = mBasePath.Path() + NormalizedFileName;
    }
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

void TSqliteSampleDescriptorPool::ShutdownDatabase()
{
  if (mDatabase.IsOpen())
  {
    mDatabase.Close();
  }
}

// -------------------------------------------------------------------------------------------------

void TSqliteSampleDescriptorPool::InitializeDatabase()
{
  bool CreateNewTables = false;

  // assets table already present?
  const TString AssetTableExistsStatement = TString() +
     "SELECT count(name) FROM sqlite_master "
       "WHERE type='table' AND name='" + MAssetsTableName + "'";

  if (mDatabase.ExecuteScalarInt(AssetTableExistsStatement) != 1)
  {
    TLog::SLog()->AddLine(MLogPrefix, "Creating initial set of tables...");
    CreateNewTables = true;
  }

  // check user_version
  else
  {
    const int DatabaseVersion = mDatabase.ExecuteScalarInt("PRAGMA user_version");

    if (DatabaseVersion > kCurrentVersion)
    {
      // don't touch newer database files
      throw TReadableException(
        MText("Unknown database version: " + ToString(DatabaseVersion) + ". " +
         "The database maybe got created by a newer version of the crawler."));
    }
    else if (DatabaseVersion < kCurrentVersion)
    { 
      // drop entire database content to force creating new one
      TLog::SLog()->AddLine(MLogPrefix, "Dropping all existing records. "
        "Database is at version: %d but current version is: %d", 
        DatabaseVersion, kCurrentVersion);

      CreateNewTables = true;

      const TString FileNameAndPath = mDatabase.FileNameAndPath();
      
      // try trashing the entire db first
      mDatabase.Close();
      const bool DeleteSucceeded = TFile(FileNameAndPath).Unlink();
      
      if (!mDatabase.Open(FileNameAndPath)) 
      {
        throw TReadableException(
          MText("Failed to (re)open the database file for upgrading."));
      }

      // drop all tables as fallback
      if (!DeleteSucceeded)
      { 
        TLog::SLog()->AddLine(MLogPrefix,
          "Database unlinking failed: dropping all tables...");

        try
        {
          TDatabase::TTransaction Transaction(mDatabase);
          {
            mDatabase.Execute("DROP table 'assets'");

            if (mDescriptorSet == TSampleDescriptors::kHighLevelDescriptors)
            {
              mDatabase.Execute("DROP table 'classes'");
            }
          }
          Transaction.Commit();

          // free up empty space in database
          mDatabase.Execute("VACUUM");
        }
        catch (const TReadableException& Exception)
        {
          TLog::SLog()->AddLine(MLogPrefix, "Drop statement failed: %s",
            Exception.what());
        }
      }
    }
  }

  if (CreateNewTables)
  {
    try
    {
      TDatabase::TTransaction Transaction(mDatabase);
      {
        // . set version

        mDatabase.Execute("PRAGMA user_version = '" + ToString(kCurrentVersion) + "'");

        // . create assets table
        {
          TList<TString> ColumnNameAndTypes = MakeList<TString>(
            "filename TEXT PRIMARY KEY", "modtime INTEGER", "status TEXT");

          // create a dummy descriptor with default values - we only need the types
          TSampleDescriptors ExampleDescriptors;

          // get name an sqlite type from the descriptor
          const TList<TSampleDescriptor*> Descriptors =
            ExampleDescriptors.Descriptors(mDescriptorSet);
            
          for (int i = 0; i < Descriptors.Size(); ++i) 
          {
            const TList<TPair<TString, TSampleDescriptor::TValue>>
              DescriptorValues(Descriptors[i]->Values());

            for (int j = 0; j < DescriptorValues.Size(); ++j)
            {
              const TString BaseName = DescriptorValues[j].First();

              const TString NamePostfix = boost::apply_visitor(
                TDescriptorValueNamePostfix(*Descriptors[i]),
                DescriptorValues[j].Second());

              const TString SqliteType = boost::apply_visitor(
                TDescriptorValueSqliteType(*Descriptors[i]),
                DescriptorValues[j].Second());

              ColumnNameAndTypes.Append(
                BaseName + "_" + NamePostfix + " " + SqliteType);
            }
          }

          mDatabase.Execute(TString() +
            "CREATE TABLE " + MAssetsTableName + 
              "(" + SJoinStrings(ColumnNameAndTypes, ",") + ")");
        }

        // . create classes table

        if (mDescriptorSet == TSampleDescriptors::kHighLevelDescriptors)
        {
          mDatabase.Execute(TString() + 
            "CREATE TABLE classes (classifier TEXT PRIMARY KEY, classes BLOB)");
        }
      }
      Transaction.Commit();
    }
    catch (const TReadableException& Exception)
    {
      TLog::SLog()->AddLine(MLogPrefix, "Table initialization failed: %s",
        Exception.what());

      throw Exception;
    }
  }
}

// -------------------------------------------------------------------------------------------------

bool TSqliteSampleDescriptorPool::IsEmpty() const
{
  if (mDatabase.IsOpen())
  {
    try
    {
      TSqliteSampleDescriptorPool* pMutableThis =
        const_cast<TSqliteSampleDescriptorPool*>(this);

      TDatabase::TStatement Statement(pMutableThis->mDatabase,
        "SELECT COUNT(*) FROM assets;");

      if (Statement.Step())
      {
        MAssert(Statement.ColumnName(0) == "COUNT(*)",
          "Unexpected statement result.");

        return Statement.ColumnInt(0) == 0;
      }
    }
    catch (const TReadableException& Exception)
    {
      TLog::SLog()->AddLine(MLogPrefix, "Unexpected DB error: %s",
        Exception.what());
      throw;
    }
  }

  return true;
}

// -------------------------------------------------------------------------------------------------

int TSqliteSampleDescriptorPool::NumberOfSamples() const
{
  if (mDatabase.IsOpen())
  {
    try
    {
      TSqliteSampleDescriptorPool* pMutableThis =
        const_cast<TSqliteSampleDescriptorPool*>(this);

      TDatabase::TStatement Statement(pMutableThis->mDatabase,
        "SELECT COUNT(*) FROM assets WHERE status='succeeded';");

      if (Statement.Step())
      {
        MAssert(Statement.ColumnName(0) == "COUNT(*)",
          "Unexpected statement result.");

        return Statement.ColumnInt(0);
      }
    }
    catch (const TReadableException& Exception)
    {
      TLog::SLog()->AddLine(MLogPrefix, "Unexpected DB error: %s",
        Exception.what());
      throw;
    }
  }

  return 0;
}

// -------------------------------------------------------------------------------------------------

TOwnerPtr<TSampleDescriptors> TSqliteSampleDescriptorPool::Sample(int Index) const
{
  if (mDatabase.IsOpen())
  {
    try
    {
      TSqliteSampleDescriptorPool* pMutableThis =
        const_cast<TSqliteSampleDescriptorPool*>(this);

      TDatabase::TStatement Statement(pMutableThis->mDatabase,
        TString() + "SELECT * FROM " + MAssetsTableName + " " +
        "WHERE status='succeeded' LIMIT 1 OFFSET :offset;");

      Statement.BindInt(":offset", Index);

      if (Statement.Step())
      {
        if (Statement.ColumnName(0) != "filename")
        {
          throw TReadableException(
            "Unexpected table layout - expected a 'filename' column as primary column");
        }

        const TString FileName = Statement.ColumnText(0);

        if (Statement.ColumnName(1) != "modtime")
        {
          throw TReadableException(
            "Unexpected table layout - expected a 'modtime' column as second column");
        }
        else if (Statement.ColumnName(2) != "status")
        {
          throw TReadableException(
            "Unexpected table layout - expected a 'status' column as third column");
        }

        MAssert(Statement.ColumnText(2) == "succeeded",
          "Expecting to extract succeeded samples only");

        const TString NormalizedFileName = RelativeFilenamePath(FileName);

        // create new sample and unserialize all descriptors
        TOwnerPtr<TSampleDescriptors> pResults(new TSampleDescriptors);
        pResults->mFileName = NormalizedFileName;

        // unserialize all descriptor values
        const TList<TSampleDescriptor*> Descriptors =
          pResults->Descriptors(mDescriptorSet);

        int ColumnIndexOffset = 3;
        for (int i = 0; i < Descriptors.Size(); ++i)
        {
          const TList<TPair<TString, TSampleDescriptor::TValue>>
            DescriptorValues(Descriptors[i]->Values());

          for (int j = 0; j < DescriptorValues.Size(); ++j)
          {
            const TString BaseName = DescriptorValues[j].First();

            const TString NamePostfix = boost::apply_visitor(
              TDescriptorValueNamePostfix(*Descriptors[i]),
              DescriptorValues[j].Second());

            const TString ColumnName = BaseName + "_" + NamePostfix;

            // search column from the descriptor's name and postfix
            bool FoundColumn = false;
            
            const int ColumnCount = Statement.ColumnCount();
            for (int c = 0; c < ColumnCount; ++c)
            {
              const int ColumnIndex = (ColumnIndexOffset + c) % ColumnCount;
              if (Statement.ColumnName(ColumnIndex) == ColumnName)
              {
                // unserialize contents
                boost::apply_visitor(
                  TUnserializeDescriptorValueFromStatement(
                    *Descriptors[i], Statement, ColumnIndex, ColumnName),
                  DescriptorValues[j].Second());

                // start searching the next descriptor from the next column
                ColumnIndexOffset = ColumnIndex + 1;
                FoundColumn = true;
                break;
              }
            }

            if (!FoundColumn)
            {
              throw TReadableException(
                "Could not find required column '" + ColumnName + "'");
            }
          }
        }

        return pResults;
      }
    }
    catch (const TReadableException& Exception)
    {
      TLog::SLog()->AddLine(MLogPrefix, "Unexpected DB error: %s",
        Exception.what());
      throw;
    }
  }

  return TOwnerPtr<TSampleDescriptors>();
}

// -------------------------------------------------------------------------------------------------

TList<TPair<TString, int>> TSqliteSampleDescriptorPool::SampleModificationDates() const
{
  TList<TPair<TString, int>> Ret;

  try
  {
    TSqliteSampleDescriptorPool* pMutableThis =
      const_cast<TSqliteSampleDescriptorPool*>(this);

    TDatabase::TStatement Statement(pMutableThis->mDatabase,
      TString() + "SELECT filename, modtime FROM " + MAssetsTableName);

    while (Statement.Step())
    {
      const TString AbsFileName = AbsFilenamePath(Statement.ColumnText(0));
      const int ModTime = Statement.ColumnInt(1);
      Ret.Append(MakePair(AbsFileName, ModTime));
    }
  }
  catch (const TReadableException& Exception)
  {
    TLog::SLog()->AddLine(MLogPrefix, "Unexpected DB error: %s",
      Exception.what());
    throw;
  }

  return Ret;
}

// -------------------------------------------------------------------------------------------------

void TSqliteSampleDescriptorPool::InsertSample(
  const TString&             FileName,
  const TSampleDescriptors&  Results)
{
  const TString RelFilename = RelativeFilenamePath(FileName);
  const int ModificationStatTime = TFile(FileName).ModificationStatTime();

  try
  {
    TDatabase::TTransaction Transaction(mDatabase);
    {
      // get keys from results
      TList<TString> Keys = MakeList<TString>("filename", "modtime", "status");

      const TList<const TSampleDescriptor*> Descriptors =
        Results.Descriptors(mDescriptorSet);
      for (int i = 0; i < Descriptors.Size(); ++i)
      {
        const TList<TPair<TString, TSampleDescriptor::TValue>>
          DescriptorValues(Descriptors[i]->Values());
        for (int j = 0; j < DescriptorValues.Size(); ++j)
        {
          const TString BaseName = DescriptorValues[j].First();

          const TString NamePostfix = boost::apply_visitor(
            TDescriptorValueNamePostfix(*Descriptors[i]),
            DescriptorValues[j].Second());

          Keys.Append(BaseName + "_" + NamePostfix);
        }
      }

      // create statement
      TDatabase::TStatement InsertStatement(mDatabase, TString() +
        "INSERT OR REPLACE into " + MAssetsTableName + "(" + SJoinStrings(Keys, ",") + ") " +
        "values(" + (TString("?,") * Keys.Size()).RemoveLast(",") + ")");

      // bind values to the statement
      InsertStatement.BindText(1, RelFilename);
      InsertStatement.BindInt (2, ModificationStatTime);
      InsertStatement.BindText(3, "succeeded");

      int ParameterIndex = 4;

      for (int i = 0; i < Descriptors.Size(); ++i)
      {
        const TList<TPair<TString, TSampleDescriptor::TValue>>
          DescriptorValues(Descriptors[i]->Values());
        for (int j = 0; j < DescriptorValues.Size(); ++j)
        {
          boost::apply_visitor(
            TBindDescriptorValueToStatement(
              *Descriptors[i], InsertStatement, ParameterIndex),
            DescriptorValues[j].Second());
          ++ParameterIndex;
        }
      }

      InsertStatement.Execute();
    }
    Transaction.Commit();
  }
  catch (const TReadableException& Exception)
  {
    TLog::SLog()->AddLine(MLogPrefix, "Inserting %s failed: %s",
      RelFilename.StdCString().c_str(), Exception.what());

    throw Exception;
  }
}

// -------------------------------------------------------------------------------------------------

void TSqliteSampleDescriptorPool::InsertFailedSample(
  const TString& FileName,
  const TString& Reason)
{
  const TString RelFilename = RelativeFilenamePath(FileName);
  const int ModificationStatTime = TFile(FileName).ModificationStatTime();

  try
  {
    TDatabase::TTransaction Transaction(mDatabase);
    {
      TDatabase::TStatement InsertStatement(mDatabase, TString() +
        "INSERT OR REPLACE into " + MAssetsTableName + "(filename, modtime, status) "
          "values (?,?,?)");
      
      InsertStatement.BindText(1, RelFilename);
      InsertStatement.BindInt (2, ModificationStatTime);
      InsertStatement.BindText(3, "error: " + Reason);

      InsertStatement.Execute();
    }
    Transaction.Commit();
  }
  catch (const TReadableException& Exception)
  {
    TLog::SLog()->AddLine(MLogPrefix, "Inserting %s as failed, failed: %s",
      RelFilename.StdCString().c_str(), Exception.what());

    throw Exception;
  }
}

// -------------------------------------------------------------------------------------------------

void TSqliteSampleDescriptorPool::RemoveSample(const TString& FileName)
{
  RemoveSamples(MakeList(FileName));
}

// -------------------------------------------------------------------------------------------------

void TSqliteSampleDescriptorPool::RemoveSamples(const TList<TString>& FileNames)
{
  // make sure all pased names are relative, as stored in the database
  TList<TString> RelFilenames;
  RelFilenames.PreallocateSpace(FileNames.Size());
  for (int i = 0; i < FileNames.Size(); ++i) 
  {
    RelFilenames.Append(RelativeFilenamePath(FileNames[i]));
  }

  try
  {
    // delete samples in batches: this is more efficient than trying to delete 
    // all in one go (when there are thousands to delete) and also one by one.
    const int NumberOfSamplesPerDeleteBatch = 100;

    TDatabase::TStatement RemoveStatement(mDatabase, TString() +
      "DELETE from " + MAssetsTableName + " WHERE filename MATCH ?");

    while (!RelFilenames.IsEmpty()) 
    {
      TDatabase::TTransaction Transaction(mDatabase);
      for (int i = 0; i < NumberOfSamplesPerDeleteBatch && !RelFilenames.IsEmpty(); ++i)
      {
        RemoveStatement.BindText(1, RelFilenames.GetAndDeleteLast());
        RemoveStatement.Execute();
      }
      Transaction.Commit();
    }
  }
  catch (const TReadableException& Exception)
  {
    TLog::SLog()->AddLine(MLogPrefix, "Removing samples failed: %s",
      Exception.what());

    throw Exception;
  }
}

// -------------------------------------------------------------------------------------------------

void TSqliteSampleDescriptorPool::InsertClassifier(
  const TString&        ClassifierName,
  const TList<TString>& Classes)
{
  MAssert(mDescriptorSet == TSampleDescriptors::kHighLevelDescriptors,
    "Available for high level dbs only");

  try
  {
    TDatabase::TTransaction Transaction(mDatabase);
    {
      TDatabase::TStatement InsertStatement(mDatabase, TString() +
        "INSERT OR REPLACE into " + MClassesTableName + "(classifier, classes) "
          "values(?,?)");

      const TSampleDescriptor::TExportFlags ExportFlags = 0;
      InsertStatement.BindText(1, ClassifierName);
      InsertStatement.BindText(2, SToJSON(Classes, ExportFlags));

      InsertStatement.Execute();
    }
    Transaction.Commit();
  }
  catch (const TReadableException& Exception)
  {
    TLog::SLog()->AddLine(MLogPrefix, "Inserting classifier %s failed : %s",
      ClassifierName.StdCString().c_str(), Exception.what());

    throw Exception;
  }
}

