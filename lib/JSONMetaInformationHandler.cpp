#include "JSONMetaInformationHandler.h"
#include <iostream>

namespace dcmqi {

    JSONMetaInformationHandler::JSONMetaInformationHandler() {
    }

    JSONMetaInformationHandler::JSONMetaInformationHandler(const char *filename) {
        this->filename = filename;
        if (!this->read()) {
            JSONReadErrorException jsonException;
            throw jsonException;
        }
    }

    JSONMetaInformationHandler::~JSONMetaInformationHandler() {
        if (this->segmentsAttributes.size() > 0){
            for (vector<SegmentAttributes*>::iterator it = this->segmentsAttributes.begin() ; it != this->segmentsAttributes.end(); ++it)
                delete *it;
        }
    }

    bool JSONMetaInformationHandler::read() {
        if (this->filename != NULL && this->isValid(this->filename)) {
            try {
                ifstream metainfoStream(this->filename, ifstream::binary);

                metainfoStream >> this->metaInfoRoot;
                this->readSeriesAttributes(this->metaInfoRoot);
                this->readSegmentAttributes(this->metaInfoRoot);
            } catch (exception& e) {
                cout << e.what() << endl;
                return false;
            }
            return true;
        }
        return false;
    }

    bool JSONMetaInformationHandler::write(const char *filename) {
        if (this->segmentsAttributes.size() == 0)
            return false;
        // TODO: add checks for validity here....

        std::ofstream outputFile;
        outputFile.open(filename);
        Json::Value data;

        data["seriesAttributes"] = writeSeriesAttributes();
        data["segmentAttributes"] = writeSegmentAttributes();

        Json::StyledWriter styledWriter;
        outputFile << styledWriter.write(data);

        outputFile.close();
        return true;
    }

    Json::Value JSONMetaInformationHandler::writeSegmentAttributes() {
        Json::Value values(Json::arrayValue);
        for (vector<SegmentAttributes*>::iterator it = segmentsAttributes.begin() ; it != segmentsAttributes.end(); ++it) {
            Json::Value segment;
            segment["LabelID"] = (*it)->getLabelID();
            segment["SegmentDescription"] = (*it)->getSegmentDescription();
            segment["SegmentAlgorithmType"] = (*it)->getSegmentAlgorithmType();
            if ((*it)->getSegmentAlgorithmName().length() > 0)
                segment["SegmentAlgorithmName"] = (*it)->getSegmentAlgorithmName();

            if ((*it)->getSegmentedPropertyCategoryCode()){
                segment["SegmentedPropertyCategoryCodeSequence"] = codeSequence2Json((*it)->getSegmentedPropertyCategoryCode());
              }

            if ((*it)->getSegmentedPropertyType())
                segment["SegmentedPropertyTypeCodeSequence"] = codeSequence2Json((*it)->getSegmentedPropertyType());

            if ((*it)->getSegmentedPropertyTypeModifier())
                segment["SegmentedPropertyTypeModifierCodeSequence"] = codeSequence2Json((*it)->getSegmentedPropertyTypeModifier());

            if ((*it)->getAnatomicRegion())
                segment["AnatomicRegion"] = codeSequence2Json((*it)->getAnatomicRegion());

            if ((*it)->getAnatomicRegionModifier())
                segment["AnatomicRegionModifierCodeSequence"] = codeSequence2Json((*it)->getAnatomicRegionModifier());

            Json::Value rgb(Json::arrayValue);
            rgb.append(Helper::toString((*it)->getRecommendedDisplayRGBValue()[0]));
            rgb.append(Helper::toString((*it)->getRecommendedDisplayRGBValue()[1]));
            rgb.append(Helper::toString((*it)->getRecommendedDisplayRGBValue()[2]));
            segment["RecommendedDisplayRGBValue"] = rgb;
            values.append(segment);
        }
        return values;
    }

    Json::Value JSONMetaInformationHandler::writeSeriesAttributes() {
        Json::Value value;
        value["ReaderID"] = readerID;
        value["SessionID"] = sessionID;
        value["TimePointID"] = timePointID;
        value["SeriesDescription"] = seriesDescription;
        value["SeriesNumber"] = seriesNumber;
        value["InstanceNumber"] = instanceNumber;
        value["BodyPartExamined"] = bodyPartExamined;
        return value;
    }

