
// DCMTK
#include <dcmtk/config/osconfig.h>   // make sure OS specific configuration is included first
#include <dcmtk/ofstd/ofstream.h>
#include <dcmtk/ofstd/oftest.h>
#include <dcmtk/ofstd/ofstd.h>

#include <dcmtk/dcmsr/dsrdoc.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmiod/modhelp.h>

#include <dcmtk/dcmsr/codes/dcm.h>
#include <dcmtk/dcmsr/codes/sct.h>
#include <dcmtk/dcmsr/cmr/tid1500.h>
#include <dcmtk/dcmsr/dsrnumtn.h>
#include <dcmtk/dcmsr/dsrtextn.h>

#include <dcmtk/dcmdata/dcdeftag.h>

#include <dcmtk/dcmsr/codes/dcm.h>

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

using namespace std;

// not used yet
static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

// CLP includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "tid1500writerCLP.h"

#define CHECK_BOOL(condition) \
  do { \
    if (!condition) { \
      std::cerr << "Expected True in " __FILE__ << ":" << __LINE__ << " " << std::cout; \
      throw -1; \
    } \
  } while (0);

#define STATIC_ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

DSRCodedEntryValue json2cev(Json::Value& j){
  return DSRCodedEntryValue(j["CodeValue"].asCString(),
    j["CodingSchemeDesignator"].asCString(),
    j["CodeMeaning"].asCString());
}

DcmFileFormat addFileToEvidence(DSRDocument &doc, string dirStr, string fileStr){
  DcmFileFormat ff;
  OFString fullPath;

  if(dirStr.size())
    OFStandard::combineDirAndFilename(fullPath,dirStr.c_str(),fileStr.c_str());
  else
    fullPath = OFString(fileStr.c_str());
  CHECK_COND(ff.loadFile(fullPath));

  CHECK_COND(doc.getCurrentRequestedProcedureEvidence().addItem(*ff.getDataset()));
  return ff;
}


typedef dcmqi::Helper helper;


