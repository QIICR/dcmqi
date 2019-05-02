// CLP includes
#include "paramap2itkimageCLP.h"

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/ParaMapConverter.h"
#include "dcmqi/internal/VersionConfigure.h"


typedef dcmqi::Helper helper;


int main(int argc, char *argv[])
{
  std::cout << dcmqi_INFO << std::endl;

  PARSE_ARGS;

  if(helper::isUndefinedOrPathDoesNotExist(inputFileName, "Input DICOM file")
     || helper::isUndefinedOrPathDoesNotExist(outputDirName, "Output directory"))
    return EXIT_FAILURE;

  DcmFileFormat sliceFF;
  std::cout << "Opening input file " << inputFileName.c_str() << std::endl;
  CHECK_COND(sliceFF.loadFile(inputFileName.c_str()));
  DcmDataset* dataset = sliceFF.getDataset();

  try {
    pair <FloatImageType::Pointer, string> result =  dcmqi::ParaMapConverter::paramap2itkimage(dataset);

    string fileExtension = helper::getFileExtensionFromType(outputType);

    typedef itk::ImageFileWriter<FloatImageType> WriterType;
    string outputPrefix = prefix.empty() ? "" : prefix + "-";
    WriterType::Pointer writer = WriterType::New();
    stringstream imageFileNameSStream;
    imageFileNameSStream << outputDirName << "/" << outputPrefix << "pmap" << fileExtension;
    writer->SetFileName(imageFileNameSStream.str().c_str());
    writer->SetInput(result.first);
    writer->SetUseCompression(1);
    writer->Update();

    stringstream jsonOutput;
    jsonOutput << outputDirName << "/" << outputPrefix << "meta.json";

    ofstream outputFile;
    outputFile.open(jsonOutput.str().c_str());
    outputFile << result.second;
    outputFile.close();

    return EXIT_SUCCESS;
  } catch (int e) {
    std::cerr << "Fatal error encountered." << std::endl;
    return EXIT_FAILURE;
  }
}
