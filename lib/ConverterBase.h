#ifndef DCMQI_CONVERTERBASE_H
#define DCMQI_CONVERTERBASE_H

#include <vector>
#include <dcmtk/dcmiod/iodmacro.h>
#include <dcmtk/dcmiod/modequipment.h>

// UIDs
#include "QIICRUIDs.h"

// versioning
#include "dcmqiVersionConfigure.h"

#include "Exceptions.h"


using namespace std;

namespace dcmqi {

    class ConverterBase {

    public:
        static int CHECK_COND(const OFCondition& condition);

    protected:
        static IODGeneralEquipmentModule::EquipmentInfo getEquipmentInfo();
        static ContentIdentificationMacro createContentIdentificationInformation();
    };

}


#endif //DCMQI_CONVERTERBASE_H
