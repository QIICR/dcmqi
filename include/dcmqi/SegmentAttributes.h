#ifndef SegmentAttributes_h
#define SegmentAttributes_h

// STD includes
#include <map>
#include <cmath>

#include <math.h>

// DCMQI includes
#include "dcmqi/Helper.h"
#include "dcmqi/JSONMetaInformationHandlerBase.h"

using namespace std;

namespace dcmqi {

  class SegmentAttributes {
  public:
    SegmentAttributes();
    SegmentAttributes(unsigned labelID);
    void initAttributes();
    ~SegmentAttributes();

    void setLabelID(unsigned labelID);
    void setSegmentDescription(const string &segmentDescription);
    void setSegmentAlgorithmType(const string& algorithmType);
    void setSegmentAlgorithmName(const string& algorithmName);
    void setRecommendedDisplayRGBValue(const unsigned& r, const unsigned& g, const unsigned& b);
    void setRecommendedDisplayRGBValue(const unsigned rgb[3]);

    void setSegmentedPropertyCategoryCode(const string& code, const string& designator, const string& meaning);
    void setSegmentedPropertyCategoryCode(const CodeSequenceMacro& codeSequence);
    void setSegmentedPropertyType(const string& code, const string& designator, const string& meaning);
    void setSegmentedPropertyType(const CodeSequenceMacro& codeSequence);
    void setSegmentedPropertyTypeModifier(const string& code, const string& designator, const string& meaning);
    void setSegmentedPropertyTypeModifier(const CodeSequenceMacro* codeSequence);
    void setAnatomicRegion(const string& code, const string& designator, const string& meaning);
    void setAnatomicRegion(const CodeSequenceMacro& codeSequence);
    void setAnatomicRegionModifier(const string& code, const string& designator, const string& meaning);
    void setAnatomicRegionModifier(const CodeSequenceMacro& codeSequence);

    unsigned int getLabelID() const { return labelID; }
    string getSegmentDescription() const { return segmentDescription; }
    string getSegmentAlgorithmType() const { return segmentAlgorithmType; }
    string getSegmentAlgorithmName() const { return segmentAlgorithmName; }
    unsigned* getRecommendedDisplayRGBValue() { return recommendedDisplayRGBValue; }

    CodeSequenceMacro* getAnatomicRegion() const { return anatomicRegion; }
    CodeSequenceMacro* getSegmentedPropertyCategoryCode() const { return segmentedPropertyCategoryCode; }
    CodeSequenceMacro* getSegmentedPropertyType() const { return segmentedPropertyType; }
    CodeSequenceMacro* getSegmentedPropertyTypeModifier() const { return segmentedPropertyTypeModifier; }
    CodeSequenceMacro* getAnatomicRegionModifier() const { return anatomicRegionModifier; }

    void PrintSelf();

  private:
    unsigned labelID;
    string segmentDescription;
    string segmentAlgorithmType;
    string segmentAlgorithmName;
    unsigned recommendedDisplayRGBValue[3];
    CodeSequenceMacro* anatomicRegion;
    CodeSequenceMacro* anatomicRegionModifier;
    CodeSequenceMacro* segmentedPropertyCategoryCode;
    CodeSequenceMacro* segmentedPropertyType;
    CodeSequenceMacro* segmentedPropertyTypeModifier;
  };

}

#endif
