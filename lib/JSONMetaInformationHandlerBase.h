#ifndef DCMQI_JSON_METAINFORMATION_HANDLER_H
#define DCMQI_JSON_METAINFORMATION_HANDLER_H

#include <json/json.h>

#include <vector>

#include "SegmentAttributes.h"
#include "Exceptions.h"


using namespace std;

namespace dcmqi {

  class JSONMetaInformationHandlerBase {

  public:
    JSONMetaInformationHandlerBase();
    JSONMetaInformationHandlerBase(string filename);
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

  protected:
    virtual bool isValid(string filename)=0;

    string seriesDescription;
    string seriesNumber;
    string instanceNumber;
    string bodyPartExamined;

    string filename;

    Json::Value codeSequence2Json(CodeSequenceMacro *codeSequence);
  };
}


#endif //DCMQI_JSON_METAINFORMATION_HANDLER_H
