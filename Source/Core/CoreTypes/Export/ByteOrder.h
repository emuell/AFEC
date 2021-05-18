#pragma once

#ifndef _Byteorder_h_
#define _Byteorder_h_

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/CompilerDefines.h"

// =================================================================================================

#define MMotorolaByteOrder 0
#define MIntelByteOrder    1

#if defined(MArch_X86) || defined(MArch_X64)
  #define MSystemByteOrder MIntelByteOrder
  
#elif defined(MArch_PPC)
  #define MSystemByteOrder MMotorolaByteOrder

#else
  #error Unknown Architecture
#endif

// =================================================================================================

namespace TByteOrder
{
  enum TByteOrder
  {
    kIntel    = MIntelByteOrder,    // LittleEndian
    kMotorola = MMotorolaByteOrder, // BigEndian
    
    kSystemByteOrder = MSystemByteOrder
  };


  //@{ ... Swap single basetype values
  
  template<typename T> 
  void Swap(T& Value)
  {
    // Should be only used for basetypes
    MStaticAssert(sizeof(T) == 1 || sizeof(T) == 2 || 
      sizeof(T) == 4 || sizeof(T) == 8);

    const T Temp(Value);

    for (size_t i = 0; i < sizeof(T); ++i)
    {
      ((char*)&Value)[i] = ((char*)&Temp)[sizeof(T) - i - 1];
    }
  }

  // the generic version will not compile for T24
  template<> inline void Swap(T24& Value)
  {
    const char Temp(Value[0]); 
    Value[0] = Value[2];
    Value[2] = Temp;
  }

  // need no swapping for:
  template<> inline void Swap(bool&) { }
  template<> inline void Swap(char&) { }
  template<> inline void Swap(signed char&) { }
  template<> inline void Swap(unsigned char&) { }
  //@}
  
  
  //@{ ... swap basetype buffers
  
  template<typename T> 
  void Swap(T *pBuffer, size_t BufferSize)
  {
    // Should be only used for basetypes
    MStaticAssert(sizeof(T) == 1 || sizeof(T) == 2 || 
      sizeof(T) == 3 || sizeof(T) == 4 || sizeof(T) == 8);
    
    for (size_t i = 0; i < BufferSize; ++i)
    {
      Swap(pBuffer[i]);
    }
  }

  // need no swapping for:
  template<> inline void Swap(bool*, size_t) { }
  template<> inline void Swap(char*, size_t) { }
  template<> inline void Swap(signed char*, size_t) { }
  template<> inline void Swap(unsigned char*, size_t) { }
  //@}
}


#endif // _Byteorder_h_

