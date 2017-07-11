//
// Created by Andrey Fedorov on 3/11/17.
//

#ifndef DCMQI_SEGMENTATIONIMAGEOBJECT_H
#define DCMQI_SEGMENTATIONIMAGEOBJECT_H

// DCMQI includes
#include "MultiframeObject.h"
#include "Helper.h"

// DCMTK includes
#include <dcmtk/dcmfg/fgderimg.h>
#include <dcmtk/dcmfg/fgseg.h>
#include <dcmtk/dcmseg/segdoc.h>
#include <dcmtk/dcmseg/segment.h>
#include <dcmtk/dcmseg/segutils.h>
#include <dcmtk/dcmdata/dcrledrg.h>

// ITK includes
#include <itkImageDuplicator.h>


class SegmentationImageObject : public MultiframeObject {

public:

  typedef short ShortPixelType;
  typedef itk::Image<ShortPixelType, 3> ShortITKImageType;

  SegmentationImageObject(){
    segmentation = NULL;
  }

  int initializeFromDICOM(DcmDataset* sourceDataset);

  map<unsigned,ShortITKImageType::Pointer> getITKRepresentation() const {
    // TODO: think about naming
    return segment2image;
  }

protected:
  // Data containers specific to this object
  ShortITKImageType::Pointer itkImage;

  // ITK images corresponding to the individual segments
  map<unsigned,ShortITKImageType::Pointer> segment2image;

  DcmSegmentation* segmentation;

  // returns a vector with a size equal to the number of frames each holding segmentID and sliceNumber
  vector< pair<Uint16 , long> > matchFramesWithSegmentIdAndSliceNumber(FGInterface &fgInterface);

  int unpackFramesAndWriteSegmentImage(vector< pair<Uint16 , long> > matchingSegmentIDsAndSliceNumbers);
  int initializeMetaDataFromDICOM(DcmDataset*);

  int createNewSegmentImage(Uint16 segmentId);

  Json::Value getSegmentAttributesMetadata();

  Uint16 getSegmentId(FGInterface &fgInterface, size_t frameId) const;
};


#endif //DCMQI_SEGMENTATIONIMAGEOBJECT_H
