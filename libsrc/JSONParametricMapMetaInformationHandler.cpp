#include "JSONParametricMapMetaInformationHandler.h"

namespace dcmqi {

  JSONParametricMapMetaInformationHandler::JSONParametricMapMetaInformationHandler()
      : JSONMetaInformationHandlerBase(),
        measurementUnitsCode(NULL),
        measurementMethodCode(NULL),
        quantityValueCode(NULL),
        anatomicRegionCode(NULL) {
  }

  JSONParametricMapMetaInformationHandler::JSONParametricMapMetaInformationHandler(string jsonInput)
      : JSONMetaInformationHandlerBase(jsonInput),
        measurementUnitsCode(NULL),
        measurementMethodCode(NULL),
        quantityValueCode(NULL),
        anatomicRegionCode(NULL) {
  }

  JSONParametricMapMetaInformationHandler::~JSONParametricMapMetaInformationHandler() {
    if (this->measurementUnitsCode)
      delete this->measurementUnitsCode;
    if (this->measurementMethodCode)
      delete this->measurementMethodCode;
    if (this->quantityValueCode)
      delete this->quantityValueCode;
    if (this->anatomicRegionCode)
      delete this->anatomicRegionCode;
  }

  void JSONParametricMapMetaInformationHandler::setFrameLaterality(const string& value) {
    this->frameLaterality = value;
  };

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

  void JSONParametricMapMetaInformationHandler::setAnatomicRegion(const string& code, const string& designator,
                                                                  const string& meaning) {
    this->anatomicRegionCode = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void JSONParametricMapMetaInformationHandler::setAnatomicRegion(const CodeSequenceMacro& codeSequence) {
    this->anatomicRegionCode = new CodeSequenceMacro(codeSequence);
  }

  void JSONParametricMapMetaInformationHandler::setLastValueMapped(const short &value) {
    this->lastValueMapped = value;
  }

  void JSONParametricMapMetaInformationHandler::setFirstValueMapped(const short &value) {
    this->firstValueMapped = value;
  }

  void JSONParametricMapMetaInformationHandler ::read() {
    try {
      istringstream metainfoStream(this->jsonInput);
      metainfoStream >> this->metaInfoRoot;
      this->seriesDescription = this->metaInfoRoot.get("SeriesDescription", "Segmentation").asString();
      this->seriesNumber = this->metaInfoRoot.get("SeriesNumber", "300").asString();
      this->instanceNumber = this->metaInfoRoot.get("InstanceNumber", "1").asString();
      this->bodyPartExamined = this->metaInfoRoot.get("BodyPartExamined", "").asString();
      this->realWorldValueSlope = this->metaInfoRoot.get("RealWorldValueSlope", "1").asString();
      this->realWorldValueIntercept = this->metaInfoRoot.get("RealWorldValueIntercept", "0").asString();
      this->derivedPixelContrast = this->metaInfoRoot.get("DerivedPixelContrast", "").asString();
      this->frameLaterality = this->metaInfoRoot.get("FrameLaterality", "").asString();

      Json::Value elem = this->metaInfoRoot["QuantityValueCode"];
      if (!elem.isNull()) {
        this->setQuantityValueCode(elem.get("CodeValue", "").asString(),
                                   elem.get("CodingSchemeDesignator", "").asString(),
                                   elem.get("CodeMeaning", "").asString());
      }

      elem = this->metaInfoRoot["MeasurementUnitsCode"];
      if (!elem.isNull()) {
        this->setMeasurementUnitsCode(elem.get("CodeValue", "").asString(),
                                      elem.get("CodingSchemeDesignator", "").asString(),
                                      elem.get("CodeMeaning", "").asString());
      }

      elem = this->metaInfoRoot["MeasurementMethodCode"];
      if (!elem.isNull()) {
        this->setMeasurementMethodCode(elem.get("CodeValue", "").asString(),
                                       elem.get("CodingSchemeDesignator", "").asString(),
                                       elem.get("CodeMeaning", "").asString());
      }

      elem = this->metaInfoRoot["AnatomicRegionCode"];
      if (!elem.isNull()) {
        this->setAnatomicRegion(elem.get("CodeValue", "").asString(),
                                elem.get("CodingSchemeDesignator", "").asString(),
                                elem.get("CodeMeaning", "").asString());
      }

    } catch (exception& e) {
      cout << e.what() << endl;
      throw JSONReadErrorException();
    }
  }

  bool JSONParametricMapMetaInformationHandler::write(string filename) {
    ofstream outputFile;
    outputFile.open(filename.c_str());
    outputFile << this->getJSONOutputAsString();
    outputFile.close();
    return true;
  };

  string JSONParametricMapMetaInformationHandler::getJSONOutputAsString() {
    Json::Value data;
    std::stringstream ss;

    data["SeriesDescription"] = this->seriesDescription;
    data["SeriesNumber"] = this->seriesNumber;
    data["InstanceNumber"] = this->instanceNumber;
    data["BodyPartExamined"] = this->bodyPartExamined;
    data["RealWorldValueSlope"] = this->realWorldValueSlope;
    data["DerivedPixelContrast"] = this->derivedPixelContrast;
    data["FrameLaterality"] = this->frameLaterality;

    if (this->measurementUnitsCode)
      data["MeasurementUnitsCode"] = codeSequence2Json(this->measurementUnitsCode);
    if (this->measurementMethodCode)
      data["MeasurementMethodCode"] = codeSequence2Json(this->measurementMethodCode);
    if (this->quantityValueCode)
      data["QuantityValueCode"] = codeSequence2Json(this->quantityValueCode);
    if (this->anatomicRegionCode)
      data["AnatomicRegionCode"] = codeSequence2Json(this->anatomicRegionCode);

    Json::StyledWriter styledWriter;

    ss << styledWriter.write(data);

    return ss.str();
  }

}
