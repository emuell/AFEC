#!/bin/bash

SCRIPT_DIR=`dirname -- "$0"`
cd "$SCRIPT_DIR"

./createModel.sh "./../../../afec-classifiers/OneShot-Categories"
