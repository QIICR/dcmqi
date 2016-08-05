#include "JSONSegmentationMetaInformationHandler.h"

using namespace std;

namespace dcmqi {

  JSONSegmentationMetaInformationHandler::JSONSegmentationMetaInformationHandler(string jsonInput) {
    this->jsonInput = jsonInput;
  }

  JSONSegmentationMetaInformationHandler::~JSONSegmentationMetaInformationHandler() {
    if (this->segmentsAttributes.size() > 0) {
      for (vector<SegmentAttributes *>::iterator it = this->segmentsAttributes.begin();
           it != this->segmentsAttributes.end(); ++it)
        delete *it;
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
      istringstream str(this->jsonInput);
      str >> this->metaInfoRoot;
      this->readSeriesAttributes();
      this->readSegmentAttributes();
    } catch (exception &e) {
      cout << e.what() << '\n';
      throw JSONReadErrorException();
    }
  }

  bool JSONSegmentationMetaInformationHandler::write(string filename) {
    if (this->segmentsAttributes.size() == 0)
      return false;
    // TODO: add checks for validity here....

    ofstream outputFile;
    outputFile.open(filename.c_str());
    Json::Value data;

    data["seriesAttributes"] = createAndGetSeriesAttributes();
    data["segmentAttributes"] = createAndGetSegmentAttributes();

    Json::StyledWriter styledWriter;
    outputFile << styledWriter.write(data);

    outputFile.close();
    return true;
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
    Json::Value values(Json::arrayValue);
    for (vector<SegmentAttributes *>::iterator it = segmentsAttributes.begin();
       it != segmentsAttributes.end(); ++it) {
      Json::Value segment;
      segment["LabelID"] = (*it)->getLabelID();
      segment["SegmentDescription"] = (*it)->getSegmentDescription();
      segment["SegmentAlgorithmType"] = (*it)->getSegmentAlgorithmType();
      if ((*it)->getSegmentAlgorithmName().length() > 0)
        segment["SegmentAlgorithmName"] = (*it)->getSegmentAlgorithmName();

      if ((*it)->getSegmentedPropertyCategoryCode())
        segment["SegmentedPropertyCategoryCode"] = codeSequence2Json((*it)->getSegmentedPropertyCategoryCode());

      if ((*it)->getSegmentedPropertyType())
        segment["SegmentedPropertyType"] = codeSequence2Json((*it)->getSegmentedPropertyType());

      if ((*it)->getSegmentedPropertyTypeModifier())
        segment["SegmentedPropertyTypeModifier"] = codeSequence2Json((*it)->getSegmentedPropertyTypeModifier());

      if ((*it)->getAnatomicRegion())
        segment["AnatomicRegion"] = codeSequence2Json((*it)->getAnatomicRegion());

      if ((*it)->getAnatomicRegionModifier())
        segment["AnatomicRegionModifier"] = codeSequence2Json((*it)->getAnatomicRegionModifier());

      Json::Value rgb(Json::arrayValue);
      rgb.append(Helper::toString((*it)->getRecommendedDisplayRGBValue()[0]));
      rgb.append(Helper::toString((*it)->getRecommendedDisplayRGBValue()[1]));
      rgb.append(Helper::toString((*it)->getRecommendedDisplayRGBValue()[2]));
      segment["RecommendedDisplayRGBValue"] = rgb;
      values.append(segment);
    }
    return values;
  }

  SegmentAttributes *JSONSegmentationMetaInformationHandler::createAndGetNewSegment(unsigned labelID) {
    for (vector<SegmentAttributes *>::iterator it = this->segmentsAttributes.begin();
       it != this->segmentsAttributes.end(); ++it) {
      SegmentAttributes *segmentAttributes = *it;
      if (segmentAttributes->getLabelID() == labelID)
        return NULL;
    }
    SegmentAttributes *segment = new SegmentAttributes(labelID);
    this->segmentsAttributes.push_back(segment);
    return segment;
  }

  void JSONSegmentationMetaInformationHandler::readSegmentAttributes() {
    Json::Value segmentAttributes = this->metaInfoRoot["segmentAttributes"];
    // TODO: default parameters should be taken from json schema file
    for (Json::ValueIterator itr = segmentAttributes.begin(); itr != segmentAttributes.end(); itr++) {
      Json::Value segment = (*itr);
      SegmentAttributes *segmentAttribute = new SegmentAttributes(segment.get("LabelID", "1").asUInt());
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
      Json::Value rgbArray = segment.get("RecommendedDisplayRGBValue", "128,174,128");
      if (rgbArray.size() > 0) {
        unsigned rgb[3];
        for (unsigned int index = 0; index < rgbArray.size(); ++index)
          rgb[index] = atoi(rgbArray[index].asCString());
        segmentAttribute->setRecommendedDisplayRGBValue(rgb);
      }
      segmentsAttributes.push_back(segmentAttribute);
    }
  }

  bool JSONSegmentationMetaInformationHandler::isValid(string filename) {
    // TODO: add validation of json file here
    return true;
  }
}