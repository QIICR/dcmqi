// CLP includes
#include "paramap2itkimageCLP.h"

#include "ParaMapConverter.h"

int main(int argc, char *argv[])
{
  PARSE_ARGS;

  return dcmqi::ParaMapConverter::paramap2itkimage(inputFileName, outputDirName);
}
