#ifndef DCMQI_SEGMENTATION_CONVERTER_H
#define DCMQI_SEGMENTATION_CONVERTER_H

// DCMTK includes
#include <dcmtk/dcmfg/fgderimg.h>
#include <dcmtk/dcmfg/fgseg.h>
#include <dcmtk/dcmseg/segdoc.h>
#include <dcmtk/dcmseg/segment.h>
#include <dcmtk/dcmseg/segutils.h>
#include <dcmtk/dcmdata/dcrledrg.h>

// ITK includes
#include <itkImageDuplicator.h>
#include <itkImageRegionConstIterator.h>
#include <itkLabelStatisticsImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkChangeInformationImageFilter.h>

// DCMQI includes
#include "dcmqi/ConverterBase.h"
#include "dcmqi/JSONSegmentationMetaInformationHandler.h"

using namespace std;


typedef itk::LabelImageToLabelMapFilter<ShortImageType> LabelToLabelMapFilterType;

namespace dcmqi {

  class ImageSEGConverter : public ConverterBase {

  public:
    static DcmDataset* itkimage2dcmSegmentation(vector<DcmDataset*> dcmDatasets,
                                                vector<ShortImageType::Pointer> segmentations,
                                                const string &metaData,
                                                bool skipEmptySlices=true);


    static pair<map<unsigned, ShortImageType::Pointer>, string> dcmSegmentation2itkimage(DcmDataset *segDataset);
    static map<unsigned, ShortImageType::Pointer> dcmSegmentation2itkimage(
        DcmSegmentation *segdoc,
        JSONSegmentationMetaInformationHandler *metaInfo=NULL);

    static void populateMetaInformationFromDICOM(DcmDataset *segDataset, DcmSegmentation *segdoc,
                                                 JSONSegmentationMetaInformationHandler &metaInfo);
  };

}

#endif //DCMQI_SEGMENTATION_CONVERTER_H
