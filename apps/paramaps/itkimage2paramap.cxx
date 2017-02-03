// CLP includes
#include "itkimage2paramapCLP.h"

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/ParaMapConverter.h"


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

  if(dicomDirectory.size()){
    vector<string> dicomFileList = dcmqi::Helper::getFileListRecursively(dicomDirectory.c_str());
    dicomImageFileList.insert(dicomImageFileList.end(), dicomFileList.begin(), dicomFileList.end());
  }

  vector<DcmDataset*> dcmDatasets = dcmqi::Helper::loadDatasets(dicomImageFileList);

  if(dcmDatasets.empty()){
    cerr << "Error: no DICOM could be loaded from the specified list/directory" << endl;
    return EXIT_FAILURE;
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
