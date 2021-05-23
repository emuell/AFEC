#!/bin/bash

SCRIPT_DIR=`dirname -- "$0"`
cd "$SCRIPT_DIR"

./createModel.sh "./../../../AFEC-Classifiers/OneShot-vs-Loops"
