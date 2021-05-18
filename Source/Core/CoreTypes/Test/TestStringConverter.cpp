#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"
#include "CoreTypes/Export/Str.h"

#include "CoreTypes/Test/TestStringConverter.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreTypesTest::StringConverter()
{
  // randomize array size too
  TArray<char> Array(0x20 + (::rand() & 0x200));
                        
  for (int i = 0; i < Array.Size(); ++i)
  {
    Array[i] = (char)((::rand() % 0xfe) + 1);
  }

  TArray<char> Temp;       
  TStringConverter::ToBase64(Array, Temp);

  TArray<char> ConvertedArray;
  TStringConverter::FromBase64(Temp, ConvertedArray);

  BOOST_CHECK_ARRAYS_EQUAL(Array, Array.Size(), ConvertedArray, ConvertedArray.Size())
}

