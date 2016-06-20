// CLP includes
#include "itkimage2segimageCLP.h"

#include "ImageSEGConverter.h"

int main(int argc, char *argv[])
{
    PARSE_ARGS;

    return dcmqi::ImageSEGConverter::itkimage2dcmSegmentation(inputDICOMImageFileNames, inputSegmentationsFileNames,
                                           metaDataFileName.c_str(), outputSEGFileName.c_str());
}
