#pragma once

#ifndef _SampleBuffer_h_
#define _SampleBuffer_h_

// =================================================================================================

#include "CoreTypes/Export/Array.h"

// =================================================================================================

/*! 
 * An 32 byte-aligned sample buffer with configurable amount of channels. 
 *
 * Channel buffers are also allocated in one memory block to avoid cache 
 * misses when processing multi-channel audio together.
!*/

template <size_t sNumberOfChannels>
class TSampleBuffer
{
public:
  TSampleBuffer(int InitialNumberOfFrames = 0);


  //@{ ... Properties

  //! Number of available channels (fixed)
  int NumberOfChannels()const;

  //! Buffer size in frames
  int NumberOfFrames()const;
  //! Set new buffer size. NB: Resizing does NOT clear the buffer.
  void SetNumberOfFrames(int NumberOfSamples);

  //! Release internal buffers (same as SetNumberOfFrames(0))
  void ReleaseBuffer();
  //@}

  
  //@{ ... Buffer access

  //! Access to the buffer
  const float* FirstRead(int Channel)const;
  float* FirstWrite(int Channel)const;

  //! Clear entire buffer with zeros.
  void Clear();
  //@}

private:
  int mNumberOfFrames;
  float* mChannelPointers[sNumberOfChannels];

  TAlignedFloatArray mBuffer;
};

// =================================================================================================

/*! 
 * A simple mono sample buffer with aligned buffers (see TSampleBuffer). 
!*/

class TMonoSampleBuffer : public TSampleBuffer<1>
{
public:
  TMonoSampleBuffer(int InitialNumberOfFrames = 0);


  //@{ ... Buffer access (overridden from TSampleBuffer)

  //! Access to the buffer
  const float* FirstRead()const;
  float* FirstWrite()const;
  //@}
};

// =================================================================================================

/*! 
 * A simple stereo sample buffer with aligned buffers (see TSampleBuffer).
!*/

class TStereoSampleBuffer : public TSampleBuffer<2>
{
public:
  TStereoSampleBuffer(int InitialNumberOfFrames = 0);


  //@{ ... Buffer access (overridden from TSampleBuffer)

  //! Access to the buffer
  const float* FirstReadLeft()const;
  float* FirstWriteLeft()const;

  const float* FirstReadRight()const;
  float* FirstWriteRight()const;
  //@}
};

// =================================================================================================

#include "AudioTypes/Export/SampleBuffers.inl"


#endif  // _SampleBuffer_h_

