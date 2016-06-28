#ifndef DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H
#define DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H

#include "JSONMetaInformationHandlerBase.h"

using namespace std;

namespace dcmqi {

    class JSONSegmentationMetaInformationHandler : public JSONMetaInformationHandlerBase {

    public:
        JSONSegmentationMetaInformationHandler(){}
        JSONSegmentationMetaInformationHandler(string filename);
        ~JSONSegmentationMetaInformationHandler();

        vector<SegmentAttributes*> segmentsAttributes;

        virtual void read();
        virtual bool write(string filename);

        SegmentAttributes* createAndGetNewSegment(unsigned labelID);

    protected:

        Json::Value writeSegmentAttributes();

        void readSegmentAttributes(const Json::Value &root);
    };

}


#endif //DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H
