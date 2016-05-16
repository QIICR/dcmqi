#ifndef SegmentAttributes_h
#define SegmentAttributes_h

#include <map>
#include <cmath>

#include <math.h>

#include "Helper.h"

namespace dcmqi {

    class SegmentAttributes {
    public:
        SegmentAttributes() { };

        SegmentAttributes(unsigned labelID) {
            this->labelID = labelID;
        }

        ~SegmentAttributes() { };

        void setLabelID(unsigned labelID) {
            this->labelID = labelID;
        }

        std::string lookupAttribute(std::string key) {
            if (attributesDictionary.find(key) == attributesDictionary.end())
                return "";
            return attributesDictionary[key];
        }

        int populateAttributesFromString(std::string attributesStr) {
            std::vector<std::string> tupleList;
            Helper::TokenizeString(attributesStr, tupleList, ";");
            for (std::vector<std::string>::const_iterator t = tupleList.begin(); t != tupleList.end(); ++t) {
                std::vector<std::string> tuple;
                Helper::TokenizeString(*t, tuple, ":");
                if (tuple.size() == 2)
                    attributesDictionary[tuple[0]] = tuple[1];
            }
            return 0;
        }

        void PrintSelf() {
            std::cout << "LabelID: " << labelID << std::endl;
            for (std::map<std::string, std::string>::const_iterator mIt = attributesDictionary.begin();
                 mIt != attributesDictionary.end(); ++mIt) {
                std::cout << (*mIt).first << " : " << (*mIt).second << std::endl;
            }
            std::cout << std::endl;
        }

    private:
        unsigned labelID;
        std::map<std::string, std::string> attributesDictionary;
    };

}

#endif
