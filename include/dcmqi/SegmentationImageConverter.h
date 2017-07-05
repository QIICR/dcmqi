#ifndef DCMQI_SEGMENTATION_CONVERTER_H
#define DCMQI_SEGMENTATION_CONVERTER_H

// ZLIB includes
#ifdef WITH_ZLIB
#include <zlib.h>           /* for zlibVersion() */
#endif

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
#include "dcmqi/MultiframeConverter.h"
#include "dcmqi/JSONSegmentationMetaInformationHandler.h"

using namespace std;


typedef itk::LabelImageToLabelMapFilter<ShortImageType> LabelToLabelMapFilterType;

namespace dcmqi {

  class SegmentationImageConverter : public MultiframeConverter {

  public:
    static DcmDataset* itkimage2dcmSegmentation(vector<DcmDataset*> dcmDatasets,
                                                vector<ShortImageType::Pointer> segmentations,
                                                const string &metaData,
                                                bool skipEmptySlices=true);


    static pair <map<unsigned,ShortImageType::Pointer>, string> dcmSegmentation2itkimage(DcmDataset *segDataset);

 private:

    static void populateMetaInformationFromDICOM(DcmDataset *segDataset, DcmSegmentation *segdoc,
                                                 JSONSegmentationMetaInformationHandler &metaInfo);
  };

}

#endif //DCMQI_SEGMENTATION_CONVERTER_H
