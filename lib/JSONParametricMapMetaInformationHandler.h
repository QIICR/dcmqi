//
// Created by Christian Herz on 6/28/16.
//

#ifndef DCMQI_JSONPARAMETRICMAPMETAINFORMATIONHANDLER_H
#define DCMQI_JSONPARAMETRICMAPMETAINFORMATIONHANDLER_H

#include "JSONMetaInformationHandlerBase.h"


using namespace std;

namespace dcmqi {

    class JSONParametricMapMetaInformationHandler : public JSONMetaInformationHandlerBase {

    public:
        JSONParametricMapMetaInformationHandler()
                : JSONMetaInformationHandlerBase() {}

        JSONParametricMapMetaInformationHandler(string filename)
                : JSONMetaInformationHandlerBase::JSONMetaInformationHandlerBase(filename){};
        ~JSONParametricMapMetaInformationHandler() {};

        virtual bool write(string filename){return true;};

    };

}

#endif //DCMQI_JSONPARAMETRICMAPMETAINFORMATIONHANDLER_H
