//
// Created by Andrey Fedorov on 3/11/17.
//

#include "dcmqi/ImageVolumeGeometry.h"

ImageVolumeGeometry::ImageVolumeGeometry() {
  rowDirection.set_size(3);
  columnDirection.set_size(3);
  sliceDirection.set_size(3);
  spacing.set_size(3);
  extent.set_size(3);
  origin.set_size(3);
}

int ImageVolumeGeometry::setSpacing(DoubleVectorType s) {
  for(int i=0;i<3;i++)
    spacing[i] = s[i];
  return EXIT_SUCCESS;
}

int ImageVolumeGeometry::setOrigin(PointType s) {
  for(int i=0;i<3;i++)
    origin[i] = s[i];
  return EXIT_SUCCESS;
}

int ImageVolumeGeometry::setExtent(SizeType s) {
  for (int i = 0; i < 3; i++)
    extent[i] = s[i];
  return EXIT_SUCCESS;
}

int ImageVolumeGeometry::setDirections(DirectionType d) {
  for (int i = 0; i < 3; i++)
    rowDirection[i] = d[i][0];
  for (int i = 0; i < 3; i++)
    columnDirection[i] = d[i][1];
  for (int i = 0; i < 3; i++)
    sliceDirection[i] = d[i][2];
  return EXIT_SUCCESS;
}

ImageVolumeGeometry::DummyImageType::Pointer ImageVolumeGeometry::getITKRepresentation(){
  ImageVolumeGeometry::DummyImageType::Pointer image;
  ImageVolumeGeometry::DummyImageType::IndexType index;
  ImageVolumeGeometry::DummyImageType::SizeType size;
  ImageVolumeGeometry::DummyImageType::DirectionType direction;
  ImageVolumeGeometry::DummyImageType::SpacingType spacing;
  ImageVolumeGeometry::DummyImageType::RegionType region;

  index.Fill(0);

  size[0] = extent[0];
  size[1] = extent[1];
  size[2] = extent[2];

  region.SetIndex(index);
  region.SetSize(size);

  spacing[0] = this->spacing[0];
  spacing[1] = this->spacing[1];
  spacing[2] = this->spacing[2];

  for (int i = 0; i < 3; i++)
    direction[i][0] = rowDirection[i];
  for (int i = 0; i < 3; i++)
    direction[i][1] = columnDirection[i];
  for (int i = 0; i < 3; i++)
    direction[i][2] = sliceDirection[i];

  image->SetDirection(direction);
  image->SetSpacing(spacing);

  return image;
}