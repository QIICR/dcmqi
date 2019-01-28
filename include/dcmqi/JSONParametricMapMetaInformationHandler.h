#ifndef DCMQI_JSONPARAMETRICMAPMETAINFORMATIONHANDLER_H
#define DCMQI_JSONPARAMETRICMAPMETAINFORMATIONHANDLER_H

// DCMQI includes
#include "dcmqi/JSONMetaInformationHandlerBase.h"


using namespace std;

namespace dcmqi {

  class JSONParametricMapMetaInformationHandler : public JSONMetaInformationHandlerBase {

  public:
    JSONParametricMapMetaInformationHandler();
    JSONParametricMapMetaInformationHandler(string jsonInput);
    ~JSONParametricMapMetaInformationHandler();

    void setFrameLaterality(const string& value);
    void setRealWorldValueSlope(const string& value);
    void setRealWorldValueIntercept(const string& value);
    void setDerivedPixelContrast(const string& value);
    void setDerivationDescription(const string& value);

    void setDerivationCode(const string& code, const string& designator, const string& meaning);
    void setMeasurementUnitsCode(const string& code, const string& designator, const string& meaning);
    void setMeasurementUnitsCode(const CodeSequenceMacro& codeSequence);
    void setMeasurementMethodCode(const string& code, const string& designator, const string& meaning);
    void setMeasurementMethodCode(const CodeSequenceMacro& codeSequence);
    void setQuantityValueCode(const string& code, const string& designator, const string& meaning);
    void setQuantityValueCode(const CodeSequenceMacro& codeSequence);
    void setAnatomicRegionSequence(const string &code, const string &designator, const string &meaning);
    void setAnatomicRegionSequence(const CodeSequenceMacro& codeSequence);
    void setFirstValueMapped(const short &value);
    void setLastValueMapped(const short &value);

    void addSourceImageDiffusionBValue(const string& value);

    string getFrameLaterality() const { return frameLaterality; }
    string getDerivationDescription() const { return derivationDescription; }
    string getRealWorldValueSlope() const { return realWorldValueSlope; }
    string getRealWorldValueIntercept() const { return realWorldValueIntercept; }
    string getDerivedPixelContrast() const { return derivedPixelContrast; }
    short getFirstValueMapped() const { return firstValueMapped; }
    short getLastValueMapped() const { return lastValueMapped; }
    CodeSequenceMacro* getDerivationCode() const { return derivationCode; }
    CodeSequenceMacro* getMeasurementUnitsCode() const { return measurementUnitsCode; }
    CodeSequenceMacro* getMeasurementMethodCode() const { return measurementMethodCode; }
    CodeSequenceMacro* getQuantityValueCode() const { return quantityValueCode; }
    CodeSequenceMacro* getAnatomicRegionSequence() const { return anatomicRegionSequence; }

    string getJSONOutputAsString();

    virtual void read();
    virtual bool write(string filename);
  protected:

    string realWorldValueSlope;
    string realWorldValueIntercept;
    string derivationDescription;
    string derivedPixelContrast;
    string frameLaterality;

    vector<string> diffusionBValues;

    Sint16 firstValueMapped;
    Sint16 lastValueMapped;

    CodeSequenceMacro* measurementUnitsCode;
    CodeSequenceMacro* measurementMethodCode;
    CodeSequenceMacro* quantityValueCode;
    CodeSequenceMacro* anatomicRegionSequence;
    CodeSequenceMacro* derivationCode;
  };

}

#endif //DCMQI_JSONPARAMETRICMAPMETAINFORMATIONHANDLER_H
