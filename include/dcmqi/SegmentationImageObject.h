//
// Created by Andrey Fedorov on 3/11/17.
//

#ifndef DCMQI_SEGMENTATIONIMAGEOBJECT_H
#define DCMQI_SEGMENTATIONIMAGEOBJECT_H

// DCMQI includes
#include "MultiframeObject.h"
#include "Helper.h"
#include "SegmentAttributes.h"

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
  typedef itk::Image<ShortPixelType, 3> ShortImageType;

  SegmentationImageObject(){
    segmentation = NULL;
  }

  int initializeFromDICOM(DcmDataset* sourceDataset);

  int initializeMetaDataFromDICOM(DcmDataset*);

protected:
  // Data containers specific to this object
  ShortImageType::Pointer itkImage;

  // ITK images corresponding to the individual segments
  map<unsigned,ShortImageType::Pointer> segment2image;

  dcmqi::SegmentAttributes* createAndGetNewSegment(unsigned labelID);

  // vector contains one item per input itkImageData label
  // each item is a map from labelID to segment attributes
  vector<map<unsigned,dcmqi::SegmentAttributes*> > segmentsAttributesMappingList;

  private:
  DcmSegmentation* segmentation;

};


#endif //DCMQI_SEGMENTATIONIMAGEOBJECT_H
