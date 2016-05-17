#ifndef SegmentAttributes_h
#define SegmentAttributes_h

#include <map>
#include <cmath>

#include <math.h>

#include "Helper.h"

using namespace std;


namespace dcmqi {

    class SegmentAttributes {
    public:
        SegmentAttributes();
        SegmentAttributes(unsigned labelID);
        void initAttributes();
        ~SegmentAttributes();

        void setLabelID(unsigned labelID);
        void setSegmentedPropertyCategoryCode(const string& code, const string& designator, const string& meaning);
        void setSegmentedPropertyType(const string& code, const string& designator, const string& meaning);
        void setSegmentedPropertyTypeModifier(const string& code, const string& designator, const string& meaning);
        void setSegmentAlgorithmType(const string& algorithmType);
        void setSegmentAlgorithmName(const string &algorithmName);
        void setRecommendedDisplayRGBValue(const unsigned& r, const unsigned& g, const unsigned& b);
        void setRecommendedDisplayRGBValue(const unsigned rgb[3]);
        void setAnatomicRegion(const string &anatomicRegion) { this->anatomicRegion = anatomicRegion; }

        unsigned int getLabelID() const { return labelID; }
        string getSegmentAlgorithmType() const { return segmentAlgorithmType; }
        string getSegmentAlgorithmName() const { return segmentAlgorithmName; }
        string getAnatomicRegion() const { return anatomicRegion; }
        unsigned* getRecommendedDisplayRGBValue() { return recommendedDisplayRGBValue; }
        CodeSequenceMacro getSegmentedPropertyCategoryCode() const { return segmentedPropertyCategoryCode; }
        CodeSequenceMacro getSegmentedPropertyType() const { return segmentedPropertyType; }
        CodeSequenceMacro getSegmentedPropertyTypeModifier() const { return segmentedPropertyTypeModifier; }

        void PrintSelf();

    private:
        unsigned labelID;
        string segmentAlgorithmType;
        string segmentAlgorithmName;
        string anatomicRegion;
        unsigned recommendedDisplayRGBValue[3];
        CodeSequenceMacro segmentedPropertyCategoryCode;
        CodeSequenceMacro segmentedPropertyType;
        CodeSequenceMacro segmentedPropertyTypeModifier;
    };

}

#endif
