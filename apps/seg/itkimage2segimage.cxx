// CLP includes
#include "itkimage2segimageCLP.h"

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/SegmentationImageConverter.h"
#include "dcmqi/internal/VersionConfigure.h"

typedef dcmqi::Helper helper;

int main(int argc, char *argv[])
{
  std::cout << dcmqi_INFO << std::endl;
  
  PARSE_ARGS;

  if(helper::isUndefinedOrPathsDoNotExist(segImageFiles, "Input image files")
     || helper::isUndefinedOrPathDoesNotExist(metaDataFileName, "Input metadata file")
     || helper::isUndefined(outputSEGFileName, "Output DICOM file")) {
    return EXIT_FAILURE;
  }

  if(dicomImageFiles.empty() && dicomDirectory.empty()){
    cerr << "Error: No input DICOM files specified!" << endl;
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
    if (!helper::pathExists(dicomDirectory))
      return EXIT_FAILURE;
    vector<string> dicomFileList = helper::getFileListRecursively(dicomDirectory.c_str());
    dicomImageFiles.insert(dicomImageFiles.end(), dicomFileList.begin(), dicomFileList.end());
  }

  if(!helper::pathsExist(dicomImageFiles))
    return EXIT_FAILURE;

  vector<DcmDataset*> dcmDatasets = helper::loadDatasets(dicomImageFiles);

  if(dcmDatasets.empty()){
    cerr << "Error: no DICOM could be loaded from the specified list/directory" << endl;
    return EXIT_FAILURE;
  }

  ifstream metainfoStream(metaDataFileName.c_str(), ios_base::binary);
  std::string metadata( (std::istreambuf_iterator<char>(metainfoStream) ),
                       (std::istreambuf_iterator<char>()));
  DcmDataset* result = dcmqi::SegmentationImageConverter::itkimage2dcmSegmentation(dcmDatasets, segmentations, metadata, skipEmptySlices);

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

  for(size_t i=0;i<dcmDatasets.size();i++) {
    delete dcmDatasets[i];
  }
  if (result != NULL)
    delete result;
  return EXIT_SUCCESS;
}
