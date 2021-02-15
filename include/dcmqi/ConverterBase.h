#ifndef DCMQI_CONVERTERBASE_H
#define DCMQI_CONVERTERBASE_H

// STD includes
#include <algorithm>
#include <iostream>
#include <vector>

// VNL includes
#include "vnl/vnl_cross.h"

// DCMTK includes
#include <dcmtk/config/osconfig.h>   // make sure OS specific configuration is included first
#include <dcmtk/ofstd/ofstream.h>
#include <dcmtk/oflog/loglevel.h>
#include <dcmtk/oflog/oflog.h>
#include <dcmtk/dcmiod/iodmacro.h>
#include <dcmtk/dcmiod/modenhequipment.h>
#include <dcmtk/dcmiod/modequipment.h>
#include <dcmtk/dcmfg/fginterface.h>
#include <dcmtk/dcmfg/fgplanor.h>
#include <dcmtk/dcmfg/fgplanpo.h>
#include <dcmtk/dcmfg/fgpixmsr.h>
#include <dcmtk/dcmdata/dcrledrg.h>

// ITK includes
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkLabelImageToLabelMapFilter.h>

// DCMQI includes
#include "dcmqi/Exceptions.h"
#include "dcmqi/JSONMetaInformationHandlerBase.h"
#include "dcmqi/QIICRUIDs.h"
#include "dcmqi/QIICRConstants.h"

using namespace std;

typedef short ShortPixelType;
typedef itk::Image<ShortPixelType, 3> ShortImageType;
typedef itk::ImageFileReader<ShortImageType> ShortReaderType;

namespace dcmqi {

  class ConverterBase {

  protected:
    static IODGeneralEquipmentModule::EquipmentInfo getEquipmentInfo();
    static IODEnhGeneralEquipmentModule::EquipmentInfo getEnhEquipmentInfo();
    static ContentIdentificationMacro createContentIdentificationInformation(JSONMetaInformationHandlerBase &metaInfo);

    template <class T>
    static int getImageDirections(FGInterface &fgInterface, T &dir){
      // TODO: handle the situation when FoR is not initialized
      OFBool isPerFrame;
      vnl_vector<double> rowDirection(3), colDirection(3);

      FGPlaneOrientationPatient *planorfg = OFstatic_cast(FGPlaneOrientationPatient*,
                                                          fgInterface.get(0, DcmFGTypes::EFG_PLANEORIENTPATIENT, isPerFrame));
      if(!planorfg){
        cerr << "Plane Orientation (Patient) is missing, cannot parse input " << endl;
        return EXIT_FAILURE;
      }
      OFString orientStr;
      for(int i=0;i<3;i++){
        if(planorfg->getImageOrientationPatient(orientStr, i).good()){
          rowDirection[i] = atof(orientStr.c_str());
        } else {
          cerr << "Failed to get orientation " << i << endl;
          return EXIT_FAILURE;
        }
      }
      for(int i=3;i<6;i++){
        if(planorfg->getImageOrientationPatient(orientStr, i).good()){
          colDirection[i-3] = atof(orientStr.c_str());
        } else {
          cerr << "Failed to get orientation " << i << endl;
          return EXIT_FAILURE;
        }
      }
      vnl_vector<double> sliceDirection = vnl_cross_3d(rowDirection, colDirection);
      sliceDirection.normalize();

      cout << "Row direction: " << rowDirection << endl;
      cout << "Col direction: " << colDirection << endl;

      for(int i=0;i<3;i++){
        dir[i][0] = rowDirection[i];
        dir[i][1] = colDirection[i];
        dir[i][2] = sliceDirection[i];
      }

      cout << "Z direction: " << sliceDirection << endl;

      return 0;
    }

