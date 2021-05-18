#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/Log.h"
#include "CoreTypes/Export/Exception.h"

// =================================================================================================

//! NB: when changing something here, check CoreFileFormatsCoreInit.cpp too!
//! Both are in a separate file to help the linker throwing away unused symbols.

// -------------------------------------------------------------------------------------------------

void CoreFileFormatsInit()
{
  CoreFileFormatsCoreInit(); // TSocketBase
  
  #if !defined(MNoMp3LibMpg)
    TMp3File::SInit();
  #endif

  // no QT init (is loaded when needed)
}

// -------------------------------------------------------------------------------------------------

void CoreFileFormatsExit()
{
  #if !defined(MNoMp3LibMpg)
    TMp3File::SExit();
  #endif

  CoreFileFormatsCoreExit(); // TSocketBase
}

