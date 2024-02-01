/*
 *
 *  Copyright (C) 2023, Open Connections GmbH
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
 *  Module:  dcmqi
 *
 *  Author:  Michael Onken
 *
 *  Purpose: Interface of class OverlapUtil
 *
 */

#include "dcmqi/OverlapUtil.h"
#include "dcmqi/framesorter.h"
#include "dcmtk/dcmdata/dcerror.h"
#include "dcmtk/dcmfg/fginterface.h"
#include "dcmtk/dcmfg/fgpixmsr.h"
#include "dcmtk/dcmfg/fgplanor.h"
#include "dcmtk/dcmfg/fgplanpo.h"
#include "dcmtk/dcmfg/fgseg.h"
#include "dcmtk/dcmfg/fgtypes.h"
#include "dcmtk/dcmiod/iodtypes.h"
#include "dcmtk/dcmseg/segdoc.h"
#include "dcmtk/dcmseg/segtypes.h"
#include "dcmtk/dcmseg/segutils.h"
#include "dcmtk/ofstd/ofcond.h"
#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/ofstd/oftimer.h"
#include "dcmtk/ofstd/oftypes.h"

#include <cmath>
#include <cstdlib>

makeOFConditionConst(SG_EC_FramesNotParallel, OFM_dcmseg, 7, OF_error, "Frames are not parallel");