int main(int argc, char** argv){

  std::cout << dcmqi_INFO << std::endl;

  PARSE_ARGS;

  if(helper::isUndefinedOrPathDoesNotExist(metaDataFileName, "Input metadata file")){
    return EXIT_FAILURE;
  }

  if(outputFileName.empty()) {
    cerr << "Error: Output DICOM file must be specified!" << endl;
    return EXIT_FAILURE;
  }

  Json::Value metaRoot;

  try {
      ifstream metainfoStream(metaDataFileName.c_str(), ifstream::binary);
      metainfoStream >> metaRoot;
  } catch (exception& e) {
      cout << e.what() << '\n';
      return -1;
  }

  TID1500_MeasurementReport report(CMR_CID7021::ImagingMeasurementReport);

  CHECK_COND(report.setLanguage(DSRCodedEntryValue("eng", "RFC5646", "English")));

  /* set details on the observation context */
  Json::Value observerContext = metaRoot["observerContext"];
  string observerType = observerContext["ObserverType"].asCString();
  if(observerType == "PERSON"){
    CHECK_COND(report.getObservationContext().addPersonObserver(observerContext["PersonObserverName"].asCString(), ""));
  } else if(observerType == "DEVICE"){
    std::string deviceUID;
    if(observerContext.isMember("DeviceObserverUID"))
      deviceUID = observerContext["DeviceObserverUID"].asString();
    else {
      char uid[100];
      dcmGenerateUniqueIdentifier(uid, QIICR_INSTANCE_UID_ROOT);
      deviceUID = std::string(uid);
    }
    CHECK_COND(report.getObservationContext().addDeviceObserver(
      deviceUID.c_str(),
      observerContext.get("DeviceObserverName","").asCString(),
      observerContext.get("DeviceObserverManufacturer","").asCString(),
      observerContext.get("DeviceObserverModelName","").asCString(),
      observerContext.get("DeviceObserverSerialNumber","").asCString()
      ));
  }

  // Image library must be present, even if empty

  CHECK_COND(report.getImageLibrary().createNewImageLibrary());
  CHECK_COND(report.getImageLibrary().addImageGroup());

  if(metaRoot.isMember("imageLibrary")){
    for(Json::ArrayIndex i=0;i<metaRoot["imageLibrary"].size();i++){

      DcmFileFormat ff;
      OFString dicomFilePath;

      if(imageLibraryDataDir.size())
        OFStandard::combineDirAndFilename(dicomFilePath,imageLibraryDataDir.c_str(),metaRoot["imageLibrary"][i].asCString());
      else
        dicomFilePath = metaRoot["imageLibrary"][i].asCString();

      CHECK_COND(ff.loadFile(dicomFilePath));

      CHECK_COND(report.getImageLibrary().addImageEntry(*ff.getDataset(),
        TID1600_ImageLibrary::withAllDescriptors));
    }
  }

  // This call will factor out all of the common entries at the group level
  CHECK_COND(report.getImageLibrary().moveCommonImageDescriptorsToImageGroups());

  // TODO
  //  - this is a very narrow procedure code
  // see duscussion here for improved handling, should be factored out in the
  // future, and handled by the upper-level application layers:
  // https://github.com/QIICR/dcmqi/issues/30
  if(metaRoot.isMember("procedureReported")){
    CHECK_COND(report.addProcedureReported(json2cev(metaRoot["procedureReported"])));
  } else {
    CHECK_COND(report.addProcedureReported(DSRCodedEntryValue("363679005", "SCT", "Imaging procedure")));
  }

  if(!report.isValid()){
    cerr << "Report invalid!" << endl;
    return -1;
  }

  std::cout << "Total measurement groups: " << metaRoot["Measurements"].size() << std::endl;

  // Store measurementNumProperty and measurementPopulationDescription items to be added
  //   to the document in a separate iteration over the individual measurement items.
  // Store empty Json::Value if not applicable
  std::vector<Json::Value> measurementNumProperties, measurementPopulationDescriptions;
  std::vector<Json::Value> measurementGroupAlgorithmIdentification;

  for(Json::ArrayIndex i=0;i<metaRoot["Measurements"].size();i++){
    Json::Value measurementGroup = metaRoot["Measurements"][i];

    CHECK_COND(report.addVolumetricROIMeasurements());
    /* fill volumetric ROI measurements with data */
    TID1500_MeasurementReport::TID1411_Measurements &measurements = report.getVolumetricROIMeasurements();
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

    CHECK_COND(measurements.setFinding(json2cev(measurementGroup["Finding"])));
    if(measurementGroup.isMember("FindingSite")){
      if(measurementGroup.isMember("Laterality")){
        CHECK_COND(measurements.addFindingSite(json2cev(measurementGroup["FindingSite"]),
                                               json2cev(measurementGroup["Laterality"])));
      } else {
        CHECK_COND(measurements.addFindingSite(json2cev(measurementGroup["FindingSite"])));
      }
    }

    if(measurementGroup.isMember("MeasurementMethod"))
      CHECK_COND(measurements.setMeasurementMethod(json2cev(measurementGroup["MeasurementMethod"])));

    if(measurementGroup.isMember("measurementAlgorithmIdentification")){
      measurementGroupAlgorithmIdentification.push_back(measurementGroup["measurementAlgorithmIdentification"]);
    } else {
      measurementGroupAlgorithmIdentification.push_back(Json::Value());
    }

    // TODO - handle conditional items!
    for(Json::ArrayIndex j=0;j<measurementGroup["measurementItems"].size();j++){
      Json::Value measurement = measurementGroup["measurementItems"][j];
      // TODO - add measurement method and derivation!
      const CMR_TID1411_in_TID1500::MeasurementValue numValue(measurement["value"].asCString(), json2cev(measurement["units"]));

      if(!measurements.addMeasurement(json2cev(measurement["quantity"]), numValue).good()){
        std::cerr << "WARNING: Skipping measurement with the value of " << measurement["value"].asCString() << std::endl;
        continue;
      }

      if(measurement.isMember("derivationModifier")){
          CHECK_COND(measurements.getMeasurement().setDerivation(json2cev(measurement["derivationModifier"])));
      }

      if(measurement.isMember("measurementModifiers"))
        for(Json::ArrayIndex k=0;k<measurement["measurementModifiers"].size();k++)
          CHECK_COND(measurements.getMeasurement().addModifier(json2cev(measurement["measurementModifiers"][k]["modifier"]),json2cev(measurement["measurementModifiers"][k]["modifierValue"])));

      if(measurement.isMember("measurementDerivationParameters")){
        for(Json::ArrayIndex k=0;k<measurement["measurementDerivationParameters"].size();k++){
          Json::Value derivationItem = measurement["measurementDerivationParameters"][k];
          DSRCodedEntryValue derivationParameter =
            json2cev(measurement["measurementDerivationParameters"][k]["derivationParameter"]);

          CMR_SRNumericMeasurementValue derivationParameterValue =
            CMR_SRNumericMeasurementValue(derivationItem["derivationParameterValue"].asCString(),
            json2cev(derivationItem["derivationParameterUnits"]));

          CHECK_COND(measurements.getMeasurement().addDerivationParameter(json2cev(derivationItem["derivationParameter"]), derivationParameterValue));
        }
      }

      if(measurement.isMember("measurementNumProperties")){
        measurementNumProperties.push_back(measurement["measurementNumProperties"]);
      } else {
        measurementNumProperties.push_back(Json::Value());
      }

      if(measurement.isMember("measurementPopulationDescription")){
        measurementPopulationDescriptions.push_back(measurement["measurementPopulationDescription"]);
      } else {
        measurementPopulationDescriptions.push_back(Json::Value());
      }


      if(measurement.isMember("measurementAlgorithmIdentification")){
        // TODO: add constraints to the schema - name and version both required if group is present!
        TID4019_AlgorithmIdentification &measurementAlgorithm = measurements.getMeasurement().getAlgorithmIdentification();
        measurementAlgorithm.setIdentification(measurement["measurementAlgorithmIdentification"]["AlgorithmName"].asCString(),
                                               measurement["measurementAlgorithmIdentification"]["AlgorithmVersion"].asCString());
        if(measurement["measurementAlgorithmIdentification"].isMember("AlgorithmParameters")){
          Json::Value parametersJSON = measurement["measurementAlgorithmIdentification"]["AlgorithmParameters"];
          for(Json::ArrayIndex parameterId=0;parameterId<parametersJSON.size();parameterId++)
            CHECK_COND(measurementAlgorithm.addParameter(parametersJSON[parameterId].asCString()));
        }
      }
    }

    if(measurementGroup.isMember("qualitativeEvaluations")){
      for(Json::ArrayIndex k=0;k<measurementGroup["qualitativeEvaluations"].size();k++){
        Json::Value evaluation = measurementGroup["qualitativeEvaluations"][k];
        if(evaluation["conceptValue"].type() == Json::stringValue){
          measurements.addQualitativeEvaluation(json2cev(evaluation["conceptCode"]),
            evaluation["conceptValue"].asString().c_str());
        } else {
          measurements.addQualitativeEvaluation(json2cev(evaluation["conceptCode"]),
            json2cev(evaluation["conceptValue"]));
        }
      }
    }
  }

 if(!report.isValid()){
   cerr << "Report is not valid!" << endl;
   return -1;
 }

  DSRDocument doc;
  OFCondition cond = doc.setTreeFromRootTemplate(report, OFTrue /*expandTree*/);
  if(cond.bad()){
    std::cout << "Failure: " << cond.text() << std::endl;
    return -1;
  }

  // cleanup duplicate modality from image descriptor entry
  //  - if we have any imageLibrary items supplied
  if(metaRoot.isMember("imageLibrary")){
    if(metaRoot["imageLibrary"].size()){
      DSRDocumentTree &st = doc.getTree();
      size_t nnid = st.gotoAnnotatedNode("TID 1601 - Row 1");
      while (nnid) {
        nnid = st.gotoNamedChildNode(CODE_DCM_Modality);
        if (nnid) {
          CHECK_COND(st.removeSubTree());
          nnid = st.gotoNextAnnotatedNode("TID 1601 - Row 1");
        }
      }
    }
  }

  // add Algorithm identification at the group level - note this is not in the standard,
  // CP pending
  {
    DSRDocumentTree &st = doc.getTree();
    size_t nnid   = st.gotoAnnotatedNode("TID 1411 - Row 3");
    unsigned measurementID = 0;
    while(nnid){
      Json::Value thisGroupAlgorithmIdentification = measurementGroupAlgorithmIdentification[measurementID];

      if(!thisGroupAlgorithmIdentification.empty()){
        DSRTextTreeNode* node = new DSRTextTreeNode(DSRTypes::RT_hasConceptMod);
        node->setConceptName(CODE_DCM_AlgorithmName);
        node->setValue(thisGroupAlgorithmIdentification["AlgorithmName"].asCString());
        CHECK_COND(st.addContentItem(node, DSRTypes::AM_afterCurrent, OFTrue));

        node = new DSRTextTreeNode(DSRTypes::RT_hasConceptMod);
        node->setConceptName(CODE_DCM_AlgorithmVersion);
        node->setValue(thisGroupAlgorithmIdentification["AlgorithmVersion"].asCString());
        CHECK_COND(st.addContentItem(node, DSRTypes::AM_afterCurrent, OFTrue));

        if(thisGroupAlgorithmIdentification.isMember("AlgorithmParameters")){
          for(Json::ArrayIndex k=0;k<thisGroupAlgorithmIdentification["AlgorithmParameters"].size();k++){
            node = new DSRTextTreeNode(DSRTypes::RT_hasConceptMod);
            node->setConceptName(CODE_DCM_AlgorithmParameters);
            node->setValue(thisGroupAlgorithmIdentification["AlgorithmParameters"][k].asCString());
            CHECK_COND(st.addContentItem(node, DSRTypes::AM_afterCurrent, OFTrue));
          }
        }

      }
      nnid = st.gotoNextAnnotatedNode("TID 1411 - Row 3");
      measurementID++;
    }
  }

  // add measurement properties manually, since they cannot be added via
  // template-specific API
  {
    DSRDocumentTree &st = doc.getTree();
    size_t nnid   = st.gotoAnnotatedNode("TID 1419 - Row 5");
    unsigned measurementID = 0;
    while(nnid){
      Json::Value thisMeasurementNumProperties = measurementNumProperties[measurementID];
      Json::Value thisMeasurementPopulationDescription = measurementPopulationDescriptions[measurementID];
      //Json::Value thisMeasurementAlgorithmIdentification = measurementAlgorithmIdentifications[measurementID];

      if(!thisMeasurementPopulationDescription.empty()){
        DSRTextTreeNode* node = new DSRTextTreeNode(DSRTypes::RT_hasProperties);
        node->setConceptName(CODE_DCM_PopulationDescription);
        node->setValue(thisMeasurementPopulationDescription.asCString());

        if(st.addContentItem(node, DSRTypes::AM_belowCurrent, OFTrue).good()){
          st.goUp();
        }

        //node->setConceptName(DSRCodedEntryValue("373099004", "SCT", "Mean Value of population"));
        //node->setValue("10", DSRCodedEntryValue("373099004", "SCT", "Mean Value of population")
      }

      if(!thisMeasurementNumProperties.empty()){
        for(int measurementNumPropertyID=0;measurementNumPropertyID<thisMeasurementNumProperties.size();measurementNumPropertyID++){
          Json::Value propertyConcept = thisMeasurementNumProperties[measurementNumPropertyID]["numProperty"];
          Json::Value propertyValue = thisMeasurementNumProperties[measurementNumPropertyID]["numPropertyValue"];
          Json::Value propertyUnits = thisMeasurementNumProperties[measurementNumPropertyID]["numPropertyUnits"];
          //std::cout << "Setting value to " << propertyValue.asCString() << endl;
          DSRNumTreeNode* node = new DSRNumTreeNode(
            DSRTypes::RT_hasProperties);
          node->setValue(propertyValue.asCString(), DSRCodedEntryValue(propertyUnits["CodeValue"].asCString(), propertyUnits["CodingSchemeDesignator"].asCString(), propertyUnits["CodeMeaning"].asCString()));
          node->setConceptName(DSRCodedEntryValue(propertyConcept["CodeValue"].asCString(), propertyConcept["CodingSchemeDesignator"].asCString(), propertyConcept["CodeMeaning"].asCString()));
          node->setMeasurementUnit(DSRCodedEntryValue(propertyUnits["CodeValue"].asCString(), propertyUnits["CodingSchemeDesignator"].asCString(), propertyUnits["CodeMeaning"].asCString()));

          if(st.addContentItem(node, DSRTypes::AM_belowCurrent, OFTrue).good()){
            st.goUp();
          }

        }
      }

      /*
      if(!thisMeasurementAlgorithmIdentification.empty()){
        Json::Value algorithmName = thisMeasurementAlgorithmIdentification["AlgorithmName"];
        Json::Value algorithmVersion = thisMeasurementAlgorithmIdentification["AlgorithmVersion"];

        DSRTextTreeNode* nameNode = new DSRTextTreeNode(DSRTypes::RT_hasConceptMod);

        nameNode->setValue(algorithmName.asCString());
        nameNode->setConceptName(CODE_DCM_AlgorithmName);
        if(!st.addContentItem(nameNode, DSRTypes::AM_belowCurrent, OFTrue).good()){
          std::cerr << "Fatal error: Failed to add algorithm name node" << std::endl;
        }

        DSRTextTreeNode* versionNode = new DSRTextTreeNode(DSRTypes::RT_hasConceptMod);
        versionNode->setValue(algorithmVersion.asCString());
        versionNode->setConceptName(CODE_DCM_AlgorithmVersion);
        if(!st.addContentItem(versionNode, DSRTypes::AM_afterCurrent, OFTrue).good()){
          std::cerr << "Fatal error: Failed to add algorithm version node" << std::endl;
        }

        st.goUp();
      }*/

      //node->setConceptName(DSRCodedEntryValue("373099004", "SCT", "Mean Value of population"));
      //node->setValue("10", DSRCodedEntryValue("373099004", "SCT", "Mean Value of population"));
      nnid = st.gotoNextAnnotatedNode("TID 1419 - Row 5");
      measurementID++;
    }
  }

  if(metaRoot.isMember("SeriesDescription")) {
    CHECK_COND(doc.setSeriesDescription(metaRoot["SeriesDescription"].asCString()));
  }

  if(metaRoot.isMember("CompletionFlag")) {
    if (DSRTypes::enumeratedValueToCompletionFlag(metaRoot["CompletionFlag"].asCString())
        == DSRTypes::CF_Complete) {
      doc.completeDocument();
    }
  }

  // TODO: we should think about storing those information in json as well
  if(metaRoot.isMember("VerificationFlag") && observerType=="PERSON" && doc.getCompletionFlag() == DSRTypes::CF_Complete) {
    if (DSRTypes::enumeratedValueToVerificationFlag(metaRoot["VerificationFlag"].asCString()) ==
        DSRTypes::VF_Verified) {
      // TODO: get organization from meta information?
      CHECK_COND(doc.verifyDocument(metaRoot["observerContext"]["PersonObserverName"].asCString(), "QIICR"));
    }
  }

  if(metaRoot.isMember("InstanceNumber")) {
    CHECK_COND(doc.setInstanceNumber(metaRoot["InstanceNumber"].asCString()))
  }

  if(metaRoot.isMember("SeriesNumber")) {
    CHECK_COND(doc.setSeriesNumber(metaRoot["SeriesNumber"].asCString()))
  }

  // WARNING: no consistency checks between the referenced UIDs and the
  //  referencedDICOMFileNames ...
  DcmFileFormat ccFileFormat;
  bool compositeContextInitialized = false;
  if(metaRoot.isMember("compositeContext")){
    for(Json::ArrayIndex i=0;i<metaRoot["compositeContext"].size();i++){
      cout << "Adding to compositeContext: " << metaRoot["compositeContext"][i].asString() << endl;
      ccFileFormat = addFileToEvidence(doc,compositeContextDataDir,metaRoot["compositeContext"][i].asString());
      compositeContextInitialized = true;
    }
  }

  if(metaRoot.isMember("imageLibrary")){
    for(Json::ArrayIndex i=0;i<metaRoot["imageLibrary"].size();i++){
      addFileToEvidence(doc,imageLibraryDataDir,metaRoot["imageLibrary"][i].asString());
    }
  }

  OFCHECK_EQUAL(doc.getDocumentType(), DSRTypes::DT_EnhancedSR);

  DcmFileFormat ff;
  DcmDataset *dataset = ff.getDataset();

  OFString contentDate, contentTime;
  DcmDate::getCurrentDate(contentDate);
  DcmTime::getCurrentTime(contentTime);

  CHECK_COND(doc.setManufacturer(QIICR_MANUFACTURER));
  CHECK_COND(doc.setDeviceSerialNumber(QIICR_DEVICE_SERIAL_NUMBER));
  CHECK_COND(doc.setManufacturerModelName(QIICR_MANUFACTURER_MODEL_NAME));
  CHECK_COND(doc.setSoftwareVersions(QIICR_SOFTWARE_VERSIONS));

  CHECK_COND(doc.setSeriesDate(contentDate.c_str()));
  CHECK_COND(doc.setSeriesTime(contentTime.c_str()));

  CHECK_COND(doc.write(*dataset));

  if(compositeContextInitialized){
    DcmModuleHelpers::copyPatientModule(*ccFileFormat.getDataset(),*dataset);
    DcmModuleHelpers::copyPatientStudyModule(*ccFileFormat.getDataset(),*dataset);
    DcmModuleHelpers::copyGeneralStudyModule(*ccFileFormat.getDataset(),*dataset);
    cout << "Composite Context has been initialized" << endl;
  } else {
    cerr << "WARNING: Composite context not initialized! Patient, Study and General Study modules were NOT propagated!" << endl;
  }

  CHECK_COND(ff.saveFile(outputFileName.c_str(), EXS_LittleEndianExplicit));
  std::cout << "SR saved!" << std::endl;

  return 0;
}
