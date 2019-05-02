
// DCMQI includes
#include "dcmqi/JSONParametricMapMetaInformationHandler.h"

namespace dcmqi {

  JSONParametricMapMetaInformationHandler::JSONParametricMapMetaInformationHandler()
      : JSONMetaInformationHandlerBase(),
        measurementUnitsCode(NULL),
        measurementMethodCode(NULL),
        quantityValueCode(NULL),
        anatomicRegionSequence(NULL),
        derivationCode(NULL){
  }

  JSONParametricMapMetaInformationHandler::JSONParametricMapMetaInformationHandler(string jsonInput)
      : JSONMetaInformationHandlerBase(jsonInput),
        measurementUnitsCode(NULL),
        measurementMethodCode(NULL),
        quantityValueCode(NULL),
        anatomicRegionSequence(NULL),
        derivationCode(NULL){
  }

  JSONParametricMapMetaInformationHandler::~JSONParametricMapMetaInformationHandler() {
    if (this->measurementUnitsCode)
      delete this->measurementUnitsCode;
    if (this->measurementMethodCode)
      delete this->measurementMethodCode;
    if (this->quantityValueCode)
      delete this->quantityValueCode;
    if (this->anatomicRegionSequence)
      delete this->anatomicRegionSequence;
    if (this->derivationCode)
      delete this->derivationCode;
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

  void JSONParametricMapMetaInformationHandler::setDerivationDescription(const string& value) {
    this->derivationDescription = value;
  };

  void JSONParametricMapMetaInformationHandler::setMeasurementUnitsCode(const string& code, const string& designator,
                                                                        const string& meaning) {
    this->measurementUnitsCode = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void JSONParametricMapMetaInformationHandler::setDerivationCode(const string &code, const string &designator,
                                                                  const string &meaning) {
    this->derivationCode = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void JSONParametricMapMetaInformationHandler::addSourceImageDiffusionBValue(const string& value){
    this->diffusionBValues.push_back(value);
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

  void JSONParametricMapMetaInformationHandler::setAnatomicRegionSequence(const string &code, const string &designator,
                                                                          const string &meaning) {
    this->anatomicRegionSequence = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
  }

  void JSONParametricMapMetaInformationHandler::setAnatomicRegionSequence(const CodeSequenceMacro& codeSequence) {
    this->anatomicRegionSequence = new CodeSequenceMacro(codeSequence);
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
      //std::cout << this->metaInfoRoot.asString() << std::endl;
      this->seriesDescription = this->metaInfoRoot.get("SeriesDescription", "Segmentation").asString();
      this->seriesNumber = this->metaInfoRoot.get("SeriesNumber", "300").asString();
      this->instanceNumber = this->metaInfoRoot.get("InstanceNumber", "1").asString();
      this->bodyPartExamined = this->metaInfoRoot.get("BodyPartExamined", "").asString();
      this->realWorldValueSlope = this->metaInfoRoot.get("RealWorldValueSlope", "1.0").asString();
      this->realWorldValueIntercept = this->metaInfoRoot.get("RealWorldValueIntercept", "0").asString();
      this->derivedPixelContrast = this->metaInfoRoot.get("DerivedPixelContrast", "").asString();
      this->derivationDescription = this->metaInfoRoot.get("DerivationDescription", "").asString();
      this->frameLaterality = this->metaInfoRoot.get("FrameLaterality", "U").asString();

      if (this->metaInfoRoot.isMember("QuantityValueCode")) {
        Json::Value elem = this->metaInfoRoot["QuantityValueCode"];
        this->setQuantityValueCode(elem.get("CodeValue", "").asString(),
                                   elem.get("CodingSchemeDesignator", "").asString(),
                                   elem.get("CodeMeaning", "").asString());
      }

      if (this->metaInfoRoot.isMember("MeasurementUnitsCode")) {
        Json::Value elem  = this->metaInfoRoot["MeasurementUnitsCode"];
        this->setMeasurementUnitsCode(elem.get("CodeValue", "").asString(),
                                      elem.get("CodingSchemeDesignator", "").asString(),
                                      elem.get("CodeMeaning", "").asString());
      }

      if (this->metaInfoRoot.isMember("MeasurementMethodCode")) {
        Json::Value elem  = this->metaInfoRoot["MeasurementMethodCode"];
        this->setMeasurementMethodCode(elem.get("CodeValue", "").asString(),
                                       elem.get("CodingSchemeDesignator", "").asString(),
                                       elem.get("CodeMeaning", "").asString());
      }

      if (this->metaInfoRoot.isMember("AnatomicRegionSequence")) {
        Json::Value elem  = this->metaInfoRoot["AnatomicRegionSequence"];
        this->setAnatomicRegionSequence(elem.get("CodeValue", "").asString(),
                                          elem.get("CodingSchemeDesignator", "").asString(),
                                          elem.get("CodeMeaning", "").asString());
      }

      if (this->metaInfoRoot.isMember("DerivationCode")) {
        Json::Value elem  = this->metaInfoRoot["DerivationCode"];
        this->setDerivationCode(elem.get("CodeValue", "").asString(),
                                elem.get("CodingSchemeDesignator", "").asString(),
                                elem.get("CodeMeaning", "").asString());
      }

    } catch (exception& e) {
      cerr << "ERROR: JSON parameter file could not be parsed!" << std::endl;
      cerr << "You can validate the JSON file here: http://qiicr.org/dcmqi/#/validators" << std::endl;
      cerr << "Exception details (probably not very useful): " << e.what() << endl;
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
    data["DerivationDescription"] = this->derivationDescription;

    if (this->measurementUnitsCode)
      data["MeasurementUnitsCode"] = codeSequence2Json(this->measurementUnitsCode);
    if (this->measurementMethodCode)
      data["MeasurementMethodCode"] = codeSequence2Json(this->measurementMethodCode);
    if (this->quantityValueCode)
      data["QuantityValueCode"] = codeSequence2Json(this->quantityValueCode);
    if (this->anatomicRegionSequence)
      data["AnatomicRegionSequence"] = codeSequence2Json(this->anatomicRegionSequence);
    if (this->derivationCode)
      data["DerivationCode"] = codeSequence2Json(this->derivationCode);

    if (this->diffusionBValues.size() > 0) {
      data["SourceImageDiffusionBValues"] = Json::Value(Json::arrayValue);
      for (vector<string>::iterator it = this->diffusionBValues.begin() ; it != this->diffusionBValues.end(); ++it)
        data["SourceImageDiffusionBValues"].append(*it);
    }

    Json::StyledWriter styledWriter;

    ss << styledWriter.write(data);

    return ss.str();
  }

}
