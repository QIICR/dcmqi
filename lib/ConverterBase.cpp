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

  ContentIdentificationMacro ConverterBase::createContentIdentificationInformation() {
		ContentIdentificationMacro ident;
		CHECK_COND(ident.setContentCreatorName("QIICR"));
		CHECK_COND(ident.setContentDescription("Iowa QIN segmentation result"));
		CHECK_COND(ident.setContentLabel("QIICR QIN IOWA"));
		return ident;
  }

  int ConverterBase::CHECK_COND(const OFCondition& condition) {
		if (condition.bad()) {
			cerr << condition.text() << " in " __FILE__ << ":" << __LINE__  << endl;
			throw OFConditionBadException();
		}
		return 0;
  }

}
