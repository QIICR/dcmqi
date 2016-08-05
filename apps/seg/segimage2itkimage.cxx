// CLP includes
#include "segimage2itkimageCLP.h"

#include "ImageSEGConverter.h"

int main(int argc, char *argv[])
{
  PARSE_ARGS;

  DcmFileFormat sliceFF;
  CHECK_COND(sliceFF.loadFile(inputSEGFileName.c_str()));
  DcmDataset* dataset = sliceFF.getDataset();

  return dcmqi::ImageSEGConverter::dcmSegmentation2itkimage(dataset, outputDirName);
}
