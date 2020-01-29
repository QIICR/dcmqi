
// DCMQI includes
#include "dcmqi/JSONSegmentationMetaInformationHandler.h"

using namespace std;

namespace dcmqi {

  JSONSegmentationMetaInformationHandler::JSONSegmentationMetaInformationHandler(string jsonInput)
      : JSONMetaInformationHandlerBase(jsonInput){
  }

  JSONSegmentationMetaInformationHandler::~JSONSegmentationMetaInformationHandler() {
    if (this->segmentsAttributesMappingList.size() > 0) {
      for (vector<map<unsigned,SegmentAttributes *> >::const_iterator vIt = this->segmentsAttributesMappingList.begin();
           vIt != this->segmentsAttributesMappingList.end(); ++vIt){
        for (map<unsigned,SegmentAttributes *>::const_iterator mIt = (*vIt).begin();
             mIt != (*vIt).end(); ++mIt){
          delete mIt->second;
        }
      }
    }
  }

  void JSONSegmentationMetaInformationHandler::setClinicalTrialCoordinatingCenterName(const string &coordinatingCenterName) {
    this->coordinatingCenterName = coordinatingCenterName;
  }

  void JSONSegmentationMetaInformationHandler::setContentCreatorName(const string &creatorName) {
    this->contentCreatorName = creatorName;
  }

  void JSONSegmentationMetaInformationHandler::setClinicalTrialSeriesID(const string &seriesID) {
    this->clinicalTrialSeriesID = seriesID;
  }

  void JSONSegmentationMetaInformationHandler::setClinicalTrialTimePointID(const string &timePointID) {
    this->clinicalTrialTimePointID = timePointID;
  }

  void JSONSegmentationMetaInformationHandler::read() {
    try {
      istringstream metainfoStream(this->jsonInput);
      metainfoStream >> this->metaInfoRoot;
      this->contentCreatorName = this->metaInfoRoot.get("ContentCreatorName", "Reader1").asString();
      this->coordinatingCenterName =  this->metaInfoRoot.get("ClinicalTrialCoordinatingCenterName", "").asString();
      this->clinicalTrialSeriesID = this->metaInfoRoot.get("ClinicalTrialSeriesID", "Session1").asString();
      this->clinicalTrialTimePointID = this->metaInfoRoot.get("ClinicalTrialTimePointID", "1").asString();
      this->seriesDescription = this->metaInfoRoot.get("SeriesDescription", "Segmentation").asString();
      this->seriesNumber = this->metaInfoRoot.get("SeriesNumber", "300").asString();
      this->instanceNumber = this->metaInfoRoot.get("InstanceNumber", "1").asString();
      this->bodyPartExamined = this->metaInfoRoot.get("BodyPartExamined", "").asString();

      this->readSegmentAttributes();
    } catch (exception& e) {
      cerr << "ERROR: JSON parameter file could not be parsed!" << std::endl;
      cerr << "You can validate the JSON file here: http://qiicr.org/dcmqi/#/validators" << std::endl;
      cerr << "Exception details (probably not very useful): " << e.what() << endl;
      throw JSONReadErrorException();
    }
  }

  bool JSONSegmentationMetaInformationHandler::write(string filename) {
    ofstream outputFile;
    outputFile.open(filename.c_str());
    outputFile << this->getJSONOutputAsString();
    outputFile.close();
    return true;
  }

  string JSONSegmentationMetaInformationHandler::getJSONOutputAsString() {
    if (this->segmentsAttributesMappingList.size() == 0)
      return string();
    // TODO: add checks for validity here....

    Json::Value data;
    std::stringstream ss;

    data["ContentCreatorName"] = this->contentCreatorName;
    if (this->coordinatingCenterName.size())
      data["ClinicalTrialCoordinatingCenterName"] = this->coordinatingCenterName;
    data["ClinicalTrialSeriesID"] = this->clinicalTrialSeriesID;
    data["ClinicalTrialTimePointID"] = this->clinicalTrialTimePointID;
    data["SeriesDescription"] = this->seriesDescription;
    data["SeriesNumber"] = this->seriesNumber;
    data["InstanceNumber"] = this->instanceNumber;
    data["BodyPartExamined"] = this->bodyPartExamined;

    data["segmentAttributes"] = createAndGetSegmentAttributes();

    Json::StyledWriter styledWriter;
    ss << styledWriter.write(data);

    return ss.str();
  }

