#!/bin/bash

# runs the modeltester for the specified classification model train data folder, 
# e.g "OneShot-vs-Loops" in the afec/classification-packs repository

SCRIPT_DIR=`dirname -- "$0"`
cd "$SCRIPT_DIR"

# ------------------------------------------------------------------------------------------------

# verify classifcation path argument
CLASSIFICATION_FOLDER=`realpath $1`
if [ ! -d "$CLASSIFICATION_FOLDER" ]; then
  echo "*** Missing or invalid argument: Can't find classification data set folder at '$1'"
  echo "*** It's expected next to this 'afec' repository. You can download the classification packs via:"
  echo "*** git clone --depth 1 https://git.renoise.com/afec/classification-packs.git \"afec-classifiers\""
  exit 1
fi

# verify dist dir
BIN_DIR=./../../Dist/Release
if [ ! -d "$BIN_DIR" ]; then
  echo "*** Can't find binaries at '$BIN_DIR'. Run crawler build scripts first."
  exit 1
fi

# ------------------------------------------------------------------------------------------------

# run crawler to create descriptors, if necessary
if [ ! -t "$CLASSIFICATION_FOLDER/afec-ll.db" ]; then
  $BIN_DIR/Crawler -l low -o "$CLASSIFICATION_FOLDER" "$CLASSIFICATION_FOLDER" || {
    echo "*** Crawler failed to run"
    exit 1
  }
fi

# run model tester
$BIN_DIR/ModelTester --repeat=5 --seed=300 --all=true --bagging=false "$CLASSIFICATION_FOLDER/afec-ll.db" || {
  echo "*** ModelTester failed to run"
  exit 1
}

# copy results: NB: assumes GBDT is the default model
cp -f ../../Projects/Crawler/XModelTester/Resources/Results/GBDT/* "$CLASSIFICATION_FOLDER" || {
  echo "*** Failed to copy model statistics"
  exit 1
}
