//#include "TID1500Converter.h"

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

#include "tid1500writerCLP.h"

#define CHECK_BOOL(condition) \
  do { \
    if (!condition) { \
      std::cerr << "Expected True in " __FILE__ << ":" << __LINE__ << " " << std::cout; \
      throw -1; \
    } \
  } while (0);

DSRBasicCodedEntry json2bce(Json::Value& j){
  return DSRBasicCodedEntry(j["codeValue"].asCString(),
    j["codingSchemeDesignator"].asCString(),
    j["codeMeaning"].asCString());
}

DSRCodedEntryValue json2cev(Json::Value& j){
  return DSRCodedEntryValue(j["codeValue"].asCString(),
    j["codingSchemeDesignator"].asCString(),
    j["codeMeaning"].asCString());
}

void copyElement(const DcmTag& tag, DcmDataset* d1, DcmDataset* d2){
  const char* str = NULL;
  if(d1->findAndGetString(tag,str).good()){
    OFCHECK(d2->putAndInsertString(tag,str).good());
  }
}

void addFileToEvidence(DSRDocument &doc, string dirStr, string fileStr){
  DcmFileFormat ff;
  OFString dir = dirStr.c_str(),
     imageLibItem = fileStr.c_str(), fullPath;
  CHECK_COND(ff.loadFile(OFStandard::combineDirAndFilename(fullPath,dir,imageLibItem)));
  doc.getCurrentRequestedProcedureEvidence().addItem(*ff.getDataset());
}

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

  /* set the language */
  CHECK_COND(report.setLanguage(DSRCodedEntryValue("eng", "RFC5646", "English")));

  /* set details on the observation context */
  string observerType = metaRoot["observerContext"]["ObserverType"].asCString();
  if(observerType == "PERSON"){
    CHECK_COND(report.getObservationContext().addPersonObserver(metaRoot["observerContext"]["PersonObserverName"].asCString(), ""));
  } else if(observerType == "DEVICE"){
    CHECK_COND(report.getObservationContext().addDeviceObserver(metaRoot["observerContext"]["DeviceObserverUID"].asCString()));
  }

  // Image library must be present, even if empty

  CHECK_COND(report.getImageLibrary().createNewImageLibrary());
  CHECK_COND(report.getImageLibrary().addImageGroup());

  if(metaRoot.isMember("imageLibrary")){
    for(int i=0;i<metaRoot["imageLibrary"].size();i++){
      DcmFileFormat ff;
      // Not sure what is a safe way to combine path components ...
      string dicomFilePath = (imageLibraryDataDir+"/"+metaRoot["imageLibrary"][i].asCString());
      cout << "Loading " << dicomFilePath << endl;
      CHECK_COND(ff.loadFile(dicomFilePath.c_str()));


      DcmDataset imageLibDataset;
      DcmTag tagsToCopy[] = {DCM_SOPClassUID,DCM_SOPInstanceUID,DCM_Modality,DCM_StudyDate,DCM_Columns,DCM_Rows,DCM_PixelSpacing,DCM_BodyPartExamined,DCM_ImageOrientationPatient};
      // no DCM_ImagePositionPatient - it should be under individual image
      for(int t=0;t<9;t++){
        copyElement(tagsToCopy[t], ff.getDataset(), &imageLibDataset);
      }

      if(i==0)
        CHECK_COND(report.getImageLibrary().addImageEntryDescriptors(imageLibDataset));

      // TODO: would also be nice to include ImagePositionPatient under each individual image entry!
      CHECK_COND(report.getImageLibrary().addImageEntry(*ff.getDataset()));
    }
  }

  // TODO
  //  - this is a very narrow procedure code
  // see duscussion here for improved handling, should be factored out in the
  // future, and handled by the upper-level application layers:
  // https://github.com/QIICR/dcmqi/issues/30
  CHECK_COND(report.addProcedureReported(DSRCodedEntryValue("P0-0099A", "SRT", "Imaging procedure")));

  if(!report.isValid()){
    cerr << "Report invalid!" << endl;
    return -1;
  }

  std::cout << "Total measurement groups: " << metaRoot["Measurements"].size() << std::endl;

  for(int i=0;i<metaRoot["Measurements"].size();i++){
    Json::Value measurementGroup = metaRoot["Measurements"][i]["MeasurementGroup"];

    CHECK_COND(report.addVolumetricROIMeasurements());
    /* fill volumetric ROI measurements with data */
    TID1500_MeasurementReport::TID1411_Measurements &measurements =   report.getVolumetricROIMeasurements();
    //std::cout << measurementGroup["TrackingIdentifier"] << std::endl;
    CHECK_COND(measurements.setTrackingIdentifier(measurementGroup["TrackingIdentifier"].asCString()));

    if(metaRoot.isMember("activitySession"))
      CHECK_COND(measurements.setActivitySession(metaRoot["activitySession"].asCString()));
    if(metaRoot.isMember("timePoint"))
      CHECK_COND(measurements.setTimePoint(metaRoot["timePoint"].asCString()));

    if(measurementGroup.isMember("TrackingUniqueIdentifier")) {
      CHECK_COND(measurements.setTrackingUniqueIdentifier(measurementGroup["TrackingUniqueIdentifier"].asCString()));
    } else {
      char uid[100];
      dcmGenerateUniqueIdentifier(uid, QIICR_INSTANCE_UID_ROOT);
      CHECK_COND(measurements.setTrackingUniqueIdentifier(uid));
    }

    CHECK_COND(measurements.setSourceSeriesForSegmentation(measurementGroup["SourceSeriesForImageSegmentation"].asCString()));

    if(measurementGroup.isMember("rwvmMapUsedForMeasurement")){
      CHECK_COND(measurements.setRealWorldValueMap(DSRCompositeReferenceValue(UID_RealWorldValueMappingStorage, measurementGroup["rwvmMapUsedForMeasurement"].asCString())));
    }

    DSRImageReferenceValue segment(UID_SegmentationStorage, measurementGroup["segmentationSOPInstanceUID"].asCString());
    segment.getSegmentList().addItem(measurementGroup["ReferencedSegment"].asInt());
    CHECK_COND(measurements.setReferencedSegment(segment));

    CHECK_COND(measurements.setFinding(json2bce(measurementGroup["Finding"])));
    CHECK_COND(measurements.setFindingSite(json2bce(measurementGroup["FindingSite"])));

    if(measurementGroup.isMember("MeasurementMethod"))
      CHECK_COND(measurements.setMeasurementMethod(json2cev(measurementGroup["MeasurementMethod"])));

    // TODO - handle conditional items!
    for(int j=0;j<measurementGroup["measurementItems"].size();j++){
      Json::Value measurement = measurementGroup["measurementItems"][j];
      // TODO - add measurement method and derivation!
      const CMR_TID1411_in_TID1500::MeasurementValue numValue(measurement["value"].asCString(),
        json2cev(measurement["units"]));

      if(measurement.isMember("derivationModifier")){
          measurements.addMeasurement(json2cev(measurement["quantity"]), numValue, DSRCodedEntryValue(), json2cev(measurement["derivationModifier"]));
      } else {
        CHECK_COND(measurements.addMeasurement(json2cev(measurement["quantity"]), numValue));
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
  DcmFileFormat ccFileFormat;
  if(metaRoot.isMember("compositeContext")){
    for(int i=0;i<metaRoot["compositeContext"].size();i++){
      addFileToEvidence(doc,compositeContextDataDir,metaRoot["compositeContext"][i].asString());
    }
  }

  if(metaRoot.isMember("imageLibrary")){
    for(int i=0;i<metaRoot["imageLibrary"].size();i++){
      addFileToEvidence(doc,imageLibraryDataDir,metaRoot["imageLibrary"][i].asString());
    }
  }

  OFCHECK_EQUAL(doc.getDocumentType(), DSRTypes::DT_EnhancedSR);

  std::cout << "About to write the document" << std::endl;
  if(!outputFileName.empty()){
    DcmFileFormat ff;
    DcmDataset *dataset = ff.getDataset();
    CHECK_COND(doc.write(*dataset));

    DcmModuleHelpers::copyPatientModule(*ccFileFormat.getDataset(),*dataset);
    DcmModuleHelpers::copyPatientStudyModule(*ccFileFormat.getDataset(),*dataset);
    DcmModuleHelpers::copyGeneralStudyModule(*ccFileFormat.getDataset(),*dataset);

    CHECK_COND(ff.saveFile(outputFileName.c_str(), EXS_LittleEndianExplicit));
    std::cout << "SR saved!" << std::endl;
  }

  return 0;
}
