#pragma once

#ifndef _TestHelpers_h_
#define _TestHelpers_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Directory.h"
#include "CoreTypes/Export/ProductVersion.h"
#include "CoreTypes/Export/File.h"
#include "CoreTypes/Export/Pair.h"

#include "../../Boost/Export/BoostUnitTest.h"

template <typename T>
class TObservableConstBaseType;

// =================================================================================================

inline std::ostream& operator<<( 
  std::ostream& stream, const TString& String )
{
  return stream << String.CString();
}

inline std::ostream& operator<<( 
  std::ostream& stream, const TDirectory& Directory )
{
  return stream << Directory.Path().CString();
}

inline std::ostream& operator<<( 
  std::ostream& stream, const TProductVersion& Version )
{
  return stream << ToString(Version).CString();
}

template <typename T1, typename T2>
inline std::ostream& operator<<( 
  std::ostream& stream, const TPair<T1, T2>& Pair )
{
  return stream << ToString(Pair).CString();
}

template <typename T>
inline std::ostream& operator<<( 
  std::ostream& stream, const TList<T>& List )
{
  stream << "[";
  for ( int i = 0; i < List.Size(); ++i )
  {
    stream << ", " << List[i];
  }
  stream << "]";
  return stream;
}

template <typename T>
inline std::ostream& operator<<( 
  std::ostream& stream, const TArray<T>& Array )
{
  stream << "[";
  for ( int i = 0; i < Array.Size(); ++i )
  {
    stream << ", " << Array[i];
  }
  stream << "]";
  return stream;
}

template <typename T>
inline std::ostream& operator<<( 
  std::ostream& stream, const TObservableConstBaseType<T>& Observable )
{
  return stream << ToString(Observable).CString();
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

#define BOOST_CHECK_EQUAL_EPSILON(value1, value2, epsilon) \
  BOOST_TEST_CONTEXT(BOOST_STRINGIZE(value1) << ": " << value1 << " " << \
    BOOST_STRINGIZE(value2) << ": " << value2) \
  { \
    BOOST_CHECK_SMALL(value1 - value2, epsilon); \
  } 

// -------------------------------------------------------------------------------------------------

#define BOOST_CHECK_ARRAYS_EQUAL(array1, array1_Size, array2, array2_Size) \
  BOOST_TEST(array1_Size == array2_Size); \
  if (array1_Size == array2_Size) \
  { \
    for (int i = 0; i < array1_Size; i++) \
    { \
      if (array1[i] != array2[i]) \
      { \
        BOOST_TEST_ERROR(BOOST_STRINGIZE(array1) " == " BOOST_STRINGIZE(array2) << \
          " failed at index " << i << " [" << array1[i] << " != " << array2[i] << "]"); \
        break; \
      } \
    } \
  }

// -------------------------------------------------------------------------------------------------

#define BOOST_CHECK_ARRAYS_EQUAL_EPSILON(array1, array1_Size, array2, array2_Size, epsilon) \
  BOOST_TEST(array1_Size == array2_Size); \
  if (array1_Size == array2_Size) \
  { \
    for (int i = 0; i < array1_Size; i++) \
    { \
      if (! boost::math::fpc::is_small(array1[i] - array2[i], epsilon)) \
      { \
        BOOST_TEST_ERROR(BOOST_STRINGIZE(array1) " == " BOOST_STRINGIZE(array2) << \
          " failed at index " << i << " [" << array1[i] << " != " << array2[i] << "]" << \
          " with epsilon " << epsilon); \
        break; \
      } \
    } \
  }

// -------------------------------------------------------------------------------------------------

#define BOOST_CHECK_FILES_EQUAL(Fname1, Fname2) \
  { \
    TFile f1(Fname1); \
    TFile f2(Fname2); \
      \
    if ( f1.Open(TFile::kRead) )  \
    { \
      if ( f2.Open(TFile::kRead) )  \
      { \
        char ch1, ch2;  \
        \
        for (;;)  \
        { \
          const bool Got1 = f1.ReadByte(ch1); \
          const bool Got2 = f2.ReadByte(ch2); \
          \
          if ( !Got1 && !Got2 ) \
          { \
            break;  \
          } \
            \
          if ( (Got1 && !Got2) || (!Got1 && Got2) || (ch1 != ch2) )  \
          { \
            BOOST_TEST_ERROR((TString(Fname1) + " == " + TString(Fname2)).CString()); \
          } \
        } \
      } \
      else  \
      { \
        BOOST_TEST_ERROR((TString(Fname2) + " does not exist").CString()); \
      } \
    } \
    else  \
    { \
      BOOST_TEST_ERROR((TString(Fname1) + " does not exist").CString()); \
    } \
  }


#endif // _TestHelpers_h_

