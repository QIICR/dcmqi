// CLP includes
#include "itkimage2paramapCLP.h"

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/ParaMapConverter.h"
#include "dcmqi/internal/VersionConfigure.h"


typedef dcmqi::Helper helper;

int main(int argc, char *argv[])
{
  std::cout << dcmqi_INFO << std::endl;

  PARSE_ARGS;

  if(helper::isUndefinedOrPathDoesNotExist(inputFileName, "Input image file")
     || helper::isUndefinedOrPathDoesNotExist(metaDataFileName, "Input metadata file")
     || helper::isUndefined(outputParaMapFileName, "Output DICOM file")) {
    return EXIT_FAILURE;
  }

  FloatReaderType::Pointer reader = FloatReaderType::New();
  reader->SetFileName(inputFileName.c_str());
  reader->Update();
  FloatImageType::Pointer parametricMapImage = reader->GetOutput();

  if(dicomDirectory.size()){
    if (!helper::pathExists(dicomDirectory))
      return EXIT_FAILURE;
    vector<string> dicomFileList = helper::getFileListRecursively(dicomDirectory.c_str());
    dicomImageFileList.insert(dicomImageFileList.end(), dicomFileList.begin(), dicomFileList.end());
  }

  vector<DcmDataset*> dcmDatasets = helper::loadDatasets(dicomImageFileList);

  if(dcmDatasets.empty()){
    cerr << "ERROR: no DICOM could be loaded from the specified list/directory" << endl;
    return EXIT_FAILURE;
  }

  ifstream metainfoStream(metaDataFileName.c_str(), ios_base::binary);
  std::string metadata( (std::istreambuf_iterator<char>(metainfoStream) ),
                        (std::istreambuf_iterator<char>()));

  try {
    DcmDataset* result = dcmqi::ParaMapConverter::itkimage2paramap(parametricMapImage, dcmDatasets, metadata);

    if (result == NULL) {
      std::cerr << "ERROR: Conversion failed." << std::endl;
      return EXIT_FAILURE;
    } else {
      DcmFileFormat segdocFF(result);
      CHECK_COND(segdocFF.saveFile(outputParaMapFileName.c_str(), EXS_LittleEndianExplicit));

      std::cout << "Saved parametric map as " << outputParaMapFileName << endl;
      return EXIT_SUCCESS;
    }
  } catch (int e) {
    std::cerr << "Fatal error encountered." << std::endl;
    return EXIT_FAILURE;
  }
}
