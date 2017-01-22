#!/bin/bash

if [[ $1 = 'itkimage2paramap' ]]; then
   /usr/src/dcmqi-superbuild/dcmqi-build/bin/itkimage2paramap "${@:2}"
elif [[ $1 = 'paramap2itkimage' ]]; then
   /usr/src/dcmqi-superbuild/dcmqi-build/bin/paramap2itkimage "${@:2}"
elif [[ $1 = 'itkimage2segimage' ]]; then
  /usr/src/dcmqi-superbuild/dcmqi-build/bin/itkimage2segimage "${@:2}"
elif [[ $1 = 'segimage2itkimage' ]]; then
  /usr/src/dcmqi-superbuild/dcmqi-build/bin/segimage2itkimage "${@:2}"
elif [[ $1 = 'tid1500reader' ]]; then
   /usr/src/dcmqi-superbuild/dcmqi-build/bin/tid1500reader "${@:2}"
elif [[ $1 = 'tid1500writer' ]]; then
   /usr/src/dcmqi-superbuild/dcmqi-build/bin/tid1500writer "${@:2}"
else
  echo "Command not recognized!" \
    "Allowed commands: itkimage2paramap, " \
    "paramap2itkimage, itkimage2segimage, " \
    "segimage2itkimage, tid1500reader, " \
    "tid1500writer"
fi
