//
// Created by Andrey Fedorov on 3/11/17.
//

#include <dcmqi/Helper.h>
#include "dcmqi/QIICRConstants.h"
#include "dcmqi/ParametricMapObject.h"


int ParametricMapObject::initializeFromITK(Float32ITKImageType::Pointer inputImage,
                                           const string &metaDataStr,
                                           std::vector<DcmDataset *> derivationDatasets) {

  setDerivationDatasets(derivationDatasets);

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
  createDICOMParametricMap();

  // populate metadata about patient/study, from derivation
  //  datasets or from metadata
  initializeCompositeContext();
  initializeSeriesSpecificAttributes(parametricMap->getIODGeneralSeriesModule(),
                                     parametricMap->getIODGeneralImageModule());

  // populate functional groups
  std::vector<std::pair<DcmTag,DcmTag> > dimensionTags;
  dimensionTags.push_back(std::pair<DcmTag,DcmTag>(DCM_ImagePositionPatient, DCM_PlanePositionSequence));
  initializeDimensions(dimensionTags);

  initializePixelMeasuresFG();
  CHECK_COND(parametricMap->addForAllFrames(pixelMeasuresFG));

  initializePlaneOrientationFG();
  CHECK_COND(parametricMap->addForAllFrames(planeOrientationPatientFG));

  // PM-specific FGs
  initializeFrameAnatomyFG();
  initializeRWVMFG();

  // Mapping from parametric map volume slices to the DICOM frames
  vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> > slice2frame;

  // initialize referenced instances
  // this is done using this utility function from the parent class, since this functionality will
  // be needed both in the PM and SEG objects
  mapVolumeSlicesToDICOMFrames(derivationDatasets, slice2frame);

  initializeCommonInstanceReferenceModule(this->parametricMap->getCommonInstanceReference(), slice2frame);

  initializeFrames(slice2frame);

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
    MultiframeObject::initializeVolumeGeometryFromDICOM(parametricMap->getFunctionalGroups());
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

int ParametricMapObject::initializeEquipmentInfo() {
  if(sourceRepresentationType == ITK_REPR){
    enhancedEquipmentInfoModule = IODEnhGeneralEquipmentModule::EquipmentInfo(QIICR_MANUFACTURER,
                                                                              QIICR_DEVICE_SERIAL_NUMBER,
                                                                              QIICR_MANUFACTURER_MODEL_NAME,
                                                                              QIICR_SOFTWARE_VERSIONS);
    /*
    enhancedEquipmentInfoModule.m_Manufacturer = QIICR_MANUFACTURER;
    enhancedEquipmentInfoModule.m_DeviceSerialNumber = QIICR_DEVICE_SERIAL_NUMBER;
    enhancedEquipmentInfoModule.m_ManufacturerModelName = QIICR_MANUFACTURER_MODEL_NAME;
    enhancedEquipmentInfoModule.m_SoftwareVersions = QIICR_SOFTWARE_VERSIONS;
    */

  } else { // DICOM_REPR
  }
  return EXIT_SUCCESS;
}

int ParametricMapObject::createDICOMParametricMap() {

  // create Parametric map object

  // TODO: revisit intialization of the modality - if source images are available, modality should match
  //  that in the source images!
  OFvariant<OFCondition,DPMParametricMapIOD> obj =
      DPMParametricMapIOD::create<IODFloatingPointImagePixelModule>(metaDataJson["Modality"].asCString(),
                                                                    metaDataJson["SeriesNumber"].asCString(),
                                                                    metaDataJson["InstanceNumber"].asCString(),
                                                                    volumeGeometry.extent[1],
                                                                    volumeGeometry.extent[0],
                                                                    enhancedEquipmentInfoModule,
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

  /* Initialize dimension module */
  IODMultiframeDimensionModule &mfdim = parametricMap->getIODMultiframeDimensionModule();
  OFCondition result = mfdim.addDimensionIndex(DCM_ImagePositionPatient, dcmqi::Helper::generateUID(),
                                               DCM_PlanePositionSequence, "ImagePositionPatient");

  return EXIT_SUCCESS;
}

int ParametricMapObject::createITKParametricMap() {

  initializeVolumeGeometry();

  // Initialize the image
  itkImage = volumeGeometry.getITKRepresentation<Float32ITKImageType>();

  DPMParametricMapIOD::FramesType obj = parametricMap->getFrames();
  if (OFCondition* pCondition = OFget<OFCondition>(&obj)) {
    throw -1;
  }

  FGInterface &fgInterface = parametricMap->getFunctionalGroups();
  DPMParametricMapIOD::Frames<Float32PixelType> frames = *OFget<DPMParametricMapIOD::Frames<Float32PixelType> >(&obj);

  createITKImageFromFrames(fgInterface, frames);
  return EXIT_SUCCESS;
}

int ParametricMapObject::createITKImageFromFrames(FGInterface &fgInterface,
                                                  DPMParametricMapIOD::Frames<Float32PixelType> frames) {
  for(int frameId=0;frameId<volumeGeometry.extent[2];frameId++){
    bool isPerFrame;

    FGPlanePosPatient *planposfg =
      OFstatic_cast(FGPlanePosPatient*,fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
    assert(planposfg);

    FGFrameContent *fracon =
      OFstatic_cast(FGFrameContent*,fgInterface.get(frameId, DcmFGTypes::EFG_FRAMECONTENT, isPerFrame));
    assert(fracon);

    Float32PixelType *frame = frames.getFrame(frameId);
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
  return EXIT_SUCCESS;
}

int ParametricMapObject::initializeCompositeContext() {
  // TODO: should this be done in the parent?
  if(derivationDcmDatasets.size()){
    /* Import GeneralSeriesModule - content for the reference
     from include/dcmtk/dcmiod/modgeneralseries.h:
     * Modality: (CS, 1, 1)
     *  Series Instance Number: (UI, 1, 1)
     *  Series Number: (IS, 1, 2)
     *  Laterality: (CS, 1, 2C)
     *  Series Date: (DA, 1, 3)
     *  Series Time: (TM, 1, 3)
     *  Performing Physician's Name: (PN, 1, 3)
     *  Protocol Name: (LO, 1, 3)
     *  Series Description: (LO, 1, 3)
     *  Operators' Name: (PN, 1-n, 3)
     *  Body Part Examined: (CS, 1, 3)
     *  Patient Position: (CS, 1, 2C)
     */
    CHECK_COND(parametricMap->import(*derivationDcmDatasets[0],
                                     OFTrue, // Patient
                                     OFTrue, // Study
                                     OFTrue, // Frame of reference
                                     OFFalse)); // Series

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

  CHECK_COND(parametricMap->addForAllFrames(frameAnatomyFG));

  return EXIT_SUCCESS;
}

int ParametricMapObject::initializeRWVMFG() {
  FGRealWorldValueMapping::RWVMItem* realWorldValueMappingItem =
      new FGRealWorldValueMapping::RWVMItem();

  realWorldValueMappingItem->setRealWorldValueSlope(metaDataJson["RealWorldValueSlope"].asFloat());
  realWorldValueMappingItem->setRealWorldValueIntercept(0);

  // Calculate intensity range - required
  MinMaxCalculatorType::Pointer calculator = MinMaxCalculatorType::New();
  calculator->SetImage(itkImage);
  calculator->Compute();

  realWorldValueMappingItem->setRealWorldValueFirstValueMappedSigned(calculator->GetMinimum());
  realWorldValueMappingItem->setRealWorldValueLastValueMappedSigned(calculator->GetMaximum());

  if(metaDataJson.isMember("MeasurementUnitsCode")){
    CodeSequenceMacro& unitsCodeDcmtk = realWorldValueMappingItem->getMeasurementUnitsCode();
    unitsCodeDcmtk.set(metaDataJson["MeasurementUnitsCode"]["CodeValue"].asCString(),
                       metaDataJson["MeasurementUnitsCode"]["CodingSchemeDesignator"].asCString(),
                       metaDataJson["MeasurementUnitsCode"]["CodeMeaning"].asCString());
    cout << "Measurements units initialized to " <<
         dcmqi::Helper::codeSequenceMacroToString(unitsCodeDcmtk);

    realWorldValueMappingItem->setLUTExplanation(metaDataJson["MeasurementUnitsCode"]["CodeMeaning"].asCString());
    realWorldValueMappingItem->setLUTLabel(metaDataJson["MeasurementUnitsCode"]["CodeValue"].asCString());
  }

  /*
  if(metaDataJson.isMember("QuantityValueCode")){
    ContentItemMacro* item = initializeContentItemMacro(CodeSequenceMacro("G-C1C6", "SRT", "Quantity"),
      dcmqi::Helper::jsonToCodeSequenceMacro(metaDataJson["QuantityValueCode"]));
    realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(item);
  }*/

  ContentItemMacro* quantity = new ContentItemMacro;
  CodeSequenceMacro* qCodeName = new CodeSequenceMacro("G-C1C6", "SRT", "Quantity");
  CodeSequenceMacro* qSpec = createCodeSequenceFromMetadata("QuantityValueCode");

  if (!quantity || !qSpec || !qCodeName)
  {
    return EXIT_FAILURE;
  }

  quantity->getEntireConceptNameCodeSequence().push_back(qCodeName);
  quantity->getEntireConceptCodeSequence().push_back(qSpec);
  realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(quantity);
  quantity->setValueType(ContentItemMacro::VT_CODE);

  /*
  // TODO: factor out defined CodeSequenceMacros into definitions as in dcmsr/include/dcmtk/dcmsr/codes
  if(metaDataJson.isMember("MeasurementMethodCode")){
    ContentItemMacro* item = initializeContentItemMacro(CodeSequenceMacro("G-C306", "SRT", "Measurement Method"),
                                                                dcmqi::Helper::jsonToCodeSequenceMacro(metaDataJson["MeasurementMethodCode"]));
    realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(item);
  } */

  if(metaDataJson.isMember("MeasurementMethodCode")){
    ContentItemMacro* measureMethod = new ContentItemMacro;
    CodeSequenceMacro* qCodeName = new CodeSequenceMacro("G-C306", "SRT", "Measurement Method");
    CodeSequenceMacro* qSpec = createCodeSequenceFromMetadata("MeasurementMethodCode");

    if (!measureMethod || !qSpec || !qCodeName)
    {
      return EXIT_FAILURE;
    }

    measureMethod->getEntireConceptNameCodeSequence().push_back(qCodeName);
    measureMethod->getEntireConceptCodeSequence().push_back(qSpec);
    realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(measureMethod);
    measureMethod->setValueType(ContentItemMacro::VT_CODE);
  }

  /*
  if(metaDataJson.isMember("ModelFittingMethodCode")){
    // TODO: update this once CP-1665 is integrated into the standard
    ContentItemMacro* item = initializeContentItemMacro(CodeSequenceMacro("113241", "DCM", "Model Fitting Method"),
                                                        dcmqi::Helper::jsonToCodeSequenceMacro(metaDataJson["ModelFittingMethodCode"]));
    realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(item);
  } */

  if(metaDataJson.isMember("ModelFittingMethodCode")){
    ContentItemMacro* fittingMethod = new ContentItemMacro;
    CodeSequenceMacro* qCodeName = new CodeSequenceMacro("113241", "DCM", "Model fitting method");
    CodeSequenceMacro* qSpec = createCodeSequenceFromMetadata("ModelFittingMethodCode");

    if (!fittingMethod || !qSpec || !qCodeName)
    {
      return EXIT_FAILURE;
    }

    fittingMethod->getEntireConceptNameCodeSequence().push_back(qCodeName);
    fittingMethod->getEntireConceptCodeSequence().push_back(qSpec);
    realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(fittingMethod);
    fittingMethod->setValueType(ContentItemMacro::VT_CODE);
  }


  if(metaDataJson.isMember("SourceImageDiffusionBValues")) {
    for (int bvalId = 0; bvalId < metaDataJson["SourceImageDiffusionBValues"].size(); bvalId++) {
      ContentItemMacro *bval = new ContentItemMacro;
      CodeSequenceMacro *bvalUnits = new CodeSequenceMacro("s/mm2", "UCUM", "s/mm2");
      // TODO: update this once CP-1665 is integrated into the standard
      CodeSequenceMacro *qCodeName = new CodeSequenceMacro("113240", "DCM", "Source image diffusion b-value");

      if (!bval || !bvalUnits || !qCodeName) {
        return EXIT_FAILURE;
      }

      bval->setValueType(ContentItemMacro::VT_NUMERIC);
      bval->getEntireConceptNameCodeSequence().push_back(qCodeName);
      bval->getEntireMeasurementUnitsCodeSequence().push_back(bvalUnits);
      if (bval->setNumericValue(metaDataJson["SourceImageDiffusionBValues"][bvalId].asCString()).bad())
        cout << "Failed to insert the value!" << endl;;
      realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(bval);
    }
  }

  rwvmFG.getRealWorldValueMapping().push_back(realWorldValueMappingItem);
  parametricMap->addForAllFrames(rwvmFG);

  return EXIT_SUCCESS;
}

CodeSequenceMacro *ParametricMapObject::createCodeSequenceFromMetadata(const string &codeName) const {
  return new CodeSequenceMacro(
        metaDataJson[codeName]["CodeValue"].asCString(),
        metaDataJson[codeName]["CodingSchemeDesignator"].asCString(),
        metaDataJson[codeName]["CodeMeaning"].asCString());
}


int ParametricMapObject::initializeFromDICOM(DcmDataset * sourceDataset) {

  sourceRepresentationType = DICOM_REPR;
  dcmRepresentation = sourceDataset;

  DcmRLEDecoderRegistration::registerCodecs();

  OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");
  dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);

  OFvariant<OFCondition,DPMParametricMapIOD*> result = DPMParametricMapIOD::loadDataset(*sourceDataset);
  if (OFCondition* pCondition = OFget<OFCondition>(&result)) {
    throw -1;
  }

  parametricMap = *OFget<DPMParametricMapIOD*>(&result);

  initializeMetaDataFromDICOM();
  createITKParametricMap();

  return EXIT_SUCCESS;
}

void ParametricMapObject::initializeMetaDataFromDICOM() {

  OFString temp;
  parametricMap->getSeries().getSeriesDescription(temp);
  metaDataJson["SeriesDescription"] = temp.c_str();

  parametricMap->getSeries().getSeriesNumber(temp);
  metaDataJson["SeriesNumber"] = temp.c_str();

  parametricMap->getIODGeneralImageModule().getInstanceNumber(temp);
  metaDataJson["InstanceNumber"] = temp.c_str();

  using namespace dcmqi;

  parametricMap->getSeries().getBodyPartExamined(temp);
  metaDataJson["BodyPartExamined"] = temp.c_str();

  if (parametricMap->getNumberOfFrames() > 0) {
    FGInterface& fg = parametricMap->getFunctionalGroups();
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

bool ParametricMapObject::isDerivationFGRequired(vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> >& slice2frame) {
  // if there is a derivation item for at least one frame, DerivationImageSequence must be present for every frame.
  unsigned nSlices = itkImage->GetLargestPossibleRegion().GetSize()[2];
  for (unsigned long sliceNumber=0;sliceNumber<nSlices; sliceNumber++) {
    if(!slice2frame[sliceNumber].empty()){
      return true;
    }
  }
  return false;
}

int ParametricMapObject::initializeFrames(vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> >& slice2frame){

  FGPlanePosPatient* fgppp = FGPlanePosPatient::createMinimal("1","1","1");
  FGFrameContent* fgfc = new FGFrameContent();
  FGDerivationImage* fgder = new FGDerivationImage();
  OFVector<FGBase*> perFrameFGs;

  unsigned nSlices = itkImage->GetLargestPossibleRegion().GetSize()[2];

  bool derivationFGRequired = isDerivationFGRequired(slice2frame);

  for (unsigned long sliceNumber = 0;sliceNumber < nSlices; sliceNumber++) {

    perFrameFGs.push_back(fgppp);
    perFrameFGs.push_back(fgfc);

    fgder->clearData();
    if(!slice2frame[sliceNumber].empty()){
        if(metaDataJson.isMember("DerivationCode")){
          CodeSequenceMacro purposeOfReference = CodeSequenceMacro("121322","DCM","Source image for image processing operation");
          CodeSequenceMacro derivationCode = CodeSequenceMacro(metaDataJson["DerivationCode"]["CodeValue"].asCString(),
                                                               metaDataJson["DerivationCode"]["CodingSchemeDesignator"].asCString(),
                                                               metaDataJson["DerivationCode"]["CodeMeaning"].asCString());
          addDerivationItemToDerivationFG(fgder, slice2frame[sliceNumber], purposeOfReference, derivationCode);
        } else {
          addDerivationItemToDerivationFG(fgder, slice2frame[sliceNumber]);
        }
    }

    if(derivationFGRequired)
      perFrameFGs.push_back(fgder);

    Float32ITKImageType::RegionType sliceRegion;
    Float32ITKImageType::IndexType sliceIndex;
    Float32ITKImageType::SizeType inputSize = itkImage->GetBufferedRegion().GetSize();

    sliceIndex[0] = 0;
    sliceIndex[1] = 0;
    sliceIndex[2] = sliceNumber;

    inputSize[2] = 1;

    sliceRegion.SetIndex(sliceIndex);
    sliceRegion.SetSize(inputSize);

    const unsigned frameSize = inputSize[0] * inputSize[1];

    OFVector<IODFloatingPointImagePixelModule::value_type> data(frameSize);

    itk::ImageRegionConstIteratorWithIndex<Float32ITKImageType> sliceIterator(itkImage, sliceRegion);

    unsigned framePixelCnt = 0;
    for(sliceIterator.GoToBegin();!sliceIterator.IsAtEnd(); ++sliceIterator, ++framePixelCnt){
      data[framePixelCnt] = sliceIterator.Get();
      Float32ITKImageType::IndexType idx = sliceIterator.GetIndex();
      //      cout << framePixelCnt << " " << idx[1] << "," << idx[0] << endl;
    }

    // Plane Position
    Float32ITKImageType::PointType sliceOriginPoint;
    itkImage->TransformIndexToPhysicalPoint(sliceIndex, sliceOriginPoint);
    fgppp->setImagePositionPatient(
        dcmqi::Helper::floatToStrScientific(sliceOriginPoint[0]).c_str(),
        dcmqi::Helper::floatToStrScientific(sliceOriginPoint[1]).c_str(),
        dcmqi::Helper::floatToStrScientific(sliceOriginPoint[2]).c_str());

    // Frame Content
    OFCondition result = fgfc->setDimensionIndexValues(sliceNumber+1 /* value within dimension */, 0 /* first dimension */);

    DPMParametricMapIOD::FramesType frames = parametricMap->getFrames();
    result = OFget<DPMParametricMapIOD::Frames<IODFloatingPointImagePixelModule::value_type> >(&frames)->addFrame(&*data.begin(), frameSize, perFrameFGs);

    perFrameFGs.clear();

  }

  return EXIT_SUCCESS;
}
