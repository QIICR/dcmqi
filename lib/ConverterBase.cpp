#include "ConverterBase.h"


namespace dcmqi {

  IODGeneralEquipmentModule::EquipmentInfo ConverterBase::getEquipmentInfo() {
  	return IODGeneralEquipmentModule::EquipmentInfo(QIICR_MANUFACTURER, QIICR_DEVICE_SERIAL_NUMBER,
																										QIICR_MANUFACTURER_MODEL_NAME, QIICR_SOFTWARE_VERSIONS);
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
      CHECK_COND(ident.setContentDescription("Image segmentation"));
    }
    if(metaInfo.metaInfoRoot["seriesAttributes"].isMember("ContentLabel")){
      CHECK_COND(ident.setContentLabel(metaInfo.metaInfoRoot["seriesAttributes"]["ContentLabel"].asCString()));
    } else {
      CHECK_COND(ident.setContentLabel("SEGMENTATION"));
    }
    return ident;
  }
}
