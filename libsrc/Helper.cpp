
// DCMQI includes
#include "dcmqi/Helper.h"

// DCMTK includes
#include <dcmtk/ofstd/oflist.h>

namespace dcmqi {

  bool Helper::isUndefinedOrPathDoesNotExist(const string &var, const string &humanReadableName) {
    return Helper::isUndefined(var, humanReadableName) || !Helper::pathExists(var);
  }

  bool Helper::isUndefinedOrPathsDoNotExist(vector<string> &var, const string &humanReadableName) {
    return Helper::isUndefined(var, humanReadableName) || !Helper::pathsExist(var);
  }

  bool Helper::pathsExist(const vector<string> &paths) {
    bool allExist = true;
    for(vector<string>::const_iterator iter = paths.begin(); iter != paths.end(); ++iter) {
      if (!Helper::pathExists(*iter)){
        allExist = false;
      }
    }
    return allExist;
  }

  bool Helper::pathExists(const string &path) {
    struct stat buffer;
    if (stat (path.c_str(), &buffer) != 0) {
      cerr << "Error: " << path << " not found!" << endl;
      return false;
    } else {
      return true;
    }
  }

  string Helper::getFileExtensionFromType(const string& type) {
    string extension = ".nrrd";
    if (type == "nii" || type == "nifti")
      extension = ".nii.gz";
    else if (type == "mhd")
      extension = ".mhd";
    else if (type == "mha")
      extension = ".mha";
    else if (type == "img")
      extension = ".img";
    else if (type == "hdr")
      extension = ".hdr";
    else if (type == "nrrd")
      extension = ".nrrd";
    return extension;
  }

  vector<string> Helper::getFileListRecursively(string directory) {
    OFList<OFString> fileList;
    vector<string> dicomImageFiles;
#if _WIN32
    replace(directory.begin(), directory.end(), '/', PATH_SEPARATOR);
#endif
    cout << "Searching recursively " << directory << " for DICOM files" << endl;
    if(OFStandard::searchDirectoryRecursively(directory.c_str(), fileList)) {
      for(OFListIterator(OFString) fileListIterator=fileList.begin(); fileListIterator!=fileList.end(); fileListIterator++) {
        dicomImageFiles.push_back((*fileListIterator).c_str());
      }
    }
    return dicomImageFiles;
  }

  vector<DcmDataset*> Helper::loadDatasets(const vector<string>& dicomImageFiles) {
    vector<DcmDataset*> dcmDatasets;
    OFString tmp, sopInstanceUID;
    DcmFileFormat* sliceFF = new DcmFileFormat();
    for(size_t dcmFileNumber=0; dcmFileNumber<dicomImageFiles.size(); dcmFileNumber++){
      if(sliceFF->loadFile(dicomImageFiles[dcmFileNumber].c_str()).good()){
        DcmDataset* currentDataset = sliceFF->getAndRemoveDataset();
        if(!currentDataset->tagExistsWithValue(DCM_PixelData)){
          std::cerr << "Source DICOM file does not contain PixelData, skipping: " << std::endl
             << "  >>>   " << dicomImageFiles[dcmFileNumber] << std::endl;
          continue;
        };
        currentDataset->findAndGetOFString(DCM_SOPInstanceUID, sopInstanceUID);
        bool exists = false;
        for(size_t i=0;i<dcmDatasets.size();i++) {
          dcmDatasets[i]->findAndGetOFString(DCM_SOPInstanceUID, tmp);
          if (tmp == sopInstanceUID) {
            cout << dicomImageFiles[dcmFileNumber].c_str() << " with SOPInstanceUID: " << sopInstanceUID
                 << " already exists" << endl;
            exists = true;
            break;
          }
        }
        if (!exists) {
          dcmDatasets.push_back(currentDataset);
        }
      } else {
        cerr << "Failed to read " << dicomImageFiles[dcmFileNumber] << ". Skipping it." << endl;
      }
    }
    delete sliceFF;
    return dcmDatasets;
  }


  string Helper::floatToStrScientific(float f) {
    ostringstream sstream;
    sstream << scientific << f;
    return sstream.str();
  }

