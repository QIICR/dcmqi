// CLP includes
#include "itkimage2paramapCLP.h"

#include "ParaMapConverter.h"

int main(int argc, char *argv[])
{
  PARSE_ARGS;

  return dcmqi::ParaMapConverter::paraMap2itkimage(inputFileName, metaDataFileName, outputParaMapFileName);
}
