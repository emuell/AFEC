
# Third Party Libs

The structure in here is important!
Every 3rd party lib has to follow this directory structure to get picked up correctly by the build system:

    <LibraryName>/
    ├── Dist
    ├── Export
    ├── Library
    │   ├── Linux
    │   │   ├── x86
    │   │   └── x86_64
    │   └── Win
    │       ├── x64
    │       └── x86
    ├── Linux
    └── Win

## Dist

Holds the 3rd party source code.

## Export

Holds the header files that we use in our C/C++ files to include the 3rd party lib.

## Library

Holds the precompiled static libraries.
When you create the static libraries, rename them and put a underscore behind the filename and before the suffix to ensure we're linking our libs and not ones from the system lib path.

## Linux/Win

Contains a build.sh/build.bat, (with instructions on how) to build the libraries.

