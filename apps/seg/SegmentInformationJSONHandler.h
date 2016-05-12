#ifndef DCMQI_SEGMENTINFORMATIONJSONHANDLER_H
#define DCMQI_SEGMENTINFORMATIONJSONHANDLER_H

#include <json/json.h>
//#include "SegmentAttributes.h"


using namespace std;

class SegmentInformationJSONHandler {

public:

    SegmentInformationJSONHandler();
    SegmentInformationJSONHandler(const char* filename);

protected:
    bool isValid(const char* filename);

private:
    bool read();
    const char* filename;


};


#endif //DCMQI_SEGMENTINFORMATIONJSONHANDLER_H
