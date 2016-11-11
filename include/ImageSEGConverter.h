#ifndef DCMQI_SEGMENTATION_CONVERTER_H
#define DCMQI_SEGMENTATION_CONVERTER_H

#ifdef WITH_ZLIB
#include <zlib.h>           /* for zlibVersion() */
#endif

#include "dcmtk/dcmseg/segdoc.h"
#include "dcmtk/dcmseg/segment.h"

#include "dcmtk/dcmfg/fgderimg.h"
#include "dcmtk/dcmfg/fgseg.h"

#include "dcmtk/dcmseg/segutils.h"

#include "dcmtk/dcmdata/dcrledrg.h"


#include <itkImageDuplicator.h>
#include <itkImageRegionConstIterator.h>
#include <itkLabelStatisticsImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkChangeInformationImageFilter.h>

#include "JSONSegmentationMetaInformationHandler.h"

#include "ConverterBase.h"

using namespace std;

typedef short PixelType;
typedef itk::Image<PixelType, 3> ImageType;
typedef itk::ImageFileReader<ImageType> ReaderType;
typedef itk::LabelImageToLabelMapFilter<ImageType> LabelToLabelMapFilterType;

static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

namespace dcmqi {

  class ImageSEGConverter : public ConverterBase {

  public:
    static DcmDataset* itkimage2dcmSegmentation(vector<DcmDataset*> dcmDatasets,
                                                vector<ImageType::Pointer> segmentations,
                                                const string &metaData,
												bool skipEmptySlices=true);


    static pair <map<unsigned,ImageType::Pointer>, string> dcmSegmentation2itkimage(DcmDataset *segDataset);

  private:
    static vector<vector<int> > getSliceMapForSegmentation2DerivationImage(const vector<DcmDataset*> dcmDatasets,
                                                                           const itk::Image<short, 3>::Pointer &labelImage);

    static void populateMetaInformationFromDICOM(DcmDataset *segDataset, DcmSegmentation *segdoc,
                                                 JSONSegmentationMetaInformationHandler &metaInfo);
  };

}

#endif //DCMQI_SEGMENTATION_CONVERTER_H
