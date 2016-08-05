#include "JSONMetaInformationHandlerBase.h"

namespace dcmqi {

  JSONMetaInformationHandlerBase::JSONMetaInformationHandlerBase() {
  }

  JSONMetaInformationHandlerBase::JSONMetaInformationHandlerBase(string filename) : filename(filename){
  }

  JSONMetaInformationHandlerBase::~JSONMetaInformationHandlerBase() {
  }

  void JSONMetaInformationHandlerBase::setSeriesDescription(const string &seriesDescription) {
    this->seriesDescription = seriesDescription;
  }

  void JSONMetaInformationHandlerBase::setSeriesNumber(const string &seriesNumber) {
    this->seriesNumber = seriesNumber;
  }

  void JSONMetaInformationHandlerBase::setInstanceNumber(const string &instanceNumber) {
    this->instanceNumber = instanceNumber;
  }

  void JSONMetaInformationHandlerBase::setBodyPartExamined(const string &bodyPartExamined) {
    this->bodyPartExamined = bodyPartExamined;
  }

  Json::Value JSONMetaInformationHandlerBase::codeSequence2Json(CodeSequenceMacro *codeSequence) {
    Json::Value value;
    value["codeValue"] = getCodeSequenceValue(codeSequence);
    value["codingSchemeDesignator"] = getCodeSequenceDesignator(codeSequence);
    value["codeMeaning"] = getCodeSequenceMeaning(codeSequence);
    return value;
  }

  string JSONMetaInformationHandlerBase::getCodeSequenceValue(CodeSequenceMacro* codeSequence) {
    OFString value;
    codeSequence->getCodeValue(value);
    return value.c_str();
  }

  string JSONMetaInformationHandlerBase::getCodeSequenceDesignator(CodeSequenceMacro* codeSequence) {
    OFString designator;
    codeSequence->getCodingSchemeDesignator(designator);
    return designator.c_str();
  }

  string JSONMetaInformationHandlerBase::getCodeSequenceMeaning(CodeSequenceMacro* codeSequence) {
    OFString meaning;
    codeSequence->getCodeMeaning(meaning);
    return meaning.c_str();
  }

}