  void Helper::checkValidityOfFirstSrcImage(DcmSegmentation *segdoc) {
    FGInterface &fgInterface = segdoc->getFunctionalGroups();
    bool isPerFrame = false;
    FGDerivationImage *derimgfg = OFstatic_cast(FGDerivationImage*, fgInterface.get(0, DcmFGTypes::EFG_DERIVATIONIMAGE,
                                                                                    isPerFrame));
    if(!derimgfg){
      cout << "Debug: No derivation items present in the segmentation dataset" << endl;
    }
    assert(isPerFrame);

    OFVector<DerivationImageItem *> &deritems = derimgfg->getDerivationImageItems();

    OFVector<SourceImageItem *> &srcitems = deritems[0]->getSourceImageItems();
    OFString codeValue;
    if(srcitems.size()>0){
      CodeSequenceMacro &code = srcitems[0]->getPurposeOfReferenceCode();
      if (!code.getCodeValue(codeValue).good()) {
        cout << "Failed to look up purpose of reference code" << endl;
        abort();
      }
    } else {
      cout << "Warning: Source images are not initialized!" << endl;
    }
  }

  void Helper::tokenizeString(string str, vector<string> &tokens, string delimiter) {
    // http://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
    size_t pos = 0;
    while ((pos = str.find(delimiter)) != string::npos) {
      string token = str.substr(0, pos);
      tokens.push_back(token);
      str.erase(0, pos + delimiter.length());
    }
    tokens.push_back(str);
  };

  void Helper::splitString(string str, string &head, string &tail, string delimiter) {
    // http://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
    size_t pos = str.find(delimiter);
    if (pos != string::npos) {
      head = str.substr(0, pos);
      tail = str.substr(pos + delimiter.length(), str.length() - 1);
    }
  };

  string Helper::toString(const unsigned int& value) {
    ostringstream oss;
    oss << value;
    return oss.str();
  }

  /**
   <p>Convert RGB values in sRGB to CIEXYZ in ICC PCS.</p>

   <p>SRGB Observer = 2°, Illuminant = D65, XYZ PCS Illuminant = D50.</p>

    @see <a
    href="http://en.wikipedia.org/wiki/SRGB#The_reverse_transformation">Wikipedia SRGB Reverse Transformation</a>

    @param rgb   array of length 3 containing R,G,B values each from 0 to 255
    return   array of length 3 containing X,Y,Z values
   **/
  float *Helper::getCIEXYZFromRGB(unsigned *rgb, float *cieXYZ) {
    double var_R = ((double) rgb[0]) / 255.0;
    double var_G = ((double) rgb[1]) / 255.0;
    double var_B = ((double) rgb[2]) / 255.0;

    if (var_R > 0.04045)
      var_R = pow((var_R + 0.055) / 1.055, 2.4);
    else
      var_R = var_R / 12.92;

    if (var_G > 0.04045)
      var_G = pow((var_G + 0.055) / 1.055, 2.4);
    else
      var_G = var_G / 12.92;

    if (var_B > 0.04045)
      var_B = pow((var_B + 0.055) / 1.055, 2.4);
    else
      var_B = var_B / 12.92;

    var_R = var_R * 100;
    var_G = var_G * 100;
    var_B = var_B * 100;

    // SRGB Observer = 2°, Illuminant = D65, XYZ PCS Illuminant = D50
    cieXYZ[0] = (float) (var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805);
    cieXYZ[1] = (float) (var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722);
    cieXYZ[2] = (float) (var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505);

    return cieXYZ;
  }

