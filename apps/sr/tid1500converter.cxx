#include <iostream>
#include <string>
#include <vector>

// DCMTK includes
#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/dcmsr/dsrdoc.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmdata/dcdeftag.h"
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

#include "json/json.h"

#include "tid1500converterCLP.h"

int main(int argc, char** argv){
  PARSE_ARGS;

  Json::Value root;
  std::fstream test("test.json", std::ifstream::binary);
  test >> root;

  TID1500_MeasurementReport report(CMR_CID7021::ImagingMeasurementReport);
  DSRCodedEntryValue title;

  /* create a new report */
  OFCHECK(report.createNewMeasurementReport(CMR_CID7021::ImagingMeasurementReport).good());

  /* set the language */
  OFCHECK(report.setLanguage(CID5000_Languages::English).good());

  /* set details on the observation context */
  OFCHECK(report.getObservationContext().addPersonObserver("Doe^Jane", "Some Organization").good());

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


  return 0;
}
