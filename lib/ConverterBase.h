#ifndef DCMQI_CONVERTERBASE_H
#define DCMQI_CONVERTERBASE_H

#include <vector>
#include "vnl/vnl_cross.h"

#include <dcmtk/dcmiod/iodmacro.h>
#include <dcmtk/dcmiod/modenhequipment.h>
#include <dcmtk/dcmiod/modequipment.h>
#include <dcmtk/dcmfg/fginterface.h>
#include <dcmtk/dcmfg/fgplanor.h>
#include <dcmtk/dcmfg/fgplanpo.h>
#include <dcmtk/dcmfg/fgpixmsr.h>

// UIDs
#include "QIICRUIDs.h"
#include "QIICRConstants.h"

// versioning
#include "dcmqiVersionConfigure.h"

#include "Exceptions.h"

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkLabelImageToLabelMapFilter.h>

using namespace std;

namespace dcmqi {

    class ConverterBase {

    public:
        static int CHECK_COND(const OFCondition& condition);

    protected:
        static IODGeneralEquipmentModule::EquipmentInfo getEquipmentInfo();
        static IODEnhGeneralEquipmentModule::EquipmentInfo getEnhEquipmentInfo();
        static ContentIdentificationMacro createContentIdentificationInformation();
    };

}


#endif //DCMQI_CONVERTERBASE_H
