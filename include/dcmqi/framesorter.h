/*
 *
 *  Copyright (C) 2014, OFFIS e.V.
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation were developed by
 *
 *    OFFIS e.V.
 *    R&D Division Health
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *
 *  Module: dcmfg
 *
 *  Author: Michael Onken
 *
 *  Purpose: Abstract base class for sorting frames of a functional group
 *
 */

#ifndef FRAMESORTER_H
#define FRAMESORTER_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/ofstd/ofcond.h"
#include "dcmtk/ofstd/ofvector.h"
#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/dcmfg/fginterface.h"
#include "dcmtk/dcmfg/fgplanpo.h"
#include "dcmtk/dcmfg/fgplanor.h"
#include "dcmtk/ofstd/ofcond.h"

#include <stdlib.h>
#include <math.h>

typedef OFVector<Float64> ImagePosition;      // Image Position Patient
typedef OFPair<Uint32, Float64> FrameWithPos; // DICOM frame number and Image Position Patient

/** Abstract class for sorting a set of frames in a functional group. The
 *  sorting criteria are up to the actual implementation classes.
 */
class FrameSorter
{

public:

  /** Structure that transports the results of a frame sorting operation
   */
  struct Results
  {
    /** Default constructor, initializes empty results
     */
    Results() :
      errorCode(EC_Normal),
      frameNumbers(),
      key(DCM_UndefinedTagKey),
      fgSequenceKey(DCM_UndefinedTagKey),
      fgPrivateCreator() { }

    void clear()
    {
      errorCode = EC_Normal;
      frameNumbers.clear();
      key = DCM_UndefinedTagKey;
      fgSequenceKey = DCM_UndefinedTagKey;
      fgPrivateCreator = "";
    }

    /// Error code: EC_Normal if sorting was successful, error code otherwise.
    /// The error code should be set in any case (default: EC_Normal)
    OFCondition errorCode;
    /// The frame numbers, in sorted order (default: empty)
    OFVector<Uint32> frameNumbers;
    /// The frame positions, in sorted order, if provided
    /// by the sorter (default: empty). If not empty, contains the same number
    /// of items as frameNumbers.
    OFVector<ImagePosition> framePositions;
    /// Tag key that contains the information that was crucial for sorting.
    /// This is especially useful for creating dimension indices. Should be
    /// set to (0xffff,0xfff) if none was used (default).
    DcmTagKey key;
    /// Tag functional group sequence key that contains the tag key (see other member)
    /// that was crucial for sorting.
    /// This is especially useful for creating dimension indices. Should be
    /// set to (0xffff,0xfff) if none was used (default).
    DcmTagKey fgSequenceKey;
    /// Tag functional group sequence's private creator string for the fgSequenceKey
    /// result member if fgSequenceKey is a private attributes.
    /// This is especially useful for creating dimension indices that base on private
    /// attibutes. Should be left empty if fgSequenceKey is not private or fgSequenceKey
    /// is not used at all (default).
    OFString fgPrivateCreator;
  };

  struct FramePositions
  {
    OFVector<ImagePosition> positions;
  };

  /** Default constructor, does nothing
   */
  FrameSorter(){};

  /** Set input data for this sorter
   *  @param  fg The functional groups to work on. Ownership
   *          of pointer stays with the caller.
   */
  void setSorterInput(FGInterface* fg)
  {
    m_fg = fg;
  }

  /** Virtual default desctructor, does nothing
   */
  virtual ~FrameSorter() {}

  /** Return a frame order that is determined by the implementation of the particular
   *  derived class. E.g. a sorting by Plane Position (Patient) could be implemented.
   *  @param  results The results of the sorting procedure. Should be empty (cleared)
   *          when handed into the function.
   */
  virtual void sort(Results& results) =0;

  /** Get description of the sorting algorithm this class uses.
   *  @return Free text description of the sorting algorithm used.
   */
  virtual OFString getDescription() =0;


  // Derived classes may add further functions, e.g. to provide further parameters,
  // like the main dataset, frame data, etc.


protected:

  FGInterface* m_fg;
};

/** Dummy sorter implementing the FrameSorter interface,
  *  but not doing any sorting at all. As a result it provides
  *  a list of frames in their natural order, as found in the underlying
  *  DICOM dataset. The results will not contain frame position
  *  to make this implementation as lightweight and "stupid" as possible.
 */
class FrameSorterIdentity : public FrameSorter
{

public:

  FrameSorterIdentity(){};

  virtual ~FrameSorterIdentity()
  {

  }

  virtual OFString getDescription()
  {
    return "Returns frames in the order defined in the functional group, i.e. as defined in the image file";
  }

  /** Performs actual sorting. Does only set Results.frameNumbers and errorCode,
   *  leaving the rest untouched.
   *  @param  results The results produced by dummy sorter (list of frame numbers as
   *          found in the underlying DICOM dataset, and EC_Normal as error code)
   */
  virtual void sort(Results& results)
  {
    if (m_fg == NULL)
    {
      results.errorCode = FG_EC_InvalidData;
      return;
    }

    size_t numFrames = m_fg->getNumberOfFrames();
    if (numFrames == 0)
    {
      results.errorCode = FG_EC_NotEnoughItems;
      return;
    }

    for (Uint32 count = 0; count < numFrames; count++)
    {
      results.frameNumbers.push_back(count);
    }
    return;
  }

};

