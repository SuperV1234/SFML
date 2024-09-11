#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

killall python3
((cd $SCRIPT_DIR/build_vrdev_emcc && ninja $1 ; emrun ./bin/$1.html) || (cd $SCRIPT_DIR/build_vrdev_emcc && ninja $1 ; node ./bin/$1.js) &)
