#ifndef DCMQI_PARAMAP_CONVERTER_H
#define DCMQI_PARAMAP_CONVERTER_H

#include "dcmtk/dcmpmap/dpmparametricmapiod.h"

#include <stdlib.h>

#include <itkImageRegionConstIteratorWithIndex.h>

#include "JSONParametricMapMetaInformationHandler.h"

#include "ConverterBase.h"

#include "itkMinimumMaximumImageCalculator.h"

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

    static int paramap2itkimage(const string &inputSEGFileName, const string &outputDirName);
  protected:
    static OFCondition addFrame(DPMParametricMapIOD &map, const ImageType::Pointer &parametricMapImage,
                                const JSONParametricMapMetaInformationHandler &metaInfo, const unsigned long frameNo);

    static void populateMetaInformationFromDICOM(DcmDataset *pmapDataset,
                                                 JSONParametricMapMetaInformationHandler &metaInfo);
  };

}

#endif //DCMQI_PARAMAP_CONVERTER_H
