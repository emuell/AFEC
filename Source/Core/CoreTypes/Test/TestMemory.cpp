#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"
#include "CoreTypes/Export/Memory.h"
#include "CoreTypes/Export/CompilerDefines.h"

#include "CoreTypes/Test/TestMemory.h"
                  
#include <limits>

// =================================================================================================

namespace
{
  int Random(int MaxValue)
  {
    return ::rand() % MaxValue;
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreTypesTest::Memory()
{
  // fixing strict alias handling on GCC
  #if defined(MCompiler_GCC)
    typedef volatile TUInt32 __attribute__((__may_alias__)) TUInt32_a;
    typedef volatile TUInt16 __attribute__((__may_alias__)) TUInt16_a;
  #else
    typedef TUInt32 TUInt32_a;
    typedef TUInt16 TUInt16_a;
  #endif
  
  static const int kBufferSize = 1024;
  // CopyMMX uses a different function for buffers < 64
  static const int kBufferSizeSmall = 63; 

  static const TUInt8 kEmptyMagicByte = Random(4) == 3 ? 
    std::numeric_limits<TUInt8>::max() :
    (TUInt8)Random(std::numeric_limits<TUInt8>::max());

  static const TUInt16 kEmptyMagicWord = Random(4) == 3 ? 
    std::numeric_limits<TUInt16>::max() :
    (TUInt16)Random(std::numeric_limits<TUInt16>::max());

  static const TUInt32 kEmptyMagicDWord = Random(4) == 3 ? 
    std::numeric_limits<TUInt32>::max() :
    (TUInt32)Random(std::numeric_limits<TUInt32>::max());;

  TUInt8 FromBuffer[kBufferSize];
  TUInt8 ToBuffer[kBufferSize];
  
  TUInt8 FromBufferSmall[kBufferSizeSmall];
  TUInt8 ToBufferSmall[kBufferSizeSmall];

  for (int i = 0; i < kBufferSize; ++i)
  {
    FromBuffer[i] = (TUInt8)(Random(256));
    ToBuffer[i] = FromBuffer[i];
  }
  
  // check if we have overwritten the memory blocks (we had a bug that once did)

  // ... SetByte
  
  TMemory::SetByte(ToBuffer, kEmptyMagicByte, kBufferSize - 7);
  
  for (int i = 0; i < kBufferSize - 7; ++i)
  {
    BOOST_CHECK_EQUAL(ToBuffer[i], kEmptyMagicByte);
  }

  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 1], FromBuffer[kBufferSize - 1]);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 2], FromBuffer[kBufferSize - 2]);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 3], FromBuffer[kBufferSize - 3]);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 4], FromBuffer[kBufferSize - 4]);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 5], FromBuffer[kBufferSize - 5]);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 6], FromBuffer[kBufferSize - 6]);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 7], FromBuffer[kBufferSize - 7]);

  for (int i = 0; i < kBufferSize; ++i)
  {
    FromBuffer[i] = (TUInt8)(Random(256));
    ToBuffer[i] = FromBuffer[i];
  }
  
  // ... SetWord
          
  TMemory::SetWord(ToBuffer, kEmptyMagicWord, kBufferSize / 2 - 1);

  for (int i = 0; i < kBufferSize / 2 - 1; ++i)
  {
    BOOST_CHECK_EQUAL(((TUInt16_a*)ToBuffer)[i], kEmptyMagicWord);
  }

  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 1], FromBuffer[kBufferSize - 1]);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 2], FromBuffer[kBufferSize - 2]);
  
  for (int i = 0; i < kBufferSize; ++i)
  {
    FromBuffer[i] = (TUInt8)(Random(256));
    ToBuffer[i] = FromBuffer[i];
  }
  
  // ... SetDWord

  TMemory::SetDWord(ToBuffer, kEmptyMagicDWord, kBufferSize / 4 - 1);

  for (int i = 0; i < kBufferSize / 4 - 1; ++i)
  {
    BOOST_CHECK_EQUAL(((TUInt32_a*)ToBuffer)[i], kEmptyMagicDWord);
  }

  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 1], FromBuffer[kBufferSize - 1]);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 2], FromBuffer[kBufferSize - 2]);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 3], FromBuffer[kBufferSize - 3]);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 4], FromBuffer[kBufferSize - 4]);


  // ... Copy (large buffer)
  
  TMemory::SetByte(ToBuffer, kEmptyMagicByte, sizeof(ToBuffer));
  TMemory::Copy(ToBuffer, FromBuffer, kBufferSize - 7);

  for (int i = 0; i < kBufferSize - 7; ++i)
  {
    BOOST_CHECK_EQUAL(FromBuffer[i], ToBuffer[i]);
  }

  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 1], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 2], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 3], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 4], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 5], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 6], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBuffer[kBufferSize - 7], kEmptyMagicByte);


  // ... Copy (small buffer)
  
  for (int i = 0; i < kBufferSizeSmall; ++i)
  {
    FromBufferSmall[i] = (TUInt8)(Random(256));
    ToBufferSmall[i] = kEmptyMagicByte;
  }
  
  TMemory::Copy(ToBufferSmall, FromBufferSmall, kBufferSizeSmall - 7);

  for (int i = 0; i < kBufferSizeSmall - 7; ++i)
  {
    BOOST_CHECK_EQUAL(FromBufferSmall[i], ToBufferSmall[i]);
  }

  BOOST_CHECK_EQUAL(ToBufferSmall[kBufferSizeSmall - 1], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBufferSmall[kBufferSizeSmall - 2], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBufferSmall[kBufferSizeSmall - 3], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBufferSmall[kBufferSizeSmall - 4], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBufferSmall[kBufferSizeSmall - 5], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBufferSmall[kBufferSizeSmall - 6], kEmptyMagicByte);
  BOOST_CHECK_EQUAL(ToBufferSmall[kBufferSizeSmall - 7], kEmptyMagicByte);
}

