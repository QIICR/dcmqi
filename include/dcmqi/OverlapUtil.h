/*
 *
 *  Copyright (C) 1994-2023, Open Connections GmbH
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
 *  Module:  dcmseg
 *
 *  Author:  Michael Onken
 *
 *  Purpose: Interface of class OverlapUtil
 *
 */

#ifndef SEGOVLAP_H
#define SEGOVLAP_H

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oftypes.h"
#include "dcmtk/ofstd/ofvector.h"
#include "dcmtk/ofstd/ofcond.h"
#include "dcmtk/dcmiod/iodtypes.h"
#include <set>

class DcmSegmentation;

/// error: frames are not parallel
extern const OFConditionConst SG_EC_FramesNotParallel;

namespace dcmqi {

class OverlapUtil
{
public:

  // Image Position Patient tuple (x,y,z)
  typedef OFVector<Float64> ImagePosition; // might be defined in respective functional group

  struct FramePositionAndNumber
  {
    /** Default constructor required for vector initialization
     */
    FramePositionAndNumber() : m_position(), m_frameNumber(0) {}
    /** Constructor
     *  @param  pos Image position
    *   @param  num Pyhsical frame number
    */
    FramePositionAndNumber(const ImagePosition& pos, const Uint32& num) : m_position(pos), m_frameNumber(num) {}
    /// Frame position in space
    ImagePosition m_position;
    /// Physical frame number (number of frame in DICOM object)
    Uint32 m_frameNumber;
  };

  /// Physical Frame number with its respective position
  typedef OFVector<FramePositionAndNumber> FramePositions;

  /// Logical Frame, represented by various physical frames (numbers) at the same position
  typedef OFVector<Uint32> LogicalFrame;

  /// All distinct positions and for each position the physical frame numbers that can be found at it
  typedef OFVector<LogicalFrame> DistinctFramePositions;

  /// Lookup position of a frame in DistinctFramePositions vector by providing the frame number,
  /// i.e. index is frame number, value is position in DistinctFramePositions vector;
  /// or: which physical frame number is at which logical frame number/position?
  typedef OFVector<size_t> LogicalFrameToPositionIndex;

  /// Lists frames for each segment where segment with index i is represented by the vector at index i,
  /// and index 0 is unused. I.e. index i is segment number, value is vector of physical frame numbers.
  typedef OFVector<OFVector<Uint32> > FramesForSegment;

  // Implements comparision operator to be used for sorting of frame positions,
  // making the sorting order depend on the coordinate given in the constructor
  struct ComparePositions
  {
      /** Constructor, used to configure coordinate position to be used for sorting
       *  @param  c Coordinate position to be used for sorting
       */
      ComparePositions(size_t c) : m_coordinate(c) { }

      /** Compare two frames*/
      bool operator()(const FramePositionAndNumber& a, const FramePositionAndNumber& b) const
      {
          return a.m_position[m_coordinate] < b.m_position[m_coordinate];
      }
      /// Coordinate position (0-2, i.e. x-z) to be used for sorting
      size_t m_coordinate;
  };

  typedef OFVector<OFVector<Sint8> > OverlapMatrix;
  // TODO: Check whether a vector would also do
  typedef OFVector< std::set<Uint32> > SegmentGroups;


  /** Represents a segment number and a logical frame number it is found at
   */
  struct SegNumAndFrameNum
  {
    /** Constructor
     *  @param  s Segment number
     *  @param  f Logical frame number
     */
    SegNumAndFrameNum(const Uint16& s, const Uint16 f) : m_segmentNumber(s), m_frameNumber(f) {}
    /// Segment number as used in DICOM segmentation object (1-n)
    Uint16 m_segmentNumber;
    /// Logical frame number (number of frame in DistinctFramePositions vector)
    Uint16 m_frameNumber;
    /** Comparison operator
     *  @param  rhs Right-hand side of comparison
     *  @return OFTrue if left-hand side is smaller than right-hand side
     */
    bool operator<(const SegNumAndFrameNum& rhs) const
    {
      return m_segmentNumber < rhs.m_segmentNumber;
    }
  };

