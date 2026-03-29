// CLP includes
#include "segimage2itkimageCLP.h"
#include <itkSmartPointer.h>

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/Dicom2ItkConverterBin.h"
#include "dcmqi/internal/VersionConfigure.h"

// DCMTK includes
#include <dcmtk/oflog/configrt.h>

typedef dcmqi::Helper helper;
typedef itk::ImageFileWriter<ShortImageType> ShortWriterType;
typedef itk::ImageFileWriter<CharImageType> CharWriterType;

template<typename TImageType, typename TNextFn>
int writeImages(itk::SmartPointer<TImageType> itkImage,
                TNextFn nextFn,
                const string& outputDirName, const string& outputPrefix,
                const string& fileExtension, size_t& fileIndex)
{
  typedef itk::ImageFileWriter<TImageType> WriterType;
  while (itkImage)
  {
    stringstream imageFileNameSStream;
    cout << "Writing itk image to " << outputDirName << "/" << outputPrefix << fileIndex << fileExtension;
    imageFileNameSStream << outputDirName << "/" << outputPrefix << fileIndex << fileExtension;

    try {
      typename WriterType::Pointer writer = WriterType::New();
      writer->SetFileName(imageFileNameSStream.str().c_str());
      writer->SetInput(itkImage);
      writer->SetUseCompression(1);
      writer->Update();
      cout << " ... done" << endl;
    } catch (itk::ExceptionObject & error) {
      std::cerr << "fatal ITK error: " << error << std::endl;
      return EXIT_FAILURE;
    }
    itkImage = nextFn();
    fileIndex++;
  }
  return EXIT_SUCCESS;
}


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

  DcmRLEDecoderRegistration::registerCodecs();

  DcmFileFormat sliceFF;
  std::cout << "Loading DICOM SEG file " << inputSEGFileName << std::endl;
  CHECK_COND(sliceFF.loadFile(inputSEGFileName.c_str()));
  DcmDataset* dataset = sliceFF.getDataset();

  try {
    // Get labelmap or binary segmentation converter based on the SOP Class UID of the input dataset
    std::unique_ptr<dcmqi::Dicom2ItkConverterBase> converter(dcmqi::Dicom2ItkConverter::getConverter(dataset));
    std::string metaInfo;
    OFCondition result  =  converter->dcmSegmentation2itkimage(dataset, metaInfo, mergeSegments);
    if (result.bad())
    {
      std::cerr << "ERROR: Failed to convert DICOM SEG to ITK image: " << result.text() << std::endl;
      return EXIT_FAILURE;
    }

    string outputPrefix = prefix.empty() ? "" : prefix + "-";
    string fileExtension = dcmqi::Helper::getFileExtensionFromType(outputType);
    size_t fileIndex = 1;

    int writeResult;
    bool is16Bit = converter->bytesPerPixel() > 1;
    if (is16Bit || !converter->isLabelmap())
    {
      writeResult = writeImages(converter->begin16Bit(),
        [&]() { return converter->next16Bit(); },
        outputDirName, outputPrefix, fileExtension, fileIndex);
    } else {
      writeResult = writeImages(converter->begin8Bit(),
        [&]() { return converter->next8Bit(); },
        outputDirName, outputPrefix, fileExtension, fileIndex);
    }
    if (writeResult != EXIT_SUCCESS)
      return writeResult;

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
