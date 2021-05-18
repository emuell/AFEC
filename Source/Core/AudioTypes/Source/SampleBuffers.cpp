#include "AudioTypesPrecompiledHeader.h"

#include <cstring> // memmove

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TMonoSampleBuffer::TMonoSampleBuffer(int InitialNumberOfFrames)
  : TSampleBuffer<1>(InitialNumberOfFrames)
{
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TStereoSampleBuffer::TStereoSampleBuffer(int InitialNumberOfFrames)
  : TSampleBuffer<2>(InitialNumberOfFrames)
{
}
