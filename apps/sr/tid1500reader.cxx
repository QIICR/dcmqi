
// DCMTK includes
#include <dcmtk/config/osconfig.h>   // make sure OS specific configuration is included first
#include <dcmtk/ofstd/ofstd.h>
#include <dcmtk/ofstd/ofstream.h>
#include <dcmtk/ofstd/oftest.h>

#include <dcmtk/dcmsr/dsrdoc.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmiod/modhelp.h>

#include <dcmtk/dcmsr/codes/dcm.h>
#include <dcmtk/dcmsr/codes/sct.h>
#include <dcmtk/dcmsr/codes/umls.h>
#include <dcmtk/dcmsr/codes/ncit.h>
#include <dcmtk/dcmsr/cmr/tid1500.h>

#include <dcmtk/dcmdata/dcdeftag.h>

// STD includes
#include <iostream>
#include <exception>

#include <json/json.h>

// DCMQI includes
#include "dcmqi/Exceptions.h"
#include "dcmqi/QIICRConstants.h"
#include "dcmqi/QIICRUIDs.h"
#include "dcmqi/internal/VersionConfigure.h"
#include "dcmqi/Helper.h"
#include "dcmqi/TID1500Reader.h"

using namespace std;

// not used yet
static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

// CLP includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "tid1500readerCLP.h"


#define STATIC_ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

bool isCompositeEvidence(OFString& sopClassUID) {
  const char* compositeContextSOPClasses[] = {UID_SegmentationStorage, UID_RealWorldValueMappingStorage};
  for( unsigned int i=0; i<STATIC_ARRAY_SIZE(compositeContextSOPClasses); i++)
    if (sopClassUID == compositeContextSOPClasses[i])
      return true;
  return false;
}

int main(int argc, char** argv){
  std::cout << dcmqi_INFO << std::endl;

  PARSE_ARGS;

  if(dcmqi::Helper::isUndefinedOrPathDoesNotExist(inputSRFileName, "Input DICOM file")) {
    return EXIT_FAILURE;
  }

  Json::Value metaRoot;

  // first read the dataset
  DcmFileFormat sliceFF;
  CHECK_COND(sliceFF.loadFile(inputSRFileName.c_str()));
  DcmDataset& dataset = *sliceFF.getDataset();

  // then, read the SR document from the DICOM dataset
  DSRDocument doc;
  if (doc.read(dataset).good()) {
    TID1500Reader reader(doc.getTree());

    Json::Value procedureCode;
    procedureCode = reader.getProcedureReported();
    if(procedureCode.isMember("CodeValue")){
      metaRoot["procedureReported"] = procedureCode;
    }

    Json::Value observerContext = reader.getObserverContext();
    metaRoot["observerContext"] = observerContext;

    //DSRDocumentTreeNodeCursor rootCursor;
    //if(doc.getTree().getCursorToRootNode(rootCursor) == 1)
    //  std::cout << "Have root node: " << rootCursor.getNode()->getNodeID() << std::endl;

    metaRoot["Measurements"] = reader.getMeasurements();
  }

  OFString temp;
  doc.getSeriesDescription(temp);
  metaRoot["SeriesDescription"] = temp.c_str();
  doc.getSeriesNumber(temp);
  metaRoot["SeriesNumber"] = temp.c_str();
  doc.getInstanceNumber(temp);
  metaRoot["InstanceNumber"] = temp.c_str();

  metaRoot["VerificationFlag"] = DSRTypes::verificationFlagToEnumeratedValue(doc.getVerificationFlag());
  metaRoot["CompletionFlag"] = DSRTypes::completionFlagToEnumeratedValue(doc.getCompletionFlag());

  Json::Value compositeContextUIDs(Json::arrayValue);
  Json::Value imageLibraryUIDs(Json::arrayValue);

  // TODO: We need to think about that, because actually the file names are stored in the json and not the UIDs
  DSRSOPInstanceReferenceList &evidenceList = doc.getCurrentRequestedProcedureEvidence();
  OFCondition cond = evidenceList.gotoFirstItem();
  OFString sopInstanceUID;
  OFString sopClassUID;
  while(cond.good()) {
    evidenceList.getSOPClassUID(sopClassUID);
    evidenceList.getSOPInstanceUID(sopInstanceUID).c_str();
    if (isCompositeEvidence(sopClassUID)) {
      compositeContextUIDs.append(sopInstanceUID.c_str());
    }else {
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
