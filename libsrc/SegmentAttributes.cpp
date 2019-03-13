
// DCMQI includes
#include "dcmqi/SegmentAttributes.h"


namespace dcmqi {

  SegmentAttributes::SegmentAttributes() {
    this->initAttributes();
  }

  SegmentAttributes::SegmentAttributes(unsigned labelID) {
    this->initAttributes();
    this->labelID = labelID;
  }

  void SegmentAttributes::initAttributes() {
    //    TODO: fill from defaults?
    this->setLabelID(1);
    this->setRecommendedDisplayRGBValue(128, 174, 128);
    this->anatomicRegionSequence = NULL;
    this->anatomicRegionModifierSequence = NULL;
    this->segmentedPropertyCategoryCodeSequence = NULL;
    this->segmentedPropertyTypeCodeSequence = NULL;
    this->segmentedPropertyTypeModifierCodeSequence = NULL;
    this->trackingIdentifier = "";
    this->trackingUniqueIdentifier = "";
  }

  SegmentAttributes::~SegmentAttributes() {
    if (this->anatomicRegionSequence)
      delete this->anatomicRegionSequence;
    if (this->anatomicRegionModifierSequence)
      delete this->anatomicRegionModifierSequence;
    if (this->segmentedPropertyCategoryCodeSequence)
      delete this->segmentedPropertyCategoryCodeSequence;
    if (this->segmentedPropertyTypeCodeSequence)
      delete this->segmentedPropertyTypeCodeSequence;
    if (this->segmentedPropertyTypeModifierCodeSequence)
      delete this->segmentedPropertyTypeModifierCodeSequence;
  }

  void SegmentAttributes::setLabelID(unsigned labelID) {
    this->labelID = labelID;
  }

  void SegmentAttributes::setSegmentDescription(const string &segmentDescription) {
    this->segmentDescription = segmentDescription;
  }

  void SegmentAttributes::setSegmentLabel(const string &segmentLabel) {
    this->segmentLabel = segmentLabel;
  }

  void SegmentAttributes::setSegmentAlgorithmType(const string& algorithmType) {
    this->segmentAlgorithmType = algorithmType;
  }

  void SegmentAttributes::setSegmentAlgorithmName(const string &algorithmName) {
    this->segmentAlgorithmName = algorithmName;
  }

  void SegmentAttributes::setRecommendedDisplayRGBValue(const unsigned& r, const unsigned& g, const unsigned& b) {
    this->recommendedDisplayRGBValue[0] = r;
    this->recommendedDisplayRGBValue[1] = g;
    this->recommendedDisplayRGBValue[2] = b;
  }

  void SegmentAttributes::setRecommendedDisplayRGBValue(const unsigned rgb[3]) {
    this->setRecommendedDisplayRGBValue(rgb[0], rgb[1], rgb[2]);
  }

  void SegmentAttributes::setSegmentedPropertyCategoryCodeSequence(const string &code, const string &designator,
                                                                   const string &meaning) {
    this->segmentedPropertyCategoryCodeSequence = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void SegmentAttributes::setSegmentedPropertyCategoryCodeSequence(const CodeSequenceMacro &codeSequence) {
    this->segmentedPropertyCategoryCodeSequence = new CodeSequenceMacro(codeSequence);
  }

  void SegmentAttributes::setSegmentedPropertyTypeCodeSequence(const string &code, const string &designator,
                                                               const string &meaning) {
    this->segmentedPropertyTypeCodeSequence = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void SegmentAttributes::setSegmentedPropertyTypeCodeSequence(const CodeSequenceMacro &codeSequence) {
    this->segmentedPropertyTypeCodeSequence = new CodeSequenceMacro(codeSequence);
  }

  void SegmentAttributes::setSegmentedPropertyTypeModifierCodeSequence(const string &code, const string &designator,
                                                                       const string &meaning) {
    this->segmentedPropertyTypeModifierCodeSequence = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void SegmentAttributes::setSegmentedPropertyTypeModifierCodeSequence(const CodeSequenceMacro *codeSequence) {
    this->segmentedPropertyTypeModifierCodeSequence = new CodeSequenceMacro(*codeSequence);
  }

  void SegmentAttributes::setAnatomicRegionSequence(const string &code, const string &designator, const string &meaning) {
    this->anatomicRegionSequence = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void SegmentAttributes::setAnatomicRegionSequence(const CodeSequenceMacro &codeSequence) {
    this->anatomicRegionSequence = new CodeSequenceMacro(codeSequence);
  }

  void SegmentAttributes::setAnatomicRegionModifierSequence(const string &code, const string &designator,
                                                            const string &meaning) {
    this->anatomicRegionModifierSequence = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void SegmentAttributes::setAnatomicRegionModifierSequence(const CodeSequenceMacro &codeSequence) {
    this->anatomicRegionModifierSequence = new CodeSequenceMacro(codeSequence);
  }

  void SegmentAttributes::setTrackingIdentifier(const string &trackingIdentifier) {
    this->trackingIdentifier = trackingIdentifier;
  }

  void SegmentAttributes::setTrackingUniqueIdentifier(const string &trackingUniqueIdentifier) {
    this->trackingUniqueIdentifier = trackingUniqueIdentifier;
  }

  void SegmentAttributes::PrintSelf() {
    cout << "labelID: " << this->labelID << endl;
//    for (map<string, string>::const_iterator mIt = attributesDictionary.begin();
//       mIt != attributesDictionary.end(); ++mIt) {
//      cout << (*mIt).first << " : " << (*mIt).second << endl;
//    }
    cout << endl;
  }
}
