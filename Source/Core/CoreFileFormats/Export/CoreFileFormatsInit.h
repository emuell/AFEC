#pragma once

#ifndef _CoreFileFormatsInit_h_
#define _CoreFileFormatsInit_h_

// =================================================================================================

//! To be called by applications on init (entering main)
//! and exit (leaving main). See also CoreFileFormatsCoreInit/Exit.
void CoreFileFormatsInit();
void CoreFileFormatsExit();

//! Alternative init/exit functions, which will only init/exit the core FileIO 
//! classes (Sockets, ZipArchive) and skip audio, video and subtitle formats.
//! Avoids that else unused functionality gets linked into executables.
void CoreFileFormatsCoreInit();
void CoreFileFormatsCoreExit();


#endif // _CoreFileFormatsInit_h_

