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

#include "preproc.h"

#include <itkImageFileReader.h>
#include <itkLabelImageToLabelMapFilter.h>
#include <itkImageRegionConstIterator.h>
#include <itkLabelStatisticsImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>

//#include "Helper.h"

#include "JSONMetaInformationHandler.h"

using namespace std;

namespace dcmqi {

    class Converter {

    public:
        static bool itkimage2dcmSegmentation(vector<string> dicomImageFileNames, vector<string> segmentationFileNames,
                                             const char *metaDataFileName, const char *outputFileName);

        static bool dcmSegmentation2itkimage();

    private:
        static IODGeneralEquipmentModule::EquipmentInfo getEquipmentInfo();

    };

}

#endif //DCMQI_CONVERTER_H
