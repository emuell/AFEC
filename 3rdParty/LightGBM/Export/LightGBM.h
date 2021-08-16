#pragma once

#ifndef _LightGBM_h_
#define _LightGBM_h_

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("LightGBM")
#endif

// =================================================================================================

#if defined(MCompiler_Clang)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif

#if defined(MWindows)
  #ifdef SIZE // defined in OpenBLAS
    #undef SIZE
  #endif
#endif

#include <LightGBM/utils/common.h>
#include <LightGBM/utils/text_reader.h>

#include <LightGBM/network.h>
#include <LightGBM/dataset.h>
#include <LightGBM/dataset_loader.h>
#include <LightGBM/boosting.h>
#include <LightGBM/objective_function.h>
#include <LightGBM/prediction_early_stop.h>
#include <LightGBM/metric.h>

#if defined(MCompiler_Clang)
  #pragma clang diagnostic pop
#endif


#endif // _LightGBM_h_

