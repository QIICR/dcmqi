
// DCMTK includes
#include <dcmtk/config/osconfig.h>   // make sure OS specific configuration is included first
#include <dcmtk/ofstd/ofstd.h>
#include <dcmtk/ofstd/ofstream.h>
#include <dcmtk/ofstd/oftest.h>

#include <dcmtk/dcmsr/dsrdoc.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmiod/modhelp.h>

#include <dcmtk/dcmsr/codes/dcm.h>
#include <dcmtk/dcmsr/codes/srt.h>
#include <dcmtk/dcmsr/cmr/tid1500.h>

#include <dcmtk/dcmdata/dcdeftag.h>

// STD includes
#include <iostream>
#include <exception>

#include <json/json.h>

// DCMQI includes
#include "dcmqiVersionConfigure.h" // versioning
#include "dcmqi/Exceptions.h"
#include "dcmqi/QIICRUIDs.h" // UIDs

using namespace std;

// not used yet
static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

#include "tid1500readerCLP.h"


#define STATIC_ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

bool isCompositeEvidence(OFString& sopClassUID) {
  const char* compositeContextSOPClasses[] = {UID_SegmentationStorage, UID_RealWorldValueMappingStorage};
  for( unsigned int i=0; i<STATIC_ARRAY_SIZE(compositeContextSOPClasses); i++)
    if (sopClassUID == compositeContextSOPClasses[i])
      return true;
  return false;
}

Json::Value DSRCodedEntryValue2CodeSequence(const DSRCodedEntryValue &value) {
  Json::Value codeSequence;
  codeSequence["CodeValue"] = value.getCodeValue().c_str();
  codeSequence["CodeMeaning"] = value.getCodeMeaning().c_str();
  codeSequence["CodingSchemeDesignator"] = value.getCodingSchemeDesignator().c_str();
  return codeSequence;
}

Json::Value getMeasurements(DSRDocument &doc) {
  Json::Value measurements(Json::arrayValue);
  DSRDocumentTree &st = doc.getTree();

  DSRDocumentTreeNodeCursor cursor;
  st.getCursorToRootNode(cursor);
  if(st.gotoNamedChildNode(CODE_DCM_ImagingMeasurements)) {
    size_t nnid = st.gotoNamedChildNode(CODE_DCM_MeasurementGroup);
    do {
      if (nnid) {
        Json::Value measurement;
        if (st.gotoNamedChildNode(DSRCodedEntryValue("C67447", "NCIt", "Activity Session"))) {
          // TODO: think about it
          cout << "Activity Session: " << st.getCurrentContentItem().getStringValue().c_str() << endl;
          measurement["ActivitySession"] = st.getCurrentContentItem().getStringValue().c_str();
        }
        st.gotoNode(nnid);
        if (st.gotoNamedChildNode(CODE_DCM_ReferencedSegment)) {
          DSRImageReferenceValue referenceImage = st.getCurrentContentItem().getImageReference();
          OFVector<Uint16> items;
          referenceImage.getSegmentList().getItems(items);
          cout << "Reference Segment: " << items[0] << endl;
          measurement["ReferencedSegment"] = items[0];
          if (!referenceImage.getSOPInstanceUID().empty()){
            measurement["segmentationSOPInstanceUID"] = referenceImage.getSOPInstanceUID().c_str();
          }
        }
        st.gotoNode(nnid);
        if (st.gotoNamedChildNode(CODE_DCM_SourceSeriesForSegmentation)) {
          cout << "SourceSeriesForImageSegmentation: " << st.getCurrentContentItem().getStringValue().c_str() << endl;
          measurement["SourceSeriesForImageSegmentation"] = st.getCurrentContentItem().getStringValue().c_str();
        }
        st.gotoNode(nnid);
        if (st.gotoNamedChildNode(CODE_DCM_TrackingIdentifier)) {
          cout << "TrackingIdentifier: " << st.getCurrentContentItem().getStringValue().c_str() << endl;
          measurement["TrackingIdentifier"] = st.getCurrentContentItem().getStringValue().c_str();
        }
        st.gotoNode(nnid);
        if (st.gotoNamedChildNode(CODE_DCM_Finding)) {
          measurement["Finding"] = DSRCodedEntryValue2CodeSequence(st.getCurrentContentItem().getCodeValue());
        }
        st.gotoNode(nnid);
        if (st.gotoNamedChildNode(DSRCodedEntryValue("G-C0E3", "SRT", "Finding Site"))) {
          measurement["FindingSite"] = DSRCodedEntryValue2CodeSequence(st.getCurrentContentItem().getCodeValue());
        }
        st.gotoNode(nnid);
        st.gotoChild();

        Json::Value measurementItems(Json::arrayValue);
        while (st.gotoNext()){
          if (st.getCurrentContentItem().getNumericValuePtr() != NULL) {
            DSRNumericMeasurementValue measurementValue = st.getCurrentContentItem().getNumericValue();

            Json::Value localMeasurement;
            localMeasurement["value"] = measurementValue.getNumericValue().c_str();
            localMeasurement["units"] = DSRCodedEntryValue2CodeSequence(measurementValue.getMeasurementUnit());
            localMeasurement["quantity"] = DSRCodedEntryValue2CodeSequence(st.getCurrentContentItem().getConceptName());

            if(st.gotoNamedChildNode(CODE_DCM_Derivation)){
              localMeasurement["derivationModifier"] = DSRCodedEntryValue2CodeSequence(st.getCurrentContentItem().getCodeValue());
              st.gotoParent();
            }
            measurementItems.append(localMeasurement);
          }
        }
        measurement["measurementItems"] = measurementItems;
        measurements.append(measurement);
      }
      st.gotoNode(nnid);
      nnid = st.gotoNextNamedNode(CODE_DCM_MeasurementGroup);
    } while (nnid);
  }
  return measurements;
}

