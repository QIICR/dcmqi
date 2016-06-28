#ifndef DCMQI_PARAMAP_CONVERTER_H
#define DCMQI_PARAMAP_CONVERTER_H

#include "dcmtk/config/osconfig.h"   // make sure OS specific configuration is included first

#include <vector>

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/oflog/oflog.h"

#include "dcmtk/oflog/loglevel.h"

#include "dcmqiVersionConfigure.h"

#include "ConverterBase.h"

#include "JSONParametricMapMetaInformationHandler.h"

using namespace std;

static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

namespace dcmqi {

    class ParaMapConverter : public ConverterBase {

    public:
        static int itkimage2dcmParaMap(const string &inputFileName, const string &metaDataFileName,
                                       const string &outputFileName);

        static int paraMap2itkimage(const string &inputSEGFileName, const string &outputDirName);
    };

}

#endif //DCMQI_PARAMAP_CONVERTER_H
