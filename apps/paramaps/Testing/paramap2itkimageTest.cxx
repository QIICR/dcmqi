#include "dcmqi/itkTestMain.h"

void RegisterTests()
{
  REGISTER_TEST(paramap2itkimageTest);
}

#undef main
#define main paramap2itkimageTest
#include "../paramap2itkimage.cxx"
