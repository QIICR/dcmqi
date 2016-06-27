#include "SeriesAttributes.h"


namespace dcmqi {
    void SeriesAttributes::setReaderID(const string &readerID) {
        this->readerID = readerID;
    }

    void SeriesAttributes::setSessionID(const string &sessionID) {
        this->sessionID = sessionID;
    }

    void SeriesAttributes::setTimePointID(const string &timePointID) {
        this->timePointID = timePointID;
    }

    void SeriesAttributes::setSeriesDescription(const string &seriesDescription) {
        this->seriesDescription = seriesDescription;
    }

    void SeriesAttributes::setSeriesNumber(const string &seriesNumber) {
        this->seriesNumber = seriesNumber;
    }

    void SeriesAttributes::setInstanceNumber(const string &instanceNumber) {
        this->instanceNumber = instanceNumber;
    }

    void SeriesAttributes::setBodyPartExamined(const string &bodyPartExamined) {
        this->bodyPartExamined = bodyPartExamined;
    }
}