  /**
   <p>Convert CIEXYZ in ICC PCS to RGB values in sRGB.</p>

   <p>SRGB Observer = 2°, Illuminant = D65, XYZ PCS Illuminant = D50.</p>

    @see <a href="http://en.wikipedia.org/wiki/SRGB#The_forward_transformation_.28CIE_xyY_or_CIE_XYZ_to_sRGB.29">Wikipedia SRGB Forward Transformation</a>

    @param cieXYZ  array of length 3 containing X,Y,Z values
    return   array of length 3 containing R,G,B values each from 0

   **/
  unsigned *Helper::getRGBFromCIEXYZ(float *cieXYZ, unsigned *rgb) {
    // per http://www.easyrgb.com/index.php?X=MATH&H=01#text1
    double var_X = cieXYZ[0] / 100;    //X from 0 to  95.047    (Observer = 2°, Illuminant = D65)
    double var_Y = cieXYZ[1] / 100;    //Y from 0 to 100.000
    double var_Z = cieXYZ[2] / 100;    //Z from 0 to 108.883

    double var_R = var_X * 3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
    double var_G = var_X * -0.9689 + var_Y * 1.8758 + var_Z * 0.0415;
    double var_B = var_X * 0.0557 + var_Y * -0.2040 + var_Z * 1.0570;

    if (var_R > 0.0031308)
      var_R = 1.055 * pow(var_R, 1 / 2.4) - 0.055;
    else
      var_R = 12.92 * var_R;

    if (var_G > 0.0031308)
      var_G = 1.055 * pow(var_G, 1 / 2.4) - 0.055;
    else
      var_G = 12.92 * var_G;

    if (var_B > 0.0031308)
      var_B = 1.055 * pow(var_B, 1 / 2.4) - 0.055;
    else
      var_B = 12.92 * var_B;

    rgb[0] = (int) floor(var_R * 255. + .5);
    rgb[1] = (int) floor(var_G * 255. + .5);
    rgb[2] = (int) floor(var_B * 255. + .5);
    //System.err.println("CIEXYZ ("+cieXYZ[0]+","+cieXYZ[1]+","+cieXYZ[2]+") -> RGB ("+rgb[0]+","+rgb[1]+","+rgb[2]+")");
    return rgb;
  }

  /**
   <p>Convert CIEXYZ to CIE 1976 L*, a*, b*.</p>

   @see <a href="http://en.wikipedia.org/wiki/Lab_color_space#CIELAB-CIEXYZ_conversions">Wikipedia CIELAB-CIEXYZ_conversions</a>

   @param cieXYZ  array of length 3 containing X,Y,Z values
   return   array of length 3 containing L*,a*,b* values
  **/
  float *Helper::getCIELabFromCIEXYZ(float *cieXYZ, float *cieLab) {
    double var_X = cieXYZ[0] / 95.047;  //ref_X =  95.047   Observer= 2°, Illuminant= D65
    double var_Y = cieXYZ[1] / 100.000;   //ref_Y = 100.000
    double var_Z = cieXYZ[2] / 108.883;   //ref_Z = 108.883

    if (var_X > 0.008856)
      var_X = pow(var_X, 1.0 / 3);
    else
      var_X = (7.787 * var_X) + (16.0 / 116);

    if (var_Y > 0.008856)
      var_Y = pow(var_Y, 1.0 / 3);
    else
      var_Y = (7.787 * var_Y) + (16.0 / 116);

    if (var_Z > 0.008856)
      var_Z = pow(var_Z, 1.0 / 3);
    else
      var_Z = (7.787 * var_Z) + (16.0 / 116);

    cieLab[0] = (float) ((116 * var_Y) - 16);    // CIE-L*
    cieLab[1] = (float) (500 * (var_X - var_Y));   // CIE-a*
    cieLab[2] = (float) (200 * (var_Y - var_Z));   // CIE-b*

    return cieLab;
  }

