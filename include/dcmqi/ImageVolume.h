//
// Created by Andrey Fedorov on 3/9/17.
//

#ifndef DCMQI_IMAGEVOLUME_H
#define DCMQI_IMAGEVOLUME_H

#include <vnl/vnl_vector.h>
#include <dcmtk/dcmfg/fginterface.h>
#include <dcmtk/dcmiod/modfloatingpointimagepixel.h>

#include <itkImage.h>

namespace dcmqi {

  // Maintain properties of the image volume
  // Attributes ara parallel to those of itkImageData, but limited to what we need for the task of conversion,
  // and the class is not templated over the pixel type, since we may not know the pixel type
  // at the time class is instantiated.
  //
  // Initially, limit implementation and support to just Float32 used by the original PM converter.

  class ImageVolume {
  public:
    // pixel types that are relevant for the types of objects we want to support
    enum {
      FLOAT32 = 0, // PM
      FLOAT64,     // PM
      UINT16       // PM or SEG
    };

    typedef IODFloatingPointImagePixelModule::value_type Float32PixelType;
    typedef itk::Image<Float32PixelType, 3> Float32ITKImageType;

    ImageVolume(){
      rowDirection.set_size(3);
      columnDirection.set_size(3);
      sliceDirection.set_size(3);
      origin.set_size(3);
      spacing.set_size(3);
      pixelData = NULL;
    }

    // while going from DICOM PM/SEG, we get volume information from FGInterface
    int initializeFromDICOM(FGInterface&);
    int initializeFromITK(Float32ITKImageType::Pointer);

    // TODO - inherited class? or separate segments before passing to this one?
    // int initializeFromSegment(FGInterface&, unsigned);

  protected:
    int initializeDirections(FGInterface &);
    int initializeExtent(FGInterface &);
    bool getDeclaredSliceSpacing(FGInterface&);
    bool getCalculatedSliceSpacing();

    int setDirections(vnl_vector<double> rowDirection, vnl_vector<double> columnDirection, vnl_vector<double> sliceDirection);
    int setOrigin(vnl_vector<double>);

  private:

    // use vnl_vector to simplify support of vector calculations
    vnl_vector<double> rowDirection, columnDirection, sliceDirection;
    vnl_vector<double> origin;
    unsigned sliceExtent;
    vnl_vector<double> spacing;
    void* pixelData;
    int pixelDataType;
  };

};


#endif //DCMQI_IMAGEVOLUME_H
