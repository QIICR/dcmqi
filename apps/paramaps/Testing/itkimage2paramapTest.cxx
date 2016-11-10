#include "dcmqi/itkTestMain.h"

void RegisterTests()
{
  REGISTER_TEST(itkimage2paramapTest);
}

#undef main
#define main itkimage2paramapTest
#include "../itkimage2paramap.cxx"
