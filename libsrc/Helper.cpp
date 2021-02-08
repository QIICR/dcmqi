
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


  string Helper::floatToStr(float f) {
    ostringstream sstream;
    sstream.imbue(std::locale::classic());
    sstream.precision(9);
    sstream << f;
    //string f_str = sstream.str();
    //cout << "Formatted float (length): " << f_str << "(" << f_str.size() << ")" << endl;
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
