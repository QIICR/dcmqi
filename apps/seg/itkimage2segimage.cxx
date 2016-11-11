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

  DcmFileFormat* sliceFF = new DcmFileFormat();
  for(int dcmFileNumber=0; dcmFileNumber<dicomImageFiles.size(); dcmFileNumber++){
    CHECK_COND(sliceFF->loadFile(dicomImageFiles[dcmFileNumber].c_str()));
    dcmDatasets.push_back(sliceFF->getAndRemoveDataset());
  }

  ifstream metainfoStream(metaDataFileName.c_str(), ios_base::binary);
  std::string metadata( (std::istreambuf_iterator<char>(metainfoStream) ),
                       (std::istreambuf_iterator<char>()));
  DcmDataset* result = dcmqi::ImageSEGConverter::itkimage2dcmSegmentation(dcmDatasets, segmentations, metadata, !noSkipEmptySlices);

  if (result == NULL){
    return EXIT_FAILURE;
  } else {
    DcmFileFormat segdocFF(result);
    bool compress = false;
    if(compress){
      CHECK_COND(segdocFF.saveFile(outputSEGFileName.c_str(), EXS_DeflatedLittleEndianExplicit));
    } else {
      CHECK_COND(segdocFF.saveFile(outputSEGFileName.c_str(), EXS_LittleEndianExplicit));
    }

    COUT << "Saved segmentation as " << outputSEGFileName << endl;
  }

  delete sliceFF;
  for(int i=0;i<dcmDatasets.size();i++) {
    delete dcmDatasets[i];
  }
  if (result != NULL)
    delete result;
  return EXIT_SUCCESS;
}
