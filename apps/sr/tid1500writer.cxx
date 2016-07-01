#include "TID1500Converter.h"

#include <Exceptions.h>

#include "tid1500writerCLP.h"

#define CHECK_COND(condition) \
  do { \
    if (condition.bad()) { \
      std::cerr << "Condition failed: " << condition.text() << " in " __FILE__ << ":" << __LINE__ << std::endl; \
      throw -1; \
    } \
  } while (0);

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
  OFCHECK(report.setLanguage(CID5000_Languages::English).good());

  /* set details on the observation context */
  // TODO - parameterize
  OFCHECK(report.getObservationContext().addPersonObserver("Doe^Jane", "Some Organization").good());

  // TODO - image library (note: invalid document if no Image Library present)
  OFCHECK(report.getImageLibrary().createNewImageLibrary().good());

  // TODO
  //  - this is a very narrow procedure code
  //  - discuss with Jörg - need to be able to use private code
  //  - discuss with David - generic parameter
  OFCHECK(report.addProcedureReported(CMR_CID100::PETWholeBody).good());

  CHECK_BOOL(report.isValid());

  std::cout << "Total measurement groups: " << metaRoot["Measurements"].size() << std::endl;

  for(int i=0;i<metaRoot["Measurements"].size();i++){
    Json::Value measurementGroup = metaRoot["Measurements"][i]["MeasurementGroup"];

    OFCHECK(report.addVolumetricROIMeasurements().good());
    /* fill volumetric ROI measurements with data */
    TID1500_MeasurementReport::TID1411_Measurements &measurements =   report.getVolumetricROIMeasurements();
    //std::cout << measurementGroup["TrackingIdentifier"] << std::endl;
    OFCHECK(measurements.setTrackingIdentifier(measurementGroup["TrackingIdentifier"].asString().c_str()).good());

    // TODO - init tracking UID if not provided by the user, or populate

    OFCHECK(measurements.setSourceSeriesForSegmentation(measurementGroup["SourceSeriesForImageSegmentation"].asString().c_str()).good());

    OFCHECK(measurements.setRealWorldValueMap(DSRCompositeReferenceValue(UID_RealWorldValueMappingStorage, measurementGroup["rwvmMapUsedForMeasurement"].asCString())).good());

    DSRImageReferenceValue segment(UID_SegmentationStorage, measurementGroup["SourceSeriesForImageSegmentation"].asString().c_str());
    segment.getSegmentList().addItem(measurementGroup["ReferencedSegment"].asInt());
    OFCHECK(measurements.setReferencedSegment(segment).good());

    // TODO - a good cndidate to factor out
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
      //CMR_TID1411_in_TID1500::MeasurementValue numVal(measurement["value"],
      //  DSRCodedEntryValue());
      // TODO - add measurement method and derivation!
      const CMR_TID1411_in_TID1500::MeasurementValue numValue(measurement["value"].asCString(), unitsCode);
      measurements.addMeasurement(quantityCode, numValue);
    }
  }

  CHECK_BOOL(report.isValid());

  DSRDocument doc;
  std::cout << "Setting tree from the report" << std::endl;
  OFCondition cond = doc.setTreeFromRootTemplate(report, OFTrue /*expandTree*/);
  if(cond.bad()){
    std::cout << "Failure: " << cond.text() << std::endl;
    return -1;
  }

  OFCHECK_EQUAL(doc.getDocumentType(), DSRTypes::DT_EnhancedSR);

  std::cout << "About to write the document" << std::endl;
  if(outputFileName.size()){
    DcmFileFormat ff;
    CHECK_COND(doc.write(*ff.getDataset()));
    OFCHECK(ff.saveFile(outputFileName.c_str(), EXS_LittleEndianExplicit).good());
    std::cout << "SR saved!" << std::endl;
  }

