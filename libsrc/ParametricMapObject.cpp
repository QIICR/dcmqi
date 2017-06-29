//
// Created by Andrey Fedorov on 3/11/17.
//

#include <dcmqi/Helper.h>
#include <itkMinimumMaximumImageCalculator.h>
#include "dcmqi/ParametricMapObject.h"
#include <dcmtk/dcmiod/iodcommn.h>
#include <dcmtk/dcmpmap/dpmparametricmapiod.h>


int ParametricMapObject::initializeFromITK(Float32ITKImageType::Pointer inputImage,
                                           const string &metaDataStr,
                                           std::vector<DcmDataset *> derivationDatasets) {
  sourceRepresentationType = ITK_REPR;

  itkImage = inputImage;

  initializeMetaDataFromString(metaDataStr);

  if(!metaDataIsComplete()){
    updateMetaDataFromDICOM(derivationDatasets);
  }

  initializeVolumeGeometry();

  // NB: the sequence of steps initializing different components of the object parallels that
  //  in the original converter function. It probably makes sense to revisit the sequence
  //  of these steps. It does not necessarily need to happen in this order.
  initializeEquipmentInfo();
  initializeContentIdentification();

  // TODO: consider creating parametric map object after all FGs are initialized instead
  createParametricMap();

  // populate metadata about patient/study, from derivation
  //  datasets or from metadata
  initializeCompositeContext();

  // populate functional groups
  std::vector<std::pair<DcmTag,DcmTag> > dimensionTags;
  dimensionTags.push_back(std::pair<DcmTag,DcmTag>(DCM_ImagePositionPatient, DCM_PlanePositionSequence));
  initializeDimensions(dimensionTags);

  initializePixelMeasuresFG();
  initializePlaneOrientationFG();

  // PM-specific FGs
  initializeFrameAnatomyFG();
  initializeRWVMFG();

  // Mapping from parametric map volume slices to the DICOM frames
  vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> > slice2frame;

  // initialize referenced instances
  // this is done using this utility function from the parent class, since this functionality will
  // be needed both in the PM and SEG objects
  mapVolumeSlicesToDICOMFrames(this->volumeGeometry, derivationDatasets, slice2frame);

  initializeCommonInstanceReferenceModule(this->parametricMap->getCommonInstanceReference(), slice2frame);

  return EXIT_SUCCESS;
}

int ParametricMapObject::initializeVolumeGeometry() {
  if(sourceRepresentationType == ITK_REPR){

    Float32ToDummyCasterType::Pointer caster =
        Float32ToDummyCasterType::New();
    caster->SetInput(itkImage);
    caster->Update();

    MultiframeObject::initializeVolumeGeometryFromITK(caster->GetOutput());
  } else {

  }
  return EXIT_SUCCESS;
}

int ParametricMapObject::updateMetaDataFromDICOM(std::vector<DcmDataset *> dcmList) {
  if(!dcmList.size())
    return EXIT_FAILURE;

  DcmDataset* dcm = dcmList[0];
  metaDataJson["Modality"] =
      std::string(dcmqi::Helper::getTagAsOFString(dcm, DCM_Modality).c_str());

  return EXIT_SUCCESS;
}

int ParametricMapObject::createParametricMap() {

  // create Parametric map object

  OFvariant<OFCondition,DPMParametricMapIOD> obj =
      DPMParametricMapIOD::create<IODFloatingPointImagePixelModule>(metaDataJson["Modality"].asCString(),
                                                                    metaDataJson["SeriesNumber"].asCString(),
                                                                    metaDataJson["InstanceNumber"].asCString(),
                                                                    volumeGeometry.extent[0],
                                                                    volumeGeometry.extent[1],
                                                                    equipmentInfoModule,
                                                                    contentIdentificationMacro,
                                                                    "VOLUME", "QUANTITY",

                                                                    DPMTypes::CQ_RESEARCH);
  // TODO: look into the following, check with @che85 on the purpose of this line!
  if (OFCondition* pCondition = OFget<OFCondition>(&obj)) {
    std::cerr << "Failed to create parametric map object!" << std::endl;
    return EXIT_FAILURE;
  }

  parametricMap = new DPMParametricMapIOD( *OFget<DPMParametricMapIOD>(&obj) );

  // These FG are constant
  FGIdentityPixelValueTransformation idTransFG;
  CHECK_COND(parametricMap->addForAllFrames(idTransFG));

  FGParametricMapFrameType frameTypeFG;
  std::string frameTypeStr = "DERIVED\\PRIMARY\\VOLUME\\QUANTITY";
  frameTypeFG.setFrameType(frameTypeStr.c_str());
  CHECK_COND(parametricMap->addForAllFrames(frameTypeFG));

  return EXIT_SUCCESS;
}

