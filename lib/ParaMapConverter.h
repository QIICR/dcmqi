#ifndef DCMQI_PARAMAP_CONVERTER_H
#define DCMQI_PARAMAP_CONVERTER_H

#include "dcmtk/config/osconfig.h"   // make sure OS specific configuration is included first

#include <vector>

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/oflog/oflog.h"

#include "dcmtk/oflog/loglevel.h"

// UIDs
#include "QIICRUIDs.h"

#include "dcmqiVersionConfigure.h"

#include "preproc.h"

#include "Exceptions.h"

using namespace std;

static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

namespace dcmqi {

    class ParaMapConverter {

    public:

        static int itkimage2dcmParaMap(vector<string> dicomImageFileNames, vector<string> segmentationFileNames,
                                                  const char *metaDataFileName, const char *outputFileName);
    };

}

#endif //DCMQI_PARAMAP_CONVERTER_H
