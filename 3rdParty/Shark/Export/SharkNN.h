#pragma once

#ifndef _SharkNN_h_
#define _SharkNN_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("Shark")
  #pragma MAddLibrary("BoostSerialization")
  #pragma MAddLibrary("BoostSystem")

  #if defined(MRelease)
    #pragma MAddLibrary("OpenBLAS")
  #endif

  #pragma warning (push)
  #pragma warning (disable: 4800 4702 4297 4267 4244 4189 4100)

  // avoid #pragma library for boost libs
  #define BOOST_ALL_NO_LIB

#elif defined(MCompiler_Clang)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wunused-variable"
  #pragma clang diagnostic ignored "-Wshift-count-overflow"
#endif

// =================================================================================================

#if defined(MCompiler_VisualCPP) && defined(MRelease)
  #define SHARK_USE_OPENMP 1
  #define SHARK_USE_CBLAS 1
#endif

#include <shark/Data/Dataset.h>
#include <shark/Data/Csv.h>

#include <shark/Models/LinearModel.h>
#include <shark/Models/Normalizer.h>
#include <shark/Models/NearestNeighborClassifier.h>

#include <shark/Models/Trees/KDTree.h>
#include <shark/Models/Trees/KHCTree.h>

#include <shark/Models/Kernels/MonomialKernel.h>
#include <shark/Models/Kernels/GaussianRbfKernel.h>
#include <shark/Models/Kernels/LinearKernel.h>

#include <shark/Algorithms/NearestNeighbors/TreeNearestNeighbors.h>
#include <shark/Algorithms/NearestNeighbors/SimpleNearestNeighbors.h>
#include <shark/Algorithms/Trainers/NormalizeComponentsUnitVariance.h>


#include <shark/ObjectiveFunctions/Loss/ZeroOneLoss.h>

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma warning(pop)

#elif defined(MCompiler_Clang)
  #pragma clang diagnostic pop
#endif


#endif // _SharkNN_h_

