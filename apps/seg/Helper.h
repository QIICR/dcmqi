#ifndef DCMQI_HELPER_H
#define DCMQI_HELPER_H

#include "dcmtk/dcmseg/segdoc.h"
#include "dcmtk/dcmfg/fgderimg.h"
#include "dcmtk/dcmiod/iodmacro.h"
#include "dcmtk/dcmsr/dsrcodtn.h"

#include <vector>
#include <map>

using namespace std;


namespace dcmqi {


    class Helper {

    public:

        static string floatToStrScientific(float f);
        static void tokenizeString(string str, vector<string> &tokens, string delimiter);
        static void splitString(string str, string &head, string &tail, string delimiter);

        static float *getCIEXYZFromRGB(unsigned *rgb, float *cieXYZ);
        static float *getCIEXYZFromCIELab(float *cieLab, float *cieXYZ);
        static float *getCIELabFromCIEXYZ(float *cieXYZ, float *cieLab);
        static float *getCIELabFromIntegerScaledCIELab(unsigned *cieLabScaled, float *cieLab);
        static unsigned *getIntegerScaledCIELabFromCIELab(float *cieLab, unsigned *cieLabScaled);
        static unsigned *getRGBFromCIEXYZ(float *cieXYZ, unsigned *rgb);

        static CodeSequenceMacro stringToCodeSequenceMacro(string str);
        static DSRCodedEntryValue stringToDSRCodedEntryValue(string str);

        static void checkValidityOfFirstSrcImage(DcmSegmentation *segdoc);
    };

}

#endif //DCMQI_HELPER_H