    Json::Value JSONMetaInformationHandler::codeSequence2Json(CodeSequenceMacro *codeSequence) {
        Json::Value value;
        value["CodeValue"] = SegmentAttributes::getCodeSequenceValue(codeSequence);
        value["CodingSchemeDesignator"] = SegmentAttributes::getCodeSequenceDesignator(codeSequence);
        value["CodeMeaning"] = SegmentAttributes::getCodeSequenceMeaning(codeSequence);
        return value;
    }

    SegmentAttributes* JSONMetaInformationHandler::createAndGetNewSegment(unsigned labelID) {
        for (vector<SegmentAttributes*>::iterator it = this->segmentsAttributes.begin() ; it != this->segmentsAttributes.end(); ++it) {
            SegmentAttributes* segmentAttributes = *it;
            if (segmentAttributes->getLabelID() == labelID)
                return NULL;
        }
        SegmentAttributes* segment = new SegmentAttributes(labelID);
        this->segmentsAttributes.push_back(segment);
        return segment;
    }

    void JSONMetaInformationHandler::readSegmentAttributes(const Json::Value &root) {
        Json::Value segmentAttributes = root["segmentAttributes"];
        // TODO: default parameters should be taken from json schema file
        for(Json::ValueIterator itr = segmentAttributes.begin() ; itr != segmentAttributes.end() ; itr++ ) {
            Json::Value segment = (*itr);
            SegmentAttributes *segmentAttribute = new SegmentAttributes(segment.get("LabelID", "1").asUInt());
            Json::Value segmentDescription = segment["SegmentDescription"];
            if (!segmentDescription.isNull()) {
                segmentAttribute->setSegmentDescription(segmentDescription.asString());
            }
            Json::Value elem = segment["SegmentedPropertyCategoryCodeSequence"];
            if (!elem.isNull()) {
                segmentAttribute->setSegmentedPropertyCategoryCode(elem.get("CodeValue", "T-D0050").asString(),
                                                                   elem.get("CodingSchemeDesignator", "SRT").asString(),
                                                                   elem.get("CodeMeaning", "Tissue").asString());
            }
            elem = segment["SegmentedPropertyTypeCodeSequence"];
            if (!elem.isNull()) {
                segmentAttribute->setSegmentedPropertyType(elem.get("CodeValue", "T-D0050").asString(),
                                                           elem.get("CodingSchemeDesignator", "SRT").asString(),
                                                           elem.get("CodeMeaning", "Tissue").asString());
            }
            elem = segment["SegmentedPropertyTypeModifierCodeSequence"];
            if (!elem.isNull()) {
                segmentAttribute->setSegmentedPropertyTypeModifier(elem.get("CodeValue", "").asString(),
                                                                   elem.get("CodingSchemeDesignator", "").asString(),
                                                                   elem.get("CodeMeaning", "").asString());
            }
            elem = segment["AnatomicRegionCodeSequence"];
            if (!elem.isNull()) {
                segmentAttribute->setAnatomicRegion(elem.get("CodeValue", "").asString(),
                                                    elem.get("CodingSchemeDesignator", "").asString(),
                                                    elem.get("CodeMeaning", "").asString());
            }
            elem = segment["AnatomicRegionModifierCodeSequence"];
            if (!elem.isNull()) {
                segmentAttribute->setAnatomicRegionModifier(elem.get("CodeValue", "").asString(),
                                                            elem.get("CodingSchemeDesignator", "").asString(),
                                                            elem.get("CodeMeaning", "").asString());
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

    void JSONMetaInformationHandler::readSeriesAttributes(const Json::Value &root) {
        Json::Value seriesAttributes = root["seriesAttributes"];
        readerID = seriesAttributes.get("ReaderID", "Reader1").asString();
        sessionID = seriesAttributes.get("SessionID", "Session1").asString();
        timePointID = seriesAttributes.get("TimePointID", "1").asString();
        seriesDescription = seriesAttributes.get("SeriesDescription", "Segmentation").asString();
        seriesNumber = seriesAttributes.get("SeriesNumber", "300").asString();
        instanceNumber = seriesAttributes.get("InstanceNumber", "1").asString();
        bodyPartExamined = seriesAttributes.get("BodyPartExamined", "").asString();
    }

    bool JSONMetaInformationHandler::isValid(const char *filename) {
        // TODO: add validation of json file here
        return true;
    }

}
