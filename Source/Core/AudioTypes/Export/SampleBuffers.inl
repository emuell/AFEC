#pragma once

#ifndef _SampleBuffers_inl_
#define _SampleBuffers_inl_

// =================================================================================================

#include "CoreTypes/Export/Memory.h"

#include "AudioTypes/Export/AudioMath.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

template <size_t sNumberOfChannels>
TSampleBuffer<sNumberOfChannels>::TSampleBuffer(int InitialNumberOfFrames)
  : mNumberOfFrames(-1) // force "SetNumberOfFrames" to clear everything
{
  SetNumberOfFrames(InitialNumberOfFrames);
}

// -------------------------------------------------------------------------------------------------

template <size_t sNumberOfChannels>
MForceInline int TSampleBuffer<sNumberOfChannels>::NumberOfFrames()const
{
  return mNumberOfFrames;
}

// -------------------------------------------------------------------------------------------------

template <size_t sNumberOfChannels>
void TSampleBuffer<sNumberOfChannels>::SetNumberOfFrames(int NumberOfFrames)
{
  MAssert(NumberOfFrames >= 0, "Invalid frame count");
  
  if (mNumberOfFrames != NumberOfFrames)
  {
    mNumberOfFrames = NumberOfFrames;
    
    if (NumberOfFrames > 0)
    {
      // keep each channel 32 byte aligned
      const int Step = 32 / sizeof(float);

      const int AllocatedFramesPerChannel = 
        (NumberOfFrames + (Step - 1)) / Step * Step;
      
      mBuffer.SetSize(AllocatedFramesPerChannel * 
        (int)sNumberOfChannels);

      // memorize channel ptrs
      for (size_t i = 0; i < sNumberOfChannels; ++i)
      {
        mChannelPointers[i] = mBuffer.FirstWrite() + 
          i * AllocatedFramesPerChannel;

        MAssert(((intptr_t)mChannelPointers[i] & 31) == 0, 
          "Should be aligned")
      }
    }
    else
    {
      mBuffer.Empty();

      for (size_t i = 0; i < sNumberOfChannels; ++i)
      {
        mChannelPointers[i] = NULL;
      }
    }
  }
}

// -------------------------------------------------------------------------------------------------

template <size_t sNumberOfChannels>
void TSampleBuffer<sNumberOfChannels>::ReleaseBuffer()
{
  SetNumberOfFrames(0);
}

// -------------------------------------------------------------------------------------------------

template <size_t sNumberOfChannels>
MForceInline int TSampleBuffer<sNumberOfChannels>::NumberOfChannels()const
{
  return sNumberOfChannels;
}

// -------------------------------------------------------------------------------------------------

template <size_t sNumberOfChannels>
MForceInline const float* TSampleBuffer<sNumberOfChannels>::FirstRead(int Channel)const
{
  MAssert(Channel >= 0 && Channel < (int)sNumberOfChannels, "");
  return mChannelPointers[Channel];
}

// -------------------------------------------------------------------------------------------------

template <size_t sNumberOfChannels>
MForceInline float* TSampleBuffer<sNumberOfChannels>::FirstWrite(int Channel)const
{
  MAssert(Channel >= 0 && Channel < (int)sNumberOfChannels, "");
  return mChannelPointers[Channel];
}



template <size_t sNumberOfChannels>
void TSampleBuffer<sNumberOfChannels>::Clear()
{
  if (! mBuffer.IsEmpty())
  {
    // clear entire buffer in one go
    TAudioMath::ClearBuffer(mBuffer.FirstWrite(), mBuffer.Size());
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline const float* TMonoSampleBuffer::FirstRead()const
{
  return TSampleBuffer<1>::FirstRead(0);
}

// -------------------------------------------------------------------------------------------------

MForceInline float* TMonoSampleBuffer::FirstWrite()const
{
  return TSampleBuffer<1>::FirstWrite(0);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

MForceInline const float* TStereoSampleBuffer::FirstReadLeft()const
{
  return TSampleBuffer<2>::FirstRead(0);
}

// -------------------------------------------------------------------------------------------------

MForceInline float* TStereoSampleBuffer::FirstWriteLeft()const
{
  return TSampleBuffer<2>::FirstWrite(0);
}

// -------------------------------------------------------------------------------------------------

MForceInline const float* TStereoSampleBuffer::FirstReadRight()const
{
  return TSampleBuffer<2>::FirstRead(1);
}

// -------------------------------------------------------------------------------------------------

MForceInline float* TStereoSampleBuffer::FirstWriteRight()const
{
  return TSampleBuffer<2>::FirstWrite(1);
}

#endif  // _SampleBuffers_inl_

