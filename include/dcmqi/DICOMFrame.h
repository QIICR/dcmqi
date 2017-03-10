//
// Created by Andrey Fedorov on 3/9/17.
//

#ifndef DCMQI_DICOMFRAME_H
#define DCMQI_DICOMFRAME_H

#include <dcmtk/dcmdata/dcdatset.h>

namespace dcmqi {

  // Describe input/output frames for the purposes of sorting and associating with the
  // slices of the ITK volume
  class DICOMFrame {
  public:
    // distinguish between the frames from the legacy data and enhanced multiframe objects
    enum {
      LegacyInstanceFrame = 0,
      EnhancedInstanceFrame
    };

    DICOMFrame(DcmDataset *dataset, int number) :
        frameNumber(number),
        frameDataset(dataset) {

    };

  private:
    int frameType;
    DcmDataset *frameDataset;
    int frameNumber;
    OFString originStr; // revisit this
  };
}

#endif //DCMQI_DICOMFRAME_H
