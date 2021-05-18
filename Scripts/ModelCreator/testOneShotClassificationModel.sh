#!/bin/bash

SCRIPT_DIR=`dirname -- "$0"`
cd "$SCRIPT_DIR"

./testModel.sh "./../../../afec-classifiers/OneShot-Categories"
