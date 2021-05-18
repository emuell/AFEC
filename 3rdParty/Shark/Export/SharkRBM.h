#pragma once

#ifndef _SharkRBM_h_
#define _SharkRBM_h_

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
#include <shark/Data/DataDistribution.h>
#include <shark/Data/Csv.h>
#include <shark/Data/Pgm.h>

#include <shark/Algorithms/Trainers/NormalizeComponentsUnitVariance.h>
#include <shark/Algorithms/GradientDescent/Rprop.h>
#include <shark/Algorithms/GradientDescent/SteepestDescent.h>

#include <shark/Models/FFNet.h>
#include <shark/Unsupervised/RBM/BipolarRBM.h>

#include <shark/ObjectiveFunctions/ErrorFunction.h>
#include <shark/ObjectiveFunctions/NoisyErrorFunction.h>
#include <shark/ObjectiveFunctions/Loss/SquaredLoss.h> 
#include <shark/ObjectiveFunctions/Loss/CrossEntropy.h>
#include <shark/ObjectiveFunctions/Loss/ZeroOneLoss.h>
#include <shark/ObjectiveFunctions/Regularizer.h>



#if defined(MCompiler_VisualCPP)
  #pragma warning(pop)

#elif defined(MCompiler_Clang)
  #pragma clang diagnostic pop
#endif


#endif // _SharkRBM_h_

