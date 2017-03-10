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
#include "dcmqi/MultiframeConverter.h"
#include "dcmqi/JSONParametricMapMetaInformationHandler.h"
#include "dcmqi/ImageVolume.h"

typedef IODFloatingPointImagePixelModule::value_type FloatPixelType;
typedef itk::Image<FloatPixelType, 3> FloatImageType;
typedef itk::ImageFileReader<FloatImageType> FloatReaderType;
typedef itk::MinimumMaximumImageCalculator<FloatImageType> MinMaxCalculatorType;

using namespace std;

namespace dcmqi {

  DcmDataset* itkimage2paramapReplacement(const FloatImageType::Pointer &parametricMapImage, vector<DcmDataset*> dcmDatasets,
                                                 const string &metaData);
  pair <FloatImageType::Pointer, string> paramap2itkimageReplacement(DcmDataset *pmapDataset);

  class ParametricMapConverter : public MultiframeConverter {

  public:
    // placeholder to replace static function going from ITK to DICOM
    ParametricMapConverter(const FloatImageType::Pointer &parametricMapImage, vector<DcmDataset*> dcmDatasets,
                           const string &metaData);

    // placeholder to replace static function going from DICOM to ITK
    ParametricMapConverter(DcmDataset*);


    static DcmDataset* itkimage2paramap(const FloatImageType::Pointer &parametricMapImage, vector<DcmDataset*> dcmDatasets,
                                 const string &metaData);
    static pair <FloatImageType::Pointer, string> paramap2itkimage(DcmDataset *pmapDataset);

    // do the conversion
    int convert();

    // get the result
    FloatImageType::Pointer getFloat32ITKImage() const;
    string getMetaData() const;
    DcmDataset* getDataset() const;

  protected:

    ParametricMapConverter() : parametricMapVolume(NULL), parametricMapDataset(NULL) {};

    static OFCondition addFrame(DPMParametricMapIOD &map, const FloatImageType::Pointer &parametricMapImage,
                                const JSONParametricMapMetaInformationHandler &metaInfo, const unsigned long frameNo, OFVector<FGBase*> perFrameGroups);

    static void populateMetaInformationFromDICOM(DcmDataset *pmapDataset,
                                                 JSONParametricMapMetaInformationHandler &metaInfo);
  private:
    // these are the items we convert to and from, essentially
    ImageVolume* parametricMapVolume;
    DcmDataset* parametricMapDataset;

    // these are the items we will need in the process of conversion
    vector<DcmDataset*> referencedDatasets;
    string metaData;
  };

}

#endif //DCMQI_PARAMAP_CONVERTER_H
