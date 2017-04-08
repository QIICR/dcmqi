//
// Created by Andrey Fedorov on 3/9/17.
//

#include "dcmqi/DICOMFrame.h"

namespace dcmqi {
  int DICOMFrame::initializeFrameGeometryFromLegacyInstance() {
    OFString ippStr;
    for(int j=0;j<3;j++){
      CHECK_COND(frameDataset->findAndGetOFString(DCM_ImagePositionPatient, ippStr, j));
      frameIPP[j] = atof(ippStr.c_str());
    }
    return EXIT_SUCCESS;
  }

  int DICOMFrame::initializeFrameGeometryFromEnhancedInstance() {
    OFString ippStr;
  }
}