#!/bin/bash

BIN_PATH=/usr/src/dcmqi-superbuild/dcmqi-build/bin

CLIs=`cd $BIN_PATH;find . -type f -perm 755 | cut -d'/' -f 2 | tr '\n' ' '`

requestedCLI=${BIN_PATH}/${1}

if [ -x $requestedCLI]; then
  $requestedCLI ${@:2}
else
  echo "Command not recognized! Recognized dcmqi command line converters: ${CLIs}"
fi
