// CLP includes
#include "segimage2itkimageCLP.h"
#include <itkSmartPointer.h>

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/Dicom2ItkConverter.h"
#include "dcmqi/internal/VersionConfigure.h"

// DCMTK includes
#include <dcmtk/oflog/configrt.h>

typedef dcmqi::Helper helper;
typedef itk::ImageFileWriter<ShortImageType> WriterType;


int main(int argc, char *argv[])
{
  std::cout << dcmqi_INFO << std::endl;

  PARSE_ARGS;

  if (verbose) {
    // Display DCMTK debug, warning, and error logs in the console
    dcmtk::log4cplus::BasicConfigurator::doConfigure();
  }

  if (mergeSegments && outputType != "nrrd") {
    std::cerr << "ERROR: mergeSegments option is only supported when output format is NRRD!" << std::endl;
    return EXIT_FAILURE;
  }

  if(helper::isUndefinedOrPathDoesNotExist(inputSEGFileName, "Input DICOM file")
     || helper::isUndefinedOrPathDoesNotExist(outputDirName, "Output directory"))
    return EXIT_FAILURE;

  DcmRLEDecoderRegistration::registerCodecs();

  DcmFileFormat sliceFF;
  CHECK_COND(sliceFF.loadFile(inputSEGFileName.c_str()));
  DcmDataset* dataset = sliceFF.getDataset();

  try {
    dcmqi::Dicom2ItkConverter converter;
    std::string metaInfo;
    OFCondition result  =  converter.dcmSegmentation2itkimage(dataset, metaInfo, mergeSegments);
    if (result.bad())
    {
      std::cerr << "ERROR: Failed to convert DICOM SEG to ITK image: " << result.text() << std::endl;
      return EXIT_FAILURE;
    }

    string outputPrefix = prefix.empty() ? "" : prefix + "-";

    string fileExtension = dcmqi::Helper::getFileExtensionFromType(outputType);
    itk::SmartPointer<ShortImageType> itkImage = converter.begin();
    size_t fileIndex = 1;
    while (itkImage)
    {
      stringstream imageFileNameSStream;
      cout << "Writing itk image to " << outputDirName << "/" << outputPrefix << fileIndex << fileExtension;
      imageFileNameSStream << outputDirName << "/" << outputPrefix << fileIndex << fileExtension;

      try {
        WriterType::Pointer writer = WriterType::New();
        writer->SetFileName(imageFileNameSStream.str().c_str());
        writer->SetInput(itkImage);
        writer->SetUseCompression(1);
        writer->Update();
        cout << " ... done" << endl;
      } catch (itk::ExceptionObject & error) {
        std::cerr << "fatal ITK error: " << error << std::endl;
        return EXIT_FAILURE;
      }
      itkImage = converter.next();
      fileIndex++;
    }

    stringstream jsonOutput;
    jsonOutput << outputDirName << "/" << outputPrefix << "meta.json";

    ofstream outputFile;
    outputFile.open(jsonOutput.str().c_str());
    outputFile << metaInfo;
    outputFile.close();

    return EXIT_SUCCESS;
  } catch (int e) {
    std::cerr << "Fatal error encountered." << std::endl;
    return EXIT_FAILURE;
  }
}
