#ifndef DCMQI_PARAMAP_CONVERTER_H
#define DCMQI_PARAMAP_CONVERTER_H

// DCMTK includes
#include <dcmtk/dcmpmap/dpmparametricmapiod.h>

// STD includes
#include <stdlib.h>

// ITK includes
#include <itkImageRegionConstIteratorWithIndex.h>
#include <itkMinimumMaximumImageCalculator.h>

// DCMQI includes
#include "ConverterBase.h"
#include "JSONParametricMapMetaInformationHandler.h"

typedef IODFloatingPointImagePixelModule::value_type PixelType;
typedef itk::Image<PixelType, 3> ImageType;
typedef itk::ImageFileReader<ImageType> ReaderType;
typedef itk::MinimumMaximumImageCalculator<ImageType> MinMaxCalculatorType;

using namespace std;

static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

namespace dcmqi {

  class ParaMapConverter : public ConverterBase {

  public:
    static DcmDataset* itkimage2paramap(const ImageType::Pointer &parametricMapImage, DcmDataset* dcmDataset,
                                        const string &metaData);

    static pair <ImageType::Pointer, string> paramap2itkimage(DcmDataset *pmapDataset);
  protected:
    static OFCondition addFrame(DPMParametricMapIOD &map, const ImageType::Pointer &parametricMapImage,
                                const JSONParametricMapMetaInformationHandler &metaInfo, const unsigned long frameNo);

    static void populateMetaInformationFromDICOM(DcmDataset *pmapDataset,
                                                 JSONParametricMapMetaInformationHandler &metaInfo);
  };

}

#endif //DCMQI_PARAMAP_CONVERTER_H
