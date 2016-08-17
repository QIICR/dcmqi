//#include "TID1500Converter.h"

#include "dcmtk/config/osconfig.h"   // make sure OS specific configuration is included first

// UIDs
#include "QIICRUIDs.h"

// versioning
#include "dcmqiVersionConfigure.h"

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/dcmsr/dsrdoc.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmsr/dsriodcc.h"
#include "dcmtk/dcmiod/modhelp.h"
//#include "dcmtk/dcmdata/modhelp.h"

#include "dcmtk/ofstd/oftest.h"

#include "dcmtk/dcmsr/dsrdoctr.h"
#include "dcmtk/dcmsr/dsrcontn.h"
#include "dcmtk/dcmsr/dsrnumtn.h"
#include "dcmtk/dcmsr/dsruidtn.h"
#include "dcmtk/dcmsr/dsrtextn.h"
#include "dcmtk/dcmsr/dsrcodtn.h"
#include "dcmtk/dcmsr/dsrimgtn.h"
#include "dcmtk/dcmsr/dsrcomtn.h"
#include "dcmtk/dcmsr/dsrpnmtn.h"

#include "dcmtk/dcmsr/codes/dcm.h"
#include "dcmtk/dcmsr/codes/srt.h"
#include "dcmtk/dcmsr/cmr/tid1500.h"

//#include "JSONMetaInformationHandlerBase.h"

#include <iostream>
#include <exception>

#include <json/json.h>

#include "Exceptions.h"

using namespace std;

static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

#include "tid1500writerCLP.h"

#define CHECK_BOOL(condition) \
  do { \
    if (!condition) { \
      std::cerr << "Expected True in " __FILE__ << ":" << __LINE__ << " " << std::cout; \
      throw -1; \
    } \
  } while (0);

