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

    void setRealWorldValueSlope(const string& value);
    void setRealWorldValueIntercept(const string &value);
    void setDerivedPixelContrast(const string& value);
    void setMeasurementUnitsCode(const string& code, const string& designator, const string& meaning);
    void setMeasurementUnitsCode(const CodeSequenceMacro& codeSequence);
    void setMeasurementMethodCode(const string& code, const string& designator, const string& meaning);
    void setMeasurementMethodCode(const CodeSequenceMacro& codeSequence);
    void setQuantityValueCode(const string& code, const string& designator, const string& meaning);
    void setQuantityValueCode(const CodeSequenceMacro& codeSequence);
    void setFirstValueMapped(const short &value);
    void setLastValueMapped(const short &value);

    string getRealWorldValueSlope() const { return realWorldValueSlope; }
    string getRealWorldValueIntercept() const { return realWorldValueIntercept; }
    string getDerivedPixelContrast() const { return derivedPixelContrast; }
    short getFirstValueMapped() const { return firstValueMapped; }
    short getLastValueMapped() const { return lastValueMapped; }
    CodeSequenceMacro* getMeasurementUnitsCode() const { return measurementUnitsCode; }
    CodeSequenceMacro* getMeasurementMethodCode() const { return measurementMethodCode; }
    CodeSequenceMacro* getQuantityValueCode() const { return quantityValueCode; }

    virtual void read();
    virtual bool write(string filename);
  protected:
    virtual bool isValid(string filename);

    string realWorldValueSlope;
    string realWorldValueIntercept;
    string derivedPixelContrast;

    Sint16 firstValueMapped;
    Sint16 lastValueMapped;

    CodeSequenceMacro* measurementUnitsCode;
    CodeSequenceMacro* measurementMethodCode;
    CodeSequenceMacro* quantityValueCode;
  };

}

#endif //DCMQI_JSONPARAMETRICMAPMETAINFORMATIONHANDLER_H
