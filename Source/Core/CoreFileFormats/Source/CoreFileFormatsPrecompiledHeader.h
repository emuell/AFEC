#ifndef _CoreFileFormatsPrecompiledHeader_h_
#define _CoreFileFormatsPrecompiledHeader_h_

#ifndef MCoreFileFormats
  #error "Can't include precompiled header files from other projects!"
#endif

// =================================================================================================

#include "CoreFileFormats/Export/RiffFile.h"
#include "CoreFileFormats/Export/AudioFile.h"
#include "CoreFileFormats/Export/WaveFile.h"
#include "CoreFileFormats/Export/AifFile.h"
#include "CoreFileFormats/Export/FlacFile.h"
#include "CoreFileFormats/Export/OggVorbisFile.h"
#include "CoreFileFormats/Export/Mp3File.h"
#include "CoreFileFormats/Export/SampleConverter.h"
#include "CoreFileFormats/Export/ZipFile.h"
#include "CoreFileFormats/Export/ZipArchive.h"
#include "CoreFileFormats/Export/RtfFile.h"
#include "CoreFileFormats/Export/Database.h"
#include "CoreFileFormats/Export/CoreFileFormatsInit.h"

// =================================================================================================
  
#include "CoreFileFormats/Source/ZipArchiveRoot.h"
#include "CoreFileFormats/Source/ZipFileHeader.h"

#if defined(MWindows)
  #include "CoreFileFormats/Source/WinMediaFoundationAudioFile.h"
  
#elif defined (MMac)
  #include "CoreFileFormats/Source/MacCoreAudioAudioFile.h"

#endif


#endif // _CoreFileFormatsPrecompiledHeader_h_

