#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"
#include "CoreTypes/Export/Directory.h"

#include "CoreFileFormats/Test/TestAudioFile.h"
                  
// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreFileFormatsTest::AudioFile()
{
  const TDirectory TempDir = gTempDir(); 
  
  // make number of test samples random too but > 32
  const int NumTestSamples = 0x20 + (::rand() & 0x200);

  // no errors for 32bit files allowed
  const float Allowed32BitFloatError = 0.0f;
  const float Allowed32BitIntFloatError = 0.0f;
  const float Allowed24BitIntError = 1.0f / 256.0f;
  const float Allowed16BitIntError = 1.0f;
  const float Allowed8BitIntError = 256.0f;
  
  TArray<float> FloatSamplesLeft(NumTestSamples);
  TArray<float> FloatSamplesRight(NumTestSamples);
                                                 
  TArray<float> DestFloatSamplesLeft(NumTestSamples);
  TArray<float> DestFloatSamplesRight(NumTestSamples);

  // always enable loop mode, but choose a random mode
  const TAudioTypes::TLoopMode LoopMode = (TAudioTypes::TLoopMode)
    (1 + TRandom::Integer(TAudioTypes::kNumberOfLoopModes - 2));
  
  int LoopStart = TRandom::Integer(NumTestSamples);
  int LoopEnd = TRandom::Integer(NumTestSamples);
  if (LoopStart > LoopEnd)
  {
    TMathT<int>::Swap(LoopStart, LoopEnd);
  }
  // LoopStart must be != LoopEnd for wave files
  else if (LoopStart == LoopEnd) 
  { 
    if (LoopStart > 0) 
    {
      --LoopStart;
    }
    else
    {
      ++LoopEnd;
    }
  }

  for (int i = 0; i < NumTestSamples; ++i)
  {
    FloatSamplesLeft[i] = (float)((TMath::RandFloat() - 0.5f) * M16BitSampleRange);
    TMathT<float>::Clip(FloatSamplesLeft[i], M16BitSampleMin, M16BitSampleMax);

    FloatSamplesRight[i] = (float)((TMath::RandFloat() - 0.5f) * M16BitSampleRange);
    TMathT<float>::Clip(FloatSamplesRight[i], M16BitSampleMin, M16BitSampleMax);
  }

  
  // ... test sample converter 8,16,32 bit int conversion

  for (int i = 0; i < NumTestSamples; ++i)
  {
    const TInt8 Byte = TSampleConverter::S16BitFloatTo8BitSigned(
      FloatSamplesLeft[i]);
    const TInt8 ByteBack = TSampleConverter::S16BitFloatTo8BitSigned(
      TSampleConverter::S8BitSignedTo16BitFloat(Byte));
    BOOST_CHECK_EQUAL(Byte, ByteBack);

    const TUInt8 UByte = TSampleConverter::S16BitFloatTo8BitUnsigned(
      FloatSamplesLeft[i]);
    const TUInt8 UByteBack = TSampleConverter::S16BitFloatTo8BitUnsigned(
      TSampleConverter::S8BitUnsignedTo16BitFloat(UByte));
    BOOST_CHECK_EQUAL(UByte, UByteBack);

    const TInt16 Short = TSampleConverter::S16BitFloatTo16BitSigned(
      FloatSamplesLeft[i]);
    const TInt16 ShortBack = TSampleConverter::S16BitFloatTo16BitSigned(
      TSampleConverter::S16BitSignedTo16BitFloat(Short));
    BOOST_CHECK_EQUAL(Short, ShortBack);

    const TUInt16 UShort = TSampleConverter::S16BitFloatTo16BitUnsigned(
      FloatSamplesLeft[i]);
    const TUInt16 UShortBack = TSampleConverter::S16BitFloatTo16BitUnsigned(
      TSampleConverter::S16BitUnsignedTo16BitFloat(UShort));
    BOOST_CHECK_EQUAL(UShort, UShortBack);

    const TInt32 Long = TSampleConverter::S16BitFloatTo32BitSigned(
      FloatSamplesLeft[i]);
    const TInt32 LongBack = TSampleConverter::S16BitFloatTo32BitSigned(
      TSampleConverter::S32BitSignedTo16BitFloat(Long));
    BOOST_CHECK_EQUAL(Long, LongBack);

    const TUInt32 ULong = TSampleConverter::S16BitFloatTo32BitUnsigned(
      FloatSamplesLeft[i]);
    const TUInt32 ULongBack = TSampleConverter::S16BitFloatTo32BitUnsigned(
      TSampleConverter::S32BitUnsignedTo16BitFloat(ULong));
    BOOST_CHECK_EQUAL(ULong, ULongBack);
  }


  // ... test saving Waves (44100 and 96000, stereo and mono in native float format)

  TArray<const float*> StereoChannelPtrs(2);
  StereoChannelPtrs[0] = FloatSamplesLeft.FirstRead();
  StereoChannelPtrs[1] = FloatSamplesRight.FirstRead();

  TArray<const float*> MonoChannelPtrs(1);
  MonoChannelPtrs[0] = FloatSamplesLeft.FirstRead();

  TArray<float*> DestStereoChannelPtrs(2);
  DestStereoChannelPtrs[0] = DestFloatSamplesLeft.FirstWrite();
  DestStereoChannelPtrs[1] = DestFloatSamplesRight.FirstWrite();

  TArray<float*> DestMonoChannelPtrs(1);
  DestMonoChannelPtrs[0] = DestFloatSamplesLeft.FirstWrite();

  const bool Dither = true;

  TPtr<TWaveFile> p441000StereoWaveLoops(new TWaveFile());
  p441000StereoWaveLoops->SetLoop(LoopMode, LoopStart, LoopEnd);
  p441000StereoWaveLoops->OpenForWrite(TempDir.Path() + "44100StereoLoops.wav", 
    44100, TAudioFile::k32BitFloat, 2);
  p441000StereoWaveLoops->Stream()->WriteSamples(StereoChannelPtrs, NumTestSamples, Dither);
  p441000StereoWaveLoops->Close();

  TPtr<TWaveFile> p441000StereoWave(new TWaveFile());
  p441000StereoWave->OpenForWrite(TempDir.Path() + "44100Stereo.wav", 
    44100, TAudioFile::k32BitFloat, 2);
  p441000StereoWave->Stream()->WriteSamples(StereoChannelPtrs, NumTestSamples, Dither);
  p441000StereoWave->Close();
  
  TPtr<TWaveFile> p441000MonoWaveLoops(new TWaveFile());
  p441000MonoWaveLoops->SetLoop(LoopMode, LoopStart, LoopEnd);
  p441000MonoWaveLoops->OpenForWrite(TempDir.Path() + "44100MonoLoops.wav", 
    44100, TAudioFile::k32BitFloat, 1);
  p441000MonoWaveLoops->Stream()->WriteSamples(MonoChannelPtrs, NumTestSamples, Dither);
  p441000MonoWaveLoops->Close();

  TPtr<TWaveFile> p96000StereoWave(new TWaveFile());
  p96000StereoWave->OpenForWrite(TempDir.Path() + "96000Stereo.wav", 
    96000, TAudioFile::k32BitFloat, 2);
  p96000StereoWave->Stream()->WriteSamples(StereoChannelPtrs, NumTestSamples, Dither);
  p96000StereoWave->Close();

  TPtr<TWaveFile> p96000MonoWave(new TWaveFile());
  p96000MonoWave->OpenForWrite(TempDir.Path() + "96000Mono.wav", 
    96000, TAudioFile::k32BitFloat, 1);
  p96000MonoWave->Stream()->WriteSamples(MonoChannelPtrs, NumTestSamples, Dither);
  p96000MonoWave->Close();


  // ... test loading Wave: 44100 stereo and mono

  p441000StereoWaveLoops = new TWaveFile();
  p441000StereoWaveLoops->OpenForRead(TempDir.Path() + "44100StereoLoops.wav");
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops->LoopMode(), LoopMode);
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops->LoopStart(), LoopStart);
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops->LoopEnd(), LoopEnd);
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops->SamplingRate(), 44100);
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops->SampleType(), TAudioFile::k32BitFloat);
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops->NumChannels(), 2);
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops->NumSamples(), NumTestSamples);
  p441000StereoWaveLoops->Stream()->ReadSamples(DestStereoChannelPtrs, NumTestSamples);
  p441000StereoWaveLoops->Close();
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed32BitFloatError);
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesRight, FloatSamplesRight.Size(), 
    DestFloatSamplesRight, DestFloatSamplesRight.Size(), Allowed32BitFloatError);
  
  // test if loop points are correctly reset on reopening
  p441000StereoWaveLoops->OpenForRead(TempDir.Path() + "44100Stereo.wav");
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops->LoopMode(), TAudioTypes::kNoLoop);
  
  p441000MonoWaveLoops = new TWaveFile();
  p441000MonoWaveLoops->OpenForRead(TempDir.Path() + "44100MonoLoops.wav");
  BOOST_CHECK_EQUAL(p441000MonoWaveLoops->LoopMode(), LoopMode);
  BOOST_CHECK_EQUAL(p441000MonoWaveLoops->LoopStart(), LoopStart);
  BOOST_CHECK_EQUAL(p441000MonoWaveLoops->LoopEnd(), LoopEnd);
  BOOST_CHECK_EQUAL(p441000MonoWaveLoops->SamplingRate(), 44100);
  BOOST_CHECK_EQUAL(p441000MonoWaveLoops->SampleType(), TAudioFile::k32BitFloat);
  BOOST_CHECK_EQUAL(p441000MonoWaveLoops->NumChannels(), 1);
  BOOST_CHECK_EQUAL(p441000MonoWaveLoops->NumSamples(), NumTestSamples);
  p441000MonoWaveLoops->Stream()->ReadSamples(DestMonoChannelPtrs, NumTestSamples);
  p441000MonoWaveLoops->Close();

  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed32BitFloatError);


  p96000StereoWave = new TWaveFile();
  p96000StereoWave->OpenForRead(TempDir.Path() + "96000Stereo.wav");
  BOOST_CHECK_EQUAL(p96000StereoWave->LoopMode(), TAudioTypes::kNoLoop);
  BOOST_CHECK_EQUAL(p96000StereoWave->SamplingRate(), 96000);
  BOOST_CHECK_EQUAL(p96000StereoWave->SampleType(), TAudioFile::k32BitFloat);
  BOOST_CHECK_EQUAL(p96000StereoWave->NumChannels(), 2);
  BOOST_CHECK_EQUAL(p96000StereoWave->NumSamples(), NumTestSamples);
  p96000StereoWave->Stream()->ReadSamples(DestStereoChannelPtrs, NumTestSamples);
  p96000StereoWave->Close();

  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed32BitFloatError);
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesRight, FloatSamplesRight.Size(), 
    DestFloatSamplesRight, DestFloatSamplesRight.Size(), Allowed32BitFloatError);


  p96000MonoWave = new TWaveFile();
  p96000MonoWave->OpenForRead(TempDir.Path() + "96000Mono.wav");
  BOOST_CHECK_EQUAL(p96000MonoWave->LoopMode(), TAudioTypes::kNoLoop);
  BOOST_CHECK_EQUAL(p96000MonoWave->SamplingRate(), 96000);
  BOOST_CHECK_EQUAL(p96000MonoWave->SampleType(), TAudioFile::k32BitFloat);
  BOOST_CHECK_EQUAL(p96000MonoWave->NumChannels(), 1);
  BOOST_CHECK_EQUAL(p96000MonoWave->NumSamples(), NumTestSamples);
  p96000MonoWave->Stream()->ReadSamples(DestMonoChannelPtrs, NumTestSamples);
  p96000MonoWave->Close();
                 
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed32BitFloatError);
      


  // ... test saving Waves (44100 stereo in 32bit int format)

  TPtr<TWaveFile> p441000StereoWaveInt(new TWaveFile());
  p441000StereoWaveInt->OpenForWrite(TempDir.Path() + "44100StereoInt.wav", 
    44100, TAudioFile::k32BitInt, 2);
  p441000StereoWaveInt->Stream()->WriteSamples(StereoChannelPtrs, NumTestSamples, Dither);
  p441000StereoWaveInt->Close();


  // ... test loading Waves (44100 mono in 32bit int format)

  p441000StereoWaveInt = new TWaveFile();
  p441000StereoWaveInt->OpenForRead(TempDir.Path() + "44100StereoInt.wav");
  BOOST_CHECK_EQUAL(p441000StereoWaveInt->LoopMode(), TAudioTypes::kNoLoop);
  BOOST_CHECK_EQUAL(p441000StereoWaveInt->SamplingRate(), 44100);
  BOOST_CHECK_EQUAL(p441000StereoWaveInt->SampleType(), TAudioFile::k32BitInt);
  BOOST_CHECK_EQUAL(p441000StereoWaveInt->NumChannels(), 2);
  BOOST_CHECK_EQUAL(p441000StereoWaveInt->NumSamples(), NumTestSamples);
  p441000StereoWaveInt->Stream()->ReadSamples(DestStereoChannelPtrs, NumTestSamples);
  p441000StereoWaveInt->Close();
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed32BitIntFloatError);
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesRight, FloatSamplesRight.Size(), 
    DestFloatSamplesRight, DestFloatSamplesRight.Size(), Allowed32BitIntFloatError);


  // ... test saving FLAC (44100 stereo and 192000 mono in 32 bit)

  const bool NoDither = false;

  BOOST_CHECK(TFlacFile::SIsSampleRateSupported(44100));
  
  TPtr<TFlacFile> p441000StereoFlacLoops(new TFlacFile());
  p441000StereoFlacLoops->SetLoop(LoopMode, LoopStart, LoopEnd);
  p441000StereoFlacLoops->OpenForWrite(TempDir.Path() + "44100StereoLoops.flac", 
    44100, TAudioFile::k32BitInt, 2);
  p441000StereoFlacLoops->Stream()->WriteSamples(StereoChannelPtrs, NumTestSamples, NoDither);
  p441000StereoFlacLoops->Close();

  BOOST_CHECK(TFlacFile::SIsSampleRateSupported(192000));
  
  TPtr<TFlacFile> p192000MonoFlacLoops(new TFlacFile());
  p192000MonoFlacLoops->SetLoop(LoopMode, LoopStart, LoopEnd);
  p192000MonoFlacLoops->OpenForWrite(TempDir.Path() + "19200MonoLoops.flac", 
    192000, TAudioFile::k32BitInt, 1);
  p192000MonoFlacLoops->Stream()->WriteSamples(MonoChannelPtrs, NumTestSamples, NoDither);
  p192000MonoFlacLoops->Close();
  
  TPtr<TFlacFile> p192000MonoFlac(new TFlacFile());
  p192000MonoFlac->OpenForWrite(TempDir.Path() + "19200Mono.flac", 
    192000, TAudioFile::k32BitInt, 1);
  p192000MonoFlac->Stream()->WriteSamples(MonoChannelPtrs, NumTestSamples, NoDither);
  p192000MonoFlac->Close();


  // ... test loading FLAC: 44100 stereo and 192000 mono

  p441000StereoFlacLoops = new TFlacFile();
  p441000StereoFlacLoops->OpenForRead(TempDir.Path() + "44100StereoLoops.flac");
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops->LoopMode(), LoopMode);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops->LoopStart(), LoopStart);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops->LoopEnd(), LoopEnd);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops->SamplingRate(), 44100);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops->SampleType(), TAudioFile::k32BitInt);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops->NumChannels(), 2);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops->NumSamples(), NumTestSamples);
  p441000StereoFlacLoops->Stream()->ReadSamples(DestStereoChannelPtrs, NumTestSamples);
  p441000StereoFlacLoops->Close();
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed32BitIntFloatError);
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesRight, FloatSamplesRight.Size(), 
    DestFloatSamplesRight, DestFloatSamplesRight.Size(), Allowed32BitIntFloatError);

  // test if loop points are correctly reset on reopening
  p192000MonoFlacLoops->OpenForRead(TempDir.Path() + "19200Mono.flac");
  BOOST_CHECK_EQUAL(p192000MonoFlacLoops->LoopMode(), TAudioTypes::kNoLoop);
  
  p192000MonoFlacLoops = new TFlacFile();
  p192000MonoFlacLoops->OpenForRead(TempDir.Path() + "19200MonoLoops.flac");
  BOOST_CHECK_EQUAL(p192000MonoFlacLoops->LoopMode(), LoopMode);
  BOOST_CHECK_EQUAL(p192000MonoFlacLoops->LoopStart(), LoopStart);
  BOOST_CHECK_EQUAL(p192000MonoFlacLoops->LoopEnd(), LoopEnd);
  BOOST_CHECK_EQUAL(p192000MonoFlacLoops->SamplingRate(), 192000);
  BOOST_CHECK_EQUAL(p192000MonoFlacLoops->SampleType(), TAudioFile::k32BitInt);
  BOOST_CHECK_EQUAL(p192000MonoFlacLoops->NumChannels(), 1);
  BOOST_CHECK_EQUAL(p192000MonoFlacLoops->NumSamples(), NumTestSamples);
  p192000MonoFlacLoops->Stream()->ReadSamples(DestMonoChannelPtrs, NumTestSamples);
  p192000MonoFlacLoops->Close();

  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed32BitIntFloatError);


  // ... test saving Waves (44100 stereo in 24bit int format)

  TPtr<TWaveFile> p441000StereoWaveLoops24(new TWaveFile());
  p441000StereoWaveLoops24->OpenForWrite(TempDir.Path() + "44100Stereo24.wav", 
    44100, TAudioFile::k24Bit, 2);
  p441000StereoWaveLoops24->Stream()->WriteSamples(StereoChannelPtrs, NumTestSamples, Dither);
  p441000StereoWaveLoops24->Close();


  // ... test loading Waves (44100 stereo in 24bit int format)

  p441000StereoWaveLoops24 = new TWaveFile();
  p441000StereoWaveLoops24->OpenForRead(TempDir.Path() + "44100Stereo24.wav");
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops24->LoopMode(), TAudioTypes::kNoLoop);
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops24->SamplingRate(), 44100);
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops24->SampleType(), TAudioFile::k24Bit);
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops24->NumChannels(), 2);
  BOOST_CHECK_EQUAL(p441000StereoWaveLoops24->NumSamples(), NumTestSamples);
  p441000StereoWaveLoops24->Stream()->ReadSamples(DestStereoChannelPtrs, NumTestSamples);
  p441000StereoWaveLoops24->Close();
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed24BitIntError);
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesRight, FloatSamplesRight.Size(), 
    DestFloatSamplesRight, DestFloatSamplesRight.Size(), Allowed24BitIntError);


  // ... test saving 24bit FLAC: 44100 stereo

  TPtr<TFlacFile> p441000StereoFlacLoops24(new TFlacFile());
  p441000StereoFlacLoops24->OpenForWrite(TempDir.Path() + "44100Stereo24.flac", 
    44100, TAudioFile::k24Bit, 2);
  p441000StereoFlacLoops24->Stream()->WriteSamples(StereoChannelPtrs, NumTestSamples, NoDither);
  p441000StereoFlacLoops24->Close();


  // ... test loading 24bit FLAC: 44100 stereo

  p441000StereoFlacLoops24 = new TFlacFile();
  p441000StereoFlacLoops24->OpenForRead(TempDir.Path() + "44100Stereo24.flac");
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops24->LoopMode(), TAudioTypes::kNoLoop);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops24->SamplingRate(), 44100);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops24->SampleType(), TAudioFile::k24Bit);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops24->NumChannels(), 2);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops24->NumSamples(), NumTestSamples);
  p441000StereoFlacLoops24->Stream()->ReadSamples(DestStereoChannelPtrs, NumTestSamples);
  p441000StereoFlacLoops24->Close();
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed24BitIntError);
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesRight, FloatSamplesRight.Size(), 
    DestFloatSamplesRight, DestFloatSamplesRight.Size(), Allowed24BitIntError);


  // ... test saving 16bit WAV: 44100 stereo

  TPtr<TWaveFile> p441000StereoWav16(new TWaveFile());
  p441000StereoWav16->OpenForWrite(TempDir.Path() + "44100Stereo16.wav", 
    44100, TAudioFile::k16Bit, 2);
  p441000StereoWav16->Stream()->WriteSamples(StereoChannelPtrs, NumTestSamples, NoDither);
  p441000StereoWav16->Close();


  // ... test loading 16bit WAV: 44100 stereo

  p441000StereoWav16 = new TWaveFile();
  p441000StereoWav16->OpenForRead(TempDir.Path() + "44100Stereo16.wav");
  BOOST_CHECK_EQUAL(p441000StereoWav16->LoopMode(), TAudioTypes::kNoLoop);
  BOOST_CHECK_EQUAL(p441000StereoWav16->SamplingRate(), 44100);
  BOOST_CHECK_EQUAL(p441000StereoWav16->SampleType(), TAudioFile::k16Bit);
  BOOST_CHECK_EQUAL(p441000StereoWav16->NumChannels(), 2);
  BOOST_CHECK_EQUAL(p441000StereoWav16->NumSamples(), NumTestSamples);
  p441000StereoWav16->Stream()->ReadSamples(DestStereoChannelPtrs, NumTestSamples);
  p441000StereoWav16->Close();
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed16BitIntError);
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesRight, FloatSamplesRight.Size(), 
    DestFloatSamplesRight, DestFloatSamplesRight.Size(), Allowed16BitIntError);


  // ... test saving 16bit FLAC: 44100 stereo

  TPtr<TFlacFile> p441000StereoFlacLoops16(new TFlacFile());
  p441000StereoFlacLoops16->OpenForWrite(TempDir.Path() + "44100Stereo16.flac", 
    44100, TAudioFile::k16Bit, 2);
  p441000StereoFlacLoops16->Stream()->WriteSamples(StereoChannelPtrs, NumTestSamples, NoDither);
  p441000StereoFlacLoops16->Close();


  // ... test loading 16bit FLAC: 44100 stereo

  p441000StereoFlacLoops16 = new TFlacFile();
  p441000StereoFlacLoops16->OpenForRead(TempDir.Path() + "44100Stereo16.flac");
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops16->LoopMode(), TAudioTypes::kNoLoop);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops16->SamplingRate(), 44100);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops16->SampleType(), TAudioFile::k16Bit);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops16->NumChannels(), 2);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops16->NumSamples(), NumTestSamples);
  p441000StereoFlacLoops16->Stream()->ReadSamples(DestStereoChannelPtrs, NumTestSamples);
  p441000StereoFlacLoops16->Close();
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed16BitIntError);
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesRight, FloatSamplesRight.Size(), 
    DestFloatSamplesRight, DestFloatSamplesRight.Size(), Allowed16BitIntError);


  // ... test saving 8bit WAV: 44100 stereo

  TPtr<TWaveFile> p441000StereoWav8(new TWaveFile());
  p441000StereoWav8->OpenForWrite(TempDir.Path() + "44100Stereo8.wav", 
    44100, TAudioFile::k8BitUnsigned, 2);
  p441000StereoWav8->Stream()->WriteSamples(StereoChannelPtrs, NumTestSamples, NoDither);
  p441000StereoWav8->Close();


  // ... test loading 8bit WAV: 44100 stereo

  p441000StereoWav8 = new TWaveFile();
  p441000StereoWav8->OpenForRead(TempDir.Path() + "44100Stereo8.wav");
  BOOST_CHECK_EQUAL(p441000StereoWav8->LoopMode(), TAudioTypes::kNoLoop);
  BOOST_CHECK_EQUAL(p441000StereoWav8->SamplingRate(), 44100);
  BOOST_CHECK_EQUAL(p441000StereoWav8->SampleType(), TAudioFile::k8BitUnsigned);
  BOOST_CHECK_EQUAL(p441000StereoWav8->NumChannels(), 2);
  BOOST_CHECK_EQUAL(p441000StereoWav8->NumSamples(), NumTestSamples);
  p441000StereoWav8->Stream()->ReadSamples(DestStereoChannelPtrs, NumTestSamples);
  p441000StereoWav8->Close();
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed8BitIntError);
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesRight, FloatSamplesRight.Size(), 
    DestFloatSamplesRight, DestFloatSamplesRight.Size(), Allowed8BitIntError);


  // ... test saving 8bit FLAC: 44100 stereo

  TPtr<TFlacFile> p441000StereoFlacLoops8(new TFlacFile());
  p441000StereoFlacLoops8->OpenForWrite(TempDir.Path() + "44100Stereo8.flac", 
    44100, TAudioFile::k8BitSigned, 2);
  p441000StereoFlacLoops8->Stream()->WriteSamples(StereoChannelPtrs, NumTestSamples, NoDither);
  p441000StereoFlacLoops8->Close();


  // ... test loading 8bit FLAC: 44100 stereo

  p441000StereoFlacLoops8 = new TFlacFile();
  p441000StereoFlacLoops8->OpenForRead(TempDir.Path() + "44100Stereo8.flac");
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops8->LoopMode(), TAudioTypes::kNoLoop);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops8->SamplingRate(), 44100);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops8->SampleType(), TAudioFile::k8BitSigned);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops8->NumChannels(), 2);
  BOOST_CHECK_EQUAL(p441000StereoFlacLoops8->NumSamples(), NumTestSamples);
  p441000StereoFlacLoops8->Stream()->ReadSamples(DestStereoChannelPtrs, NumTestSamples);
  p441000StereoFlacLoops8->Close();
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesLeft, FloatSamplesLeft.Size(), 
    DestFloatSamplesLeft, DestFloatSamplesLeft.Size(), Allowed8BitIntError);
  
  BOOST_CHECK_ARRAYS_EQUAL_EPSILON(FloatSamplesRight, FloatSamplesRight.Size(), 
    DestFloatSamplesRight, DestFloatSamplesRight.Size(), Allowed8BitIntError);
}