  /**
   <p>Convert CIE 1976 L*, a*, b* to CIEXYZ.</p>

    @see <a
    href="http://en.wikipedia.org/wiki/Lab_color_space#CIELAB-CIEXYZ_conversions">Wikipedia CIELAB-CIEXYZ_conversions</a>

    @param cieLab  array of length 3 containing L*,a*,b* values
    return   array of length 3 containing X,Y,Z values
   **/
  float *Helper::getCIEXYZFromCIELab(float *cieLab, float *cieXYZ) {
    // per http://www.easyrgb.com/index.php?X=MATH&H=08#text8

    double var_Y = (cieLab[0] + 16) / 116;
    double var_X = cieLab[1] / 500 + var_Y;
    double var_Z = var_Y - cieLab[2] / 200;

    double var_Y_pow3 = pow(var_Y, 3.0);
    double var_X_pow3 = pow(var_X, 3.0);
    double var_Z_pow3 = pow(var_Z, 3.0);

    if (var_Y_pow3 > 0.008856)
      var_Y = var_Y_pow3;
    else
      var_Y = (var_Y - 16. / 116) / 7.787;

    if (var_X_pow3 > 0.008856)
      var_X = var_X_pow3;
    else
      var_X = (var_X - 16. / 116) / 7.787;

    if (var_Z_pow3 > 0.008856)
      var_Z = var_Z_pow3;
    else
      var_Z = (var_Z - 16. / 116) / 7.787;

    cieXYZ[0] = (float) (95.047 * var_X);   //ref_X =  95.047   Observer= 2°, Illuminant= D65
    cieXYZ[1] = (float) (100.000 * var_Y);   //ref_Y = 100.000
    cieXYZ[2] = (float) (108.883 * var_Z);   //ref_Z = 108.883

    return cieXYZ;
  }

  /**
   *  <p>Convert floating point CIELab values to the 16 bit fractional
   * integer scaled representation used in ICC profiles and DICOM.</p>
   *
   * <p>See ICC v4.3 Tables 12 and 13, and DICOM PS 3.3 C.10.7.1.1.</p>
   *
   * @param cieLab  array of length 3 containing L*,a*,b* values with L*
   * from 0.0 to 100.0, and a* and b* from -128.0 to +127.0
   * return   array of length 3 containing 16 bit scaled L*,a*,b*
   * values from  0 to 65535
   **/
  unsigned *Helper::getIntegerScaledCIELabFromCIELab(float *cieLab, unsigned *cieLabScaled) {
    // per PS 3.3 C.10.7.1.1 ... scale same way as ICC profile encodes them
    cieLabScaled[0] = (int) floor(cieLab[0] * 65535. / 100. + .5);
    cieLabScaled[1] = (int) floor((cieLab[1] + 128) * 65535. / 255. + .5);
    cieLabScaled[2] = (int) floor((cieLab[2] + 128) * 65535. / 255. + .5);
    return cieLabScaled;
  }

  /**
    <p>Convert 16 bit fractional integer scaled CIELab values used in ICC profiles and DICOM to floating point.</p>

    <p>See ICC v4.3 Tables 12 and 13, and DICOM PS 3.3 C.10.7.1.1.</p>

     @param cieLabScaled  array of length 3 containing 16 bit scaled L*,a*,b* values from  0 to 65535
     return     array of length 3 containing L*,a*,b* values with L* from 0.0 to 100.0, and a* and b* from -128.0 to +127.0
    **/
  float *Helper::getCIELabFromIntegerScaledCIELab(unsigned *cieLabScaled, float *cieLab) {
    cieLab[0] = (float) (((double) cieLabScaled[0]) / 65535 * 100);
    cieLab[1] = (float) (((double) cieLabScaled[1]) / 65535 * 255 - 128);
    cieLab[2] = (float) (((double) cieLabScaled[2]) / 65535 * 255 - 128);
    return cieLab;
  }

  CodeSequenceMacro Helper::stringToCodeSequenceMacro(string str) {
    string tail, code, designator, meaning;
    splitString(str, code, tail, ",");
    splitString(tail, designator, meaning, ",");
    return CodeSequenceMacro(code.c_str(), designator.c_str(), meaning.c_str());
  }

  DSRCodedEntryValue Helper::stringToDSRCodedEntryValue(string str) {
    string tail, code, designator, meaning;
    splitString(str, code, tail, ",");
    splitString(tail, designator, meaning, ",");
    return DSRCodedEntryValue(code.c_str(), designator.c_str(), meaning.c_str());
  }

  CodeSequenceMacro* Helper::createNewCodeSequence(const string& code, const string& designator, const string& meaning) {
    if (code.empty() || designator.empty() || meaning.empty())
      throw CodeSequenceValueException();
    return new CodeSequenceMacro(code.c_str(), designator.c_str(), meaning.c_str());
  }


}
