#pragma once

#ifndef _AudioMath_h_
#define _AudioMath_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"
#include "CoreTypes/Export/Array.h"
#include "CoreTypes/Export/Timer.h"

#include "AudioTypes/Export/AudioTypes.h"

// =================================================================================================

//! audible signal consts
#define MMinusInfInDb -200.0f

//! frequency helpers
#define MNyquistFrequency 22050.0f

// =================================================================================================

/*!
 * Collection of common audio related math functions and buffer operations.
 *
 * Buffer operations are implemented in SSE/Altivec but do also support 
 * unaligned in and output buffers.
!*/

namespace TAudioMath
{
  void Init();
  

  // ... TDisableSseDenormals 

  #if defined(MArch_X86) || defined(MArch_X64)

    /*!
     * Small helper class which enables SSE DAZ and FZ bits when constructed,
     * and restores the old mode when destructing
    !*/

    struct TDisableSseDenormals 
    {
      TDisableSseDenormals();
      ~TDisableSseDenormals();

    private:
      unsigned int mOldMXCSR;
    };

  #endif


  // ... Add

  void AddBuffer(
    const float*  pSourceBuffer,
    float*        pDestBuffer,
    int           NumberOfSamples);
  void AddBuffer(
    const double* pSourceBuffer,
    double*       pDestBuffer,
    int           NumberOfSamples);
  

  // ... Copy

  void CopyBuffer(
    const float*  pSourceBuffer,
    float*        pDestBuffer,
    int           NumberOfSamples);
  void CopyBuffer(
    const double* pSourceBuffer,
    double*       pDestBuffer,
    int           NumberOfSamples);

    
  // ... Scale

  void ScaleBuffer(
    float*  pSourceBuffer,
    int     NumberOfSamples,
    float   ScaleFactor);
  void ScaleBuffer(
    double* pSourceBuffer,
    int     NumberOfSamples,
    double  ScaleFactor);
    
  
  
  // ... Add Scaled

  void AddBufferScaled(
    const float*  pSourceBuffer,
    float*        pDestBuffer,
    int           NumberOfSamples,
    float         SrcScaleFactor);
  void AddBufferScaled(
    const double* pSourceBuffer,
    double*       pDestBuffer,
    int           NumberOfSamples,
    double        SrcScaleFactor);
    
  
    
  // ... Copy Scaled

  void CopyBufferScaled(
    const float*  pSourceBuffer,
    float*        pDestBuffer,
    int           NumberOfSamples,
    float         ScaleFactor);
  void CopyBufferScaled(
    const double* pSourceBuffer,
    double*       pDestBuffer,
    int           NumberOfSamples,
    float         ScaleFactor);

    
    
  // ... Products

  void MultiplyBuffers(
    const float*  pSourceBufferA,
    const float*  pSourceBufferB,
    float*        pDestBuffer,
    int           NumberOfSamples);
  void MultiplyBuffers(
    const double* pSourceBufferA,
    const double* pSourceBufferB,
    double*       pDestBuffer,
    int           NumberOfSamples);
    
    
  // ... Clear

  void ClearBuffer(
    float*  pBuffer,
    int     NumberOfSamples);
  void ClearBuffer(
    double* pBuffer,
    int     NumberOfSamples);
  

  // ... Magnitude and Power spectrum

  //! sqrt(re*re + img*img)
  void Magnitude(
    const float* pComplexBufferReal,
    const float* pComplexBufferImag,
    float*       pDestBinBuffer,
    int          NumberOfBins);
  void Magnitude(
    const double* pComplexBufferReal,
    const double* pComplexBufferImag,
    double*       pDestBinBuffer,
    int           NumberOfBins);

  //! re*re + img*img
  void PowerSpectrum(
    const float* pComplexBufferReal,
    const float* pComplexBufferImag,
    float*       pDestBinBuffer,
    int          NumberOfBins);
  void PowerSpectrum(
    const double* pComplexBufferReal,
    const double* pComplexBufferImag,
    double*       pDestBinBuffer,
    int           NumberOfBins);


  //! atan2(img, real);
  void Phase(
    const float* pComplexBufferReal,
    const float* pComplexBufferImag,
    float*       pDestBinBuffer,
    int          NumberOfBins);
  void Phase(
    const double* pComplexBufferReal,
    const double* pComplexBufferImag,
    double*       pDestBinBuffer,
    int           NumberOfBins);


  // ... Conversions

  //! convert a MIDI note value (69 = A4 = 440 Hz) to Hz
  double NoteFrequency(double Note);
  //! standard MIDI logarithmic note velocity volume conversion
  float NoteVolumeFromVelocity(int Velocity);

  //! convert a frequency in Hz to a note (69 = A4 = 440 Hz) and cents (+/-)
  void FrequencyToNote(double Frequency, int& Note, int& CentsDeviation);

  //! convert linear to dB gains (MMinusInfInDb for a linear gain of 0.0)
  //! FastLinToDb uses TMath::FastLog and has an error of ~0.00001
  float FastLinToDb(float Value);
  float LinToDb(float val);
  double LinToDb(double val);

  //! convert dB to linear gains
  //! FastDbToLin uses TMath::FastExp and has an error of ~0.00001
  float FastDbToLin(float val);
  float DbToLin(float val);
  double DbToLin(double val);

  //! helper funcion convert from/to samples to/from ms
  int MsToSamples(int SamplesPerSec, float Ms);
  float SamplesToMs(int SamplesPerSec, int Samples);
}

// =================================================================================================

#include "AudioTypes/Export/AudioMath.inl"


#endif // _AudioMath_h_