/** Sorter implementing the FrameSorter interface that sorts frames
 *  by their Image Position (Patient) attribute. The sorting is done
 *  by projecting the Image Position (Patient) on the slice direction
 *  (as defined by the Image Orientation (Patient) attribute).
 */
class FrameSorterIPP : public FrameSorter
{
public:

  struct OrderedFrameItem
  {
    OrderedFrameItem() :
      key(),
      frameId()
    {}

    /// The key relevant for sorting
    Float64 key;
    /// The DICOM frame number
    Uint32 frameId;
    /// The frame position as found in Image Position (Patient)
    ImagePosition framePos;
  };


  FrameSorterIPP(){};

  ~FrameSorterIPP(){};

  OFString getDescription(){
    return "Returns frames in the order defined by projecting the ImagePositionPatient on the slice direction.";
  }

  void getSliceDirection(Results &results)
  {
    OFBool isPerFrame;
    FGPlaneOrientationPatient *planorfg = OFstatic_cast(FGPlaneOrientationPatient*,
                                                        m_fg->get(0, DcmFGTypes::EFG_PLANEORIENTPATIENT, isPerFrame));
    if(!planorfg || isPerFrame){
      results.errorCode = FG_EC_InvalidData;
      return;
    }

    OFVector<Float64> dirX, dirY;
    OFString orientStr;
    for(int i=0;i<3;i++){
      if(planorfg->getImageOrientationPatient(orientStr, i).good()){
        dirX.push_back(atof(orientStr.c_str()));
      } else {
        results.errorCode = FG_EC_InvalidData;
        break;
      }
    }
    for(int i=3;i<6;i++){
      if(planorfg->getImageOrientationPatient(orientStr, i).good()){
        dirY.push_back(atof(orientStr.c_str()));
      } else {
        results.errorCode = FG_EC_InvalidData;
        break;
      }
    }

    if(results.errorCode != EC_Normal)
      return;

    sliceDirection = cross_3d(dirX, dirY);
    normalize(sliceDirection);
  }

  void sort(Results& results)
  {
    if(m_fg == NULL){
      results.errorCode = FG_EC_InvalidData;
      return;
    }

    getSliceDirection(results);
    if(results.errorCode != EC_Normal){
      return;
    }

    OFBool isPerFrame;
    size_t numFrames = m_fg->getNumberOfFrames();
    OrderedFrameItem* orderedFrameItems = new OrderedFrameItem[numFrames];

    for(int frameId=0;frameId<numFrames;frameId++)
    {
      FGPlanePosPatient *planposfg =
        OFstatic_cast(FGPlanePosPatient*,m_fg->get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));

      if(!planposfg || !isPerFrame){
        results.errorCode = FG_EC_InvalidData;
        return;
      }

      ImagePosition sOrigin;
      for(int j=0;j<3;j++){
        OFString planposStr;
        if(planposfg->getImagePositionPatient(planposStr, j).good()){
          sOrigin.push_back(atof(planposStr.c_str()));
        } else {
          results.errorCode = FG_EC_InvalidData;
          return;
        }
      }

      Float64 dist;
      dist = dot(sliceDirection, sOrigin);
      orderedFrameItems[frameId].key = dist;
      orderedFrameItems[frameId].frameId = frameId;
      orderedFrameItems[frameId].framePos = sOrigin;
    }

    qsort(&orderedFrameItems[0], numFrames, sizeof(OrderedFrameItem), &compareIPPKeys);

    for(Uint32 count=0;count<numFrames;count++)
    {
      results.frameNumbers.push_back(orderedFrameItems[count].frameId);
      results.framePositions.push_back(orderedFrameItems[count].framePos);
    }

    delete [] orderedFrameItems;

    return;
  }

private:

  OFVector<Float64> cross_3d(OFVector<Float64> v1, OFVector<Float64> v2){
    OFVector<Float64> result;
    result.push_back(v1[1]*v2[2]-v1[2]*v2[1]);
    result.push_back(v1[2]*v2[0]-v1[0]*v2[2]);
    result.push_back(v1[0]*v2[1]-v1[1]*v2[0]);

    return result;
  }

  Float64 dot(OFVector<Float64> v1, OFVector<Float64> v2){
    return Float64(v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2]);
  }

  void normalize(OFVector<Float64> &v){
    double norm = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    v[0] = v[0]/norm;
    v[1] = v[1]/norm;
    v[2] = v[2]/norm;
  }

  static int compareIPPKeys(const void *a, const void *b);

  OFVector<Float64> sliceDirection;

};

int FrameSorterIPP::compareIPPKeys(const void *a, const void *b)
{
  FrameSorterIPP::OrderedFrameItem *i1, *i2;
  i1 = (FrameSorterIPP::OrderedFrameItem*) a;
  i2 = (FrameSorterIPP::OrderedFrameItem*) b;
  if(i1->key > i2->key)
    return 1;
  else
    return -1;
}

#endif // FRAMESORTHER_H
