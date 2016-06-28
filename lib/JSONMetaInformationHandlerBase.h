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
        JSONMetaInformationHandlerBase(string filename);
        ~JSONMetaInformationHandlerBase();

        SeriesAttributes *seriesAttributes;

        virtual void read();
        virtual bool write(string filename)=0;

        static string getCodeSequenceValue(CodeSequenceMacro* codeSequence);
        static string getCodeSequenceDesignator(CodeSequenceMacro* codeSequence);
        static string getCodeSequenceMeaning(CodeSequenceMacro* codeSequence);

    protected:
        bool isValid(string filename);

        string filename;

        virtual void readSeriesAttributes(const Json::Value &root);

        Json::Value codeSequence2Json(CodeSequenceMacro *codeSequence);

        virtual Json::Value writeSeriesAttributes();
    };
}


#endif //DCMQI_JSON_METAINFORMATION_HANDLER_H
