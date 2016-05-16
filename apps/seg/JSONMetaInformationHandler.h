#ifndef DCMQI_JSON_METAINFORMATION_HANDLER_H
#define DCMQI_JSON_METAINFORMATION_HANDLER_H

#include <json/json.h>
#include <exception>

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

        bool write(const char *filename);

    protected:
        bool isValid(const char *filename);

    private:
        bool read();

        const char *filename;
    };
}


#endif //DCMQI_JSON_METAINFORMATION_HANDLER_H
