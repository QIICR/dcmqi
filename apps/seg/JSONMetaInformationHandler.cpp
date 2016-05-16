#include "JSONMetaInformationHandler.h"
#include <iostream>

namespace dcmqi {

    JSONMetaInformationHandler::JSONMetaInformationHandler() {
    }

    JSONMetaInformationHandler::JSONMetaInformationHandler(const char *filename) {
        this->filename = filename;
        if (!this->read()) {
            JSONReadErrorException jsonException;
            throw jsonException;
        }
        std::cout << "called JSONMetaInformationHandler" << std::endl;
    }

    bool JSONMetaInformationHandler::write(const char *filename) {
        return true;
    }

    bool JSONMetaInformationHandler::read() {
        if (this->filename != NULL && this->isValid(this->filename)) {
            // TODO: read it here with jsoncpp
            return true;
        }
        return false;
    }

    bool JSONMetaInformationHandler::isValid(const char *filename) {
        // TODO: add validation of json file here
        return true;
    }

}


