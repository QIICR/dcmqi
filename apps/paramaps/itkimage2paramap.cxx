// CLP includes
#include "itkimage2paramapCLP.h"

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/ParaMapConverter.h"

#ifdef _WIN32
#include "dirent_win.h"
#else
#include <dirent.h>
#endif


int main(int argc, char *argv[])
{
  PARSE_ARGS;

  if (inputFileName.empty() || metaDataFileName.empty() || outputParaMapFileName.empty() )
  {
    return EXIT_FAILURE;
  }

  FloatReaderType::Pointer reader = FloatReaderType::New();
  reader->SetFileName(inputFileName.c_str());
  reader->Update();
  FloatImageType::Pointer parametricMapImage = reader->GetOutput();

  vector<DcmDataset*> dcmDatasets;

  DcmFileFormat* sliceFF = new DcmFileFormat();
  for(size_t dcmFileNumber=0; dcmFileNumber<dicomImageFileList.size(); dcmFileNumber++){
    if(sliceFF->loadFile(dicomImageFileList[dcmFileNumber].c_str()).good()){
      dcmDatasets.push_back(sliceFF->getAndRemoveDataset());
    }
  }

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

  DcmDataset* result = dcmqi::ParaMapConverter::itkimage2paramap(parametricMapImage, dcmDatasets, metadata);

  if (result == NULL) {
    return EXIT_FAILURE;
  } else {
    DcmFileFormat segdocFF(result);
    CHECK_COND(segdocFF.saveFile(outputParaMapFileName.c_str(), EXS_LittleEndianExplicit));

    COUT << "Saved parametric map as " << outputParaMapFileName << endl;
    return EXIT_SUCCESS;
  }
}
