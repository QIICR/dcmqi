#ifndef DCMQI_PARAMAP_CONVERTER_H
#define DCMQI_PARAMAP_CONVERTER_H

// DCMTK includes
#include <dcmtk/dcmfg/fgderimg.h>
#include <dcmtk/dcmfg/fgseg.h>
#include <dcmtk/dcmseg/segdoc.h>
#include <dcmtk/dcmseg/segment.h>
#include <dcmtk/dcmseg/segutils.h>
#include <dcmtk/dcmdata/dcrledrg.h>
#include <dcmtk/dcmpmap/dpmparametricmapiod.h>

// STD includes
#include <stdlib.h>

// ITK includes
#include <itkImageRegionConstIteratorWithIndex.h>
#include <itkMinimumMaximumImageCalculator.h>

// DCMQI includes
#include "dcmqi/ConverterBase.h"
#include "dcmqi/JSONParametricMapMetaInformationHandler.h"

typedef IODFloatingPointImagePixelModule::value_type FloatPixelType;
typedef itk::Image<FloatPixelType, 3> FloatImageType;
typedef itk::ImageFileReader<FloatImageType> FloatReaderType;
typedef itk::MinimumMaximumImageCalculator<FloatImageType> MinMaxCalculatorType;

using namespace std;


namespace dcmqi {

  class ParaMapConverter : public ConverterBase {

  public:
    static DcmDataset* itkimage2paramap(const FloatImageType::Pointer &parametricMapImage, vector<DcmDataset*> dcmDatasets,
                                        const string &metaData);

    static pair <FloatImageType::Pointer, string> paramap2itkimage(DcmDataset *pmapDataset);
  protected:
    static OFCondition addFrame(DPMParametricMapIOD &map, const FloatImageType::Pointer &parametricMapImage,
                                const JSONParametricMapMetaInformationHandler &metaInfo, const unsigned long frameNo, OFVector<FGBase*> perFrameGroups);

    static void populateMetaInformationFromDICOM(DcmDataset *pmapDataset,
                                                 JSONParametricMapMetaInformationHandler &metaInfo);
  };

}

#endif //DCMQI_PARAMAP_CONVERTER_H
