// CLP includes
#include "itkimage2paramapCLP.h"

// DCMQI includes
#include "dcmqi/ParaMapConverter.h"

int main(int argc, char *argv[])
{
  PARSE_ARGS;

  if (inputFileName.empty() || metaDataFileName.empty() || outputParaMapFileName.empty() )
  {
    return EXIT_FAILURE;
  }

  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(inputFileName.c_str());
  reader->Update();
  ImageType::Pointer parametricMapImage = reader->GetOutput();

  DcmDataset* dcmDataset = NULL;
  if (!dicomImageFileName.empty()) {
    DcmFileFormat *sliceFF = new DcmFileFormat();
    CHECK_COND(sliceFF->loadFile(dicomImageFileName.c_str()));
    dcmDataset = sliceFF->getDataset();
  }

  ifstream metainfoStream(metaDataFileName.c_str(), ios_base::binary);
  std::string metadata( (std::istreambuf_iterator<char>(metainfoStream) ),
                        (std::istreambuf_iterator<char>()));

  DcmDataset* result = dcmqi::ParaMapConverter::itkimage2paramap(parametricMapImage, dcmDataset, metadata);

  if (result == NULL) {
    return EXIT_FAILURE;
  } else {
    DcmFileFormat segdocFF(result);
    CHECK_COND(segdocFF.saveFile(outputParaMapFileName.c_str(), EXS_LittleEndianExplicit));

    COUT << "Saved parametric map as " << outputParaMapFileName << endl;
    return EXIT_SUCCESS;
  }
}
