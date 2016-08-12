#ifndef DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H
#define DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H

#include "JSONMetaInformationHandlerBase.h"

using namespace std;

namespace dcmqi {


  class JSONSegmentationMetaInformationHandler : public JSONMetaInformationHandlerBase {

  public:
    JSONSegmentationMetaInformationHandler(){}
    JSONSegmentationMetaInformationHandler(string jsonInput);
    ~JSONSegmentationMetaInformationHandler();

    void setContentCreatorName(const string &creatorName);
    void setClinicalTrialSeriesID(const string &seriesID);
    void setClinicalTrialTimePointID(const string &timePointID);

    string getContentCreatorName() const { return contentCreatorName; }
    string getClinicalTrialSeriesID() const { return clinicalTrialSeriesID; }
    string getClinicalTrialTimePointID() const { return clinicalTrialTimePointID; }

    string getJSONOutputAsString();

    vector<SegmentAttributes*> segmentsAttributes;

    void read();
    bool write(string filename);

    SegmentAttributes* createAndGetNewSegment(unsigned labelID);

  protected:

    string jsonInput;

    string contentCreatorName;
    string clinicalTrialSeriesID;
    string clinicalTrialTimePointID;

    void readSeriesAttributes();
    void readSegmentAttributes();

    Json::Value createAndGetSeriesAttributes();
    Json::Value createAndGetSegmentAttributes();

    bool isValid(string filename);
  };

}


#endif //DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H
