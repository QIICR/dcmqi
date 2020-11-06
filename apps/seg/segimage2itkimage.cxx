// CLP includes
#include "segimage2itkimageCLP.h"

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/ImageSEGConverter.h"
#include "dcmqi/internal/VersionConfigure.h"

// DCMTK includes
#include <dcmtk/oflog/configrt.h>

typedef dcmqi::Helper helper;


int main(int argc, char *argv[])
{
  std::cout << dcmqi_INFO << std::endl;

  PARSE_ARGS;

  if (verbose) {
    // Display DCMTK debug, warning, and error logs in the console
    dcmtk::log4cplus::BasicConfigurator::doConfigure();
  }

  if(helper::isUndefinedOrPathDoesNotExist(inputSEGFileName, "Input DICOM file")
     || helper::isUndefinedOrPathDoesNotExist(outputDirName, "Output directory"))
    return EXIT_FAILURE;

  DcmFileFormat sliceFF;
  CHECK_COND(sliceFF.loadFile(inputSEGFileName.c_str()));
  DcmDataset* dataset = sliceFF.getDataset();

  try {
    pair <map<unsigned,ShortImageType::Pointer>, string> result =  dcmqi::ImageSEGConverter::dcmSegmentation2itkimage(dataset);

    string outputPrefix = prefix.empty() ? "" : prefix + "-";

    string fileExtension = dcmqi::Helper::getFileExtensionFromType(outputType);

    for(map<unsigned,ShortImageType::Pointer>::const_iterator sI=result.first.begin();sI!=result.first.end();++sI){
      typedef itk::ImageFileWriter<ShortImageType> WriterType;
      stringstream imageFileNameSStream;

      imageFileNameSStream << outputDirName << "/" << outputPrefix << sI->first << fileExtension;

      WriterType::Pointer writer = WriterType::New();
      writer->SetFileName(imageFileNameSStream.str().c_str());
      writer->SetInput(sI->second);
      writer->SetUseCompression(1);
      writer->Update();
    }

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
