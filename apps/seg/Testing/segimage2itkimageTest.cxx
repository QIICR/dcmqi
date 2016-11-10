#include "dcmqi/itkTestMain.h"

void RegisterTests()
{
  REGISTER_TEST(segimage2itkimageTest);
}

#undef main
#define main segimage2itkimageTest
#include "../segimage2itkimage.cxx"
