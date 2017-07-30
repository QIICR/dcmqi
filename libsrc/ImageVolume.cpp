//
// Created by Andrey Fedorov on 3/9/17.
//

#include <iostream>
#include <map>
#include <vector>

#include <dcmtk/dcmfg/fgplanor.h>
#include <vnl/vnl_cross.h>
#include <dcmtk/dcmfg/fgplanpo.h>
#include "dcmqi/ImageVolume.h"

using namespace std;

int dcmqi::ImageVolume::initializeFromDICOM(FGInterface& fgInterface) {
  if(initializeDirections(fgInterface))
    return EXIT_FAILURE;
  if(initializeExtent(fgInterface))
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}


int dcmqi::ImageVolume::initializeFromITK(Float32ITKImageType::Pointer itkImage){
  return EXIT_SUCCESS;
}

int dcmqi::ImageVolume::initializeDirections(FGInterface &fgInterface) {
  // TODO: handle the situation when FoR is not initialized
  OFBool isPerFrame;

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
      columnDirection[i-3] = atof(orientStr.c_str());
    } else {
      cerr << "Failed to get orientation " << i << endl;
      return EXIT_FAILURE;
    }
  }
  vnl_vector<double> sliceDirection = vnl_cross_3d(rowDirection, columnDirection);
  sliceDirection.normalize();

  cout << "Row direction: " << rowDirection << endl;
  cout << "Col direction: " << columnDirection << endl;
  cout << "Z direction: " << sliceDirection << endl;

  return EXIT_SUCCESS;
}


int dcmqi::ImageVolume::initializeExtent(FGInterface &fgInterface) {
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

  double calculatedSliceSpacing = 0;

  unsigned numFrames = fgInterface.getNumberOfFrames();

  /* Framesorter is to be moved to DCMTK at some point
   * in the future. For now it is causing build issues on windows

  FrameSorterIPP fsIPP;
  FrameSorterIPP::Results sortResults;
  fsIPP.setSorterInput(&fgInterface);
  fsIPP.sort(sortResults);

  */

  // Determine ordering of the frames, keep mapping from ImagePositionPatient string
  //   to the distance, and keep track (just out of curiosity) how many frames overlap
  vnl_vector<double> refOrigin(3);
  {
    OFBool isPerFrame;
    FGPlanePosPatient *planposfg = OFstatic_cast(FGPlanePosPatient*,
                                                 fgInterface.get(0, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
    if(!isPerFrame){
      cerr << "PlanePositionPatient FG is shared ... cannot handle this!" << endl;
      return EXIT_FAILURE;
    }

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
        origin[0] = sOrigin[0];
        origin[1] = sOrigin[1];
        origin[2] = sOrigin[2];
      }
      else if(dist<minDistance){
        origin[0] = sOrigin[0];
        origin[1] = sOrigin[1];
        origin[2] = sOrigin[2];
        minDistance = dist;
      }
    } else {
      frame2overlap[sOriginStr]++;
    }
  }

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

    calculatedSliceSpacing = fabs(originDistances[0]-originDistances[1]);

    for(size_t i=1;i<originDistances.size(); i++){
      float dist1 = fabs(originDistances[i-1]-originDistances[i]);
      float delta = calculatedSliceSpacing-dist1;
    }

    sliceExtent = fabs(originDistances[0]-originDistances[originDistances.size()-1]);
    unsigned overlappingFramesCnt = 0;
    for(map<OFString, unsigned>::const_iterator it=frame2overlap.begin();
        it!=frame2overlap.end();++it){
      if(it->second>1)
        overlappingFramesCnt++;
    }

    cout << "Total frames: " << numFrames << endl;
    cout << "Total frames with unique IPP: " << originDistances.size() << endl;
    cout << "Total overlapping frames: " << overlappingFramesCnt << endl;
    cout << "Origin: " << origin << endl;
  }

  return EXIT_SUCCESS;
}