int main(int argc, char** argv){
  PARSE_ARGS;

  Json::Value metaRoot;

  try {
      ifstream metainfoStream(metaDataFileName.c_str(), ifstream::binary);
      metainfoStream >> metaRoot;
  } catch (exception& e) {
      cout << e.what() << '\n';
      return -1;
  }

  TID1500_MeasurementReport report(CMR_CID7021::ImagingMeasurementReport);
  DSRCodedEntryValue title;

  /* create a new report */
  OFCHECK(report.createNewMeasurementReport(CMR_CID7021::ImagingMeasurementReport).good());

  /* set the language */
  OFCHECK(report.setLanguage(DSRCodedEntryValue("eng", "RFC5646", "English")).good());

  /* set details on the observation context */
  string observerType = metaRoot["observerContext"]["ObserverType"].asCString();
  if(observerType == "PERSON"){
    OFCHECK(report.getObservationContext().addPersonObserver(metaRoot["observerContext"]["PersonObserverName"].asCString(), "").good());
  } else if(observerType == "DEVICE"){
    OFCHECK(report.getObservationContext().addDeviceObserver(metaRoot["observerContext"]["DeviceObserverUID"].asCString()).good());
  }

  // TODO - image library (note: invalid document if no Image Library present)
  OFCHECK(report.getImageLibrary().createNewImageLibrary().good());

  // TODO
  //  - this is a very narrow procedure code
  // see duscussion here for improved handling, should be factored out in the
  // future, and handled by the upper-level application layers:
  // https://github.com/QIICR/dcmqi/issues/30
  OFCHECK(report.addProcedureReported(DSRCodedEntryValue("P0-0099A", "SRT", "Imaging procedure")).good());

  if(!report.isValid()){
    cerr << "Report invalid!" << endl;
    return -1;
  }

  std::cout << "Total measurement groups: " << metaRoot["Measurements"].size() << std::endl;

  for(int i=0;i<metaRoot["Measurements"].size();i++){
    Json::Value measurementGroup = metaRoot["Measurements"][i]["MeasurementGroup"];

    OFCHECK(report.addVolumetricROIMeasurements().good());
    /* fill volumetric ROI measurements with data */
    TID1500_MeasurementReport::TID1411_Measurements &measurements =   report.getVolumetricROIMeasurements();
    //std::cout << measurementGroup["TrackingIdentifier"] << std::endl;
    OFCHECK(measurements.setTrackingIdentifier(measurementGroup["TrackingIdentifier"].asString().c_str()).good());

    if(measurementGroup.isMember("TrackingUniqueIdentifier")) {
      OFCHECK(measurements.setTrackingUniqueIdentifier(measurementGroup["TrackingUniqueIdentifier"].asString().c_str()).good());
    } else {
      char uid[100];
      dcmGenerateUniqueIdentifier(uid, SITE_STUDY_UID_ROOT);
      OFCHECK(measurements.setTrackingUniqueIdentifier(uid).good());
    }

    OFCHECK(measurements.setSourceSeriesForSegmentation(measurementGroup["SourceSeriesForImageSegmentation"].asString().c_str()).good());

    OFCHECK(measurements.setRealWorldValueMap(DSRCompositeReferenceValue(UID_RealWorldValueMappingStorage, measurementGroup["rwvmMapUsedForMeasurement"].asCString())).good());

    DSRImageReferenceValue segment(UID_SegmentationStorage, measurementGroup["segmentationSOPInstanceUID"].asString().c_str());
    segment.getSegmentList().addItem(measurementGroup["ReferencedSegment"].asInt());
    OFCHECK(measurements.setReferencedSegment(segment).good());

    // TODO - a good candidate to factor out
    DSRBasicCodedEntry findingCode(measurementGroup["Finding"]["codeValue"].asCString(),
      measurementGroup["Finding"]["codingSchemeDesignator"].asCString(),
      measurementGroup["Finding"]["codeMeaning"].asCString());
    OFCHECK(measurements.setFinding(findingCode).good());

    DSRBasicCodedEntry findingSiteCode(measurementGroup["FindingSite"]["codeValue"].asCString(),
      measurementGroup["FindingSite"]["codingSchemeDesignator"].asCString(),
      measurementGroup["FindingSite"]["codeMeaning"].asCString());
    OFCHECK(measurements.setFindingSite(findingSiteCode).good());

    DSRCodedEntryValue measurementMethodCode(measurementGroup["MeasurementMethod"]["codeValue"].asCString(),
      measurementGroup["MeasurementMethod"]["codingSchemeDesignator"].asCString(),
      measurementGroup["MeasurementMethod"]["codeMeaning"].asCString());
    OFCHECK(measurements.setMeasurementMethod(measurementMethodCode).good());

    // TODO - handle conditional items!
    for(int j=0;j<measurementGroup["measurementItems"].size();j++){
      Json::Value measurement = measurementGroup["measurementItems"][j];
      DSRCodedEntryValue quantityCode(measurement["quantity"]["codeValue"].asCString(),
        measurement["quantity"]["codingSchemeDesignator"].asCString(),
        measurement["quantity"]["codeMeaning"].asCString());
      DSRCodedEntryValue unitsCode(measurement["units"]["codeValue"].asCString(),
        measurement["units"]["codingSchemeDesignator"].asCString(),
        measurement["units"]["codeMeaning"].asCString());
      // TODO - add measurement method and derivation!
      const CMR_TID1411_in_TID1500::MeasurementValue numValue(measurement["value"].asCString(), unitsCode);

      if(measurement.isMember("derivationModifier")){
        DSRCodedEntryValue derivationModifier(measurement["derivationModifier"]["codeValue"].asCString(),
          measurement["derivationModifier"]["codingSchemeDesignator"].asCString(),
          measurement["derivationModifier"]["codeMeaning"].asCString());
          measurements.addMeasurement(quantityCode, numValue, DSRCodedEntryValue(), derivationModifier);
      } else {
        measurements.addMeasurement(quantityCode, numValue);
      }
    }
  }
   
 if(!report.isValid()){
   cerr << "Report is not valid!" << endl;
   return -1;
 }

  DSRDocument doc;
  std::cout << "Setting tree from the report" << std::endl;
  OFCondition cond = doc.setTreeFromRootTemplate(report, OFTrue /*expandTree*/);
  if(cond.bad()){
    std::cout << "Failure: " << cond.text() << std::endl;
    return -1;
  }

  // WARNING: no consistency checks between the referenced UIDs and the
  //  referencedDICOMFileNames ...
  if(metaRoot.isMember("referencedDICOMFileNames")){
    for(int i=0;i<metaRoot["referencedDICOMFileNames"].size();i++){
      DcmFileFormat ff;
      // Not sure what is a safe way to combine path components ...
      string dicomFilePath = (dicomDataDir+"/"+metaRoot["referencedDICOMFileNames"][i].asCString());
      cout << "Loading " << dicomFilePath << endl;
      CHECK_COND(ff.loadFile(dicomFilePath.c_str()));
      doc.getCurrentRequestedProcedureEvidence().addItem(*ff.getDataset());
    }
  }

  OFCHECK_EQUAL(doc.getDocumentType(), DSRTypes::DT_EnhancedSR);

  std::cout << "About to write the document" << std::endl;
  if(outputFileName.size()){
    DcmFileFormat ff, ffReference;
    DcmDataset *dataset = ff.getDataset();
    CHECK_COND(doc.write(*dataset));

    CHECK_COND(ffReference.loadFile(referenceDICOMFileName.c_str()));

    DcmModuleHelpers::copyPatientModule(*ffReference.getDataset(),*dataset);
    DcmModuleHelpers::copyPatientStudyModule(*ffReference.getDataset(),*dataset);
    DcmModuleHelpers::copyGeneralStudyModule(*ffReference.getDataset(),*dataset);

    OFCHECK(ff.saveFile(outputFileName.c_str(), EXS_LittleEndianExplicit).good());
    std::cout << "SR saved!" << std::endl;
  }

  return 0;
}
