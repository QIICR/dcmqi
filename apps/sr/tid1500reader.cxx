#include "dcmtk/config/osconfig.h"   // make sure OS specific configuration is included first

// UIDs
#include "QIICRUIDs.h"

// versioning
#include "dcmqiVersionConfigure.h"

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/ofstd/oftest.h"
#include "dcmtk/ofstd/ofstd.h"

#include "dcmtk/dcmsr/dsrdoc.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmiod/modhelp.h"

#include "dcmtk/dcmsr/codes/dcm.h"
#include "dcmtk/dcmsr/codes/srt.h"
#include "dcmtk/dcmsr/cmr/tid1500.h"

#include "dcmtk/dcmdata/dcdeftag.h"

#include <iostream>
#include <exception>

#include <json/json.h>

#include "Exceptions.h"

using namespace std;

// not used yet
static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

#include "tid1500readerCLP.h"

#define CHECK_BOOL(condition) \
  do { \
    if (!condition) { \
      std::cerr << "Expected True in " __FILE__ << ":" << __LINE__ << " " << std::cout; \
      throw -1; \
    } \
  } while (0);

#define STATIC_ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

DSRCodedEntryValue json2cev(Json::Value& j){
  return DSRCodedEntryValue(j["codeValue"].asCString(),
    j["codingSchemeDesignator"].asCString(),
    j["codeMeaning"].asCString());
}

void addFileToEvidence(DSRDocument &doc, string dirStr, string fileStr){
  DcmFileFormat ff;
  OFString fullPath;
  CHECK_COND(ff.loadFile(OFStandard::combineDirAndFilename(fullPath,dirStr.c_str(),fileStr.c_str())));
  CHECK_COND(doc.getCurrentRequestedProcedureEvidence().addItem(*ff.getDataset()));
}

bool isCompositeEvidence(OFString& sopClassUID) {
  const char* compositeContextSOPClasses[] = {UID_SegmentationStorage, UID_RealWorldValueMappingStorage};
  int length = sizeof(compositeContextSOPClasses)/sizeof(*compositeContextSOPClasses);
  for( unsigned int i=0; i<length; i++)
  {
    cout << "compare " + sopClassUID + " with " + compositeContextSOPClasses[i] << endl;
    if (sopClassUID == compositeContextSOPClasses[i])
      return true;
  }
  return false;
}

int main(int argc, char** argv){
  PARSE_ARGS;

  Json::Value metaRoot;

  DcmFileFormat sliceFF;
  CHECK_COND(sliceFF.loadFile(inputSRFileName.c_str()));
  DcmDataset* dataset = sliceFF.getDataset();

  DSRDocument doc;

  CHECK_COND(doc.read(*dataset));

  OFString temp;
  doc.getSeriesDescription(temp);
  metaRoot["SeriesDescription"] = temp.c_str();
  doc.getSeriesNumber(temp);
  metaRoot["SeriesNumber"] = temp.c_str();
  doc.getInstanceNumber(temp);
  metaRoot["InstanceNumber"] = temp.c_str();

  Json::Value compositeContextUIDs(Json::arrayValue);
  Json::Value imageLibraryUIDs(Json::arrayValue);

  DSRSOPInstanceReferenceList &evidenceList = doc.getCurrentRequestedProcedureEvidence();
  OFCondition cond = evidenceList.gotoFirstItem();
  OFString sopInstanceUID;
  OFString sopClassUID;
  while(cond.good()) {
    evidenceList.getSOPClassUID(sopClassUID);
    evidenceList.getSOPInstanceUID(sopInstanceUID).c_str();
    if (isCompositeEvidence(sopClassUID)) {
      cout << "add composite" << endl;
      compositeContextUIDs.append(sopInstanceUID.c_str());
    }else {
      cout << "add image library" << endl;
      imageLibraryUIDs.append(sopInstanceUID.c_str());
    }
    cond = evidenceList.gotoNextItem();
  }
  if (!imageLibraryUIDs.empty())
    metaRoot["imageLibrary"] = imageLibraryUIDs;
  if (!compositeContextUIDs.empty())
    metaRoot["compositeContext"] = compositeContextUIDs;

  ofstream outputFile;

  outputFile.open(metaDataFileName.c_str());

  outputFile << metaRoot;
  outputFile.close();

  return 0;
}