// CLP includes
#include "segimage2itkimageCLP.h"

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/ImageSEGConverter.h"

int main(int argc, char *argv[])
{
  PARSE_ARGS;

  DcmFileFormat sliceFF;
  CHECK_COND(sliceFF.loadFile(inputSEGFileName.c_str()));
  DcmDataset* dataset = sliceFF.getDataset();

  pair <map<unsigned,ImageType::Pointer>, string> result =  dcmqi::ImageSEGConverter::dcmSegmentation2itkimage(dataset);

  string outputPrefix = prefix.empty() ? "" : prefix + "-";

  for(map<unsigned,ImageType::Pointer>::const_iterator sI=result.first.begin();sI!=result.first.end();++sI){
    typedef itk::ImageFileWriter<ImageType> WriterType;
    stringstream imageFileNameSStream;

    if (niftiOutput) {
      imageFileNameSStream << outputDirName << "/" << outputPrefix << sI->first << ".nii.gz";
    } else {
      imageFileNameSStream << outputDirName << "/" << outputPrefix << sI->first << ".nrrd";
    }

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

}
