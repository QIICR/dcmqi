#ifndef DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H
#define DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H

// DCMQI includes
#include "dcmqi/JSONMetaInformationHandlerBase.h"

using namespace std;

namespace dcmqi {


  class JSONSegmentationMetaInformationHandler : public JSONMetaInformationHandlerBase {

  public:
    JSONSegmentationMetaInformationHandler(){}
    JSONSegmentationMetaInformationHandler(string jsonInput);
    ~JSONSegmentationMetaInformationHandler();

    void setContentCreatorName(const string &creatorName);
    void setClinicalTrialCoordinatingCenterName(const string &coordinatingCenterName);
    void setClinicalTrialSeriesID(const string &seriesID);
    void setClinicalTrialTimePointID(const string &timePointID);

    string getContentCreatorName() const { return contentCreatorName; }
    string getClinicalTrialCoordinatingCenterName() const { return coordinatingCenterName; }
    string getClinicalTrialSeriesID() const { return clinicalTrialSeriesID; }
    string getClinicalTrialTimePointID() const { return clinicalTrialTimePointID; }

    string getJSONOutputAsString();

    // vector contains one item per input itkImageData label
    // each item is a map from labelID to segment attributes
    vector<map<unsigned,SegmentAttributes*> > segmentsAttributesMappingList;

    void read();
    bool write(string filename);

    // segGroupNumber starts with 0 and refers to the item in segmentsAttributesMappingList.
    // if segGroupNumber is invalid, create segment within a newly created segmentation group
    // otherwise add segment to the existing segmentation group identified by segGroupNumber
    SegmentAttributes* createOrGetSegment(const int segGroupNumber, unsigned labelID);

  protected:

    string contentCreatorName;
    string coordinatingCenterName;
    string clinicalTrialSeriesID;
    string clinicalTrialTimePointID;

    void readSegmentAttributes();

    Json::Value createAndGetSegmentAttributesJSON();
  };

}


#endif //DCMQI_JSONSEGMENTATIONMETAINFORMATIONHANDLER_H
