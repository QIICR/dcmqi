#include "JSONSegmentationMetaInformationHandler.h"

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
      this->readSeriesAttributes();
      this->readSegmentAttributes();
    } catch (exception &e) {
      cout << e.what() << '\n';
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

    data["seriesAttributes"] = createAndGetSeriesAttributes();
    data["segmentAttributes"] = createAndGetSegmentAttributes();

    Json::StyledWriter styledWriter;
    ss << styledWriter.write(data);

    return ss.str();
  }

  void JSONSegmentationMetaInformationHandler::readSeriesAttributes() {
    Json::Value seriesAttributes = this->metaInfoRoot["seriesAttributes"];
    this->contentCreatorName = seriesAttributes.get("ContentCreatorName", "Reader1").asString();
    this->clinicalTrialSeriesID = seriesAttributes.get("SessionID", "Session1").asString();
    this->clinicalTrialTimePointID = seriesAttributes.get("TimePointID", "1").asString();
    this->seriesDescription = seriesAttributes.get("SeriesDescription", "Segmentation").asString();
    this->seriesNumber = seriesAttributes.get("SeriesNumber", "300").asString();
    this->instanceNumber = seriesAttributes.get("InstanceNumber", "1").asString();
    this->bodyPartExamined = seriesAttributes.get("BodyPartExamined", "").asString();
  }

  Json::Value JSONSegmentationMetaInformationHandler::createAndGetSeriesAttributes() {
    Json::Value value;
    value["ContentCreatorName"] = this->contentCreatorName;
    value["ClinicalTrialSeriesID"] = this->clinicalTrialSeriesID;
    value["TimePointID"] = this->clinicalTrialTimePointID;
    value["SeriesDescription"] = this->seriesDescription;
    value["SeriesNumber"] = this->seriesNumber;
    value["InstanceNumber"] = this->instanceNumber;
    value["BodyPartExamined"] = this->bodyPartExamined;
    return value;
  }

  Json::Value JSONSegmentationMetaInformationHandler::createAndGetSegmentAttributes() {
    // return a list of lists, where each inner list contains just one item (segment)
    Json::Value values(Json::arrayValue);
    for (vector<map<unsigned,SegmentAttributes*> >::const_iterator vIt = this->segmentsAttributesMappingList.begin();
       vIt != this->segmentsAttributesMappingList.end(); ++vIt) {
      for(map<unsigned,SegmentAttributes*>::const_iterator mIt=vIt->begin();mIt!=vIt->end();++mIt){
        Json::Value segment;
        SegmentAttributes* segmentAttributes = mIt->second;
        segment["LabelID"] = segmentAttributes->getLabelID();
        segment["SegmentDescription"] = segmentAttributes->getSegmentDescription();
        segment["SegmentAlgorithmType"] = segmentAttributes->getSegmentAlgorithmType();
        if (segmentAttributes->getSegmentAlgorithmName().length() > 0)
          segment["SegmentAlgorithmName"] = segmentAttributes->getSegmentAlgorithmName();

        if (segmentAttributes->getSegmentedPropertyCategoryCode())
          segment["SegmentedPropertyCategoryCode"] = codeSequence2Json(segmentAttributes->getSegmentedPropertyCategoryCode());

        if (segmentAttributes->getSegmentedPropertyType())
          segment["SegmentedPropertyType"] = codeSequence2Json(segmentAttributes->getSegmentedPropertyType());

        if (segmentAttributes->getSegmentedPropertyTypeModifier())
          segment["SegmentedPropertyTypeModifier"] = codeSequence2Json(segmentAttributes->getSegmentedPropertyTypeModifier());

        if (segmentAttributes->getAnatomicRegion())
          segment["AnatomicRegion"] = codeSequence2Json(segmentAttributes->getAnatomicRegion());

        if (segmentAttributes->getAnatomicRegionModifier())
          segment["AnatomicRegionModifier"] = codeSequence2Json(segmentAttributes->getAnatomicRegionModifier());

        Json::Value rgb(Json::arrayValue);
        rgb.append(Helper::toString(segmentAttributes->getRecommendedDisplayRGBValue()[0]));
        rgb.append(Helper::toString(segmentAttributes->getRecommendedDisplayRGBValue()[1]));
        rgb.append(Helper::toString(segmentAttributes->getRecommendedDisplayRGBValue()[2]));
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
        SegmentAttributes *segmentAttribute = new SegmentAttributes(segment.get("LabelID", "1").asUInt());
        labelID2SegmentAttributes[segmentAttribute->getLabelID()] = segmentAttribute;

        Json::Value segmentDescription = segment["SegmentDescription"];
        if (!segmentDescription.isNull()) {
          segmentAttribute->setSegmentDescription(segmentDescription.asString());
        }
        Json::Value elem = segment["SegmentedPropertyCategoryCodeSequence"];
        if (!elem.isNull()) {
          segmentAttribute->setSegmentedPropertyCategoryCode(elem.get("codeValue", "T-D0050").asString(),
                                                           elem.get("codingSchemeDesignator", "SRT").asString(),
                                                           elem.get("codeMeaning", "Tissue").asString());
        }
        elem = segment["SegmentedPropertyTypeCodeSequence"];
        if (!elem.isNull()) {
          segmentAttribute->setSegmentedPropertyType(elem.get("codeValue", "T-D0050").asString(),
                                                   elem.get("codingSchemeDesignator", "SRT").asString(),
                                                   elem.get("codeMeaning", "Tissue").asString());
        }
        elem = segment["SegmentedPropertyTypeModifierCodeSequence"];
        if (!elem.isNull()) {
          segmentAttribute->setSegmentedPropertyTypeModifier(elem.get("codeValue", "").asString(),
                                                           elem.get("codingSchemeDesignator", "").asString(),
                                                           elem.get("codeMeaning", "").asString());
        }
        elem = segment["AnatomicRegionSequence"];
        if (!elem.isNull()) {
          segmentAttribute->setAnatomicRegion(elem.get("codeValue", "").asString(),
                                            elem.get("codingSchemeDesignator", "").asString(),
                                            elem.get("codeMeaning", "").asString());
        }
        elem = segment["AnatomicRegionModifierSequence"];
        if (!elem.isNull()) {
          segmentAttribute->setAnatomicRegionModifier(elem.get("codeValue", "").asString(),
                                                    elem.get("codingSchemeDesignator", "").asString(),
                                                    elem.get("codeMeaning", "").asString());
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
      }
      segmentsAttributesMappingList.push_back(labelID2SegmentAttributes);
    }
  }
}
