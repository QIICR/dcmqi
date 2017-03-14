//
// Created by Andrey Fedorov on 3/11/17.
//

#ifndef DCMQI_IMAGEVOLUMEGEOMETRY_H
#define DCMQI_IMAGEVOLUMEGEOMETRY_H


#include <vnl/vnl_vector.h>
#include <itkVector.h>
#include <itkSize.h>
#include <itkImage.h>

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

  int setSpacing(DoubleVectorType);
  int setOrigin(PointType);
  int setExtent(SizeType);
  int setDirections(DirectionType);

protected:
  // use vnl_vector to simplify support of vector calculations
  vnl_vector<double> rowDirection, columnDirection, sliceDirection;
  vnl_vector<double> origin;
  vnl_vector<unsigned> extent;
  vnl_vector<double> spacing;

};


#endif //DCMQI_IMAGEVOLUMEGEOMETRY_H
