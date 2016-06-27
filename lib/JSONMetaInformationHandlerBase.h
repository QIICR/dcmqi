#ifndef DCMQI_JSON_METAINFORMATION_HANDLER_H
#define DCMQI_JSON_METAINFORMATION_HANDLER_H

#include <json/json.h>

#include <vector>

#include "SeriesAttributes.h"
#include "SegmentAttributes.h"
#include "Exceptions.h"


using namespace std;

namespace dcmqi {

    class JSONMetaInformationHandlerBase {

    public:
        JSONMetaInformationHandlerBase();
        JSONMetaInformationHandlerBase(const char *filename);
        ~JSONMetaInformationHandlerBase();

        SeriesAttributes *seriesAttributes;

    protected:
        bool isValid(const char *filename);

        const char *filename;

        virtual bool read();

        virtual void readSeriesAttributes(const Json::Value &root);

        Json::Value codeSequence2Json(CodeSequenceMacro *codeSequence);

        virtual Json::Value writeSeriesAttributes();
    };
}


#endif //DCMQI_JSON_METAINFORMATION_HANDLER_H
