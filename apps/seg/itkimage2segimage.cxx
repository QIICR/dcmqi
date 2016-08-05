// CLP includes
#include "itkimage2segimageCLP.h"

#include "ImageSEGConverter.h"

int main(int argc, char *argv[])
{
  PARSE_ARGS;

  if (segImageFiles.empty() || dicomImageFiles.empty() || metaDataFileName.empty() || outputSEGFileName.empty() )
  {
    return EXIT_FAILURE;
  }

  ReaderType::Pointer reader = ReaderType::New();
  vector<ImageType::Pointer> segmentations;

  for(int segFileNumber=0; segFileNumber<segImageFiles.size(); segFileNumber++){
    reader->SetFileName(segImageFiles[segFileNumber]);
    reader->Update();
    ImageType::Pointer labelImage = reader->GetOutput();
    segmentations.push_back(labelImage);
  }

  vector<DcmDataset*> dcmDatasets;

  for(int dcmFileNumber=0; dcmFileNumber<dicomImageFiles.size(); dcmFileNumber++){
    DcmFileFormat* sliceFF = new DcmFileFormat();
    CHECK_COND(sliceFF->loadFile(dicomImageFiles[dcmFileNumber].c_str()));
    dcmDatasets.push_back(sliceFF->getDataset());

  }

  return dcmqi::ImageSEGConverter::itkimage2dcmSegmentation(dcmDatasets, segmentations, metaDataFileName,
                                                            outputSEGFileName);
}
