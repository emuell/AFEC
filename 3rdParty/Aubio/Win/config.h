// manually created for windows visual CPP builds

#define HAVE_STDLIB_H 1
#define HAVE_STDIO_H 1
#define HAVE_COMPLEX_H 1
#define HAVE_MATH_H 1
#define HAVE_STRING_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STDARG_H 1
#define HAVE_C99_VARARGS_MACROS 1

#define HAVE_AUBIO_DOUBLE 1
#define HAVE_MEMCPY_HACKS 1

// #undef HAVE_FFTW3
// #undef HAVE_INTEL_IPP

#if defined(HAVE_INTEL_IPP)
  // force linking single-threaded static IPP libraries on Windows
  #define _IPP_SEQUENTIAL_STATIC
  // Intel's libmmt clashes with MSVC's dynamic multi-threaded debug runtime
  #if (_MSC_VER >= 1900) 
    #pragma comment(linker, "/NODEFAULTLIB:libmmt.lib")
  #endif
#endif

#include "types.h"

