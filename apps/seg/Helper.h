#ifndef DCMQI_HELPER_H
#define DCMQI_HELPER_H

#include "dcmtk/dcmseg/segdoc.h"
#include "dcmtk/dcmfg/fgderimg.h"
#include "dcmtk/dcmiod/iodmacro.h"
#include "dcmtk/dcmsr/dsrcodtn.h"

#include <vector>

namespace dcmqi {

    class Helper {

    public:

        static std::string FloatToStrScientific(float f);
        static void TokenizeString(std::string str, std::vector<std::string> &tokens, std::string delimiter);
        static void SplitString(std::string str, std::string &head, std::string &tail, std::string delimiter);

        static float *getCIEXYZFromRGB(unsigned *rgb, float *cieXYZ);
        static float *getCIEXYZFromCIELab(float *cieLab, float *cieXYZ);
        static float *getCIELabFromCIEXYZ(float *cieXYZ, float *cieLab);
        static float *getCIELabFromIntegerScaledCIELab(unsigned *cieLabScaled, float *cieLab);
        static unsigned *getIntegerScaledCIELabFromCIELab(float *cieLab, unsigned *cieLabScaled);
        static unsigned *getRGBFromCIEXYZ(float *cieXYZ, unsigned *rgb);

        static CodeSequenceMacro StringToCodeSequenceMacro(std::string str);
        static DSRCodedEntryValue StringToDSRCodedEntryValue(std::string str);

        static void checkValidityOfFirstSrcImage(DcmSegmentation *segdoc);
    };

}

#endif //DCMQI_HELPER_H
