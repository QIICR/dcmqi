//
// Created by Christian Herz on 5/12/16.
//

#include "SegmentInformationJSONHandler.h"

SegmentInformationJSONHandler::SegmentInformationJSONHandler() {
}

SegmentInformationJSONHandler::SegmentInformationJSONHandler(const char* filename) {
    this->filename = filename;
    this->read();
}

bool SegmentInformationJSONHandler::read() {
    if (this->filename != NULL && this->isValid(this->filename)) {
        return true;
    }
    return false;
}

bool SegmentInformationJSONHandler::isValid(const char *filename) {
    // TODO: add validation of json file here
    return true;
}


