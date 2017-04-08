//
// Created by Andrey Fedorov on 3/11/17.
//

#include <dcmqi/Helper.h>
#include <itkMinimumMaximumImageCalculator.h>
#include "dcmqi/ParametricMapObject.h"

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
  if (OFCondition* pCondition = OFget<OFCondition>(&obj))
    return EXIT_FAILURE;

  parametricMap = OFget<DPMParametricMapIOD>(&obj);

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

  realWorldValueMappingItem->setRealWorldValueFirstValueMappeSigned(calculator->GetMinimum());
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
        return NULL;
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