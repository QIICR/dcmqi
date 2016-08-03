#include "JSONParametricMapMetaInformationHandler.h"

namespace dcmqi {

  JSONParametricMapMetaInformationHandler::JSONParametricMapMetaInformationHandler()
      : JSONMetaInformationHandlerBase(),
        measurementUnitsCode(NULL),
        measurementMethodCode(NULL),
        quantityValueCode(NULL) {
  }

  JSONParametricMapMetaInformationHandler::JSONParametricMapMetaInformationHandler(string filename)
      : JSONMetaInformationHandlerBase::JSONMetaInformationHandlerBase(filename),
        measurementUnitsCode(NULL),
        measurementMethodCode(NULL),
        quantityValueCode(NULL) {
  }

  JSONParametricMapMetaInformationHandler::~JSONParametricMapMetaInformationHandler() {
    if (this->measurementUnitsCode)
      delete this->measurementUnitsCode;
    if (this->measurementMethodCode)
      delete this->measurementMethodCode;
    if (this->quantityValueCode)
      delete this->quantityValueCode;
  }

  void JSONParametricMapMetaInformationHandler::setRealWorldValueSlope(const string& value) {
    this->realWorldValueSlope = value;
  };

  void JSONParametricMapMetaInformationHandler::setRealWorldValueIntercept(const string &value) {
    this->realWorldValueIntercept = value;
  }

  void JSONParametricMapMetaInformationHandler::setDerivedPixelContrast(const string& value) {
    this->derivedPixelContrast = value;
  };

  void JSONParametricMapMetaInformationHandler::setMeasurementUnitsCode(const string& code, const string& designator,
                                                                        const string& meaning) {
    this->measurementUnitsCode = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void JSONParametricMapMetaInformationHandler::setMeasurementUnitsCode(const CodeSequenceMacro& codeSequence) {
    this->measurementUnitsCode = new CodeSequenceMacro(codeSequence);
  }

  void JSONParametricMapMetaInformationHandler::setMeasurementMethodCode(const string& code, const string& designator,
                                                                         const string& meaning) {
    this->measurementMethodCode = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void JSONParametricMapMetaInformationHandler::setMeasurementMethodCode(const CodeSequenceMacro& codeSequence) {
    this->measurementMethodCode = new CodeSequenceMacro(codeSequence);
  }

  void JSONParametricMapMetaInformationHandler::setQuantityValueCode(const string& code, const string& designator,
                                                                     const string& meaning) {
    this->quantityValueCode = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void JSONParametricMapMetaInformationHandler::setQuantityValueCode(const CodeSequenceMacro& codeSequence) {
    this->quantityValueCode = new CodeSequenceMacro(codeSequence);
  }

  void JSONParametricMapMetaInformationHandler::setLastValueMapped(const short &value) {
    this->lastValueMapped = value;
  }

  void JSONParametricMapMetaInformationHandler::setFirstValueMapped(const short &value) {
    this->firstValueMapped = value;
  }

  void JSONParametricMapMetaInformationHandler ::read() {
    if (this->filename.size() && this->isValid(this->filename)) {
      try {
        ifstream metainfoStream(this->filename, ios_base::binary);
        metainfoStream >> this->metaInfoRoot;
        this->seriesDescription = this->metaInfoRoot.get("SeriesDescription", "Segmentation").asString();
        this->seriesNumber = this->metaInfoRoot.get("SeriesNumber", "300").asString();
        this->instanceNumber = this->metaInfoRoot.get("InstanceNumber", "1").asString();
        this->bodyPartExamined = this->metaInfoRoot.get("BodyPartExamined", "").asString();
        this->realWorldValueSlope = this->metaInfoRoot.get("RealWorldValueSlope", "1").asString();
        this->realWorldValueIntercept = this->metaInfoRoot.get("RealWorldValueIntercept", "0").asString();
        this->derivedPixelContrast = this->metaInfoRoot.get("DerivedPixelContrast", "").asString();

        Json::Value elem = this->metaInfoRoot["QuantityValueCode"];
        if (!elem.isNull()) {
          this->setQuantityValueCode(elem.get("codeValue", "").asString(),
                                     elem.get("codingSchemeDesignator", "").asString(),
                                     elem.get("codeMeaning", "").asString());
        }

        elem = this->metaInfoRoot["MeasurementUnitsCode"];
        if (!elem.isNull()) {
          this->setMeasurementUnitsCode(elem.get("codeValue", "").asString(),
                                        elem.get("codingSchemeDesignator", "").asString(),
                                        elem.get("codeMeaning", "").asString());
        }

        elem = this->metaInfoRoot["MeasurementMethodCode"];
        if (!elem.isNull()) {
          this->setMeasurementMethodCode(elem.get("codeValue", "").asString(),
                                         elem.get("codingSchemeDesignator", "").asString(),
                                         elem.get("codeMeaning", "").asString());
        }

      } catch (exception& e) {
        cout << e.what() << endl;
        throw JSONReadErrorException();
      }
    } else
      throw JSONReadErrorException();
  }

  bool JSONParametricMapMetaInformationHandler::write(string filename) {
    ofstream outputFile;
    outputFile.open(filename);
    Json::Value data;

    data["SeriesDescription"] = this->seriesDescription;
    data["SeriesNumber"] = this->seriesNumber;
    data["InstanceNumber"] = this->instanceNumber;
    data["BodyPartExamined"] = this->bodyPartExamined;
    data["RealWorldValueSlope"] = this->realWorldValueSlope;
    data["DerivedPixelContrast"] = this->derivedPixelContrast;

    if (this->measurementUnitsCode)
      data["MeasurementUnitsCode"] = codeSequence2Json(this->measurementUnitsCode);
    if (this->measurementMethodCode)
      data["MeasurementMethodCode"] = codeSequence2Json(this->measurementMethodCode);
    if (this->quantityValueCode)
      data["QuantityValueCode"] = codeSequence2Json(this->quantityValueCode);

    Json::StyledWriter styledWriter;
    outputFile << styledWriter.write(data);

    outputFile.close();
    return true;
  };

  bool JSONParametricMapMetaInformationHandler::isValid(string filename) {
    // TODO: add validation of json file here
    return true;
  }

}
