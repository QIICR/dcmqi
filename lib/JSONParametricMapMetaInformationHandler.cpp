#include "JSONParametricMapMetaInformationHandler.h"

namespace dcmqi {

    JSONParametricMapMetaInformationHandler::JSONParametricMapMetaInformationHandler()
            : JSONMetaInformationHandlerBase() {
    }

    JSONParametricMapMetaInformationHandler::JSONParametricMapMetaInformationHandler(string filename)
        : JSONMetaInformationHandlerBase::JSONMetaInformationHandlerBase(filename){
    };

    JSONParametricMapMetaInformationHandler::~JSONParametricMapMetaInformationHandler() {
        if (this->quantityValueCode)
            delete this->quantityValueCode;
        if (this->quantityUnitsCode)
            delete this->quantityUnitsCode;
    };

    void JSONParametricMapMetaInformationHandler::setQuantityValueCode(const string& code, const string& designator, const string& meaning) {
        this->quantityValueCode = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
    }

    void JSONParametricMapMetaInformationHandler::setQuantityValueCode(const CodeSequenceMacro& codeSequence) {
        this->quantityValueCode = new CodeSequenceMacro(codeSequence);
    }

    void JSONParametricMapMetaInformationHandler::setQuantityUnitsCode(const string& code, const string& designator, const string& meaning) {
        this->quantityUnitsCode = Helper::createNewCodeSequence(code.c_str(), designator.c_str(), meaning.c_str());
    }

    void JSONParametricMapMetaInformationHandler::setQuantityUnitsCode(const CodeSequenceMacro& codeSequence) {
        this->quantityUnitsCode = new CodeSequenceMacro(codeSequence);
    }

    void JSONParametricMapMetaInformationHandler ::read() {
        if (this->filename.size() && this->isValid(this->filename)) {
            try {
                ifstream metainfoStream(this->filename, ios_base::binary);
                Json::Value root;
                metainfoStream >> root;
                this->seriesDescription = root.get("SeriesDescription", "Segmentation").asString();
                this->seriesNumber = root.get("SeriesNumber", "300").asString();
                this->instanceNumber = root.get("InstanceNumber", "1").asString();
                this->bodyPartExamined = root.get("BodyPartExamined", "").asString();
                this->realWorldValueSlope = root.get("RealWorldValueSlope", "1.0").asString();
                this->derivedPixelContrast = root.get("DerivedPixelContrast", "").asString();

                Json::Value elem = root["QuantityValueCode"];
                if (!elem.isNull()) {
                    this->setQuantityValueCode(elem.get("codeValue", "").asString(),
                                               elem.get("codingSchemeDesignator", "").asString(),
                                               elem.get("codeMeaning", "").asString());
                }

                elem = root["QuantityUnitsCode"];
                if (!elem.isNull()) {
                    this->setQuantityUnitsCode(elem.get("codeValue", "").asString(),
                                               elem.get("codingSchemeDesignator", "").asString(),
                                               elem.get("codeMeaning", "").asString());
                }
            } catch (exception& e) {
                cout << e.what() << '\n';
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

        if (this->quantityValueCode)
            data["QuantityValueCode"] = codeSequence2Json(this->quantityValueCode);

        if (this->quantityUnitsCode)
            data["QuantityUnitsCode"] = codeSequence2Json(this->quantityUnitsCode);

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