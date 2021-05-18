# AFEC


## Audio Feature Extraction

AFEC is a cross platform **audio feature extraction** and **sound classification** CLI tool written in C++. It analyzes audio files and saves a set of musically *interesting* audio-features into a sqlite database, which can then be used for other tasks - e.g. to organize sample libraries or to ease finding sounds with specific audio features (key, BPM, sound classes, RMS and so on). It's a CLI tool only to generate data - it does *NOT* provide a GUI to view the analyzed audio features or to preview audio files.

It was initially created for the [Sononym](https://sononym.net) project. This open sourced version got forked off from the initial release of Sononym at version 1.0. It's not compatible with Sononym's internal sample crawler and will not try to be in future. AFEC was released as an open source project, in the hope to be useful for other audio projects. The original authors of this project are no longer part of the Sononym project.


## Sound Classification with Machine Learning

ACEC is using a subset of the analyzed low-level audio-features to evaluate a pretrained, [bootstrap aggregated](https://en.wikipedia.org/wiki/Bootstrap_aggregating) gradient boosted machine ([LightGBM](https://github.com/Microsoft/LightGBM)) classification model. 

There are other experimental classification models in the source tree such as a simple [ANN](https://en.wikipedia.org/wiki/Artificial_neural_network), [RBM](https://en.wikipedia.org/wiki/Deep_belief_network), [SVM](https://en.wikipedia.org/wiki/Support-vector_machine), [KNN](https://en.wikipedia.org/wiki/K-nearest_neighbors_algorithm), [Naive Bayes](https://en.wikipedia.org/wiki/Naive_Bayes_classifier) and [Random Forest Tree](https://en.wikipedia.org/wiki/Random_forest) implemented in [Shark C++](http://image.diku.dk/shark/), but only the LightGBM model is used in production as pretrained model. So AFEC also can be used to experiment with audio classification. 

See [./Scripts/ModelCreator](./Scripts/ModelCreator) for scripts that train and test various classification models.

There are also various keras / tensorflow experiments in the [AFEC-Classifiers](https://github.com/emuell/afec-classifiers) repository, which are using the same data than the internal C++ tools. 


# Download

Prebuilt binaries can be downloaded [here](https://github.com/emuell/AFEC/releases/).


# Usage

## Crawler

```
Usage:
  Crawler[.exe] [options] <paths...>

Synopsis: 
  Recursively search for audio files in the path(s) and write high or low-level
  audio features into the given sqlite database.

Options:
  -h [ --help ]              Show help message.
  -v [ --version ]           Show version, build number and other infos.
  -l [ --level ] arg (=high) Create a 'high' or 'low' level database.
  -m [ --model ] arg         Specify the 'Classifiers' and 'OneShot-Categories'
                             model files that should be used for level='high'.
                             When not specified, the default models from the
                             crawler's resource dir are used. Set to 'none' to
                             explicitely avoid loading the a default model -
                             e.g. --model "None" --model "None" will disable
                             both.
  -o [ --out ] arg           Set destination directory/db_name.db or just a
                             directory. When only a directory is specified, the
                             database filename will be: 'afec-ll.db' or
                             'afec.db', depending on the level. When no
                             directory or file is specified, the database will
                             be written into the current working dir.
  --paths arg                One or more paths to a folder or single audio file
                             which should be analyzed. Can also be passed as
                             last (positional) argument.
                             When all given paths are sub paths of the 'out' db
                             path, all file paths within the database will be
                             relative to the out dir, else absolute paths.
```

## ModelTester

```
Usage:
  ModelTester[.exe] [options] <input.db>

Synopsis:
  Train and evaluate various classification models to see how they perform against
  the given input. Input is an AFEC low-level descriptor sqlite database which is
  used as train and test set.

Options:
  -h [ --help ]             Show help message.
  -a [ --all ] arg (=0)     When enabled, test all models instead of just the
                            the default model.
  -r [ --repeat ] arg (=10) Number of times the test should be repeated.
  -s [ --seed ] arg (=-1)   Set random seed, if any, in order to replicate tests.
  -b [ --bagging ] arg (=0) When enabled, test bagging ensemble models instead
                            of 'raw' ones.
  -i [ --src_database ] arg The low-level descriptor db file to create the
                            train and test data from. Can also be passed as
                            last (positional) argument.
```

## ModelCreator

```
Usage:
  ModelCreator[.exe] [options] <input.db>

Synopsis:
  Train and evaluate the default classification model which is defined in 
  `Source/Crawler/Export/DefaultClassificationModel.h` and create an ensemble
  model from the best performing ones.

Options:
  -h [ --help ]             Show help message.
  -r [ --repeat ] arg (=8)  Number of times to repeat the model creation with
                            different training set folds, to choose the best
                            one along all runs.
  -s [ --seed ] arg (=-1)   Random seed, if any, in order to replicate runs.
  -o [ --dest_model ] arg   Destination name and path of the resulting model
                            file.
                            When not specified, the model file will be written
                            into the crawler's resource directory.
  -i [ --src_database ] arg The low-level descriptor db file to create the
                            train and test data from. Can also be passed as
                            last (positional) argument.```
```


## Supported Audio File Formats

AFEC can read and thus analyze the following audio file-formats:

- Waveform Audio File (**.wav**): *Windows, OSX, Linux*
- Audio Interchange (**.aif, .aiff, .aifc**):	*Windows, OSX, Linux*
- Free Lossless Audio Codec (**.fla, .flac**): *Windows, OSX, Linux*
- OGG Vorbis (**.ogg**): *Windows, OSX, Linux*
- MPEG-1 Audio Layer 2 (**.mp2**): *Windows, OSX, Linux*
- MPEG-1 Audio Layer 3 (**.mp3**): *Windows, OSX, Linux*
- MPEG-4 Part 14 (**.mp4, .mp4a, .m4a**):	*Windows & OSX only*
- Core Audio Format (**.caf**): *OSX only*
- Windows Media Audio (**.wma**): *Windows only*
- NeXT/Sun Audio (**.au**): *Windows & OSX only*
- Advanced Audio Coding (**.aac**): *Windows & OSX only*
- Apple SouND (**.snd**):	*Windows & OSX only*


## Extracted Audio-Features

Internal analyzation **sample rate** currently is hardcoded to 44100 Hz.<br />
The **FFT Frame Size** is 2048 samples.<br />
The **FFT Hop Size** is 1024 samples.<br />


### High-Level Features

High-level features are written in a sqlite database which uses the following column names and types.<br/>
The column name ending specifies the data type (except for the first 3 columns):
- *S*: String
- *R*: Real number or integer
- *VR*: Vector of real numbers in JSON format
- *VVR*: Vector of a vector of real numbers in JSON format
- ...

#### Filepath and analyzation status
* `filename` *(TEXT)*:<br/>
  Absolute or relative path from the database path and name of the analyzed file.
* `modtime` *(INTEGER)*:<br/>
  File modification date in time_t units (unix timestamp).
* `status` *(TEXT)*:<br/>
  "succeeded" or some human readable error message, in case the file could not be opened or read.

#### Filetype info
* `file_type_S` *(TEXT)*:<br/>
  The file's normalized file extension.
* `file_size_R` *(INTEGER)*:<br/>
  The file's original raw size in bytes.
* `file_length_R` *(REAL)*:<br/>
  Audio stream's total length in seconds.
* `file_sample_rate_R` *(INTEGER)*:<br/>
  Sampling rate in HZ.
* `file_channel_count_R` *(INTEGER)*:<br/>
  Number of audio channels in the file.
* `file_bit_depth_R` *(INTEGER)*:<br/>
  Audio file-format bit depth.

#### Classification results
* `class_signature_VR` *(TEXT)*:<br/>
  JSON array of real numbers. Prediction result of the classification model.<br/>
  Can be used instad of the normalized `class_strengths_VR` to find similar sounds with a similar class signature.
* `classes_VS` *(TEXT: JSON_STRING_ARRAY)*:<br/>
  JSON array of strings. Name of "strong" predicted classes - strongest ones first.
* `class_strengths_VR` *(TEXT: JSON_TEXT_ARRAY)*:<br/>
  JSON array of real numbers. Normalized, clamped prediction result of the "strong" predicted classes - strongest ones first.

#### Categorization results
* `category_signature_VR` *(TEXT: JSON_NUMBER_ARRAY)*:<br/>
  JSON array of real numbers. Prediction result of the categorization model.<br/>
  Can be used instad of the normalized `category_strengths_VR` to find similar sounds with a similar category signature.
* `categories_VS` *(TEXT: JSON_STRING_ARRAY)*:<br/>
  JSON array of strings. Name of "strong" predicted categories - strongest ones first.
* `category_strengths_VR` *(TEXT: JSON_NUMBER_ARRAY)*:<br/>
  JSON array of real numbers. Normalized, clamped prediction result of the "strong" predicted categories - strongest ones first.
  
#### Pitch, Peak and BPM
* `base_note_R` *(REAL)*:<br/>
  Most dominant key note (if any) in the entire file. Should be used in combination with `base_note_confidence_R` only.
* `base_note_confidence_R` *(REAL)*:<br/>
  Normalized detection confidence value of the base note.
* `peak_db_R` *(REAL)*:<br/>
  Peak value accross all channels in dB.
* `rms_db_R` *(REAL)*:<br/>
  RMS value accross all channels in dB.
* `bpm_R` *(REAL)*:<br/>
  Most dominant BPM (if any) in the entire file accross all channels. Should be used in combination with `bpm_confidence_R` to be useful.
* `bpm_confidence_R` *(REAL)*:<br/>
  Normalized detection confidence value of the BPM detection.

#### Sound Characteristics
* `brightness_R` *(REAL)*:<br/>
  Overal sound's perceived brightness, calculated from the spectral centroid and rolloff.
* `noisiness_R` *(REAL)*:<br/>
  Overal sound's noisiness level, calculated from the spectral flatness.
* `harmonicity_R` *(REAL)*:<br/>
  Overal sound's harmonicity level, calculated from the auto correlation, pitch confidence and spectral flatness.

#### Spectum Arrays
* `spectrum_signature_VVR` *(TEXT: JSON_NUMBER_ARRAY_ARRAY)*:<br/>
  JSON array of an array of real numbers. 14 bands for 64 time frames (resampled), which can be used an iconic signature alike
  view of the entire audio file's spectrum.<br/>
* `spectrum_features_VR` *(TEXT: JSON_NUMBER_ARRAY)*:<br/>
  JSON array of an array of real numbers. 14 bands for each fft time frame (NOT resampled unlike the *spectrum_signature*)<br/>
  The 14 spectral bands end at 50, 100, 200, 400, 630, 920, 1270, 1720, 2320, 3150, 4400, 6400.0, 9500, 15500 HZ<br/>
* `tristimulus_VVR` *(TEXT: JSON_NUMBER_ARRAY_ARRAY)*:<br/>
  JSON array of an array of real numbers. 3 tristimulus values for for each fft time frame. Very fuzzy - should be used in 
  combination with the overall `base_note_confidence` or  `harmonicity` value only.<br/>
* `pitch_VR` *(TEXT: JSON_NUMBER_ARRAY)*:<br/>
  JSON array of real numbers. Cleaned pitch note values for for each fft time frame.
* `pitch_confidence_R` *(REAL)*:<br/>
  Mean value of all pitch note value detection confidences.
* `peak_VR` *(TEXT: JSON_NUMBER_ARRAY)*:<br/>
  JSON array of real numbers. Peak value in dB for for each fft time frame.


### Low-Level Features

Low-level features are written in a sqlite database which uses the following column names and types.<br/>
Just like for high-level features, the column name ending specifies the data type.

*_VVR* columns in the database are saved as binary [mspack](https://msgpack.org/) blobs, to save disk space. 

*Note*: All vector features contain the following additional statistical features as well:<br/>
`min`, `max`, `median`, `mean`, `gmean` (geographic mean), `variance`, `centroid`, `spread`, `skewness`, `kurtosis`, `flatness`, `dmean`, `dvariance` (1st deviation)

#### Filepath and analyzation status
* `filename` (absolute or relative path to the analyzed file)
* `modtime` (file modification date in unix timestamps)
* `status` ("succeeded" or some human readable error message)

#### Filetype info
* `file_type_S` (normalized file extension)
* `file_size_R` (bytes)
* `file_length_R` (seconds)
* `file_sample_rate_R` (Hz)
* `file_channel_count_R`
* `file_bit_depth_R`
  
#### Effective Length (skipping leading & trailing silence)
* `effectve_length_48dB_R` (gate > 48dB)
* `effectve_length_24dB_R`
* `effectve_length_12dB_R`

#### Amplitude
* `amplitude_silence_VR` (1 for silence, 0 for non silence)
* `amplitude_peak_VR`
* `amplitude_rms_VR`
* `amplitude_envelope_VR`

#### Spectral Features
* `spectral_rms_VR` (spectral [rms](https://www.audiocontentanalysis.org/code/audio-features/rms/))
* `spectral_centroid_VR` (spectral [centroid](https://www.audiocontentanalysis.org/code/audio-features/spectral-centroid/))
* `spectral_spread_VR` (spectral [spread](https://www.audiocontentanalysis.org/code/audio-features/spectral-spread/))
* `spectral_skewness_VR` (spectral [skewness](https://www.audiocontentanalysis.org/code/audio-features/spectral-skewness/))
* `spectral_kurtosis_VR` (spectral [kurtosis](https://www.audiocontentanalysis.org/code/audio-features/spectral-kurtosis/))
* `spectral_flatness_VR` (spectral [flatness](https://www.audiocontentanalysis.org/code/audio-features/spectral-flatness/))
* `spectral_rolloff_VR` (spectral [rolloff](https://www.audiocontentanalysis.org/code/audio-features/spectral-rolloff/))
* `spectral_flux_VR` (spectral [flux](https://www.audiocontentanalysis.org/code/audio-features/spectral-flux/))
* `spectral_inharmonicity_VR` ([inharmonicity](https://essentia.upf.edu/reference/streaming_Inharmonicity.html) based on a sharpened spectrum)
* `spectral_complexity_VR` (spectral [complexity](https://essentia.upf.edu/reference/streaming_SpectralComplexity.html) measure based on a sharpened spectrum)
* `spectral_contrast_VR` (spectral [contrast](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.583.7201&rep=rep1&type=pdf))

#### Fundamental Frequency
* `f0_VR` (in Hz for each FFT frame)
* `f0_confidence_VR` (0-1 for each F0)
* `failsafe_f0_VR` (falling back to last *stable* F0)

#### Tristimulus
* `tristimulus1_VR` (mixture of harmonics, [timbre](https://en.wikipedia.org/wiki/Timbre) based on the F0 detection)
* `tristimulus2_VR`
* `tristimulus3_VR`

#### auto correlation
* `auto_correlation_VR`

#### Onsets (tuned for mixed/tonal sounds)
* `rhythm_complex_onsets_VR` (onset value for each fft frame)
* `rhythm_complex_onset_count_R` (number of detected onsets)
* `rhythm_complex_onset_contrast_R`
* `rhythm_complex_onset_frequency_mean_R`
* `rhythm_complex_onset_strength_R` (overall strength)

#### Onsets (tuned for percussive sounds)
* `rhythm_percussive_onsets_VR` (see rhythm_complex)
* `rhythm_percussive_onset_count_R` 
* `rhythm_percussive_onset_contrast_R`
* `rhythm_percussive_onset_frequency_mean_R`
* `rhythm_percussive_onset_strength_R` 

#### Tempo
* `rhythm_complex_tempo_R` (BPM)
* `rhythm_complex_tempo_confidence_R` (0-1)
* `rhythm_percussive_tempo_R` 
* `rhythm_percussive_tempo_confidence_R`
* `rhythm_final_tempo_R`
* `rhythm_final_tempo_confidence_R`

#### Spectrum band features (14 bands)
* `spectral_rms_bands_VVR` (14 RMS values for every band - see also *Spectral Features*)
* `spectral_flatness_bands_VVR`
* `spectral_flux_bands_VVR`
* `spectral_complexity_bands_VVR`
* `spectral_contrast_bands_VVR`

#### Spectrum (28 bands)
* `frequency_bands_VVR` (50, 100, 150, 200, 300, 400, 510, 630, 770, 920, 
    1080, 1270, 1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400, 
    5300, 6400, 7700, 9500, 12000, 15500, 19000, 22050 Hz)

#### Cepstrum (14 bands)
* `cepstrum_bands_VVR` (MFCC values)


# Build

## Dependencies

### Windows

* cmake 3.2 or later
* VisualStudio 2015 or later with C++ support (C++14)

### OSX

* cmake 3.2 or later
* OSX 10.9 or later
* XCode with command line tools

### Linux

* cmake 3.2 or later (`apt-get install cmake` on Ubuntu)
* gcc-7.4 (ubuntu 16.04's default compiler, `apt-get install build-essentials` on Ubuntu).
* PkgConfig and Threads (usually already installed)
* libmpg123 headers and library (`apt-get install libmpg123-dev` on Ubuntu)


## Third-Party Libraries

AFEC uses the following third-party libraries, which are bundled in the `3rdParty` folder, including precompiled static libraries for Windows (Visual C++) OSX (Clang) and Linux (GCC). Note: if you're trying to build AFEC on Linux with gcc-8 or later, you may get linger errors and then need to recompile a few of the C++ third party libraries. There are build scripts in the `Linux/` sub folders in each third party library.

### Sound Classification:
* [SharkC++](http://image.diku.dk/shark/): Used for various classification test models and for the model ensemble generation.
* [LightGBM](https://github.com/Microsoft/LightGBM): The default classification model.
* [TinyDNN](https://github.com/tiny-dnn/tiny-dnn/): DNN experiments (should be removed).
### Audio Feature Extraction:
* [Aubio](https://aubio.org/): For pitch/key detection and partly for BPM detection.
* [LibXtract](https://www.jamiebullock.com/LibXtract/documentation/): To calculate Mel Frequency Cepstral Coefficients.
### Audio Files:
* [Resample](http://dominic-mazzoni.com/projects.html): Normalize sample rates of analyzed audio files. 
* [Flac](https://xiph.org/flac/): Flac file decoding.
* [OGG & OGG Vorbis](https://www.xiph.org/ogg/): OGG file decoding.
* [Mpg123](https://mpg123.de/api/): Mpg file decoding on Linux.
### Various:
* [Boost](https://www.boost.org/): Dependency of SharkC++ and used in various places internally.
* [Sqlite](https://sqlite.org/index.html): SQLite database support.
* [ZLib](http://zlib.net/): Dependency of Sqlite.
* [OpenBLAS](https://www.openblas.net/): Used on Windows as dependency of SharkC++
* [Msgpack](https://msgpack.org/): Optionally packing of JSON in sqlite database (disabled).
* [Iconv](http://www.gnu.org/software/libiconv/): Unicode string UTF8 and platform encoding.


## How to Build

go to `./Build` and run: 

```bash
./Build/build.sh|bat
```

The resulting Visual Studio (Windows), XCode (OSX) or Makefiles (Linux) files can then be found at `./Build/Out`.<br />
After building, the produced binaries can be found at `./Dist/[Debug|Release]`.


# Authors

AFEC was originally created by [Eduard Müller](https://github.com/emuell) and [Ingolf Wagner](https://github.com/mrVanDalo)


# License

GNU General Public License v3.0 or later
See [COPYING](./COPYING) to see the full text.

The bundled third-party libraries may use different licenses. Please have a look at the `3rdParty` folder to see which ones.


# Contributing

Patches are welcome: please fork the latest git repository and create a feature branch.
