#include "SegmentAttributes.h"


namespace dcmqi {

    SegmentAttributes::SegmentAttributes() {
        this->initAttributes();
    }

    SegmentAttributes::SegmentAttributes(unsigned labelID) {
        this->initAttributes();
        this->labelID = labelID;
    }

    void SegmentAttributes::initAttributes() {
//        TODO: fill from defaults?
        this->setLabelID(1);
        this->setRecommendedDisplayRGBValue(128, 174, 128);
        this->anatomicRegion = NULL;
        this->anatomicRegionModifier = NULL;
        this->primaryAnatomicStructure = NULL;
        this->primaryAnatomicStructureModifier = NULL;
        this->segmentedPropertyCategoryCode = NULL;
        this->segmentedPropertyType = NULL;
        this->segmentedPropertyTypeModifier = NULL;
    }

    SegmentAttributes::~SegmentAttributes() {
        if (this->anatomicRegion)
            delete this->anatomicRegion;
        if (this->anatomicRegionModifier)
            delete this->anatomicRegionModifier;
        if (this->primaryAnatomicStructure)
            delete this->primaryAnatomicStructure;
        if (this->primaryAnatomicStructureModifier)
            delete this->primaryAnatomicStructureModifier;
        if (this->segmentedPropertyCategoryCode)
            delete this->segmentedPropertyCategoryCode;
        if (this->segmentedPropertyType)
            delete this->segmentedPropertyType;
        if (this->segmentedPropertyTypeModifier)
            delete this->segmentedPropertyTypeModifier;
    }

    void SegmentAttributes::setLabelID(unsigned labelID) {
        this->labelID = labelID;
    }

    void SegmentAttributes::setSegmentDescription(const string &segmentDescription) {
        this->segmentDescription = segmentDescription;
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

    void SegmentAttributes::setSegmentedPropertyCategoryCode(const string& code, const string& designator, const string& meaning) {
        this->segmentedPropertyCategoryCode = new CodeSequenceMacro(code.c_str(), designator.c_str(), meaning.c_str());
    }

    void SegmentAttributes::setSegmentedPropertyCategoryCode(const CodeSequenceMacro& codeSequence) {
        this->segmentedPropertyCategoryCode = new CodeSequenceMacro(codeSequence);
    }

    void SegmentAttributes::setSegmentedPropertyType(const string& code, const string& designator, const string& meaning) {
        this->segmentedPropertyType = new CodeSequenceMacro(code.c_str(), designator.c_str(), meaning.c_str());
    }

    void SegmentAttributes::setSegmentedPropertyType(const CodeSequenceMacro& codeSequence) {
        this->segmentedPropertyType = new CodeSequenceMacro(codeSequence);
    }

    void SegmentAttributes::setSegmentedPropertyTypeModifier(const string& code, const string& designator, const string& meaning) {
        this->segmentedPropertyTypeModifier = new CodeSequenceMacro(code.c_str(), designator.c_str(), meaning.c_str());
    }

    void SegmentAttributes::setSegmentedPropertyTypeModifier(const CodeSequenceMacro& codeSequence) {
        this->segmentedPropertyTypeModifier = new CodeSequenceMacro(codeSequence);
    }

    void SegmentAttributes::setAnatomicRegion(const string& code, const string& designator, const string& meaning) {
        this->anatomicRegion = new CodeSequenceMacro(code.c_str(), designator.c_str(), meaning.c_str());
    }

    void SegmentAttributes::setAnatomicRegion(const CodeSequenceMacro& codeSequence) {
        this->anatomicRegion = new CodeSequenceMacro(codeSequence);
    }

    void SegmentAttributes::setAnatomicRegionModifier(const string& code, const string& designator, const string& meaning) {
        this->anatomicRegionModifier = new CodeSequenceMacro(code.c_str(), designator.c_str(), meaning.c_str());
    }

    void SegmentAttributes::setAnatomicRegionModifier(const CodeSequenceMacro& codeSequence) {
        this->anatomicRegionModifier = new CodeSequenceMacro(codeSequence);
    }

    void SegmentAttributes::setPrimaryAnatomicStructure(const string& code, const string& designator, const string& meaning) {
        this->primaryAnatomicStructure = new CodeSequenceMacro(code.c_str(), designator.c_str(), meaning.c_str());
    }

    void SegmentAttributes::setPrimaryAnatomicStructure(const CodeSequenceMacro& codeSequence) {
        this->primaryAnatomicStructure = new CodeSequenceMacro(codeSequence);
    }

    void SegmentAttributes::setPrimaryAnatomicStructureModifier(const string& code, const string& designator, const string& meaning) {
        this->primaryAnatomicStructureModifier = new CodeSequenceMacro(code.c_str(), designator.c_str(), meaning.c_str());
    }

    void SegmentAttributes::setPrimaryAnatomicStructureModifier(const CodeSequenceMacro& codeSequence) {
        this->primaryAnatomicStructureModifier = new CodeSequenceMacro(codeSequence);
    }

    string SegmentAttributes::getCodeSequenceValue(CodeSequenceMacro* codeSequence) {
        OFString value;
        codeSequence->getCodeValue(value);
        return value.c_str();
    }

    string SegmentAttributes::getCodeSequenceDesignator(CodeSequenceMacro* codeSequence) {
        OFString designator;
        codeSequence->getCodingSchemeDesignator(designator);
        return designator.c_str();
    }

    string SegmentAttributes::getCodeSequenceMeaning(CodeSequenceMacro* codeSequence) {
        OFString meaning;
        codeSequence->getCodeMeaning(meaning);
        return meaning.c_str();
    }

    void SegmentAttributes::PrintSelf() {
        cout << "LabelID: " << this->labelID << endl;
//        for (map<string, string>::const_iterator mIt = attributesDictionary.begin();
//             mIt != attributesDictionary.end(); ++mIt) {
//            cout << (*mIt).first << " : " << (*mIt).second << endl;
//        }
        cout << endl;
    }
}
