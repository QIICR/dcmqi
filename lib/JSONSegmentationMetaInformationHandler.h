#ifndef DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H
#define DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H

#include "JSONMetaInformationHandlerBase.h"

using namespace std;

namespace dcmqi {

    class JSONSegmentationMetaInformationHandler : public JSONMetaInformationHandlerBase {

    protected:
        bool read();

        Json::Value writeSegmentAttributes();

        void readSegmentAttributes(const Json::Value &root);

    public:
        JSONSegmentationMetaInformationHandler(){}
        JSONSegmentationMetaInformationHandler(const char *filename);
        ~JSONSegmentationMetaInformationHandler();

        vector<SegmentAttributes*> segmentsAttributes;

        bool write(const char *filename);

        SegmentAttributes* createAndGetNewSegment(unsigned labelID);
    };

}


#endif //DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H
