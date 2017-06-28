//
// Created by Andrey Fedorov on 3/11/17.
//

#ifndef DCMQI_IMAGEVOLUMEGEOMETRY_H
#define DCMQI_IMAGEVOLUMEGEOMETRY_H


#include <vnl/vnl_vector.h>
#include <itkVector.h>
#include <itkSize.h>
#include <itkImage.h>
#include <dcmtk/dcmdata/dcdatset.h>

class ImageVolumeGeometry {

  friend class MultiframeObject;
  friend class ParametricMapObject;

public:
  typedef itk::Vector<double,3> DoubleVectorType;
  typedef itk::Size<3> SizeType;

  //  implementation of the image volume geometry
  // NB: duplicated from MultiframeObject!
  typedef unsigned char DummyPixelType;
  typedef itk::Image<DummyPixelType, 3> DummyImageType;
  typedef DummyImageType::PointType PointType;
  typedef DummyImageType::DirectionType DirectionType;

  ImageVolumeGeometry();
  // initialize from DICOM
  ImageVolumeGeometry(DcmDataset*);

  int setSpacing(DoubleVectorType);
  int setOrigin(PointType);
  int setExtent(SizeType);
  int setDirections(DirectionType);

  template <typename T>
  typename T::Pointer getITKRepresentation(){
    typename T::Pointer image;
    typename T::IndexType index;
    typename T::SizeType size;
    typename T::DirectionType direction;
    typename T::SpacingType spacing;
    typename T::RegionType region;

    image = T::New();

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

protected:
  // use vnl_vector to simplify support of vector calculations
  vnl_vector<double> rowDirection, columnDirection, sliceDirection;
  vnl_vector<double> origin;
  vnl_vector<unsigned> extent;
  vnl_vector<double> spacing;

};


#endif //DCMQI_IMAGEVOLUMEGEOMETRY_H
