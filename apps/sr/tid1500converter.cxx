#include <iostream>
#include <string>
#include <vector>

// DCMTK includes
#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

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

#include "dcmtk/dcmsr/cmr/tid1500.h"

#include "tid1500converterCLP.h"

int main(int argc, char** argv){
  //PARSE_ARGS;

  TID1500_MeasurementReport report(CMR_CID7021::ImagingMeasurementReport);
  DSRCodedEntryValue title;
  /* check initial settings */
  OFCHECK(!report.isValid());
  OFCHECK(report.getDocumentTitle(title).good());
  //OFCHECK(title == CODE_DCM_ImagingMeasurementReport);
  OFCHECK(report.compareTemplateIdentication("1500", "DCMR"));
  /* create a new report */
  OFCHECK(report.createNewMeasurementReport(CMR_CID7021::PETMeasurementReport).good());
  OFCHECK(report.getDocumentTitle(title).good());
  //OFCHECK(title == CODE_DCM_PETMeasurementReport);
  /* set the language */
  OFCHECK(report.setLanguage(CID5000_Languages::English).good());
  /* set details on the observation context */
  OFCHECK(report.getObservationContext().addPersonObserver("Doe^Jane", "Some Organization").good());
  /* create new image library (needed after clear) */
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

  DcmFileFormat *ff = new DcmFileFormat();
  DcmDataset *ds = ff->getDataset();
  report.write(*ds);
  ff->saveFile("temp.dcm", EXS_LittleEndianExplicit);

  return 0;

  OFCHECK(report.hasQualitativeEvaluations());
  OFCHECK(!report.hasImagingMeasurements(OFTrue /*checkChildren*/));
  OFCHECK(!report.hasQualitativeEvaluations(OFTrue /*checkChildren*/));
  /* now, add some qualitative evaluations */
  const DSRCodedEntryValue code("1234", "99TEST", "not bad");
  OFCHECK(report.addQualitativeEvaluation(DSRBasicCodedEntry("0815", "99TEST", "Some test code"), code).good());
  OFCHECK(report.addQualitativeEvaluation(DSRBasicCodedEntry("4711", "99TEST", "Some other test code"), "very good").good());
  OFCHECK(report.hasQualitativeEvaluations(OFTrue /*checkChildren*/));

  // to be continued ...

  /* check number of content items (expected) */
  OFCHECK_EQUAL(report.getTree().countNodes(), 10);
  /* create an expanded version of the tree */
  DSRDocumentSubTree *tree = NULL;
  OFCHECK(report.getTree().createExpandedSubTree(tree).good());
  /* and check whether all content items are there */
  if (tree != NULL)
  {
      OFCHECK_EQUAL(tree->countNodes(), 12);
      delete tree;
  } else
      OFCHECK_FAIL("could create expanded tree");

  /*
  DcmFileFormat *ff = new DcmFileFormat();
  DcmDataset *ds = ff->getDataset();
  report.write(*ds);
  ff->saveFile(outputSRFileName.c_str(), EXS_LittleEndianExplicit);
  */

  return 0;
}
