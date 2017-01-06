# Building `dcmqi` from source

`dcmqi` should build on Linux, Mac and Windows. You can confirm this is the case for the current version of the code by looking at the continuous integration (CI) platforms. 
- Linux build: [![Circle CI](https://circleci.com/gh/QIICR/dcmqi.svg?style=svg)](https://circleci.com/gh/QIICR/dcmqi)
- Windows build: [![Build status](https://ci.appveyor.com/api/projects/status/04l87y2j6prboap7?svg=true)](https://ci.appveyor.com/project/fedorov/dcmqi)
- Mac OS X build: [![TravisCI](https://travis-ci.org/QIICR/dcmqi.svg?branch=master)](https://travis-ci.org/QIICR/dcmqi)

Note that the failure icons indicate that _something_ in the dashboard script failed - this could be build error, failed test, failed artifact upload, or failed download of a prerequisite. You will need to check the console output for the specific platform to see if there are problems with the build.

## Checking out the source code

We use git/github to maintain the repository. You can clone the repository using this command:

`git clone https://github.com/QIICR/dcmqi.git`

If you are not familiar with git, there are many guides to help you get started, such as this one that should take about 10 minutes: https://guides.github.com/activities/hello-world/.

## Prerequisites

1. developer environment for your platform (compiler, git)
2. recent version of [cmake](https://cmake.org/download/)

Note that you can also see the specific components and steps needed to build dcmqi by looking at the CI configuration scripts ([circle.yml](https://github.com/QIICR/dcmqi/blob/master/circle.yml) for Lunux, [.travis.yml](https://github.com/QIICR/dcmqi/blob/master/.travis.yml) for Mac, and [appveyor.yml](https://github.com/QIICR/dcmqi/blob/master/appveyor.yml) for Windows). 

If you would like to run dcmqi tests, you will need to install few extra tools for validating JSON files. These tools depend on `npm` installed as part of [Node.js](https://docs.npmjs.com/getting-started/installing-node), and are the following:
- [jsonlint](https://github.com/circlecell/jsonlintdotcom)
- [ajv-cli](https://github.com/jessedc/ajv-cli)

Both can be installed with npm as follows:

``` shell
sudo npm install jsonlint -g
sudo npm install ajv-cli -g
```

## Superbuild approach

With the superbuild approach, all of the [dependencies](https://github.com/QIICR/dcmqi/tree/master/CMakeExternals) will be build as part of the build process. This approach is the easiest, but also most time-consuming.
1. create `dcmqi-superbuild` directory
2. configure the project by running `cmake <dcmqi source directory>` from `dcmqi-superbuild`
3. run `make` from the superbuild directory

## Non-superbuild approach

You can use this approach if you have (some of) the dependency libraries already built on your platform. dcmqi dependencies are
- [DCMTK](http://dcmtk.org)
- [ITK](http://itk.org)
- [SlicerExecutionModel](https://github.com/slicer/SlicerExecutionModel)
- [zlib](http://github.com/commontk/zlib)

To reuse builds of those libraries, you will need to pass the corresponding variables to `cmake` as shown in the example below:

```
cmake DITK_DIR:PATH=C:\ITK-install \ 
  -DSlicerExecutionModel_DIR:PATH=C:\SlicerExecutionModel\SlicerExecutionModel-build \
  -DDCMTK_DIR:PATH=C:\DCMTK-install\cmake  \
  -DZLIB_ROOT:PATH=c:\zlib-install \ 
  -DZLIB_INCLUDE_DIR:PATH=c:\zlib-install\include \ 
  -DZLIB_LIBRARY:FILEPATH=c:\zlib-install\lib\zlib.lib \
  <dcmqi source directory>
```
## Troubleshooting
- Under certain conditions, line endings may be incorrectly initialized for your platform by the checkout process (reported by @CJGoch in https://github.com/QIICR/dcmqi/issues/14), which may result in errors like below:

```
dcmqi/dcmqi-build/apps/seg/itkimage2segimageCLP.h:214:1: error: stray ‘\’ in program
{
^
dcmqi/dcmqi-build/apps/seg/itkimage2segimageCLP.h:214:3: warning: missing terminating " character
{
^
dcmqi/dcmqi-build/apps/seg/itkimage2segimageCLP.h:214:1: error: missing terminating " character
{
^
```

To resolve this, check your global git settings. If you have `autocrlf` set, you may try setting it to `auto`.
