// CLP includes
#include "itkimage2segimageCLP.h"

#include "ImageSEGConverter.h"

int main(int argc, char *argv[])
{
  PARSE_ARGS;
  return dcmqi::ImageSEGConverter::itkimage2dcmSegmentation(dicomImageFiles, segImageFiles, metaDataFileName,
                                                            outputSEGFileName);
}
