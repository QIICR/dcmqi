// CLP includes
#include "itkimage2segimageCLP.h"

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/ImageSEGConverter.h"

#ifdef _WIN32
#include "dirent_win.h"
#else
#include <dirent.h>
#endif

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

  vector<DcmDataset*> dcmDatasets;

  DcmFileFormat* sliceFF = new DcmFileFormat();
  for(size_t dcmFileNumber=0; dcmFileNumber<dicomImageFiles.size(); dcmFileNumber++){
    if(sliceFF->loadFile(dicomImageFiles[dcmFileNumber].c_str()).good()){
      dcmDatasets.push_back(sliceFF->getAndRemoveDataset());
    }
  }

  /*
  from: https://github.com/cxong/tinydir
  did not work for me on mac!
  if(dicomDirectory.size()){
    cout << dicomDirectory << endl;

    tinydir_dir dir;
    tinydir_open(&dir, dicomDirectory.c_str());
    while(dir.has_next){
      tinydir_file file;
      tinydir_readfile(&dir, &file);
      cout << file.name << endl;
    }
  }*/

  // solution from
  //  http://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
  if(dicomDirectory.size()){
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (dicomDirectory.c_str())) != NULL) {
      while ((ent = readdir (dir)) != NULL) {
        if(sliceFF->loadFile((dicomDirectory+"/"+ent->d_name).c_str()).good()){
          dcmDatasets.push_back(sliceFF->getAndRemoveDataset());
        }
      }
      closedir (dir);
    } else {
      cerr << "Cannot open input DICOM directory!" << endl;
    }
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
