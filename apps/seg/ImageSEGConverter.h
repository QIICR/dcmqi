#ifndef DCMQI_CONVERTER_H
#define DCMQI_CONVERTER_H

#include "dcmtk/config/osconfig.h"   // make sure OS specific configuration is included first

#include "vnl/vnl_cross.h"

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
#include "dcmtk/dcmfg/fgseg.h"

#include "dcmtk/dcmseg/segutils.h"


#include "dcmtk/oflog/loglevel.h"

#include "dcmtk/dcmdata/dcrledrg.h"

// UIDs
#include "QIICRUIDs.h"

// versioning
#include "dcmqiVersionConfigure.h"

//#include "preproc.h"

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageDuplicator.h>
#include <itkImageRegionConstIterator.h>
#include <itkLabelImageToLabelMapFilter.h>
#include <itkLabelStatisticsImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkChangeInformationImageFilter.h>

#include "JSONMetaInformationHandler.h"

#include "Exceptions.h"

using namespace std;

typedef short PixelType;
typedef itk::Image<PixelType, 3> ImageType;
typedef itk::ImageFileReader<ImageType> ReaderType;
typedef itk::LabelImageToLabelMapFilter<ImageType> LabelToLabelMapFilterType;

static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

namespace dcmqi {

    class ImageSEGConverter {

    public:
        static bool itkimage2dcmSegmentation(vector<string> dicomImageFileNames, vector<string> segmentationFileNames,
                                             const char *metaDataFileName, const char *outputFileName);

        static bool dcmSegmentation2itkimage(const char *inputSEGFileName, const char *outputDirName);
        static int CHECK_COND(const OFCondition& condition);

    private:
        static IODGeneralEquipmentModule::EquipmentInfo getEquipmentInfo();
        static ContentIdentificationMacro createContentIdentificationInformation();
        static vector<int> getSliceMapForSegmentation2DerivationImage(const vector<string> &dicomImageFileNames,
                                                                      const itk::Image<short, 3>::Pointer &labelImage);

        static int getImageDirections(FGInterface &fgInterface, ImageType::DirectionType &dir);
        static int getDeclaredImageSpacing(FGInterface &fgInterface, ImageType::SpacingType &spacing);
        static int computeVolumeExtent(FGInterface &fgInterface, vnl_vector<double> &sliceDirection,
                                       ImageType::PointType &imageOrigin, double &sliceSpacing, double &sliceExtent);
    };

}

#endif //DCMQI_CONVERTER_H
