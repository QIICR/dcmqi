// CLP includes
#include "itkimage2segimageCLP.h"

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/ImageSEGConverter.h"


int main(int argc, char *argv[])
{
  PARSE_ARGS;

  if (segImageFiles.empty()){
    cerr << "Error: No input image files specified!" << endl;
    return EXIT_FAILURE;
  }

  if(dicomImageFiles.empty() && dicomDirectory.empty()){
    cerr << "Error: No input DICOM files specified!" << endl;
    return EXIT_FAILURE;
  }

  if(metaDataFileName.empty()){
    cerr << "Error: Input metadata file must be specified!" << endl;
    return EXIT_FAILURE;
  }

  if(outputSEGFileName.empty()){
    cerr << "Error: Output DICOM file must be specified!" << endl;
    return EXIT_FAILURE;
  }

  vector<ShortImageType::Pointer> segmentations;

  for(size_t segFileNumber=0; segFileNumber<segImageFiles.size(); segFileNumber++){
    ShortReaderType::Pointer reader = ShortReaderType::New();
    reader->SetFileName(segImageFiles[segFileNumber]);
    reader->Update();
    ShortImageType::Pointer labelImage = reader->GetOutput();
    segmentations.push_back(labelImage);
  }

  if(dicomDirectory.size()){
    OFList<OFString> fileList;
    cout << "Searching recursively " << dicomDirectory << " for DICOM files" << endl;
    if(OFStandard::searchDirectoryRecursively(dicomDirectory.c_str(), fileList)) {
      for(OFIterator<OFString> fileListIterator=fileList.begin(); fileListIterator!=fileList.end(); fileListIterator++) {
        dicomImageFiles.push_back((*fileListIterator).c_str());
      }
    }
  }

  vector<DcmDataset*> dcmDatasets;
  OFString tmp, sopInstanceUID;
  DcmFileFormat* sliceFF = new DcmFileFormat();
  for(size_t dcmFileNumber=0; dcmFileNumber<dicomImageFiles.size(); dcmFileNumber++){
    if(sliceFF->loadFile(dicomImageFiles[dcmFileNumber].c_str()).good()){
      DcmDataset* currentDataset = sliceFF->getAndRemoveDataset();
      currentDataset->findAndGetOFString(DCM_SOPInstanceUID, sopInstanceUID);
      bool exists = false;
      for(size_t i=0;i<dcmDatasets.size();i++) {
        dcmDatasets[i]->findAndGetOFString(DCM_SOPInstanceUID, tmp);
        if (tmp == sopInstanceUID) {
          cout << dicomImageFiles[dcmFileNumber].c_str() << " with SOPInstanceUID: " << sopInstanceUID
               << " already exists" << endl;
          exists = true;
          break;
        }
      }
      if (!exists) {
        dcmDatasets.push_back(currentDataset);
      }
    }
  }

  if(dcmDatasets.empty()){
    cerr << "Error: no DICOM could be loaded from the specified list/directory" << endl;
    return EXIT_FAILURE;
  }

  ifstream metainfoStream(metaDataFileName.c_str(), ios_base::binary);
  std::string metadata( (std::istreambuf_iterator<char>(metainfoStream) ),
                       (std::istreambuf_iterator<char>()));
  DcmDataset* result = dcmqi::ImageSEGConverter::itkimage2dcmSegmentation(dcmDatasets, segmentations, metadata, skipEmptySlices);

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
  for(size_t i=0;i<dcmDatasets.size();i++) {
    delete dcmDatasets[i];
  }
  if (result != NULL)
    delete result;
  return EXIT_SUCCESS;
}