#if 0
  /* create new image library (only needed after clear) */
  OFCHECK(report.getImageLibrary().createNewImageLibrary().good());
  /* set two values for "procedure reported" */
  OFCHECK(!report.isValid());
  OFCHECK(!report.hasProcedureReported());
  OFCHECK(report.addProcedureReported(CMR_CID100::PETWholeBody).good());
  OFCHECK(report.addProcedureReported(DSRCodedEntryValue("4711", "99TEST", "Some other test code")).good());
  OFCHECK(report.hasProcedureReported());
  OFCHECK(report.isValid());
  /* some further checks */
  OFCHECK(report.hasImagingMeasurements());
  OFCHECK(report.hasVolumetricROIMeasurements());
  OFCHECK(!report.hasQualitativeEvaluations());
  OFCHECK(!report.hasImagingMeasurements(OFTrue /*checkChildren*/));
  OFCHECK(!report.hasVolumetricROIMeasurements(OFTrue /*checkChildren*/));
  OFCHECK(!report.hasQualitativeEvaluations(OFTrue /*checkChildren*/));
  /* add two further volumetric ROI measurements */
  OFCHECK(report.addVolumetricROIMeasurements().good());
  OFCHECK(report.addVolumetricROIMeasurements().good());
  OFCHECK(!report.hasVolumetricROIMeasurements(OFTrue /*checkChildren*/));
  /* fill volumetric ROI measurements with data */
  TID1500_MeasurementReport::TID1411_Measurements &measurements = report.getVolumetricROIMeasurements();
  OFCHECK(!measurements.isValid());
  OFCHECK(measurements.compareTemplateIdentication("1411", "DCMR"));
  OFCHECK(measurements.setTrackingIdentifier("aorta reference region").good());
  OFCHECK(measurements.setTrackingUniqueIdentifier("1.2.3.4.5").good());
  OFCHECK(measurements.setTrackingIdentifier("some reference region").good());
  OFCHECK(measurements.setActivitySession("1").good());
  OFCHECK(measurements.setTimePoint("1.1").good());
  OFCHECK(measurements.setSourceSeriesForSegmentation("6.7.8.9.0").good());
  OFCHECK(measurements.setFinding(DSRBasicCodedEntry("0815", "99TEST", "Some test code")).good());
  OFCHECK(!measurements.isValid());
  /* test two ways of adding a referenced segment */
  DSRImageReferenceValue segment(UID_SegmentationStorage, "1.0.2.0.3.0");
  segment.getSegmentList().addItem(1);
  DcmDataset dataset;
  DcmItem *ditem = NULL;
  OFCHECK(dataset.putAndInsertString(DCM_SOPClassUID, UID_SurfaceSegmentationStorage).good());
  OFCHECK(dataset.putAndInsertString(DCM_SOPInstanceUID, "99.0").good());
  OFCHECK(dataset.findOrCreateSequenceItem(DCM_SegmentSequence, ditem).good());
  if (ditem != NULL)
  {
      OFCHECK(ditem->putAndInsertUint16(DCM_SegmentNumber, 1).good());
      OFCHECK(ditem->putAndInsertString(DCM_TrackingID, "blabla").good());
      OFCHECK(ditem->putAndInsertString(DCM_TrackingUID, "1.2.3").good());
  }
  OFCHECK(measurements.setReferencedSegment(segment).good());
  OFCHECK(measurements.setReferencedSegment(DSRImageReferenceValue(UID_SegmentationStorage, "1.0")).bad());
  OFCHECK(measurements.setReferencedSegment(dataset, 1).good());
  dataset.clear();
  OFCHECK(dataset.putAndInsertString(DCM_SOPClassUID, UID_RealWorldValueMappingStorage).good());
  OFCHECK(dataset.putAndInsertString(DCM_SOPInstanceUID, "99.9").good());
  OFCHECK(measurements.setRealWorldValueMap(DSRCompositeReferenceValue(UID_RealWorldValueMappingStorage, "2.0.3.0.4.0")).good());
  OFCHECK(measurements.setRealWorldValueMap(DSRCompositeReferenceValue(UID_CTImageStorage, "2.0")).bad());
  OFCHECK(measurements.setRealWorldValueMap(dataset).good());
  OFCHECK(measurements.setFindingSite(CODE_SRT_AorticArch).good());
  OFCHECK(measurements.setMeasurementMethod(DSRCodedEntryValue(CODE_DCM_SUVBodyWeightCalculationMethod)).good());
  OFCHECK(!measurements.isValid());
  /* add two measurement values */
  const CMR_TID1411_in_TID1500::MeasurementValue numVal1("99", CMR_CID7181::StandardizedUptakeValueBodyWeight);
  const CMR_TID1411_in_TID1500::MeasurementValue numVal2(CMR_CID42::MeasurementFailure);
  OFCHECK(measurements.addMeasurement(CMR_CID7469::SUVbw, numVal1, CMR_CID6147(), CMR_CID7464::Mean).good());
  OFCHECK(measurements.addMeasurement(CMR_CID7469::SUVbw, numVal2, DSRCodedEntryValue("0815", "99TEST", "Some test code"), CMR_CID7464::Mode).good());
  OFCHECK(measurements.isValid());
  /* now, add some qualitative evaluations */
  const DSRCodedEntryValue code("1234", "99TEST", "not bad");
  OFCHECK(report.addQualitativeEvaluation(DSRBasicCodedEntry("0815", "99TEST", "Some test code"), code).good());
  OFCHECK(report.addQualitativeEvaluation(DSRBasicCodedEntry("4711", "99TEST", "Some other test code"), "very good").good());
  /* some final checks */
  OFCHECK(report.isValid());
  OFCHECK(report.hasImagingMeasurements(OFTrue /*checkChildren*/));
  OFCHECK(report.hasVolumetricROIMeasurements(OFTrue /*checkChildren*/));
  OFCHECK(report.hasQualitativeEvaluations(OFTrue /*checkChildren*/));

  /* check number of content items (expected) */
  OFCHECK_EQUAL(report.getTree().countNodes(), 13);
  OFCHECK_EQUAL(report.getTree().countNodes(OFTrue /*searchIntoSubTemplates*/), 34);
  OFCHECK_EQUAL(report.getTree().countNodes(OFTrue /*searchIntoSubTemplates*/, OFFalse /*countIncludedTemplateNodes*/), 28);
  /* create an expanded version of the tree */
  DSRDocumentSubTree *tree = NULL;
  OFCHECK(report.getTree().createExpandedSubTree(tree).good());
  /* and check whether all content items are there */
  if (tree != NULL)
  {
      OFCHECK_EQUAL(tree->countNodes(), 28);
      OFCHECK_EQUAL(tree->countNodes(OFTrue /*searchIntoSubTemplates*/), 28);
      OFCHECK_EQUAL(tree->countNodes(OFTrue /*searchIntoSubTemplates*/, OFFalse /*countIncludedTemplateNodes*/), 28);
      delete tree;
  } else
      OFCHECK_FAIL("could create expanded tree");

  /* try to insert the root template into a document */
  DSRDocument doc;
  OFCHECK(!doc.isValid());
  OFCHECK_EQUAL(doc.getDocumentType(), DSRTypes::DT_BasicTextSR);
  OFCHECK(doc.setTreeFromRootTemplate(report, OFFalse /*expandTree*/).good());
  OFCHECK(doc.isValid());
  OFCHECK_EQUAL(doc.getDocumentType(), DSRTypes::DT_EnhancedSR);
  /* now, do the same with an expanded document tree */
  OFCHECK(doc.setTreeFromRootTemplate(report, OFTrue  /*expandTree*/).good());
  OFCHECK(doc.isValid());
  OFCHECK_EQUAL(doc.getDocumentType(), DSRTypes::DT_EnhancedSR);

  DcmFileFormat *ff = new DcmFileFormat();
  DcmDataset *ds = ff->getDataset();
  doc.write(*ds);
  ff->saveFile(outputSRFileName.c_str(), EXS_LittleEndianExplicit);
#endif

  return 0;
}
