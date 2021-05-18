#pragma once

#ifdef _MainEntry_h_
  #error "Do not include this header in other headers, "
    "but only in the main.cpp file of your executable"
#endif

#define _MainEntry_h_

// =================================================================================================

/*! 
 * Include this file in your executable's main.cpp file and nowhere else, 
 * together with a 'int gMain()' definition, to implement the main entry point 
 * of the application.
 *
 * Do not include this file for DLLs/shared library builds. This avoids that 
 * possibly conflicting 'main' exports are present in shared library builds.
 * 
 * Usage:
 *
 *   #include "CoreTypes/Export/MainEntry.h"
 * 
 *   int gMain(const TList<TString>& Arguments)
 *   {
 *     // create and run your app or do command line stuff here
 *     return (Succeeded) ? 0 : 1;
 *   }
 * 
!*/

// =================================================================================================

#define MCompilingMainEntry

// set to main's original arguments
static int gArgc = 0;
static char** gArgv = NULL;

// -------------------------------------------------------------------------------------------------

#if defined(MMac)
  #include "CoreTypes/Source/MacMainEntry.h"

  int main(int argc, char* argv[])
  {
    gArgc = argc; gArgv = argv;
    return main_platform_impl(argc, argv);
  }

#elif defined(MLinux)
  #include "CoreTypes/Source/LinuxMainEntry.h"

  int main(int argc, char* argv[])
  {
    gArgc = argc; gArgv = argv;
    return main_platform_impl(argc, argv);
  }

#elif defined(MWindows)
  #include "CoreTypes/Source/WinMainEntry.h"

  int main(int argc, char* argv[])
  {
    gArgc = argc; gArgv = argv;
    return main_platform_impl();
  }

  int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
  {
    gArgc = 0; gArgv = NULL;
    return main_platform_impl();
  }

#else
  #error "Unknown platform"
#endif