  Json::Value JSONSegmentationMetaInformationHandler::createAndGetSegmentAttributes() {
    // return a list of lists, where each inner list contains just one item (segment)
    Json::Value values(Json::arrayValue);
    for (vector<map<unsigned,SegmentAttributes*> >::const_iterator vIt = this->segmentsAttributesMappingList.begin();
       vIt != this->segmentsAttributesMappingList.end(); ++vIt) {
      for(map<unsigned,SegmentAttributes*>::const_iterator mIt=vIt->begin();mIt!=vIt->end();++mIt){
        Json::Value segment;
        SegmentAttributes* segmentAttributes = mIt->second;
        segment["labelID"] = segmentAttributes->getLabelID();
        segment["SegmentDescription"] = segmentAttributes->getSegmentDescription();
        segment["SegmentLabel"] = segmentAttributes->getSegmentLabel();
        segment["SegmentAlgorithmType"] = segmentAttributes->getSegmentAlgorithmType();
        if (segmentAttributes->getSegmentAlgorithmName().length() > 0)
          segment["SegmentAlgorithmName"] = segmentAttributes->getSegmentAlgorithmName();

        if (segmentAttributes->getSegmentedPropertyCategoryCodeSequence())
          segment["SegmentedPropertyCategoryCodeSequence"] = codeSequence2Json(
                  segmentAttributes->getSegmentedPropertyCategoryCodeSequence());

        if (segmentAttributes->getSegmentedPropertyTypeCodeSequence())
          segment["SegmentedPropertyTypeCodeSequence"] = codeSequence2Json(
                  segmentAttributes->getSegmentedPropertyTypeCodeSequence());

        if (segmentAttributes->getSegmentedPropertyTypeModifierCodeSequence())
          segment["SegmentedPropertyTypeModifierCodeSequence"] = codeSequence2Json(
                  segmentAttributes->getSegmentedPropertyTypeModifierCodeSequence());

        if (segmentAttributes->getAnatomicRegionSequence())
          segment["AnatomicRegionSequence"] = codeSequence2Json(segmentAttributes->getAnatomicRegionSequence());

        if (segmentAttributes->getAnatomicRegionModifierSequence())
          segment["AnatomicRegionModifierSequence"] = codeSequence2Json(
                  segmentAttributes->getAnatomicRegionModifierSequence());

        if (segmentAttributes->getTrackingIdentifier() != "")
          segment["TrackingIdentifier"] = segmentAttributes->getTrackingIdentifier();

        if (segmentAttributes->getTrackingUniqueIdentifier() != "")
          segment["TrackingUniqueIdentifier"] = segmentAttributes->getTrackingUniqueIdentifier();

        Json::Value rgb(Json::arrayValue);
        rgb.append(segmentAttributes->getRecommendedDisplayRGBValue()[0]);
        rgb.append(segmentAttributes->getRecommendedDisplayRGBValue()[1]);
        rgb.append(segmentAttributes->getRecommendedDisplayRGBValue()[2]);
        segment["recommendedDisplayRGBValue"] = rgb;
        Json::Value innerList(Json::arrayValue);
        innerList.append(segment);
        values.append(innerList);
      }
    }
    return values;
  }

  SegmentAttributes *JSONSegmentationMetaInformationHandler::createAndGetNewSegment(unsigned labelID) {
    for (vector<map<unsigned,SegmentAttributes*> >::const_iterator vIt = this->segmentsAttributesMappingList.begin();
       vIt != this->segmentsAttributesMappingList.end(); ++vIt) {
      for(map<unsigned,SegmentAttributes*>::const_iterator mIt = vIt->begin();mIt!=vIt->end();++mIt){
        SegmentAttributes *segmentAttributes = mIt->second;
        if (segmentAttributes->getLabelID() == labelID)
          return NULL;
      }
    }

    SegmentAttributes *segment = new SegmentAttributes(labelID);
    map<unsigned,SegmentAttributes*> tempMap;
    tempMap[labelID] = segment;
    this->segmentsAttributesMappingList.push_back(tempMap);
    return segment;
  }

