#ifndef DCMQI_JSON_METAINFORMATION_HANDLER_H
#define DCMQI_JSON_METAINFORMATION_HANDLER_H

#include <json/json.h>
#include <exception>

#include <vector>

#include "SegmentAttributes.h"


using namespace std;

namespace dcmqi {

    class JSONReadErrorException : public exception {
        virtual const char *what() const throw() {
            return "JSON Exception: file could not be read.";
        }
    };


    class JSONMetaInformationHandler {

    public:
        JSONMetaInformationHandler();

        JSONMetaInformationHandler(const char *filename);
        ~JSONMetaInformationHandler();

        bool write(const char *filename);

        string readerID;
        string sessionID;
        string timePointID;
        string seriesDescription;
        string seriesNumber;
        string instanceNumber;
        string bodyPartExamined;

        vector<SegmentAttributes*> segmentsAttributes;

    protected:
        bool isValid(const char *filename);

    private:
        bool read();

        const char *filename;

        void readSeriesAttributes(const Json::Value &root);
        void readSegmentAttributes(const Json::Value &root);
    };
}


#endif //DCMQI_JSON_METAINFORMATION_HANDLER_H
