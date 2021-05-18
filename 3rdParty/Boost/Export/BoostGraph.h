#pragma once

#if !defined(_BoostGraph_h_)
#define _BoostGraph_h_

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma pack(push)
  #pragma warning(push)
  #pragma warning (disable: 4610) // 'X' can never be instantiated - user defined 

#elif defined(MCompiler_Clang)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wunused-variable"
#endif

#include <vector>

// include more, as soon as we need more...
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>

#if defined(MCompiler_VisualCPP)
  #pragma warning(pop)
  #pragma pack(pop)

#elif defined(MCompiler_Clang)
  #pragma clang diagnostic pop
#endif


#endif // _BoostGraph_h_

