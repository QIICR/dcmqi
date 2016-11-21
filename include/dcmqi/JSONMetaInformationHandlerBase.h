#ifndef DCMQI_JSON_METAINFORMATION_HANDLER_H
#define DCMQI_JSON_METAINFORMATION_HANDLER_H

#include <json/json.h>

// STD includes
#include <vector>

// DCMQI includes
#include "dcmqi/SegmentAttributes.h"
#include "dcmqi/Exceptions.h"


using namespace std;

namespace dcmqi {

  class JSONMetaInformationHandlerBase {

  public:
    JSONMetaInformationHandlerBase();
    JSONMetaInformationHandlerBase(string jsonInput);
    virtual ~JSONMetaInformationHandlerBase();

    void setSeriesDescription(const string &seriesDescription);
    void setSeriesNumber(const string &seriesNumber);
    void setInstanceNumber(const string &instanceNumber);
    void setBodyPartExamined(const string &bodyPartExamined);

    string getSeriesDescription() const { return seriesDescription; }
    string getSeriesNumber() const { return seriesNumber; }
    string getInstanceNumber() const { return instanceNumber;}
    string getBodyPartExamined() const { return bodyPartExamined; }

    virtual void read()=0;
    virtual bool write(string filename)=0;

    static string getCodeSequenceValue(CodeSequenceMacro* codeSequence);
    static string getCodeSequenceDesignator(CodeSequenceMacro* codeSequence);
    static string getCodeSequenceMeaning(CodeSequenceMacro* codeSequence);
    static Json::Value codeSequence2Json(CodeSequenceMacro *codeSequence);

    // need to revisit
    Json::Value metaInfoRoot;

  protected:

    string jsonInput;

    string seriesDescription;
    string seriesNumber;
    string instanceNumber;
    string bodyPartExamined;
  };
}


#endif //DCMQI_JSON_METAINFORMATION_HANDLER_H
