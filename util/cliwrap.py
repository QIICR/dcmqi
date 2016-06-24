#!/usr/bin/env python
"""Allows traditional file-system based command line interface (cli)
programs to work on data pulled from urls

 cliwrap.py seg2itk http://ipfs.io/ipfs/QmTftaDyQCNFsceFwR2kkRMdz5VBbCzzD64VVEWiKRA2Go/BLSEGTest1/SEG/brainlab.dcm /tmp/out.nrrd

"""

import json
import os
import sys
import subprocess
import tempfile
import urllib

# directory to stage files to and from
workDirectory = tempfile.mkdtemp()

def stageFromHTTP(arg):
  fileName = os.path.split(arg)[1]
  filePath = tempfile.mktemp(suffix=fileName, dir=workDirectory)
  urllib.urlretrieve(arg, filePath)
  return filePath

def transformArg(arg):
  if arg.startswith('http://'):
    return stageFromHTTP(arg)
  return arg

transformedArgs = [sys.argv[1]]
for arg in sys.argv[2:]:
  transformedArgs.append(transformArg(arg))

cliProcess = subprocess.check_call(transformedArgs)
