#ifndef DCMQI_HELPER_H
#define DCMQI_HELPER_H

// DCMTK includes
#include <dcmtk/dcmfg/fgderimg.h>
#include <dcmtk/dcmiod/iodmacro.h>
#include <dcmtk/dcmseg/segdoc.h>
#include <dcmtk/dcmsr/dsrcodtn.h>

// STD includes
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

// DCMQI includes
#include "dcmqi/Exceptions.h"

#include <json/json.h>

using namespace std;

namespace dcmqi {


  class Helper {

  public:

    static bool isUndefinedOrPathDoesNotExist(const string &var, const string &humanReadableName);
    static bool isUndefinedOrPathsDoNotExist(vector<string> &var, const string &humanReadableName);

    template<typename T>
    static bool isUndefined(const T &var, const string &humanReadableName) {
      if (var.empty()) {
        cerr << "Error: " << humanReadableName << " must be specified!" << endl;
        return true;
      }
      return false;
    }
    static bool pathsExist(const vector<string> &paths);
    static bool pathExists(const string &path);

    static string getFileExtensionFromType(const string& type);
    static vector<string> getFileListRecursively(string directory);
    static vector<DcmDataset*> loadDatasets(const vector<string>& dicomImageFiles);

    static string floatToStrScientific(float f);
    static void tokenizeString(string str, vector<string> &tokens, string delimiter);
    static void splitString(string str, string &head, string &tail, string delimiter);

    static string toString(const unsigned int& value);

    static float *getCIEXYZFromRGB(unsigned *rgb, float *cieXYZ);
    static float *getCIEXYZFromCIELab(float *cieLab, float *cieXYZ);
    static float *getCIELabFromCIEXYZ(float *cieXYZ, float *cieLab);
    static float *getCIELabFromIntegerScaledCIELab(unsigned *cieLabScaled, float *cieLab);
    static unsigned *getIntegerScaledCIELabFromCIELab(float *cieLab, unsigned *cieLabScaled);
    static unsigned *getRGBFromCIEXYZ(float *cieXYZ, unsigned *rgb);

    static CodeSequenceMacro stringToCodeSequenceMacro(string str);
    static DSRCodedEntryValue stringToDSRCodedEntryValue(string str);
    static string codeSequenceMacroToString(CodeSequenceMacro);

    static CodeSequenceMacro jsonToCodeSequenceMacro(Json::Value);

    static void checkValidityOfFirstSrcImage(DcmSegmentation *segdoc);

    static CodeSequenceMacro* createNewCodeSequence(const string& code, const string& designator, const string& meaning);

    static OFString generateUID();
    static OFString getTagAsOFString(DcmDataset*, DcmTagKey);
  };

}

#endif //DCMQI_HELPER_H
