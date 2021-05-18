#pragma once

#ifndef _CompilerDefines_h_
#define _CompilerDefines_h_

// =================================================================================================

// validate MCompiler_XXX and MArch_XXX defines

#if !(defined(MCompiler_VisualCPP) || defined(MCompiler_GCC))
  #error "Unknown or unspecified Compiler"

#elif (defined(MCompiler_VisualCPP) && defined(MCompiler_GCC))
  #error "Double specified Compiler"
#endif

#if defined(MCompiler_Clang) && !defined(MCompiler_GCC)
  #error "MCompiler_Clang can only be enabled when MCompiler_GCC is enabled too"
#endif


#if !(defined(MArch_X86) || defined(MArch_PPC) || defined(MArch_X64))
  #error "Unknown or unspecified Architecture"

#elif (defined(MArch_X86) && defined(MArch_PPC)) || \
      (defined(MArch_X86) && defined(MArch_X64)) || \
      (defined(MArch_PPC) && defined(MArch_X64))
  #error "Double specified Architecture"
#endif

#if !(defined(MMac) || defined(MWindows) || defined(MLinux))
  #error "Unknown or wrong specified Platform"

#elif (defined(MMac) && defined(MWindows)) || \
      (defined(MMac) && defined(MLinux)) || \
      (defined(MWindows) && defined(MLinux))
  #error "Double specified Platform"
#endif

// =================================================================================================

// C++0X features 

#if defined(MCompiler_VisualCPP)

  #if (_MSC_VER >= 1500) // Visual CPP 2005
    #define MCompiler_Has_TypeTraitsIntrinsics
  #endif

  #if (_MSC_VER >= 1600) // Visual CPP 2010
    #define MCompiler_Has_StaticAssert
    #define MCompiler_Has_RValue_References
    #define MCompiler_Has_Auto
    #define MCompiler_Has_DeclType
  #endif

  #if (_MSC_VER >= 1800) // Visual CPP 2015
    #define MCompiler_Has_NoExcept
    #define MCompiler_Has_Lambdas
  #endif
  
#elif defined(MCompiler_Clang) 

  // all clang versions support typetraits 
  #define MCompiler_Has_TypeTraitsIntrinsics
  
  #if __has_feature(cxx_static_assert)
    #define MCompiler_Has_StaticAssert
  #endif
  #if __has_feature(cxx_rvalue_references)
    #define MCompiler_Has_RValue_References
  #endif
  #if __has_feature(cxx_auto_type)
    #define MCompiler_Has_Auto
  #endif
  #if __has_feature(cxx_decltype)
    #define MCompiler_Has_DeclType
  #endif
  #if __has_feature(cxx_lambdas)
    #define MCompiler_Has_Lambdas
  #endif
  #if __has_feature(cxx_noexcept)
    #define MCompiler_Has_NoExcept
  #endif
  
#elif defined(MCompiler_GCC)

  #if (__GNUC__ * 10 + __GNUC_MINOR__ >= 43) // GCC 4.3
    #define MCompiler_Has_TypeTraitsIntrinsics
  #endif
  
  #if defined(__GXX_EXPERIMENTAL_CXX0X__) // any GCC with '-std=c++0x'
    #define MCompiler_Has_StaticAssert
    #define MCompiler_Has_RValue_References
    #define MCompiler_Has_Auto
    #define MCompiler_Has_DeclType

    #if (__GNUC__ * 10 + __GNUC_MINOR__ >= 46) // GCC 4.6 with '-std=c++0x'
      #define MCompiler_Has_NoExcept
      #define MCompiler_Has_Lambdas
    #endif
  #endif

#else
  #error "Unknown or unsupported compiler"
#endif
 
// =================================================================================================

// noexcept

#if defined(MCompiler_Has_NoExcept)
  #define MNoExcept noexcept
  #define MNoExcept_If(Predicate) noexcept(Predicate)
#else
  #define MNoExcept
  #define MNoExcept_If(Predicate)
#endif

// =================================================================================================

// Library includes, inlining & aligning

#if defined(MCompiler_VisualCPP)

  #define MAligned(TypeName, Alignment) \
    __declspec(align(Alignment)) TypeName 

  #define MForceInline __forceinline
  
  #if defined(MDebug) 
    #define MAddLibrary(PathAndName) comment (lib, PathAndName "_d.lib")
  #else
    #define MAddLibrary(PathAndName) comment (lib, PathAndName ".lib")
  #endif

#elif defined(MCompiler_GCC)
  
  #define MAligned(TypeName, Alignment) \
    __attribute__ ((aligned(Alignment))) TypeName

  #if defined(MDebug)
    // else gdb will always stop at these inlined functions while stepping...
    #define MForceInline inline
  #else
    #define MForceInline inline __attribute__ ((always_inline))
  #endif
  
  #define MAddLibrary(PathAndName) diagnostic ignored "-Wunknown-pragmas" // :)

#else
  #pragma error "Unknown Compiler"
#endif

// =================================================================================================

// Warnings

#if defined(MCompiler_VisualCPP)
  // 'typedef' ignored on left of XX when no variable is declared
  #pragma warning(disable: 4091) 

  // unreferenced formal parameter
  #pragma warning(disable: 4100) 

  // alignment of a member was sensitive to packing
  #pragma warning(disable: 4121)

  // conditional expression is constant
  #pragma warning(disable: 4127)

  // nonstandard extension used : nameless struct/union
  #pragma warning(disable: 4201)

  // structure was padded due to __declspec(align())
  #pragma warning(disable: 4324)

  // new behavior: elements of array 'XXX' will be default initialized
  #pragma warning(disable: 4351)

  // 'this' used in base member initializer list
  #pragma warning(disable: 4355)

  // universal-character-name encountered in source (also in strings)
  #pragma warning(disable: 4428)

  // declaration of 'XXX' hides local declaration, member, global variable
  #pragma warning(disable: 4456 4457 4458 4459)

  // decorated name length exceeded, name was truncated
  #pragma warning(disable: 4503)

  // unreferenced local (member) function has been removed
  #pragma warning(disable: 4505) 

  // default,copy constructor & assignment could not be generated
  #pragma warning(disable: 4510 4511 4512) 

  // identifier was truncated to '255' characters in the debug information
  #pragma warning(disable: 4786) 

  // function 'XXX' marked as __forceinline not inlined
  #pragma warning(disable: 4714) 

  // XXX was declared deprecated
  #pragma warning(disable: 4996 4995)

#elif defined(MCompiler_GCC)
  // nothing

#else
  #error Unknown compiler
#endif

// =================================================================================================

// Other Platform Defines

#if defined(MCompiler_VisualCPP)

  #if (!defined(WINVER) || (WINVER < 0x0601))
    // our APIs require at least windows 7
    #error "Should define WINVER globally to compile fot Windows 7 or later"
  #endif

#endif

#endif // _CompilerDefines_h_

