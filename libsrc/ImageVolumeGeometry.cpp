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