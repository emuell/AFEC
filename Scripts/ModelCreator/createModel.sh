#!/bin/bash

# runs the model creator for the specified classification model train data folder, 
# e.g "OneShot-vs-Loops" in the afec/classification-packs repository
# the resulting model is saved to the crawlers resource dirs, so the crawler will use
# it the next time it's creating afec.db high level databases. 

SCRIPT_DIR=`dirname -- "$0"`
cd "$SCRIPT_DIR"

# ------------------------------------------------------------------------------------------------

# verify classifcation path argument
CLASSIFICATION_FOLDER=`realpath $1`
if [ ! -d "$CLASSIFICATION_FOLDER" ]; then
  echo "*** Missing or invalid argument: Can't find classification data set folder at '$1'"
  echo "*** It's expected next to this 'afec' repository. You can download the classification packs via:"
  echo "*** git clone https://github.com/emuell/AFEC-Classifiers"
  exit 1
fi

# verify dist dir
BIN_DIR=./../../Dist/Release
if [ ! -d "$BIN_DIR" ]; then
  echo "*** Can't find binaries at '$BIN_DIR'. Run crawler build scripts first."
  exit 1
fi

# ------------------------------------------------------------------------------------------------

# run crawler to create low level descriptors, if needed
if [ ! -t "$CLASSIFICATION_FOLDER/afec-ll.db" ]; then
  $BIN_DIR/Crawler -l low -o "$CLASSIFICATION_FOLDER" "$CLASSIFICATION_FOLDER" || {
    echo "*** Crawler failed to run"
    exit 1
  }
fi

# train and write the model
$BIN_DIR/ModelCreator "$CLASSIFICATION_FOLDER/afec-ll.db" || {
  echo "*** ModelCreator failed to run"
  exit 1
}

