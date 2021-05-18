#pragma once

#ifndef _Msgpack_h
#define _Msgpack_h

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma pack(push)
  #pragma warning(push)
  #pragma warning (disable: 4702) // unreachable code

#elif defined(MCompiler_Clang)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wunreachable-code-return"
#endif

#include <msgpack.hpp>

#if defined(MCompiler_VisualCPP)
  #pragma warning(pop)
  #pragma pack(pop)

#elif defined(MCompiler_Clang)
  #pragma clang diagnostic pop
#endif


#endif // _Msgpack_h

