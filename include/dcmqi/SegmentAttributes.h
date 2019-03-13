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
    void setSegmentLabel(const string &segmentDescription);
    void setSegmentAlgorithmType(const string& algorithmType);
    void setSegmentAlgorithmName(const string& algorithmName);
    void setRecommendedDisplayRGBValue(const unsigned& r, const unsigned& g, const unsigned& b);
    void setRecommendedDisplayRGBValue(const unsigned rgb[3]);

    void setSegmentedPropertyCategoryCodeSequence(const string &code, const string &designator, const string &meaning);
    void setSegmentedPropertyCategoryCodeSequence(const CodeSequenceMacro &codeSequence);
    void setSegmentedPropertyTypeCodeSequence(const string &code, const string &designator, const string &meaning);
    void setSegmentedPropertyTypeCodeSequence(const CodeSequenceMacro &codeSequence);
    void setSegmentedPropertyTypeModifierCodeSequence(const string &code, const string &designator,
                                                      const string &meaning);
    void setSegmentedPropertyTypeModifierCodeSequence(const CodeSequenceMacro *codeSequence);
    void setAnatomicRegionSequence(const string &code, const string &designator, const string &meaning);
    void setAnatomicRegionSequence(const CodeSequenceMacro &codeSequence);
    void setAnatomicRegionModifierSequence(const string &code, const string &designator, const string &meaning);
    void setAnatomicRegionModifierSequence(const CodeSequenceMacro &codeSequence);

    unsigned int getLabelID() const { return labelID; }
    string getSegmentDescription() const { return segmentDescription; }
    string getSegmentLabel() const { return segmentLabel; }
    string getSegmentAlgorithmType() const { return segmentAlgorithmType; }
    string getSegmentAlgorithmName() const { return segmentAlgorithmName; }
    unsigned* getRecommendedDisplayRGBValue() { return recommendedDisplayRGBValue; }

    CodeSequenceMacro* getAnatomicRegionSequence() const { return anatomicRegionSequence; }
    CodeSequenceMacro* getSegmentedPropertyCategoryCodeSequence() const { return segmentedPropertyCategoryCodeSequence; }
    CodeSequenceMacro* getSegmentedPropertyTypeCodeSequence() const { return segmentedPropertyTypeCodeSequence; }
    CodeSequenceMacro* getSegmentedPropertyTypeModifierCodeSequence() const { return segmentedPropertyTypeModifierCodeSequence; }
    CodeSequenceMacro* getAnatomicRegionModifierSequence() const { return anatomicRegionModifierSequence; }

    string getTrackingIdentifier() const { return trackingIdentifier; }
    string getTrackingUniqueIdentifier() const { return trackingUniqueIdentifier; }

    void setTrackingIdentifier(const string& trackingIdentifier);
    void setTrackingUniqueIdentifier(const string& trackingUniqueIdentifier);

    void PrintSelf();

  private:
    unsigned labelID;
    string segmentDescription;
    string segmentLabel;
    string segmentAlgorithmType;
    string segmentAlgorithmName;
    unsigned recommendedDisplayRGBValue[3];
    CodeSequenceMacro* anatomicRegionSequence;
    CodeSequenceMacro* anatomicRegionModifierSequence;
    CodeSequenceMacro* segmentedPropertyCategoryCodeSequence;
    CodeSequenceMacro* segmentedPropertyTypeCodeSequence;
    CodeSequenceMacro* segmentedPropertyTypeModifierCodeSequence;

    string trackingIdentifier;
    string trackingUniqueIdentifier;
  };

}

#endif
