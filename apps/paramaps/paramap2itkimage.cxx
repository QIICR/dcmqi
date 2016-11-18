// CLP includes
#include "paramap2itkimageCLP.h"

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/ParaMapConverter.h"

int main(int argc, char *argv[])
{
  PARSE_ARGS;

  DcmFileFormat sliceFF;
  std::cout << "Opening input file " << inputFileName.c_str() << std::endl;
  CHECK_COND(sliceFF.loadFile(inputFileName.c_str()));
  DcmDataset* dataset = sliceFF.getDataset();

  pair <ImageType::Pointer, string> result =  dcmqi::ParaMapConverter::paramap2itkimage(dataset);

  typedef itk::ImageFileWriter<ImageType> WriterType;
  WriterType::Pointer writer = WriterType::New();
  stringstream imageFileNameSStream;
  imageFileNameSStream << outputDirName << "/" << "pmap.nrrd";
  writer->SetFileName(imageFileNameSStream.str().c_str());
  writer->SetInput(result.first);
  writer->SetUseCompression(1);
  writer->Update();

  stringstream jsonOutput;
  jsonOutput << outputDirName << "/" << "meta.json";

  ofstream outputFile;
  outputFile.open(jsonOutput.str().c_str());
  outputFile << result.second;
  outputFile.close();

  return EXIT_SUCCESS;
}