int main(int argc, char** argv){
  PARSE_ARGS;

  Json::Value metaRoot;

  DcmFileFormat sliceFF;
  CHECK_COND(sliceFF.loadFile(inputSRFileName.c_str()));
  DcmDataset* dataset = sliceFF.getDataset();

  TID1500_MeasurementReport report(CMR_CID7021::ImagingMeasurementReport);
  DSRDocument doc;

  CHECK_COND(doc.read(*dataset));


  OFString temp;
  doc.getSeriesDescription(temp);
  metaRoot["SeriesDescription"] = temp.c_str();
  doc.getSeriesNumber(temp);
  metaRoot["SeriesNumber"] = temp.c_str();
  doc.getInstanceNumber(temp);
  metaRoot["InstanceNumber"] = temp.c_str();

  cout << "Number of verifying observers: " << doc.getNumberOfVerifyingObservers() << endl;

  OFString observerName, observingDateTime, organizationName;
  if (doc.getNumberOfVerifyingObservers() != 0) {
    doc.getVerifyingObserver(1, observingDateTime, observerName, organizationName);
    metaRoot["observerContext"]["ObserverType"] = "PERSON";
    metaRoot["observerContext"]["PersonObserverName"] = observerName.c_str();
  }

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
//      cout << "add composite" << endl;
      compositeContextUIDs.append(sopInstanceUID.c_str());
    }else {
//      cout << "add image library" << endl;
      imageLibraryUIDs.append(sopInstanceUID.c_str());
    }
    cond = evidenceList.gotoNextItem();
  }
  if (!imageLibraryUIDs.empty())
    metaRoot["imageLibrary"] = imageLibraryUIDs;
  if (!compositeContextUIDs.empty())
    metaRoot["compositeContext"] = compositeContextUIDs;

  metaRoot["Measurements"] = getMeasurements(doc);

  ofstream outputFile;

  outputFile.open(metaDataFileName.c_str());

  outputFile << metaRoot;
  outputFile.close();

  return 0;
}