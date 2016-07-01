#ifndef DCMQI_JSONPARAMETRICMAPMETAINFORMATIONHANDLER_H
#define DCMQI_JSONPARAMETRICMAPMETAINFORMATIONHANDLER_H

#include "JSONMetaInformationHandlerBase.h"


using namespace std;

namespace dcmqi {

    class JSONParametricMapMetaInformationHandler : public JSONMetaInformationHandlerBase {

    public:
        JSONParametricMapMetaInformationHandler();
        JSONParametricMapMetaInformationHandler(string filename);
        ~JSONParametricMapMetaInformationHandler();

        void setQuantityValueCode(const string& code, const string& designator, const string& meaning);
        void setQuantityValueCode(const CodeSequenceMacro& codeSequence);

        void setQuantityUnitsCode(const string& code, const string& designator, const string& meaning);
        void setQuantityUnitsCode(const CodeSequenceMacro& codeSequence);

        CodeSequenceMacro* getQuantityValueCode() const { return quantityValueCode; }
        CodeSequenceMacro* getQuantityUnitsCode() const { return quantityUnitsCode; }

        virtual void read();
        virtual bool write(string filename);
    protected:
        virtual bool isValid(string filename);

        string realWorldValueSlope;
        string derivedPixelContrast;

        CodeSequenceMacro* quantityValueCode;
        CodeSequenceMacro* quantityUnitsCode;
    };

}

#endif //DCMQI_JSONPARAMETRICMAPMETAINFORMATIONHANDLER_H
