//
// Created by Andrey Fedorov on 3/11/17.
//

#include <dcmqi/Helper.h>
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

  createParametricMap();

  initializeCompositeContext();

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

  return EXIT_SUCCESS;
}

int ParametricMapObject::initializeCompositeContext() {
  if(derivationDcmDatasets.size()){
    CHECK_COND(parametricMap->import(*derivationDcmDatasets[0], OFTrue, OFTrue, OFFalse, OFTrue));

  } else {
    // TODO: once we support passing of composite context in metadata, propagate it
    //   into parametricMap here
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}