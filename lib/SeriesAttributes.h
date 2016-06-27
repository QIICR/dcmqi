#ifndef DCMQI_SERIESATTRIBUTES_H
#define DCMQI_SERIESATTRIBUTES_H

#include <iostream>


using namespace std;

namespace dcmqi {

    class SeriesAttributes {
    public:
        SeriesAttributes(){};

    public:
        void setReaderID(const string &readerID);
        void setSessionID(const string &sessionID);
        void setTimePointID(const string &timePointID);
        void setSeriesDescription(const string &seriesDescription);
        void setSeriesNumber(const string &seriesNumber);
        void setInstanceNumber(const string &instanceNumber);
        void setBodyPartExamined(const string &bodyPartExamined);

        string getReaderID() const { return readerID; }
        string getSessionID() const { return sessionID; }
        string getTimePointID() const { return timePointID; }
        string getSeriesDescription() const { return seriesDescription; }
        string getSeriesNumber() const { return seriesNumber; }
        string getInstanceNumber() const { return instanceNumber;}
        string getBodyPartExamined() const { return bodyPartExamined; }

    protected:
        string readerID;
        string sessionID;
        string timePointID;
        string seriesDescription;
        string seriesNumber;
        string instanceNumber;
        string bodyPartExamined;
    };

}


#endif //DCMQI_SERIESATTRIBUTES_H