  /// Segments and their phyiscal frame number (inner set), grouped by their respective logical frame number (outer vector)
  typedef OFVector<std::set<SegNumAndFrameNum> > SegmentsByPosition;

  // ------------------------------------------ Methods ------------------------------------------


  /** Constructor. Use setSegmentationObject() to set the segmentation object to work with.
   */
  OverlapUtil();

  /** Destructor */
  ~OverlapUtil();

  /** Set the segmentation object to work with
   *  TODO: In the future, maybe have DcmSegmentation->getOverlapUtil() to access
   *  the OverlapUtil object, so that the user does not have to care about
   *  feeding the segmentation object to the OverlapUtil object.
   */
  void setSegmentationObject(DcmSegmentation* seg);

  /** Clears all internal data (except segmentation object reference)
   */
   void clear();

  /** Get all distinct frame positions and the physical frame numbers at this position
   *  @param  result Resulting vector of distinct frame positions
   *  @return EC_Normal if successful, error otherwise
   */
  OFCondition getFramesByPosition(DistinctFramePositions& result);

  /** Get all segments and their phyisical frame number, grouped by their respective logical frame number
   *  @param  result Resulting vector of segments grouped by logical frame number
   *  @return EC_Normal if successful, error otherwise
   */
  OFCondition getSegmentsByPosition(SegmentsByPosition& result) ;

  /** TODO Not yet implemented, nice extra functionality */
  OFCondition getSegmentsForFrame(const Uint32 frameNumber, OFVector<Uint32>& segments);

  /** Get phyiscal frames for a specific segment
   *  @param segmentNumber Segment number to get frames for (1..n)
   *  @param frames Resulting vector of physical frame numbers
   *         (1 is first used index, 0 is first frame number)
   *  @return EC_Normal if successful, error otherwise
   */
  OFCondition getFramesForSegment(const Uint32 segmentNumber, OFVector<Uint32>& frames);

  /** Returns computed overlap matrix
   *  @param  matrix Resulting overlap matrix
   *  @return EC_Normal if successful, error otherwise
   */
  OFCondition getOverlapMatrix(OverlapMatrix& matrix);

  /** Returns segments grouped together in a way, that no two overlapping
   *  segments will be in the same group. This method does not necessarily
   *  returns the optimal solution, but a solution that should be good enough.
   *  It is guaranteed, that segments in the same  group don't overlap.
   *
   *  It is based on the idea of a greedy algorithm  that creates a first group
   *  containing the first segment. Then it goes to the next segment, checks whether
   *  it fits into the first group with no overlaps (easily checked in overlap matrix)
   *  and inserts it into that group if no overlaps exists. Otherwise, it creates a
   *  new group and continues with the next segment (trying to insert it into
   *  the first group, then second group, otherwise creates third group, and so on).
   *  @param  segmentGroups Resulting vector of segment groups, each listing the
   *          segment numbers that are in that group
   *  @return EC_Normal if successful, error otherwise
   */
  OFCondition getNonOverlappingSegments(SegmentGroups& segmentGroups);

  /** Prints overlap matrix to given stream
   *  @param  ss The stream to dump to
   */
  void printOverlapMatrix(OFStringStream& ss);

  /** Prints groups of non-overlapping segments to given stream
   *  @param  ss The stream to dump to
   */
  void printNonOverlappingSegments(OFStringStream& ss);

protected:

  /** Builds the overlap matrix, if not already done.
   *  @return EC_Normal if successful or already existant, error otherwise
   */
  OFCondition buildOverlapMatrix();

  /** Checks if frames are parallel, i.e. if the image position patient
   *  of all frames are parallel to each other (i.e. found in the shared functional group)
   *  @return EC_Normal if parallel, SG_EC_FramesNotParallel if image orientation is not shared,
   *          error otherwise
   */
  OFCondition ensureFramesAreParallel();

