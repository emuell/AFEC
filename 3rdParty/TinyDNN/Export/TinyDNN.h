#pragma once

#ifndef _TinyDNN_h_
#define _TinyDNN_h_

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma warning (push)
  #pragma warning (disable: 4702 4244 4752)
  
#elif defined(MCompiler_Clang)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wunused-variable"
#endif

// =================================================================================================

// tinyDNN config

// NB: OMP makes things worse. tinydnn hase a native fallback impl
// #define CNN_USE_OMP 1 

#define CNN_USE_SSE 1 
// NB: does not give us a major performance boost, but adds requirements (AVX2 CPU)
// #define CNN_USE_AVX 1 


// tinyDNN includes
#include "tiny_dnn/config.h"
#include "tiny_dnn/network.h"
#include "tiny_dnn/nodes.h"

#include "tiny_dnn/core/framework/device.h"
#include "tiny_dnn/core/framework/program_manager.h"

#include "tiny_dnn/layers/input_layer.h"
#include "tiny_dnn/layers/feedforward_layer.h"
#include "tiny_dnn/layers/convolutional_layer.h"
// #include "tiny_dnn/layers/quantized_convolutional_layer.h"
// #include "tiny_dnn/layers/deconvolutional_layer.h"
// #include "tiny_dnn/layers/quantized_deconvolutional_layer.h"
#include "tiny_dnn/layers/fully_connected_layer.h"
// #include "tiny_dnn/layers/quantized_fully_connected_layer.h"
// #include "tiny_dnn/layers/average_pooling_layer.h"
#include "tiny_dnn/layers/max_pooling_layer.h"
// #include "tiny_dnn/layers/linear_layer.h"
// #include "tiny_dnn/layers/lrn_layer.h"
#include "tiny_dnn/layers/dropout_layer.h"
// #include "tiny_dnn/layers/arithmetic_layer.h"
// #include "tiny_dnn/layers/concat_layer.h"
// #include "tiny_dnn/layers/max_unpooling_layer.h"
// #include "tiny_dnn/layers/average_unpooling_layer.h"
// #include "tiny_dnn/layers/batch_normalization_layer.h"
// #include "tiny_dnn/layers/slice_layer.h"
// #include "tiny_dnn/layers/power_layer.h"

#include "tiny_dnn/activations/activation_function.h"
#include "tiny_dnn/lossfunctions/loss_function.h"
#include "tiny_dnn/optimizers/optimizer.h"

#include "tiny_dnn/util/weight_init.h"
#include "tiny_dnn/util/serialization_helper.h"
#include "tiny_dnn/util/image.h"

#include "tiny_dnn/io/layer_factory.h"

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma warning(pop)

#elif defined(MCompiler_Clang)
  #pragma clang diagnostic pop
#endif


#endif // _TinyDNN_h_