int ParametricMapObject::initializeCompositeContext() {
  // TODO: should this be done in the parent?
  if(derivationDcmDatasets.size()){
    CHECK_COND(parametricMap->import(*derivationDcmDatasets[0], OFTrue, OFTrue, OFFalse, OFTrue));

  } else {
    // TODO: once we support passing of composite context in metadata, propagate it
    //   into parametricMap here
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int ParametricMapObject::initializeFrameAnatomyFG() {
  if(metaDataJson.isMember("FrameLaterality"))
    frameAnatomyFG.setLaterality(FGFrameAnatomy::str2Laterality(metaDataJson["FrameLaterality"].asCString()));
  else
    frameAnatomyFG.setLaterality(FGFrameAnatomy::str2Laterality("U"));

  // TODO: simplify code initialization from metadata
  if(metaDataJson.isMember("AnatomicRegionSequence")){
    frameAnatomyFG.getAnatomy().getAnatomicRegion().set(
        metaDataJson["AnatomicRegionSequence"]["CodeValue"].asCString(),
        metaDataJson["AnatomicRegionSequence"]["CodingSchemeDesignator"].asCString(),
        metaDataJson["AnatomicRegionSequence"]["CodeMeaning"].asCString());
  } else {
    frameAnatomyFG.getAnatomy().getAnatomicRegion().set("T-D0050", "SRT", "Tissue");
  }

  return EXIT_SUCCESS;
}

int ParametricMapObject::initializeRWVMFG() {
  FGRealWorldValueMapping::RWVMItem* realWorldValueMappingItem =
      new FGRealWorldValueMapping::RWVMItem();

  if (!realWorldValueMappingItem )
    return EXIT_FAILURE;

  realWorldValueMappingItem->setRealWorldValueSlope(metaDataJson["RealWorldValueSlope"].asFloat());
  realWorldValueMappingItem->setRealWorldValueIntercept(0);

  // Calculate intensity range - required
  MinMaxCalculatorType::Pointer calculator = MinMaxCalculatorType::New();
  calculator->SetImage(itkImage);
  calculator->Compute();

  realWorldValueMappingItem->setRealWorldValueFirstValueMappedSigned(calculator->GetMinimum());
  realWorldValueMappingItem->setRealWorldValueLastValueMappedSigned(calculator->GetMaximum());

  if(metaDataJson.isMember("MeasurementsUnitsCode")){
    CodeSequenceMacro& unitsCodeDcmtk = realWorldValueMappingItem->getMeasurementUnitsCode();
    unitsCodeDcmtk.set(metaDataJson["MeasurementsUnitsCode"]["CodeValue"].asCString(),
                         metaDataJson["MeasurementsUnitsCode"]["CodingSchemeDesignator"].asCString(),
                         metaDataJson["MeasurementsUnitsCode"]["CodeMeaning"].asCString());
    cout << "Measurements units initialized to " <<
         dcmqi::Helper::codeSequenceMacroToString(unitsCodeDcmtk);

    realWorldValueMappingItem->setLUTExplanation(metaDataJson["MeasurementUnitsCode"]["CodeMeaning"].asCString());
    realWorldValueMappingItem->setLUTLabel(metaDataJson["MeasurementUnitsCode"]["CodeValue"].asCString());
  }

  if(metaDataJson.isMember("QuantityValueCode")){
    ContentItemMacro* item = initializeContentItemMacro(CodeSequenceMacro("G-C1C6", "SRT", "Quantity"),
                                                                dcmqi::Helper::jsonToCodeSequenceMacro(metaDataJson["QuantityValueCode"]));
    realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(item);
  }

  // TODO: factor out defined CodeSequenceMacros into definitions as in dcmsr/include/dcmtk/dcmsr/codes
  if(metaDataJson.isMember("MeasurementMethodCode")){
    ContentItemMacro* item = initializeContentItemMacro(CodeSequenceMacro("G-C306", "SRT", "Measurement Method"),
                                                                dcmqi::Helper::jsonToCodeSequenceMacro(metaDataJson["MeasurementMethodCode"]));
    realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(item);
  }

  if(metaDataJson.isMember("ModelFittingMethodCode")){
    // TODO: update this once CP-1665 is integrated into the standard
    ContentItemMacro* item = initializeContentItemMacro(CodeSequenceMacro("xxxxx2", "99DCMCP1665", "Model Fitting Method"),
                                                        dcmqi::Helper::jsonToCodeSequenceMacro(metaDataJson["ModelFittingMethodCode"]));
    realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(item);
  }

  if(metaDataJson.isMember("SourceImageDiffusionBValues")){
    for(int bvalId=0;bvalId<metaDataJson["SourceImageDiffusionBValues"].size();bvalId++){
      ContentItemMacro* bval = new ContentItemMacro;
      CodeSequenceMacro* bvalUnits = new CodeSequenceMacro("s/mm2", "UCUM", "s/mm2");
      // TODO: update this once CP-1665 is integrated into the standard
      CodeSequenceMacro* qCodeName = new CodeSequenceMacro("xxxxx1", "99DCMCP1665", "Source image diffusion b-value");

      if (!bval || !bvalUnits || !qCodeName)
      {
        return EXIT_FAILURE;
      }

      bval->setValueType(ContentItemMacro::VT_NUMERIC);
      bval->getEntireConceptNameCodeSequence().push_back(qCodeName);
      bval->getEntireMeasurementUnitsCodeSequence().push_back(bvalUnits);
      if(bval->setNumericValue(metaDataJson["SourceImageDiffusionBValues"][bvalId].asCString()).bad())
        cout << "Failed to insert the value!" << endl;;
      realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(bval);
    }
  }

  return EXIT_SUCCESS;
}


int ParametricMapObject::initializeFromDICOM(DcmDataset * sourceDataset) {

  DcmRLEDecoderRegistration::registerCodecs();

  OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");
  dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);

  OFvariant<OFCondition,DPMParametricMapIOD*> result = DPMParametricMapIOD::loadDataset(*sourceDataset);
  if (OFCondition* pCondition = OFget<OFCondition>(&result)) {
    throw -1;
  }

  DPMParametricMapIOD* pMapDoc = *OFget<DPMParametricMapIOD*>(&result);

  initializeVolumeGeometryFromDICOM(pMapDoc, sourceDataset);

  // Initialize the image
  itkImage = volumeGeometry.getITKRepresentation<Float32ITKImageType>();

  itkImage->Allocate();
  itkImage->FillBuffer(0);

  DPMParametricMapIOD::FramesType obj = pMapDoc->getFrames();
  if (OFCondition* pCondition = OFget<OFCondition>(&obj)) {
    throw -1;
  }

  DPMParametricMapIOD::Frames<Float32PixelType> frames = *OFget<DPMParametricMapIOD::Frames<Float32PixelType> >(&obj);

  FGInterface &fgInterface = pMapDoc->getFunctionalGroups();
  for(int frameId=0;frameId<volumeGeometry.extent[2];frameId++){

    Float32PixelType *frame = frames.getFrame(frameId);

    bool isPerFrame;

    FGPlanePosPatient *planposfg =
      OFstatic_cast(FGPlanePosPatient*,fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
    assert(planposfg);

    FGFrameContent *fracon =
      OFstatic_cast(FGFrameContent*,fgInterface.get(frameId, DcmFGTypes::EFG_FRAMECONTENT, isPerFrame));
    assert(fracon);

    // populate meta information needed for Slicer ScalarVolumeNode initialization
    {
    }

    Float32ITKImageType::IndexType index;
    // initialize slice with the frame content
    for(int row=0;row<volumeGeometry.extent[1];row++){
      index[1] = row;
      index[2] = frameId;
      for(int col=0;col<volumeGeometry.extent[0];col++){
        unsigned pixelPosition = row*volumeGeometry.extent[0] + col;
        index[0] = col;
        itkImage->SetPixel(index, frame[pixelPosition]);
      }
    }
  }

  initializeMetaDataFromDICOM(pMapDoc);

  return EXIT_SUCCESS;
}

template <typename T>
void ParametricMapObject::initializeMetaDataFromDICOM(T doc) {
  // TODO: move shared information retrieval to parent class

  OFString temp;
  doc->getSeries().getSeriesDescription(temp);
  metaDataJson["SeriesDescription"] = temp.c_str();

  doc->getSeries().getSeriesNumber(temp);
  metaDataJson["SeriesNumber"] = temp.c_str();

  doc->getIODGeneralImageModule().getInstanceNumber(temp);
  metaDataJson["InstanceNumber"] = temp.c_str();

  using namespace dcmqi;

  doc->getSeries().getBodyPartExamined(temp);
  metaDataJson["BodyPartExamined"] = temp.c_str();

  doc->getDPMParametricMapImageModule().getImageType(temp, 3);
  metaDataJson["DerivedPixelContrast"] = temp.c_str();

  if (doc->getNumberOfFrames() > 0) {
    FGInterface& fg = doc->getFunctionalGroups();
    FGRealWorldValueMapping* rw = OFstatic_cast(FGRealWorldValueMapping*,
                                                fg.get(0, DcmFGTypes::EFG_REALWORLDVALUEMAPPING));
    if (rw->getRealWorldValueMapping().size() > 0) {
      FGRealWorldValueMapping::RWVMItem *item = rw->getRealWorldValueMapping()[0];
      metaDataJson["MeasurementUnitsCode"] = Helper::codeSequence2Json(item->getMeasurementUnitsCode());

      Float64 slope;
      item->getData().findAndGetFloat64(DCM_RealWorldValueSlope, slope);
      metaDataJson["RealWorldValueSlope"] = slope;

      vector<string> diffusionBValues;

      for(int quantIdx=0; quantIdx<item->getEntireQuantityDefinitionSequence().size(); quantIdx++) {
//      TODO: what if there are more than one?
        ContentItemMacro* macro = item->getEntireQuantityDefinitionSequence()[quantIdx];
        CodeSequenceMacro* codeSequence= macro->getConceptNameCodeSequence();
        if (codeSequence != NULL) {
          OFString codeMeaning;
          codeSequence->getCodeMeaning(codeMeaning);
          OFString designator, meaning, value;

          if (codeMeaning == "Quantity") {
            CodeSequenceMacro* quantityValueCode = macro->getConceptCodeSequence();
            if (quantityValueCode != NULL) {
              metaDataJson["QuantityValueCode"] = Helper::codeSequence2Json(*quantityValueCode);
            }
          } else if (codeMeaning == "Measurement Method") {
            CodeSequenceMacro* measurementMethodValueCode = macro->getConceptCodeSequence();
            if (measurementMethodValueCode != NULL) {
              metaDataJson["MeasurementMethodCode"] = Helper::codeSequence2Json(*measurementMethodValueCode);
            }
          } else if (codeMeaning == "Source image diffusion b-value") {
            macro->getNumericValue(value);
            diffusionBValues.push_back(value.c_str());
          }
        }
      }

      if (diffusionBValues.size() > 0) {
        metaDataJson["SourceImageDiffusionBValues"] = Json::Value(Json::arrayValue);
        for (vector<string>::iterator it = diffusionBValues.begin() ; it != diffusionBValues.end(); ++it)
          metaDataJson["SourceImageDiffusionBValues"].append(*it);
      }
    }

    FGDerivationImage* derivationImage = OFstatic_cast(FGDerivationImage*, fg.get(0, DcmFGTypes::EFG_DERIVATIONIMAGE));
    if(derivationImage) {
      OFVector<DerivationImageItem *> &derivationImageItems = derivationImage->getDerivationImageItems();
      if (derivationImageItems.size() > 0) {
        DerivationImageItem *derivationImageItem = derivationImageItems[0];
        CodeSequenceMacro *derivationCode = derivationImageItem->getDerivationCodeItems()[0];
        if (derivationCode != NULL) {
          metaDataJson["DerivationCode"] = Helper::codeSequence2Json(*derivationCode);
        }
      }
    }

    FGFrameAnatomy* fa = OFstatic_cast(FGFrameAnatomy*, fg.get(0, DcmFGTypes::EFG_FRAMEANATOMY));
    metaDataJson["AnatomicRegionSequence"] = Helper::codeSequence2Json(fa->getAnatomy().getAnatomicRegion());

    FGFrameAnatomy::LATERALITY frameLaterality;
    fa->getLaterality(frameLaterality);
    metaDataJson["FrameLaterality"] = fa->laterality2Str(frameLaterality).c_str();
  }
}
