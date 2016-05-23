#ifndef DCMQI_CONVERTER_H
#define DCMQI_CONVERTER_H

#ifdef WITH_ZLIB
#include <zlib.h>                     /* for zlibVersion() */
#endif

#include <vector>

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/oflog/oflog.h"
#include "dcmtk/dcmseg/segdoc.h"
#include "dcmtk/dcmseg/segment.h"

#include "dcmtk/dcmfg/fgderimg.h"
#include "dcmtk/dcmfg/fgplanor.h"
#include "dcmtk/dcmfg/fgpixmsr.h"
#include "dcmtk/dcmfg/fgplanpo.h"

// UIDs
#include "QIICRUIDs.h"

// versioning
#include "dcmqiVersionConfigure.h"

//#include "preproc.h"

#include <itkImageFileReader.h>
#include <itkLabelImageToLabelMapFilter.h>
#include <itkImageRegionConstIterator.h>
#include <itkLabelStatisticsImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>

//#include "Helper.h"

#include "JSONMetaInformationHandler.h"

#include "Exceptions.h"

using namespace std;

typedef short PixelType;
typedef itk::Image<PixelType, 3> ImageType;
typedef itk::ImageFileReader<ImageType> ReaderType;
typedef itk::LabelImageToLabelMapFilter<ImageType> LabelToLabelMapFilterType;

namespace dcmqi {

    class ImageSEGConverter {

    public:
        static bool itkimage2dcmSegmentation(vector<string> dicomImageFileNames, vector<string> segmentationFileNames,
                                             const char *metaDataFileName, const char *outputFileName);

        static bool dcmSegmentation2itkimage();
        static int CHECK_COND(const OFCondition& condition);

    private:
        static IODGeneralEquipmentModule::EquipmentInfo getEquipmentInfo();
        static ContentIdentificationMacro createContentIdentificationInformation();
        static vector<int> getSliceMapForSegmentation2DerivationImage(const vector<string> &dicomImageFileNames,
                                                                      const itk::Image<short, 3>::Pointer &labelImage);
    };

}

#endif //DCMQI_CONVERTER_H
