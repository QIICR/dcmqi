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

        void setContentCreatorName(const string &creatorName);
        void setClinicalTrialSeriesID(const string &seriesID);
        void setClinicalTrialTimePointID(const string &timePointID);

        string getContentCreatorName() const { return contentCreatorName; }
        string getClinicalTrialSeriesID() const { return clinicalTrialSeriesID; }
        string getClinicalTrialTimePointID() const { return clinicalTrialTimePointID; }

        vector<SegmentAttributes*> segmentsAttributes;

        void read();
        bool write(string filename);

        SegmentAttributes* createAndGetNewSegment(unsigned labelID);

    protected:

        string contentCreatorName;
        string clinicalTrialSeriesID;
        string clinicalTrialTimePointID;

        void readSeriesAttributes(const Json::Value &root);
        void readSegmentAttributes(const Json::Value &root);

        Json::Value createAndGetSeriesAttributes();
        Json::Value createAndGetSegmentAttributes();

        bool isValid(string filename);
    };

}


#endif //DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H
