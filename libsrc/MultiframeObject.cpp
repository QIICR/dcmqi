//
// Created by Andrey Fedorov on 3/11/17.
//

#include <dcmqi/Helper.h>
#include "dcmqi/QIICRConstants.h"
#include "dcmqi/MultiframeObject.h"

int MultiframeObject::initializeFromDICOM(std::vector<DcmDataset *> sourceDataset) {
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeMetaDataFromString(const std::string &metaDataStr) {
  std::istringstream metaDataStream(metaDataStr);
  metaDataStream >> metaDataJson;
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeEquipmentInfo() {
  if(sourceRepresentationType == ITK_REPR){
    equipmentInfoModule.m_Manufacturer = QIICR_MANUFACTURER;
    equipmentInfoModule.m_DeviceSerialNumber = QIICR_DEVICE_SERIAL_NUMBER;
    equipmentInfoModule.m_ManufacturerModelName = QIICR_MANUFACTURER_MODEL_NAME;
    equipmentInfoModule.m_SoftwareVersions = QIICR_SOFTWARE_VERSIONS;
  } else { // DICOM_REPR
  }
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeContentIdentification() {
  if(sourceRepresentationType == ITK_REPR){
    CHECK_COND(contentIdentificationMacro.setContentCreatorName("dcmqi"));
    if(metaDataJson.isMember("ContentDescription")){
      CHECK_COND(contentIdentificationMacro.setContentDescription(metaDataJson["ContentDescription"].asCString()));
    } else {
      CHECK_COND(contentIdentificationMacro.setContentDescription("DCMQI"));
    }
    if(metaDataJson.isMember("ContentLabel")){
      CHECK_COND(contentIdentificationMacro.setContentLabel(metaDataJson["ContentLabel"].asCString()));
    } else {
      CHECK_COND(contentIdentificationMacro.setContentLabel("DCMQI"));
    }
    return EXIT_SUCCESS;
  } else { // DICOM_REPR
  }
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeVolumeGeometryFromITK(DummyImageType::Pointer image) {
  DummyImageType::SpacingType spacing;
  DummyImageType::PointType origin;
  DummyImageType::DirectionType directions;
  DummyImageType::SizeType extent;

  spacing = image->GetSpacing();
  directions = image->GetDirection();
  extent = image->GetLargestPossibleRegion().GetSize();

  volumeGeometry.setSpacing(spacing);
  volumeGeometry.setOrigin(origin);
  volumeGeometry.setExtent(extent);

  volumeGeometry.setDirections(directions);

  return EXIT_SUCCESS;
}

// for now, we do not support initialization from JSON only,
//  and we don't have a way to validate metadata completeness - TODO!
bool MultiframeObject::metaDataIsComplete() {
  return false;
}

// List of tags, and FGs they belong to, for initializing dimensions module
int MultiframeObject::initializeDimensions(IODMultiframeDimensionModule& dimModule,
                                           std::vector<std::pair<DcmTag,DcmTag> > dimTagList){
  OFString dimUID;

  dimUID = dcmqi::Helper::generateUID();
  for(int i=0;i<dimTagList.size();i++){
    std::pair<DcmTag,DcmTag> dimTagPair = dimTagList[i];
    CHECK_COND(dimModule.addDimensionIndex(dimTagPair.first, dimUID, dimTagPair.second,
    dimTagPair.first.getTagName()));
  }
  return EXIT_SUCCESS;
}