namespace dcmqi
{

OverlapUtil::OverlapUtil()
    : m_imageOrientation()
    , m_framePositions()
    , m_framesForSegment()
    , m_segmentsForFrame()
    , m_logicalFramePositions()
    , m_segmentsByPosition()
    , m_segmentOverlapMatrix()
    , m_nonOverlappingSegments()
    , m_seg()
{
}

OverlapUtil::~OverlapUtil()
{
    // nothing to do
}

void OverlapUtil::setSegmentationObject(DcmSegmentation* seg)
{
    m_seg = seg;
    clear();
}

void OverlapUtil::clear()
{
    m_imageOrientation.clear();
    m_framePositions.clear();
    m_framesForSegment.clear();
    m_logicalFramePositions.clear();
    m_segmentsByPosition.clear();
    m_segmentOverlapMatrix.clear();
    m_nonOverlappingSegments.clear();
}

OFCondition OverlapUtil::getFramesByPosition(DistinctFramePositions& result)
{
    OFCondition cond;
    if (!m_seg)
    {
        DCMSEG_ERROR("getFramesByPosition(): No segmentation object set");
        return EC_IllegalCall;
    }
    if (m_logicalFramePositions.empty())
    {
        cond = groupFramesByPosition();
    }
    if (cond.good())
    {
        result = m_logicalFramePositions;
    }
    return cond;
}

OFCondition OverlapUtil::getFramesForSegment(const Uint32 segmentNumber, std::set<Uint32>& frames)
{
    if (!m_seg)
    {
        DCMSEG_ERROR("getFramesForSegment(): No segmentation object set");
        return EC_IllegalCall;
    }
    if (m_framesForSegment.empty())
    {
        FGInterface& fg  = m_seg->getFunctionalGroups();
        size_t tempNum = m_seg->getNumberOfFrames();
        if (tempNum > 4294967295)
        {
            DCMSEG_ERROR("getFramesForSegment(): Number of frames " << tempNum << " exceeds maximum number of possible frames (2^32-1)");
            return EC_IllegalParameter;
        }
        Uint32 numFrames = static_cast<Uint32>(m_seg->getNumberOfFrames());
        // Use the helper function getSegmentsForFrame() to pivot the data
        // from segments on each frame, to the list of frames with contain each segment
        for (Uint32 f = 0; f < numFrames; f++)
        {
            OFCondition result;
            std::set<Uint32> segments;
            result = getSegmentsForFrame(f, segments);
            if (result.good())
            {
                for (std::set<Uint32>::iterator itr = segments.begin();
                     itr != segments.end();
                     itr++)
                {
                    m_framesForSegment[*itr].insert(f);
                }
            }
            else
            {
                DCMSEG_ERROR("getFramesForSegment(): Cannot get segments for frame " << f);
                return result;
            }
        }
    }
    frames = m_framesForSegment[segmentNumber];
    return EC_Normal;
}

OFCondition OverlapUtil::getSegmentsForFrame(const Uint32 frameNumber, std::set<Uint32>& segments)
{
    if (!m_seg)
    {
        DCMSEG_ERROR("getSegmentsForFrame(): No segmentation object set");
        return EC_IllegalCall;
    }
    if (frameNumber > m_seg->getNumberOfFrames() - 1)
    {
        DCMSEG_ERROR("getSegmentsForFrame(): Frame number " << frameNumber << " is out of range");
        return EC_IllegalParameter;
    }
    size_t tempNum = m_seg->getNumberOfFrames();
    if (tempNum > 4294967295)
    {
        DCMSEG_ERROR("getSegmentsForFrame(): Number of frames " << tempNum << " exceeds maximum number of possible frames (2^32-1)");
        return EC_IllegalParameter;
    }
    if (m_segmentsForFrame.empty())
    {
        Uint32 numFrames = static_cast<Uint32>(m_seg->getNumberOfFrames());
        m_segmentsForFrame.resize(numFrames);
        for (Uint32 f = 0; f < numFrames; f++) {
            OFCondition result;
            if (m_seg->getSegmentationType() == (DcmSegTypes::E_SegmentationType) 3) // LABELMAP
            {
                result = getSegmentsForLabelMapFrame(f, m_segmentsForFrame[f]);
                if (result.bad())
                {
                    return result;
                }
            }
            else
            {
                Uint32 segment;
                result = getSegmentForFrame(f, segment);
                if (result.good())
                {
                    m_segmentsForFrame[f].insert(segment);
                }
            }
        }
    }

    segments = m_segmentsForFrame[frameNumber];
    return EC_Normal;
}

OFCondition OverlapUtil::ensureFramesAreParallel()
{
    FGInterface& fg = m_seg->getFunctionalGroups();
    OFCondition cond;
    OFBool perFrame                = OFFalse;
    FGPlaneOrientationPatient* pop = NULL;
    // Ensure that Image Orientation Patient is shared, i.e. we have parallel frames
    m_imageOrientation.clear();
    m_imageOrientation.resize(6);
    FGBase* group = fg.get(0, DcmFGTypes::EFG_PLANEORIENTPATIENT, perFrame);
    if (group && (pop = OFstatic_cast(FGPlaneOrientationPatient*, group)))
    {
        if (perFrame == OFFalse)
        {
            DCMSEG_DEBUG("ensureFramesAreParallel(): Image Orientation Patient is shared, frames are parallel");
            m_imageOrientation.resize(6);
            cond = pop->getImageOrientationPatient(m_imageOrientation[0],
                                                   m_imageOrientation[1],
                                                   m_imageOrientation[2],
                                                   m_imageOrientation[3],
                                                   m_imageOrientation[4],
                                                   m_imageOrientation[5]);
            std::cout << "Image Orientation Patient set to : " << m_imageOrientation[0] << ", " << m_imageOrientation[1]
                      << ", " << m_imageOrientation[2] << ", " << m_imageOrientation[3] << ", " << m_imageOrientation[4]
                      << ", " << m_imageOrientation[5] << std::endl;
            return cond;
        }
        else
        {
            DCMSEG_ERROR(
                "ensureFramesAreParallel(): Image Orientation Patient is per-frame, frames are probably not parallel");
            return SG_EC_FramesNotParallel;
        }
    }
    else
    {
        DCMSEG_ERROR(
            "ensureFramesAreParallel(): Plane Orientation (Patient) FG not found, cannot check for parallel frames");
        return EC_TagNotFound;
    }
    return EC_Normal;
}

OFCondition OverlapUtil::groupFramesByPosition()
{
    if (!m_framePositions.empty())
    {
        // Already computed
        return EC_Normal;
    }

    OFCondition cond = ensureFramesAreParallel();
    if (cond.bad())
    {
        return cond;
    }

    OFTimer tm;

    // Group all frames by position into m_logicalFramePositions.
    // After that, all frames at the same position will be in the same vector
    // assigned to the same key (the frame's coordinates) in the map.
    // Group all frames by position into m_logicalFramePositions.
    FrameSorterIPP sorter;
    sorter.setSorterInput(&(m_seg->getFunctionalGroups()));
    FrameSorterIPP::Results results;
    sorter.sort(results);
    if (results.errorCode.bad())
    {
        DCMSEG_ERROR("groupFramesByPosition(): Cannot sort frames by position: " << results.errorCode.text());
        return results.errorCode;
    }
    // Copy results from frame sorter to overlap util framePositions member
    m_framePositions.clear();
    m_framePositions.reserve(results.framePositions.size());
    for (size_t i = 0; i < results.framePositions.size(); ++i)
    {
        m_framePositions.push_back(FramePositionAndNumber(results.framePositions[i], results.frameNumbers[i]));
    }
    cond = groupFramesByLogicalPosition();

    // print frame groups if debug log level is enabled:
    if (cond.good() && DCM_dcmsegLogger.isEnabledFor(OFLogger::DEBUG_LOG_LEVEL))
    {
        DCMSEG_DEBUG("groupFramesByPosition(): Frames grouped by position:");
        for (size_t i = 0; i < m_logicalFramePositions.size(); ++i)
        {
            OFStringStream ss;
            for (size_t j = 0; j < m_logicalFramePositions[i].size(); ++j)
            {
                if (j > 0)
                    ss << ", ";
                ss << m_logicalFramePositions[i][j];
            }
            DCMSEG_DEBUG("groupFramesByPosition(): Logical frame #" << i << ": " << ss.str());
        }
    }
    DCMSEG_DEBUG("groupFramesByPosition(): Grouping frames by position took " << tm.getDiff() << " s");

    if (cond.bad())
    {
        m_framePositions.clear();
        m_logicalFramePositions.clear();
    }
    return cond;
}

OFCondition OverlapUtil::getSegmentsByPosition(SegmentsByPosition& result)
{
    if (!m_seg)
    {
        DCMSEG_ERROR("getSegmentsByPosition(): No segmentation object set");
        return EC_IllegalCall;
    }
    if (!m_segmentsByPosition.empty())
    {
        // Already computed
        result = m_segmentsByPosition;
        return EC_Normal;
    }
    // Make sure prerequisites are met
    OFTimer tm;
    OFCondition cond = groupFramesByPosition();
    if (cond.bad())
    {
        return cond;
    }
    if (m_logicalFramePositions.empty())
    {
        cond = getFramesByPosition(m_logicalFramePositions);
        if (cond.bad())
            return cond;
    }
    m_segmentsByPosition.clear();
    m_segmentsByPosition.resize(m_logicalFramePositions.size());
    for (size_t l = 0; l < m_logicalFramePositions.size(); ++l)
    {
        for (size_t f = 0; f < m_logicalFramePositions[l].size(); ++f)
        {
            std::set<Uint32> segmentsInFrame;
            Uint32 frameNumber = m_logicalFramePositions[l][f];
            cond = getSegmentsForFrame(frameNumber, segmentsInFrame);
            if (cond.good())
            {
                for (std::set<Uint32>::iterator it = segmentsInFrame.begin();
                     it != segmentsInFrame.end();
                     ++it)
                {
                    m_segmentsByPosition[l].insert(SegNumAndFrameNum(*it, frameNumber));
                }
            }
            else
            {
                DCMSEG_ERROR("getSegmentsByPosition(): Cannot get segments for frame " << frameNumber);
                break;
            }
        }
        if (cond.bad())
        {
            break;
        }
    }
    // print segments per logical frame  if debug log level is enabled
    if (cond.good() && DCM_dcmsegLogger.isEnabledFor(OFLogger::DEBUG_LOG_LEVEL))
    {
        OFStringStream ss;
        printSegmentsByPosition(ss);
        DCMSEG_DEBUG(ss.str());
    }
    DCMSEG_DEBUG("getSegmentsByPosition(): Getting segments by position took " << tm.getDiff() << " s");
    return cond;
}

OFCondition OverlapUtil::getOverlapMatrix(OverlapMatrix& matrix)
{
    if (!m_seg)
    {
        DCMSEG_ERROR("getOverlapMatrix(): No segmentation object set");
        return EC_IllegalCall;
    }
    if (!m_segmentOverlapMatrix.empty())
    {
        // Already computed
        matrix = m_segmentOverlapMatrix;
        return EC_Normal;
    }
    // Make sure prerequisites are met
    OFTimer tm;
    SegmentsByPosition dontCare;
    OFCondition result = getSegmentsByPosition(dontCare);
    if (result.good())
    {
        result = buildOverlapMatrix();
    }
    if (result.good())
    {
        matrix = m_segmentOverlapMatrix;
    }
    DCMSEG_DEBUG("getOverlappingSegments(): Building overlap matrix took " << tm.getDiff() << " s");
    return result;
}

OFCondition OverlapUtil::getNonOverlappingSegments(SegmentGroups& segmentGroups)
{
    if (!m_seg)
    {
        DCMSEG_ERROR("getNonOverlappingSegments(): No segmentation object set");
        return EC_IllegalCall;
    }
    OFTimer tm;
    OFCondition result;
    if (!m_nonOverlappingSegments.empty())
    {
        // Already computed
        segmentGroups = m_nonOverlappingSegments;
        return EC_Normal;
    }
    // Make sure prerequisites are met
    result = getOverlapMatrix(m_segmentOverlapMatrix);
    if (result.good())
    {
        // Group those segments from the overlap matrix together, that do not
        // overlap with each other.
        // Go through all segments and check if they overlap with any of the already
        // grouped segments. If not, add them to the same group. If yes, create a new group
        // and add them there.
        m_nonOverlappingSegments.push_back(OFVector<Uint32>());
        for (OverlapMatrix::iterator matrixItr = m_segmentOverlapMatrix.begin();
             matrixItr != m_segmentOverlapMatrix.end();
             matrixItr++)
        {
            // Loop over all groups and check whether the current segment overlaps with any of them
            OFBool overlaps = OFFalse;
            for (size_t j = 0; j < m_nonOverlappingSegments.size(); ++j)
            {
                // Loop over all segments in the current group
                for (SegmentGroups::value_type::iterator it = m_nonOverlappingSegments[j].begin();
                     it != m_nonOverlappingSegments[j].end();
                     ++it)
                {
                    // Check if the current segment overlaps with the segment in the current group
                    if (m_segmentOverlapMatrix[matrixItr->first][*it] == 1)
                    {
                        overlaps = OFTrue;
                        break;
                    }
                }
                if (!overlaps)
                {
                    // Add segment to current group
                    m_nonOverlappingSegments[j].push_back(matrixItr->first);
                    break;
                }
            }
            if (overlaps)
            {
                // Create new group and add segment to it
                m_nonOverlappingSegments.push_back(OFVector<Uint32>());
                m_nonOverlappingSegments.back().push_back(matrixItr->first);
            }
        }
    }
    DCMSEG_DEBUG("getNonOverlappingSegments(): Grouping non-overlapping segments took " << tm.getDiff() << " s");
    if (result.good())
    {
        // print non-overlapping segments if debug log level is enabled
        if (DCM_dcmsegLogger.isEnabledFor(OFLogger::DEBUG_LOG_LEVEL))
        {
            OFStringStream ss;
            printNonOverlappingSegments(ss);
            DCMSEG_DEBUG(ss.str());
        }
    }
    if (result.good())
    {
        segmentGroups = m_nonOverlappingSegments;
    }
    return result;
}

void OverlapUtil::printSegmentsByPosition(OFStringStream& ss)
{
    ss << "printSegmentsByPosition(): Segments grouped by logical frame positions, (seg#,frame#):" << OFendl;
    for (size_t i = 0; i < m_segmentsByPosition.size(); ++i)
    {
        OFStringStream tempSS;
        for (SegmentsByPosition::value_type::iterator it = m_segmentsByPosition[i].begin();
             it != m_segmentsByPosition[i].end();
             ++it)
        {
            if (it != m_segmentsByPosition[i].begin())
                tempSS << ",";
            tempSS << "(" << (*it).m_segmentNumber << "," << (*it).m_frameNumber << ")";
        }
        ss << "printSegmentsByPosition(): Logical frame #" << i << ": " << tempSS.str();
    }
}

void OverlapUtil::printOverlapMatrix(OFStringStream& ss)
{
    ss << "printOverlapMatrix(): Overlap matrix:" << OFendl;
    for (size_t i = 0; i < m_segmentOverlapMatrix.size(); ++i)
    {
        for (size_t j = 0; j < m_segmentOverlapMatrix[i].size(); ++j)
        {
            if (m_segmentOverlapMatrix[i][j] >= 0)
                ss << OFstatic_cast(Uint32, m_segmentOverlapMatrix[i][j]);
            else
                ss << "1";
            ss << " ";
        }
        ss << OFendl;
    }
}

void OverlapUtil::printNonOverlappingSegments(OFStringStream& ss)
{
    ss << "printNonOverlappingSegments(): Non-overlapping segments:" << OFendl;
    for (size_t i = 0; i < m_nonOverlappingSegments.size(); ++i)
    {
        ss << "Group #" << i << ": ";
        for (SegmentGroups::value_type::iterator it = m_nonOverlappingSegments[i].begin();
             it != m_nonOverlappingSegments[i].end();
             ++it)
        {
            if (it != m_nonOverlappingSegments[i].begin())
                ss << ", ";
            ss << (*it);
        }
        ss << OFendl;
    }
}

OFCondition OverlapUtil::getSegmentsForLabelMapFrame(const Uint32 frameNumber, std::set<Uint32>& segments)
{
    const DcmIODTypes::FrameBase* frame = m_seg->getFrame(frameNumber);
    Uint16 rows, cols;
    rows = cols = 0;
    DcmIODImage<IODImagePixelModule<Uint8>>* ip = static_cast<DcmIODImage<IODImagePixelModule<Uint8>>*>(m_seg);
    ip->getImagePixel().getRows(rows);
    ip->getImagePixel().getColumns(cols);
    if (frame)
    {
        DCMSEG_ERROR("getSegmentsForLabelMapFrame(): Cannot access label map frame " << frameNumber);
        return EC_IllegalCall;
    }

    segments.clear();
    for (size_t n = 0; n < frame->length(); ++n)
    {
        Uint16 segmentNumber;
        frame->getUint16AtIndex(segmentNumber, n);
        segments.insert(segmentNumber);
    }
    return EC_Normal;
}

OFCondition OverlapUtil::getSegmentForFrame(const Uint32 frameNumber, Uint32& segment)
{
    FGBase* group         = NULL;
    FGSegmentation* segFG = NULL;
    FGInterface& fg       = m_seg->getFunctionalGroups();
    group                 = fg.get(frameNumber, DcmFGTypes::EFG_SEGMENTATION);
    segFG                 = OFstatic_cast(FGSegmentation*, group);
    if (segFG)
    {
        Uint16 segNum    = 0;
        OFCondition cond = segFG->getReferencedSegmentNumber(segNum);
        if (cond.good() && segNum > 0)
        {
            segment = segNum;
        }
        else if (segNum == 0)
        {
            DCMSEG_WARN("getSegmentForFrame(): Referenced Segment Number is 0 (not permitted) for frame #"
                        << frameNumber << ", ignoring");
            return EC_InvalidValue;
        }
        else
        {
            DCMSEG_ERROR(
                "getSegmentForFrame(): Referenced Segment Number not found (not permitted) for frame #"
                << frameNumber << ", cannot add segment");
            return EC_TagNotFound;
        }
    }
    return EC_Normal;
}


OFCondition OverlapUtil::buildOverlapMatrix()
{
    if (!m_seg)
    {
        DCMSEG_ERROR("getFramesForSegment(): No segmentation object set");
        return EC_IllegalCall;
    }

    std::set<Uint32> segmentNumbers;
    OFVector<DcmSegment*> segments;
    m_seg->getSegments(segments);
    for (int i = 0; i < segments.size(); i++)
    {
        segmentNumbers.insert(segments[i]->getSegmentNumber());
    }

    // Make 2 dimensional matrix of Sint8 type for (segment numbers) X (segment numbers).
    // Initialize with -1 (not checked yet)
    // Diagonal is always 0 (segment does not interfere/overlap with itself)
    for (std::set<Uint32>::iterator it1 = segmentNumbers.begin();
         it1 != segmentNumbers.end();
         it1++)
    {
        for (std::set<Uint32>::iterator it2 = segmentNumbers.begin();
             it2 != segmentNumbers.end();
             it2++)
        {
            if (*it1 == *it2)
            {
                m_segmentOverlapMatrix[*it1][*it2] = 0;
            }
            else
            {
                m_segmentOverlapMatrix[*it1][*it2] = -1;
            }
        }
    }

    // Go through all logical frame positions, and compare all segments at each position
    size_t index1, index2;
    index1 = index2 = 0;
    for (size_t i = 0; i < m_segmentsByPosition.size(); ++i)
    {
        DCMSEG_DEBUG("getOverlappingSegments(): Comparing segments at logical frame position " << i);
        // Compare all segments at this position
        for (SegmentsByPosition::value_type::iterator it = m_segmentsByPosition[i].begin();
             it != m_segmentsByPosition[i].end();
             ++it)
        {
            index1++;
            for (SegmentsByPosition::value_type::iterator it2 = m_segmentsByPosition[i].begin();
                 it2 != m_segmentsByPosition[i].end();
                 ++it2)
            {
                index2++;
                // Skip comparison of same segments in reverse order (index2 < index1)
                if (index2 <= index1)
                    continue;
                // Skip self-comparison (diagonal is always 0); (index1==index2)
                if (it->m_segmentNumber != it2->m_segmentNumber)
                {
                    // Check if we already have found an overlap on another logical frame, and if so, skip
                    Sint8 existing_result
                        = m_segmentOverlapMatrix[(*it).m_segmentNumber][(*it2).m_segmentNumber];
                    if (existing_result == 1)
                    {
                        DCMSEG_DEBUG("getOverlappingSegments(): Skipping frame comparison on pos #"
                                     << i << " for segments " << (*it).m_segmentNumber << " and "
                                     << (*it2).m_segmentNumber << " (already marked as overlapping)");
                        continue;
                    }
                    // Compare pixels of the frames referenced by each segments.
                    // If they overlap, mark as overlapping
                    OFCondition cond;
                    OFBool overlap = OFFalse;
                    if (m_seg->getSegmentationType() == (DcmSegTypes::E_SegmentationType) 3) // LABELMAP
                    {
                        cond = checkFramesOverlapLabelMap(*it, *it2, overlap);
                    }
                    else
                    {
                        cond = checkFramesOverlap(it->m_frameNumber, it2->m_frameNumber, overlap);
                    }
                    if (cond.bad())
                    {
                        DCMSEG_ERROR("getOverlappingSegments(): Error checking overlap of frames "
                                    << it->m_frameNumber << " and " << it2->m_frameNumber);
                        return cond;
                    }

                    // Enter result into overlap matrix
                    m_segmentOverlapMatrix[(*it).m_segmentNumber][(*it2).m_segmentNumber] = overlap ? 1 : 0;
                    m_segmentOverlapMatrix[(*it2).m_segmentNumber][(*it).m_segmentNumber] = overlap ? 1 : 0;
                }
            }
        }
    }
    // Since we don't compare all segments (since not all are showing up together on a single logical frame),
    // we set all remaining entries that are still not initialized (-1) to 0 (no overlap)
    for (std::set<Uint32>::iterator it1 = segmentNumbers.begin();
         it1 != segmentNumbers.end();
         it1++)
    {
        for (std::set<Uint32>::iterator it2 = segmentNumbers.begin();
             it2 != segmentNumbers.end();
             it2++)
        {
            if (m_segmentOverlapMatrix[*it1][*it2] == -1)
            {
                m_segmentOverlapMatrix[*it1][*it2] = 0;
            }
        }
    }
    // print overlap matrix if debug log level is enabled
    if (DCM_dcmsegLogger.isEnabledFor(OFLogger::DEBUG_LOG_LEVEL))
    {
        OFStringStream ss;
        printOverlapMatrix(ss);
        DCMSEG_DEBUG(ss.str());
    }
    return EC_Normal;
}

OFCondition OverlapUtil::checkFramesOverlapLabelMap(const SegNumAndFrameNum& sf1,
                                                    const SegNumAndFrameNum& sf2,
                                                    OFBool& overlap)
{
    if (sf1.m_frameNumber == sf2.m_frameNumber || sf1.m_segmentNumber == sf2.m_segmentNumber)
    {
        // The same frame or segment should not be considered overlapping
        overlap = OFFalse;
        return EC_Normal;
    }

    overlap = OFFalse;
    const Uint32& f1 = sf1.m_frameNumber;
    const Uint32& f2 = sf2.m_frameNumber;
    OFCondition result;
    const DcmIODTypes::FrameBase* f1_data = m_seg->getFrame(sf1.m_frameNumber);
    const DcmIODTypes::FrameBase* f2_data = m_seg->getFrame(sf2.m_frameNumber);
    Uint16 rows, cols;
    rows = cols = 0;
    DcmIODImage<IODImagePixelModule<Uint8>>* ip = static_cast<DcmIODImage<IODImagePixelModule<Uint8>>*>(m_seg);
    ip->getImagePixel().getRows(rows);
    ip->getImagePixel().getColumns(cols);

    DCMSEG_DEBUG("checkFramesOverlapLabelMap(): Comparing frame " << f1 << ", segment " << sf1.m_segmentNumber << ", and frame "
                                                                  << f2 << ", segment " << sf2.m_segmentNumber 
                                                                  << ", for overlap");
    if (!f1_data || !f2_data)
    {
        DCMSEG_ERROR("checkFramesOverlapLabelMap(): Cannot access label map frames " << f1 << " and " << f2 << " for comparison");
        return EC_IllegalCall;
    }
    if (f1_data->length() != f2_data->length())
    {
        DCMSEG_ERROR("checkFramesOverlapLabelMap(): Frames " << f1 << " and " << f2
                                                     << " have different length, cannot compare");
        return EC_IllegalCall;
    }

    for (size_t n = 0; n < f1_data->length(); ++n)
    {
        Uint16 f1Pixel, f2Pixel;
        f1_data->getUint16AtIndex(f1Pixel, n);
        f2_data->getUint16AtIndex(f2Pixel, n);
        if (f1Pixel == sf1.m_segmentNumber && f2Pixel == sf2.m_segmentNumber)
        {
            DCMSEG_DEBUG("checkFramesOverlapLabelMap(): Frame " << f1 << ", segment " << sf1.m_segmentNumber << ", and frame "
                                            << f2 << ", segment " << sf2.m_segmentNumber << ", do overlap at index " << n);
            overlap = OFTrue;
            break;
        }
    }
    return EC_Normal;
}

OFCondition OverlapUtil::checkFramesOverlap(const Uint32& f1, const Uint32& f2, OFBool& overlap)
{
    if (f1 == f2)
    {
        // The same frame should not be considered overlapping at all
        overlap = OFFalse;
        return EC_Normal;
    }
    overlap = OFFalse;
    OFCondition result;
    const DcmIODTypes::FrameBase* f1_data = m_seg->getFrame(f1);
    const DcmIODTypes::FrameBase* f2_data = m_seg->getFrame(f2);
    Uint16 rows, cols;
    rows = cols = 0;
    DcmIODImage<IODImagePixelModule<Uint8>>* ip = static_cast<DcmIODImage<IODImagePixelModule<Uint8>>*>(m_seg);
    ip->getImagePixel().getRows(rows);
    ip->getImagePixel().getColumns(cols);
    if (rows * cols % 8 != 0)
    {
        // We must compare pixel by pixel of the unpacked frames (for now)
        result = checkFramesOverlapUnpacked(f1, f2, dynamic_cast<DcmIODTypes::Frame<Uint8>*>(f1_data),
                        dynamic_cast<DcmIODTypes::Frame<Uint8>*>(f2_data), rows, cols, overlap);
    }
    else
    {
        // We can compare byte by byte using bitwise AND (if both have a 1 at the same position, they overlap)
        result = checkFramesOverlapBinary(f1, f2, dynamic_cast<DcmIODTypes::Frame<Uint8>*>(f1_data),
                        dynamic_cast<DcmIODTypes::Frame<Uint8>*>(f2_data), rows, cols, overlap);
    }
    if (result.good() && !overlap)
    {
        DCMSEG_DEBUG("checkFramesOverlap(): Frames " << f1 << " and " << f2 << " don't overlap");
    }
    return result;
}

OFCondition OverlapUtil::checkFramesOverlapBinary(const Uint32& f1,
                                                  const Uint32& f2,
                                                  const DcmIODTypes::Frame<Uint8>* f1_data,
                                                  const DcmIODTypes::Frame<Uint8>* f2_data,
                                                  const Uint16& rows,
                                                  const Uint16 cols,
                                                  OFBool& overlap)
{
    DCMSEG_DEBUG("checkFramesOverlap(): Comparing frames " << f1 << " and " << f2 << " for overlap (fast binary mode)");
    if (!f1_data || !f2_data)
    {
        DCMSEG_ERROR("checkFramesOverlap(): Cannot access binary frames " << f1 << " and " << f2 << " for comparison");
        return EC_IllegalCall;
    }
    if (f1_data->length() != f2_data->length())
    {
        DCMSEG_ERROR("checkFramesOverlap(): Frames " << f1 << " and " << f2
                                                     << " have different length, cannot compare");
        return EC_IllegalCall;
    }
    // Compare byte (8 pixels at once) using bitwise AND (if both have a 1 at the same position, they overlap)
    Uint8 *pixelData1 = f1_data->getPixelDataTyped();
    Uint8 *pixelData2 = f2_data->getPixelDataTyped();
    for (size_t n = 0; n < f1_data->length(); ++n)
    {
        if (pixelData1[n] & pixelData2[n])
        {
            DCMSEG_DEBUG("checkFramesOverlap(): Frames " << f1 << " and " << f2 << " do overlap, pixel value "
                                                         << OFstatic_cast(Uint16, f1_data->pixData[n]) << " at index "
                                                         << n << " is the same");
            overlap = OFTrue;
            break;
        }
    }
    return EC_Normal;
}

OFCondition OverlapUtil::checkFramesOverlapUnpacked(const Uint32& f1,
                                                    const Uint32& f2,
                                                    const DcmIODTypes::Frame<Uint8>* f1_data,
                                                    const DcmIODTypes::Frame<Uint8>* f2_data,
                                                    const Uint16& rows,
                                                    const Uint16 cols,
                                                    OFBool& overlap)
{
    DCMSEG_DEBUG("checkFramesOverlap(): Comparing frames " << f1 << " and " << f2
                                                           << " for overlap (slow unpacked mode)");
    OFunique_ptr<DcmIODTypes::Frame<Uint8>> f1_unpacked(DcmSegUtils::unpackBinaryFrame(f1_data, rows, cols));
    OFunique_ptr<DcmIODTypes::Frame<Uint8>> f2_unpacked(DcmSegUtils::unpackBinaryFrame(f2_data, rows, cols));
    if (!f1_unpacked || !f2_unpacked)
    {
        DCMSEG_ERROR("checkFramesOverlap(): Cannot unpack frames " << f1 << " and " << f2 << " for comparison");
        return EC_IllegalCall;
    }
    if (f1_unpacked->length() != f2_unpacked->length())
    {
        DCMSEG_ERROR("checkFramesOverlap(): Frames " << f1 << " and " << f2
                                                     << " have different length, cannot compare");
        return EC_IllegalCall;
    }
    // Compare pixels of both frames and check whether at least one has the same value
    DCMSEG_DEBUG("checkFramesOverlap(): Comparing frames " << f1 << " and " << f2 << " for overlap");
    Uint8 *pixelDataUnpacked1 = f1_unpacked->getPixelDataTyped();
    Uint8 *pixelDataUnpacked2 = f2_unpacked->getPixelDataTyped();
    for (size_t n = 0; n < f1_unpacked->length(); ++n)
    {
        if (pixelDataUnpacked1[n] != 0 && (pixelDataUnpacked1[n] == pixelDataUnpacked2[n]))
        {
            DCMSEG_DEBUG("checkFramesOverlap(): Frames " << f1 << " and " << f2 << " do overlap, pixel value "
                                                         << OFstatic_cast(Uint16, f1_unpacked->pixData[n])
                                                         << " at index " << n << " is the same");
            overlap = OFTrue;
            break;
        }
    }
    return EC_Normal;
}


OFCondition OverlapUtil::groupFramesByLogicalPosition()
{
    OFCondition cond;
    FGInterface& fg = m_seg->getFunctionalGroups();
    OFBool perFrame = OFFalse;
    Float64 sliceThickness   = 0.0;
    FGPixelMeasures* pm      = NULL;
    FGBase* group            = fg.get(0, DcmFGTypes::EFG_PIXELMEASURES, perFrame);
    if (group && (pm = OFstatic_cast(FGPixelMeasures*, group)))
    {
        // Get/compute Slice Thickness
        cond = pm->getSliceThickness(sliceThickness);
        if (cond.bad())
        {
            DCMSEG_ERROR("groupFramesByPosition(): Cannot get Slice Thickness from Pixel Measures FG: "
                         << cond.text());
            return cond;
        }
    }

    Uint8 relevantCoordinate = identifyChangingCoordinate(m_imageOrientation);

    // vec will contain all frame numbers that are at the same position
    OFVector<Uint32> vec;
    vec.push_back(m_framePositions[0].m_frameNumber);
    m_logicalFramePositions.push_back(vec); // Initialize for first logical frame
    for (size_t j = 1; j < m_framePositions.size(); ++j)
    {
        // If frame is close to previous frame, add it to the same vector.
        // 2.5 is chosen since it means the frames are not further away if clearly less than half a slice
        // thickness
        Float64 diff = fabs(m_framePositions[j].m_position[relevantCoordinate]
                            - m_framePositions[j - 1].m_position[relevantCoordinate]);
        DCMSEG_DEBUG("Coordinates of both frames:");
        DCMSEG_DEBUG("Frame " << j << ": " << m_framePositions[j].m_position[0] << ", "
                                << m_framePositions[j].m_position[1] << ", "
                                << m_framePositions[j].m_position[2]);
        DCMSEG_DEBUG("Frame " << j - 1 << ": " << m_framePositions[j - 1].m_position[0] << ", "
                                << m_framePositions[j - 1].m_position[1] << ", "
                                << m_framePositions[j - 1].m_position[2]);
        DCMSEG_DEBUG("groupFramesByPosition(): Frame " << j << " is " << diff
                                                        << " mm away from previous frame");
        // 1% inaccuracy for slice thickness will be considered as same logical position
        if (diff < sliceThickness * 0.01)
        {
            // Add frame to last vector
            DCMSEG_DEBUG("Assigning to same frame bucket as previous frame");
            m_logicalFramePositions.back().push_back(
                m_framePositions[j].m_frameNumber); // physical frame number
        }
        else
        {
            DCMSEG_DEBUG("Assigning to same new frame bucket");
            // Create new vector
            OFVector<Uint32> vec;
            vec.push_back(m_framePositions[j].m_frameNumber);
            m_logicalFramePositions.push_back(vec);
        }
    }

    return cond;
}

Uint8 OverlapUtil::identifyChangingCoordinate(const OFVector<Float64>& imageOrientation)
{
    Float64 cross_product[3];
    // Compute cross product of image orientation vectors.
    // We are only interested into the absolute values for later comparison
    cross_product[0] = fabs(imageOrientation[1] * imageOrientation[5] - imageOrientation[2] * imageOrientation[4]);
    cross_product[1] = fabs(imageOrientation[2] * imageOrientation[3] - imageOrientation[0] * imageOrientation[5]);
    cross_product[2] = fabs(imageOrientation[0] * imageOrientation[4] - imageOrientation[1] * imageOrientation[3]);
    // Find out which coordinate is changing the most (biggest absolute coordinate value of cross product)
    if ((cross_product[0] > cross_product[1]) && (cross_product[0] > cross_product[2]))
    {
        return 0;
    }
    if ((cross_product[1] > cross_product[0]) && (cross_product[1] > cross_product[2]))
    {
        return 1;
    }
    if ((cross_product[2] > cross_product[0]) && (cross_product[2] > cross_product[1]))
    {
        return 2;
    }
    // No clear winner
    return 3;
}

} // namespace dcmqi
