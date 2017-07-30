//
// Created by Andrey Fedorov on 3/11/17.
//

#ifndef DCMQI_SEGMENTATIONIMAGEOBJECT_H
#define DCMQI_SEGMENTATIONIMAGEOBJECT_H

// DCMQI includes
#include "MultiframeObject.h"
#include "Helper.h"
#include "QIICRConstants.h"

// DCMTK includes
#include <dcmtk/dcmfg/fgderimg.h>
#include <dcmtk/dcmfg/fgseg.h>
#include <dcmtk/dcmseg/segdoc.h>
#include <dcmtk/dcmseg/segment.h>
#include <dcmtk/dcmseg/segutils.h>
#include <dcmtk/dcmdata/dcrledrg.h>

// ITK includes
#include <itkImageDuplicator.h>
#include <itkCastImageFilter.h>


class SegmentationImageObject : public MultiframeObject {

public:

  typedef short ShortPixelType;
  typedef itk::Image<ShortPixelType, 3> ShortImageType;

  SegmentationImageObject(){
    segmentation = NULL;
  }

  int initializeFromITK(vector<ShortImageType::Pointer>, const string &, vector<DcmDataset *>, bool);

  int initializeFromDICOM(DcmDataset* sourceDataset);

  int getDICOMRepresentation(DcmDataset& dcm){
    if(segmentation)
      CHECK_COND(segmentation->writeDataset(dcm));
  };
  map<unsigned,ShortImageType::Pointer> getITKRepresentation() const {
    // TODO: think about naming
    return segment2image;
  }

protected:

  typedef itk::CastImageFilter<ShortImageType,DummyImageType> ShortToDummyCasterType;
  // Data containers specific to this object
  ShortImageType::Pointer itkImage;
  vector<ShortImageType::Pointer> itkImages;

  // ITK images corresponding to the individual segments
  map<unsigned,ShortImageType::Pointer> segment2image;

  DcmSegmentation* segmentation;

  int initializeEquipmentInfo();
  int initializeVolumeGeometry();
  int initializeCompositeContext();

  // returns a vector with a size equal to the number of frames each holding segmentID and sliceNumber
  vector< pair<Uint16 , long> > matchFramesWithSegmentIdAndSliceNumber(FGInterface &fgInterface);

  int unpackFramesAndWriteSegmentImage(vector< pair<Uint16 , long> > matchingSegmentIDsAndSliceNumbers);
  int initializeMetaDataFromDICOM(DcmDataset*);
  int createDICOMSegmentation();
  int createNewSegmentImage(Uint16 segmentId);

  Json::Value getSegmentAttributesMetadata();

  IODGeneralEquipmentModule::EquipmentInfo generalEquipmentInfoModule;

  Uint16 getSegmentId(FGInterface &fgInterface, size_t frameId) const;

  template <typename ImageType, typename ImageTypePointer>
  vector<vector<int> > getSliceMapForSegmentation2DerivationImage(const vector<DcmDataset*> dcmDatasets,
                                                                  const ImageTypePointer &labelImage);

  bool hasDerivationImages(vector<vector<int> > &slice2derimg) const;
};


#endif //DCMQI_SEGMENTATIONIMAGEOBJECT_H