    template <class T>
    static int computeVolumeExtent(FGInterface &fgInterface, vnl_vector<double> &sliceDirection, T &imageOrigin,
                                   double &sliceSpacing, double &sliceExtent) {
      // Size
      // Rows/Columns can be read directly from the respective attributes
      // For number of slices, consider that all segments must have the same number of frames.
      //   If we have FoR UID initialized, this means every segment should also have Plane
      //   Position (Patient) initialized. So we can get the number of slices by looking
      //   how many per-frame functional groups a segment has.

      vector<double> originDistances;
      map<OFString, double> originStr2distance;
      map<OFString, unsigned> frame2overlap;
      double minDistance = 0.0;

      sliceSpacing = 0;

      unsigned numFrames = fgInterface.getNumberOfFrames();

      /* Framesorter is to be moved to DCMTK at some point
       * in the future. For now it is causing build issues on windows

      FrameSorterIPP fsIPP;
      FrameSorterIPP::Results sortResults;
      fsIPP.setSorterInput(&fgInterface);
      fsIPP.sort(sortResults);

      */

      // Determine ordering of the frames, keep mapping from ImagePositionPatient string
      //   to the distance, and keep track (just out of curiousity) how many frames overlap
      vnl_vector<double> refOrigin(3);
      {
        OFBool isPerFrame;
        FGPlanePosPatient *planposfg = OFstatic_cast(FGPlanePosPatient*,
                                                     fgInterface.get(0, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
        for(int j=0;j<3;j++){
          OFString planposStr;
          if(planposfg->getImagePositionPatient(planposStr, j).good()){
            refOrigin[j] = atof(planposStr.c_str());
          } else {
            cerr << "Failed to read patient position" << endl;
          }
        }
      }

      for(size_t frameId=0;frameId<numFrames;frameId++){
        OFBool isPerFrame;
        FGPlanePosPatient *planposfg = OFstatic_cast(FGPlanePosPatient*,
                                                     fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));

        if(!planposfg){
          cerr << "PlanePositionPatient is missing" << endl;
          return EXIT_FAILURE;
        }

        if(!isPerFrame){
          cerr << "PlanePositionPatient is required for each frame!" << endl;
          return EXIT_FAILURE;
        }

        vnl_vector<double> sOrigin;
        OFString sOriginStr = "";
        sOrigin.set_size(3);
        for(int j=0;j<3;j++){
          OFString planposStr;
          if(planposfg->getImagePositionPatient(planposStr, j).good()){
            sOrigin[j] = atof(planposStr.c_str());
            sOriginStr += planposStr;
            if(j<2)
              sOriginStr+='/';
          } else {
            cerr << "Failed to read patient position" << endl;
            return EXIT_FAILURE;
          }
        }

        // check if this frame has already been encountered
        if(originStr2distance.find(sOriginStr) == originStr2distance.end()){
          vnl_vector<double> difference;
          difference.set_size(3);
          difference[0] = sOrigin[0]-refOrigin[0];
          difference[1] = sOrigin[1]-refOrigin[1];
          difference[2] = sOrigin[2]-refOrigin[2];
          double dist = dot_product(difference,sliceDirection);
          frame2overlap[sOriginStr] = 1;
          originStr2distance[sOriginStr] = dist;
          assert(originStr2distance.find(sOriginStr) != originStr2distance.end());
          originDistances.push_back(dist);

          if(frameId==0){
            minDistance = dist;
            imageOrigin[0] = sOrigin[0];
            imageOrigin[1] = sOrigin[1];
            imageOrigin[2] = sOrigin[2];
          }
          else if(dist<minDistance){
            imageOrigin[0] = sOrigin[0];
            imageOrigin[1] = sOrigin[1];
            imageOrigin[2] = sOrigin[2];
            minDistance = dist;
          }
        } else {
          frame2overlap[sOriginStr]++;
        }
      }

      cout << "Total frames: " << numFrames << endl;

      // it IS possible to have a segmentation object containing just one frame!
      if(numFrames>1){
        // WARNING: this should be improved further. Spacing should be calculated for
        //  consecutive frames of the individual segment. Right now, all frames are considered
        //  indiscriminately. Question is whether it should be computed at all, considering we do
        //  not have any information about whether the 2 frames are adjacent or not, so perhaps we should
        //  always rely on the declared spacing, and not even try to compute it?
        // TODO: discuss this with the QIICR team!

        // sort all unique distances, this will be used to check consistency of
        //  slice spacing, and also to locate the slice position from ImagePositionPatient
        //  later when we read the segments
        sort(originDistances.begin(), originDistances.end());

        sliceSpacing = fabs(originDistances[0]-originDistances[1]);

        // for(size_t i=1;i<originDistances.size(); i++){
          // float dist1 = fabs(originDistances[i-1]-originDistances[i]);
          // float delta = sliceSpacing-dist1;
        // }

        sliceExtent = fabs(originDistances[0]-originDistances[originDistances.size()-1]);
        unsigned overlappingFramesCnt = 0;
        for(map<OFString, unsigned>::const_iterator it=frame2overlap.begin();
            it!=frame2overlap.end();++it){
            if(it->second>1)
              overlappingFramesCnt++;
        }

        cout << "Total frames with unique IPP: " << originDistances.size() << endl;
        cout << "Total overlapping frames: " << overlappingFramesCnt << endl;
      }
      else{
        // Single frame has zero extent
        sliceExtent = 0.0;
        // set spacing to a valid value, it is overwritten by the actual thickness,
        // if specified in the file
        sliceSpacing = 1.0;
      }
      cout << "Origin: " << imageOrigin << endl;

      return 0;
    }

    template <class T>
    static int getDeclaredImageSpacing(FGInterface &fgInterface, T &spacing){
      OFBool isPerFrame;
      FGPixelMeasures *pixelMeasures = OFstatic_cast(FGPixelMeasures*,
                                                     fgInterface.get(0, DcmFGTypes::EFG_PIXELMEASURES, isPerFrame));
      if(!pixelMeasures){
        cerr << "Pixel measures FG is missing!" << endl;
        return EXIT_FAILURE;
      }

      /*
      In DICOM, "The first value is the row spacing in mm, that is the spacing between the centers of adjacent rows, or vertical spacing", while in ITK the first value is spacing along the X axis.
      https://github.com/QIICR/dcmqi/issues/425
      */
      pixelMeasures->getPixelSpacing(spacing[0], 1);
      pixelMeasures->getPixelSpacing(spacing[1], 0);

      Float64 spacingFloat;
      float epsilon = 1.e-5;
      if(pixelMeasures->getSpacingBetweenSlices(spacingFloat,0).good() && fabs(spacingFloat) > epsilon){
        spacing[2] = spacingFloat;
      } else if(pixelMeasures->getSliceThickness(spacingFloat,0).good() && fabs(spacingFloat) > epsilon){
        // SliceThickness can be carried forward from the source images, and may not be what we need
        // As an example, this ePAD example has 1.25 carried from CT, but true computed thickness is 1!
        cerr << "WARNING: SliceThickness is present and is " << spacingFloat << ". using it!" << endl;
        spacing[2] = spacingFloat;
      }
      return 0;
    }

    // AF: I could not quickly figure out how to template this function over image type - suggestions are welcomed!
    static vector<vector<int> > getSliceMapForSegmentation2DerivationImage(const vector<DcmDataset*> dcmDatasets,
                                                                                       const ShortImageType::Pointer &labelImage) {
      // Find mapping from the segmentation slice number to the derivation image
      // Assume that orientation of the segmentation is the same as the source series
      unsigned numLabelSlices = labelImage->GetLargestPossibleRegion().GetSize()[2];
      vector<vector<int> > slice2derimg(numLabelSlices);
      vector<bool> slice2derimgPresent(numLabelSlices, false);

      int slicesMapped = 0;
      for(size_t i=0;i<dcmDatasets.size();i++){
        OFString ippStr;
        ShortImageType::PointType ippPoint;
        ShortImageType::IndexType ippIndex;
        for(int j=0;j<3;j++){
          CHECK_COND(dcmDatasets[i]->findAndGetOFString(DCM_ImagePositionPatient, ippStr, j));
          ippPoint[j] = atof(ippStr.c_str());
        }
        if(!labelImage->TransformPhysicalPointToIndex(ippPoint, ippIndex)){
          cout << "image position: " << ippPoint << endl;
          cerr << "ippIndex: " << ippIndex << endl;
          // if certain DICOM instance does not map to a label slice, just skip it
          continue;
        }
        slice2derimg[ippIndex[2]].push_back(i);
        if(slice2derimgPresent[ippIndex[2]] == false)
          slicesMapped++;
        slice2derimgPresent[ippIndex[2]] = true;
      }
      cout << slicesMapped << " of " << slice2derimgPresent.size() << " slices mapped to source DICOM images" << endl;
      return slice2derimg;
    }

  };

}

#endif //DCMQI_CONVERTERBASE_H
