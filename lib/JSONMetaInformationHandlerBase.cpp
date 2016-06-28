#include "JSONMetaInformationHandlerBase.h"
#include "JSONSegmentationMetaInformationHandler.h"

namespace dcmqi {

    JSONMetaInformationHandlerBase::JSONMetaInformationHandlerBase()
            : seriesAttributes(new SeriesAttributes()){
    }

    JSONMetaInformationHandlerBase::JSONMetaInformationHandlerBase(string filename)
            : seriesAttributes(new SeriesAttributes()), filename(filename){
    }

    JSONMetaInformationHandlerBase::~JSONMetaInformationHandlerBase() {
        if (this->seriesAttributes != NULL) {
            delete this->seriesAttributes;
        }
    }

    void JSONMetaInformationHandlerBase ::read() {
        if (this->filename.size() && this->isValid(this->filename)) {
            try {
                ifstream metainfoStream(this->filename, ios_base::binary);
                Json::Value root;
                metainfoStream >> root;
                this->readSeriesAttributes(root);
            } catch (exception& e) {
                cout << e.what() << '\n';
                throw JSONReadErrorException();
            }
        } else
            throw JSONReadErrorException();
    }

    Json::Value JSONMetaInformationHandlerBase::writeSeriesAttributes() {
        Json::Value value;
        value["ReaderID"] = this->seriesAttributes->getReaderID();
        value["SessionID"] = this->seriesAttributes->getSessionID();
        value["TimePointID"] = this->seriesAttributes->getTimePointID();
        value["SeriesDescription"] = this->seriesAttributes->getSeriesDescription();
        value["SeriesNumber"] = this->seriesAttributes->getSeriesNumber();
        value["InstanceNumber"] = this->seriesAttributes->getInstanceNumber();
        value["BodyPartExamined"] = this->seriesAttributes->getBodyPartExamined();
        return value;
    }

    Json::Value JSONMetaInformationHandlerBase::codeSequence2Json(CodeSequenceMacro *codeSequence) {
        Json::Value value;
        value["codeValue"] = SegmentAttributes::getCodeSequenceValue(codeSequence);
        value["codingSchemeDesignator"] = SegmentAttributes::getCodeSequenceDesignator(codeSequence);
        value["codeMeaning"] = SegmentAttributes::getCodeSequenceMeaning(codeSequence);
        return value;
    }

    void JSONMetaInformationHandlerBase::readSeriesAttributes(const Json::Value &root) {
        Json::Value seriesAttributes = root["seriesAttributes"];
        this->seriesAttributes->setReaderID(seriesAttributes.get("ReaderID", "Reader1").asString());
        this->seriesAttributes->setSessionID(seriesAttributes.get("SessionID", "Session1").asString());
        this->seriesAttributes->setTimePointID(seriesAttributes.get("TimePointID", "1").asString());
        this->seriesAttributes->setSeriesDescription(seriesAttributes.get("SeriesDescription", "Segmentation").asString());
        this->seriesAttributes->setSeriesNumber(seriesAttributes.get("SeriesNumber", "300").asString());
        this->seriesAttributes->setInstanceNumber(seriesAttributes.get("InstanceNumber", "1").asString());
        this->seriesAttributes->setBodyPartExamined(seriesAttributes.get("BodyPartExamined", "").asString());
    }

    bool JSONMetaInformationHandlerBase::isValid(string filename) {
        // TODO: add validation of json file here
        return true;
    }

}


