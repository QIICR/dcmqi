
// DCMQI includes
#include "dcmqi/ConverterBase.h"


namespace dcmqi {

  IODGeneralEquipmentModule::EquipmentInfo ConverterBase::getEquipmentInfo() {
    // TODO: change to following for most recent dcmtk
    // return IODGeneralEquipmentModule::EquipmentInfo(QIICR_MANUFACTURER, QIICR_DEVICE_SERIAL_NUMBER,
    //                                                 QIICR_MANUFACTURER_MODEL_NAME, QIICR_SOFTWARE_VERSIONS);
    IODGeneralEquipmentModule::EquipmentInfo eq;
    eq.m_Manufacturer = QIICR_MANUFACTURER;
    eq.m_DeviceSerialNumber = QIICR_DEVICE_SERIAL_NUMBER;
    eq.m_ManufacturerModelName = QIICR_MANUFACTURER_MODEL_NAME;
    eq.m_SoftwareVersions = QIICR_SOFTWARE_VERSIONS;
    return eq;
  }

  IODEnhGeneralEquipmentModule::EquipmentInfo ConverterBase::getEnhEquipmentInfo() {
    return IODEnhGeneralEquipmentModule::EquipmentInfo(QIICR_MANUFACTURER, QIICR_DEVICE_SERIAL_NUMBER,
                                                       QIICR_MANUFACTURER_MODEL_NAME, QIICR_SOFTWARE_VERSIONS);
  }

  // TODO: defaults for sub classes needs to be defined
  ContentIdentificationMacro ConverterBase::createContentIdentificationInformation(JSONMetaInformationHandlerBase &metaInfo) {
    ContentIdentificationMacro ident;
    CHECK_COND(ident.setContentCreatorName("dcmqi"));
    if(metaInfo.metaInfoRoot["seriesAttributes"].isMember("ContentDescription")){
      CHECK_COND(ident.setContentDescription(metaInfo.metaInfoRoot["seriesAttributes"]["ContentDescription"].asCString()));
    } else {
      CHECK_COND(ident.setContentDescription("DCMQI"));
    }
    if(metaInfo.metaInfoRoot["seriesAttributes"].isMember("ContentLabel")){
      CHECK_COND(ident.setContentLabel(metaInfo.metaInfoRoot["seriesAttributes"]["ContentLabel"].asCString()));
    } else {
      CHECK_COND(ident.setContentLabel("DCMQI"));
    }
    return ident;
  }
}
