#ifndef DCMQI_PARAMAP_CONVERTER_H
#define DCMQI_PARAMAP_CONVERTER_H

#include "dcmtk/config/osconfig.h"   // make sure OS specific configuration is included first

#include <vector>

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/oflog/oflog.h"

#include "dcmtk/oflog/loglevel.h"

#include "dcmqiVersionConfigure.h"

#include "ConverterBase.h"

#include "JSONMetaInformationHandlerBase.h"

using namespace std;

static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

namespace dcmqi {

    class ParaMapConverter : public ConverterBase {

    public:
        static int itkimage2dcmParaMap(const char* inputFileName, const char *metaDataFileName, const char *outputFileName);
    };

}

#endif //DCMQI_PARAMAP_CONVERTER_H