  void JSONSegmentationMetaInformationHandler::readSegmentAttributes() {
    Json::Value allSegmentAttributes = this->metaInfoRoot["segmentAttributes"];
    // TODO: default parameters should be taken from json schema file
    for (Json::ValueIterator imageIt = allSegmentAttributes.begin(); imageIt != allSegmentAttributes.end(); imageIt++) {
      Json::Value imageSegmentsAttributes = (*imageIt);
      map<unsigned, SegmentAttributes*> labelID2SegmentAttributes;
      for (Json::ValueIterator itr = imageSegmentsAttributes.begin(); itr != imageSegmentsAttributes.end(); itr++) {
        Json::Value segment = (*itr);
        SegmentAttributes *segmentAttribute = new SegmentAttributes(segment.get("labelID", "1").asUInt());
        labelID2SegmentAttributes[segmentAttribute->getLabelID()] = segmentAttribute;

        Json::Value segmentDescription = segment["SegmentDescription"];
        if (!segmentDescription.isNull()) {
          segmentAttribute->setSegmentDescription(segmentDescription.asString());
        }

        Json::Value segmentLabel = segment["SegmentLabel"];
        if (!segmentLabel.isNull()) {
          segmentAttribute->setSegmentLabel(segmentLabel.asString());
        }

        if (segment.isMember("SegmentedPropertyCategoryCodeSequence")) {
          Json::Value elem = segment["SegmentedPropertyCategoryCodeSequence"];
          segmentAttribute->setSegmentedPropertyCategoryCodeSequence(elem.get("CodeValue", "85756007").asString(),
                                                                     elem.get("CodingSchemeDesignator",
                                                                              "SCT").asString(),
                                                                     elem.get("CodeMeaning", "Tissue").asString());
        }

        if (segment.isMember("SegmentedPropertyTypeCodeSequence")) {
          Json::Value elem = segment["SegmentedPropertyTypeCodeSequence"];
          segmentAttribute->setSegmentedPropertyTypeCodeSequence(elem.get("CodeValue", "85756007").asString(),
                                                                 elem.get("CodingSchemeDesignator", "SCT").asString(),
                                                                 elem.get("CodeMeaning", "Tissue").asString());
        }
        if (segment.isMember("SegmentedPropertyTypeModifierCodeSequence")) {
          Json::Value elem = segment["SegmentedPropertyTypeModifierCodeSequence"];
          segmentAttribute->setSegmentedPropertyTypeModifierCodeSequence(elem.get("CodeValue", "").asString(),
                                                                         elem.get("CodingSchemeDesignator",
                                                                                  "").asString(),
                                                                         elem.get("CodeMeaning", "").asString());
        }
        if (segment.isMember("AnatomicRegionSequence")) {
          Json::Value elem = segment["AnatomicRegionSequence"];
          segmentAttribute->setAnatomicRegionSequence(elem.get("CodeValue", "").asString(),
                                                      elem.get("CodingSchemeDesignator", "").asString(),
                                                      elem.get("CodeMeaning", "").asString());
        }
        if (segment.isMember("AnatomicRegionModifierSequence")) {
          Json::Value elem = segment["AnatomicRegionModifierSequence"];
          segmentAttribute->setAnatomicRegionModifierSequence(elem.get("CodeValue", "").asString(),
                                                              elem.get("CodingSchemeDesignator", "").asString(),
                                                              elem.get("CodeMeaning", "").asString());
        }
        segmentAttribute->setSegmentAlgorithmName(segment.get("SegmentAlgorithmName", "").asString());
        segmentAttribute->setSegmentAlgorithmType(segment.get("SegmentAlgorithmType", "SEMIAUTOMATIC").asString());
        Json::Value rgbArray = segment.get("recommendedDisplayRGBValue", "128,174,128");
        if (rgbArray.size() > 0) {
          unsigned rgb[3];
          for (unsigned int index = 0; index < rgbArray.size(); ++index)
            rgb[index] = rgbArray[index].asInt();
          segmentAttribute->setRecommendedDisplayRGBValue(rgb);
        }

        if(segment.isMember("TrackingIdentifier")) {
          Json::Value elem = segment["TrackingIdentifier"];
          segmentAttribute->setTrackingIdentifier(elem.asString());
        }

        if(segment.isMember("TrackingUniqueIdentifier")) {
          Json::Value elem = segment["TrackingUniqueIdentifier"];
          segmentAttribute->setTrackingUniqueIdentifier(elem.asString());
        }

      }
      segmentsAttributesMappingList.push_back(labelID2SegmentAttributes);
    }
  }
}
