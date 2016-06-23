#include "ConverterBase.h"


namespace dcmqi {

    IODGeneralEquipmentModule::EquipmentInfo ConverterBase::getEquipmentInfo() {
        IODGeneralEquipmentModule::EquipmentInfo eq;
        eq.m_Manufacturer = "QIICR";
        eq.m_DeviceSerialNumber = "0";
        eq.m_ManufacturerModelName = dcmqi_WC_URL;
        eq.m_SoftwareVersions = dcmqi_WC_REVISION;
        return eq;
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
