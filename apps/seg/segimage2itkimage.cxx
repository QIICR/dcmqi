// CLP includes
#include "segimage2itkimageCLP.h"

#include "ImageSEGConverter.h"

int main(int argc, char *argv[])
{
  PARSE_ARGS;
  return dcmqi::ImageSEGConverter::dcmSegmentation2itkimage(inputSEGFileName.c_str(), outputDirName.c_str());
}