  /** Groups all physical frames by their position. This also works if the physical frames
   *  have slightly differennt positions, i.e. if they are not exactly the same and are only
   *  "close enough" to be considered the same. Right now, the maximum distance treated equal
   *  is if distance is smaller than slice thickness * 0.01 (i.e. 1% of slice thickness).
   *  @return EC_Normal if successful, error otherwise
   */
  OFCondition groupFramesByPosition();

  /** Checks whether the given two frames overlap
   *  @param f1 Frame 1, provided by its physical frame number
   *  @param f2 Frame 2, provided by its physical frame number
   *  @param overlap Resulting overlap (overlaps if OFTrue, otherwise not)
   *  @return EC_Normal if successful, error otherwise
   */
  OFCondition checkFramesOverlap(const Uint32& f1, const Uint32& f2, OFBool& overlap);

  /** Checks whether the given two frames overlap by using comparing their pixel data
   *  by bitwise "and". Thi should be pretty efficient, however, only works and is called (right now),
   *  if row*cols % 8 = 0, so we can easily extract frames as binary bitsets without unpacking them.
   *  @param  f1 Frame 1, provided by its physical frame number
   *  @param  f2 Frame 2, provided by its physical frame number
   *  @param  overlap Resulting overlap (overlaps if OFTrue, otherwise not)
   *  @return EC_Normal if successful, error otherwise
   */
  OFCondition checkFramesOverlapBinary(const Uint32& f1, const Uint32& f2, const DcmIODTypes::Frame* f1_data, const DcmIODTypes::Frame* f2_data, const Uint16& rows, const Uint16 cols, OFBool& overlap);

  /** Checks whether the given two frames overlap by using comparing their pixel data
   *  after unpacking, i.e. expanding every bit to a byte, and then comparing whether the two
   *  related bytes of each frame are both non-zero. This is less efficient than checkFramesOverlapBinary(),
   *  @param  f1 Frame 1, provided by its physical frame number
   *  @param  f2 Frame 2, provided by its physical frame number
   *  @param  f1_data Pixel data of frame 1
   *  @param  f2_data Pixel data of frame 2
   *  @param  rows Number of rows of the frame(s)
   *  @param  cols Number of columns of the frame(s)
   *  @param  overlap Resulting overlap (overlaps if OFTrue, otherwise not)
   *  @return EC_Normal if successful, error otherwise
   */
  OFCondition checkFramesOverlapUnpacked(const Uint32& f1, const Uint32& f2, const DcmIODTypes::Frame* f1_data, const DcmIODTypes::Frame* f2_data, const Uint16& rows, const Uint16 cols, OFBool& overlap);

private:

  /// Frames with their respective positions (IPP)
  FramePositions m_FramePositions;

  /// Frame numbers (starting from 0) grouped by segment number (first segment is 1,
  /// i.e. index 0 is unused)
  FramesForSegment m_framesForSegment;

  /// Logical frames, ie. physical frames with the same position are
  /// grouped together in a logical frame. For every logical frame, we
  /// store the related physical frame numbers. The logical frame number
  /// is implicitly given by the index in the vector.
  DistinctFramePositions m_logicalFramePositions;

  // TODO: Not needed?
  // Allows reverse lookup of logical frames by position in m_logicalFramePositions
  // LogicalFrameToPositionIndex m_frameNumToPositionIndex;

  /// Stores for each logical frame a collection of (paired) segment and physical frame number,
  /// that exists at that position.
  SegmentsByPosition m_segmentsByPosition;

  /// Matrix that stores for each segment pair whether they overlap or not.
  /// I.e. Matrix has size n x n, where n is the number of segments.
  /// The diagonal is always 0 (no overlap), i.e. a segment never overlaps with itself.
  /// If there is an overlap, the value is 1.
  /// If the field is not initialized, the value is -1.
  OverlapMatrix m_segmentOverlapMatrix;

  //// Groups of segments that do not overlap with each other
  SegmentGroups m_nonOverlappingSegments;

  /// Reference to segmentation object to work with
  DcmSegmentation* m_seg;

};

} // namespace dcmqi

#endif // SEGOVLAP_H