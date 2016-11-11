// CLP includes
#include "segimage2itkimageCLP.h"

#include "ImageSEGConverter.h"

int main(int argc, char *argv[])
{
  PARSE_ARGS;

  DcmFileFormat sliceFF;
  CHECK_COND(sliceFF.loadFile(inputSEGFileName.c_str()));
  DcmDataset* dataset = sliceFF.getDataset();

  pair <map<unsigned,ImageType::Pointer>, string> result =  dcmqi::ImageSEGConverter::dcmSegmentation2itkimage(dataset);

  for(map<unsigned,ImageType::Pointer>::const_iterator sI=result.first.begin();sI!=result.first.end();++sI){
    typedef itk::ImageFileWriter<ImageType> WriterType;
    stringstream imageFileNameSStream;
    
    if (niftiOutput) {
      imageFileNameSStream << outputDirName << "/" << sI->first << ".nii.gz";  
    } else {
      imageFileNameSStream << outputDirName << "/" << sI->first << ".nrrd";
    }
    
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName(imageFileNameSStream.str().c_str());
    writer->SetInput(sI->second);
    //PW
    writer->SetUseCompression(1);
    writer->Update();
  }

  stringstream jsonOutput;
  jsonOutput << outputDirName << "/" << "meta.json";

  ofstream outputFile;
  outputFile.open(jsonOutput.str().c_str());
  outputFile << result.second;
  outputFile.close();

  return EXIT_SUCCESS;

}
