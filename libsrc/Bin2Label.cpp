/*
 *  Copyright (C) 2015-2025, Open Connections GmbH
 *
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation are maintained by
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
 *  Purpose: Class for converting binary to label map segmentations
 *
 */


#include "dcmtk/config/osconfig.h" // include OS configuration first
#include "dcmqi/Bin2Label.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmfg/fgfact.h"
#include "dcmtk/dcmfg/fgfracon.h"
#include "dcmtk/dcmfg/fgderimg.h"
#include "dcmtk/dcmseg/segtypes.h"
#include "dcmtk/dcmiod/cielabutil.h"
#include <zconf.h>

namespace dcmqi
{
DcmBinToLabelConverter::DcmBinToLabelConverter()
    : m_loadFlags()
    , m_convFlags()
    , m_inputDataset(OFnullptr)
    , m_inputDatasetOwned(OFFalse)
    , m_inputFileName()
    , m_inputSeg(OFnullptr)
    , m_inputXfer(E_TransferSyntax::EXS_Unknown)
    , m_outputSeg(OFnullptr)
    , m_use16Bit(OFFalse)
    , m_overlapUtil()
    , m_cielabColors()
{
}


void DcmBinToLabelConverter::setInput(DcmSegmentation* inputSeg)
{
   clear();
   m_inputSeg = inputSeg;
}


void DcmBinToLabelConverter::setInput(DcmDataset* inputDataset, const DcmSegmentation::LoadingFlags& loadFlags)
{
    clear();
    m_inputDataset = inputDataset;
    m_loadFlags = loadFlags;
}


void DcmBinToLabelConverter::setInput(OFFilename filename, const DcmSegmentation::LoadingFlags& loadFlags)
{
    clear();
    m_inputFileName = filename;
    m_loadFlags = loadFlags;
}


void DcmBinToLabelConverter::clear()
{
    if (m_inputDatasetOwned && m_inputDataset)
    {
        delete m_inputDataset;
        m_inputDataset = NULL;
    }
    if (m_inputSeg)
    {
        delete m_inputSeg;
        m_inputSeg = NULL;
    }
    m_inputSeg = NULL;
    m_inputDataset = NULL;
    m_inputDatasetOwned = OFFalse;
    m_inputFileName.clear();
    m_loadFlags.clear();
    m_convFlags.clear();
    m_outputSeg.reset();
    m_use16Bit = OFFalse;
    m_overlapUtil.clear();
    m_cielabColors.clear();
}


OFCondition DcmBinToLabelConverter::convert(const ConversionFlags& convFlags)
{
    // Check whether input is set appropriately; loads input segmentation (if necessary)
    // and checks whether its a binary segmentation object
    m_convFlags = convFlags;
    OFCondition result = loadInput();
    if (result.bad())
    {
        clear();
        return result;
    }

    // Check whether user wants to convert to PALETTE color model. For now we
    // rely on Recommended Display CIELab Value Macro to be present in the segments;
    // if they are not found, random colors are generated if forcePalette is set.
    // The call only fails (if not on hard errors) if forcePalette is not set and CIELab
    // colors are missing.
    if ( (m_convFlags.m_outputColorModel == DcmSegTypes::SLCM_PALETTE) && !checkCIELabColorsPresent() )
    {
        DCMSEG_ERROR("Cannot convert to PALETTE color model since not all segments contain Recommended Display CIELab Value Macro");
        return SG_EC_CannotConvertMissingCIELab;
    }

    // Check for overlaps which would prevent conversion
    m_overlapUtil.setSegmentationObject(m_inputSeg);
    if (m_overlapUtil.hasOverlappingSegments())
    {
        return SG_EC_OverlappingSegments;
    }
    // Get number of segments to find out whether we need 16 bit data.
    size_t numSegments = m_inputSeg->getNumberOfSegments();
    m_use16Bit    = (numSegments > 256);
    DCMSEG_DEBUG("Using " << (m_use16Bit ? "16" : "8") << " bit pixel data for " << numSegments << " segments");

    // Get Content Identification Macro (required for labelmap creation call)
    ContentIdentificationMacro& content = m_inputSeg->getContentIdentification();

    // Create Labelmap
    if (result.good())
    {
        DcmSegmentation* temp = NULL;
        DCMSEG_DEBUG("Creating labelmap output segmentation object");
        result
            = DcmSegmentation::createLabelmapSegmentation(temp,
                                                            m_inputSeg->getRows(),
                                                            m_inputSeg->getColumns(),
                                                            m_inputSeg->getEquipment().getEquipmentInfo(),
                                                            content,
                                                            m_use16Bit,
                                                            m_convFlags.m_outputColorModel);
        // Remember output segmentation in converter but also return in parameter
        m_outputSeg.reset(temp);
    }
    // Copy all common information (patient, study, series and some instance level data)
    if (result.good())
    {
        DCMSEG_DEBUG("Copying common modules from input to output segmentation");
        result = copyCommonModules(m_inputSeg, m_outputSeg.get());
        if (result.good())
        {
            // Generate new Series Instance UID and SOP Instance UID for output segmentation
            char sop[100];
            char series[100];
            // Generate new Series Instance UID
            dcmGenerateUniqueIdentifier(series, SITE_SERIES_UID_ROOT);
            m_outputSeg->getSeries().setSeriesInstanceUID(series);
            // Generate new SOP Instance UID
            dcmGenerateUniqueIdentifier(sop, SITE_INSTANCE_UID_ROOT);
            m_outputSeg->getSOPCommon().setSOPInstanceUID(sop);
        }
    }
    // Multi-frame Dimension Module, re-create:
    // Two artificial dimensions based on Stack ID and In Stack Position Number
    if (result.good())
    {
        // Create new Dimension UID
        char uid[100];
        dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT);
        result = m_outputSeg->getDimensions().addDimensionIndex(DCM_StackID, uid, DCM_FrameContentSequence, "Stack ID");
        if (result.good()) result = m_outputSeg->getDimensions().addDimensionIndex(DCM_InStackPositionNumber, uid, DCM_FrameContentSequence, "In Stack Position Number");
    }

    // Copy Shared Functional Groups
    if (result.good())
    {
        DCMSEG_DEBUG("Copying shared Functional Groups from input to output segmentation");
        const FunctionalGroups* sharedFGs = m_inputSeg->getFunctionalGroups().getShared();
        FunctionalGroups::const_iterator it = sharedFGs->begin();
        while (result.good() && it != sharedFGs->end())
        {
            FGBase* sharedDest = FGFactory::instance().create(it->second->getType());
            if (sharedDest)
            {
                result = copyComponent(it->second, sharedDest);
                if (result.good())
                {
                    result = m_outputSeg->getFunctionalGroups().addShared(*sharedDest);
                }
            }
            delete sharedDest;
            ++it;
        }
    }

    // TODO: Do we need parts of Multi-Frame FG Module?
    // - What about Content Date/Time?

    // Copy Segments
    if (result.good())
    {
        DCMSEG_DEBUG("Copying segments from input to output segmentation");
        result = copySegments(m_inputSeg, m_outputSeg.get());
    }
    // Copy pixel data and per-frame functional groups (excluding some that cannot be migrated)
    if (result.good())
    {
        DCMSEG_DEBUG("Copying per-frame information (pixel data and FGs) to output segmentation");
        result = createFramesWithMetadata(m_inputSeg);
    }

    // Create palette color lookup table if necessary
    if (result.good() && (m_convFlags.m_outputColorModel == DcmSegTypes::SLCM_PALETTE))
    {
        result = createPaletteColorLUT();
    }

    // Add Derivation Image Functional Group with Source Image Item pointing to source segmentation
    if (result.good())
    {
        DCMSEG_DEBUG("Adding Derivation Image Functional Group with Source Image Item pointing to source segmentation");
        addSourceSegmentationToDerivationImageFG(m_inputSeg, m_outputSeg.get());
    }

    return result;
}

OFCondition DcmBinToLabelConverter::copySegments(DcmSegmentation* src, DcmSegmentation* dest)
{
    OFCondition result;
    DCMSEG_DEBUG("Copying segments from source to destination");
    // Iterate over all segments in the source segmentation
    OFMap<Uint16, DcmSegment*>::const_iterator srcSegment = src->getSegments().begin();
    while (srcSegment != src->getSegments().end() && result.good())
    {
        // Get segment from source segmentation
        // Note that segment number is not relevant for labelmaps, so we don't pass it to addSegment
        // (it will be assigned automatically in addSegment)
        DCMSEG_DEBUG("Copying segment number " << srcSegment->first);
        if (srcSegment->second)
        {
            // Clone the segment and add it to the destination segmentation
            DcmSegment* clonedSegment = srcSegment->second->clone(dest);
            if (clonedSegment)
            {
                // If we write palette color model, we need to make sure that the
                // Recommended Display CIELab Value Macro will not be written
                // for each segment, as this is not allowed in labelmaps with PALETTE color model.
                if (m_convFlags.m_outputColorModel == DcmSegTypes::SLCM_PALETTE)
                {
                    clonedSegment->getIODRules()->deleteRule(DCM_RecommendedDisplayCIELabValue);
                }
                Uint16 segNumber = srcSegment->first;
                result = dest->addSegment(clonedSegment, segNumber);
                if (result.bad())
                {
                    DCMSEG_ERROR("Failed to add cloned segment to destination");
                    delete clonedSegment;
                    break;
                }
            }
            else
            {
                DCMSEG_ERROR("Failed to clone source segment");
                result = EC_MemoryExhausted;
                break;
            }
        }
        srcSegment++;
    }
    return result;
}

DcmBinToLabelConverter::~DcmBinToLabelConverter()
{
    clear();
    DCMSEG_DEBUG("Destroying DcmBinToLabelConverter");
    // Nothing to do here, no dynamic memory allocated
}

OFCondition DcmBinToLabelConverter::checkSOPClassAndSegtype(const OFString& sopClassUID,
                                                            const OFString& segType)
{
    DCMSEG_DEBUG("Checking SOP Class and Segmentation Type");
    // Check if the SOP Class UID is correct
    OFCondition result;
    if ((sopClassUID != UID_SegmentationStorage) && (sopClassUID != UID_LabelMapSegmentationStorage))
    {
        return SG_EC_NoSegmentationBasedSOPClass;
    }
    else if (sopClassUID == UID_LabelMapSegmentationStorage)
    {
        if (m_convFlags.m_errorIfAlreadyLabelMap)
        {
            return SG_EC_AlreadyLabelMap;
        }
        else
        {
            result = SG_EC_NoConversionRequired;
        }
    }
    else if (sopClassUID == UID_SegmentationStorage)
    {
        result = EC_Normal;
    }
    else
    {
        return SG_EC_NoSegmentationBasedSOPClass;
    }

    // Check if the Segmentation Type is valid
    DCMSEG_DEBUG("SOP Class acceptable for conversion to labelmap, checking Segmentation Type");
    if (DcmSegTypes::OFString2Segtype(segType) == DcmSegTypes::ST_UNKNOWN)
    {
        DCMSEG_ERROR("Segmentation Type is unknown");
        return IOD_EC_InvalidObject;
    }

    // check whether sop class and segmentation type match
    if (sopClassUID == UID_LabelMapSegmentationStorage
        && DcmSegTypes::OFString2Segtype(segType) == DcmSegTypes::ST_LABELMAP)
    {
        // result is already set to SG_EC_NoConversionRequired
        DCMSEG_DEBUG("Segmentation object for conversion is a label map");
    }
    else if (sopClassUID == UID_SegmentationStorage && DcmSegTypes::OFString2Segtype(segType) == DcmSegTypes::ST_BINARY)
    {
        DCMSEG_DEBUG("Segmentation object for conversion is a binary segmentation");
    }
    else if (sopClassUID == UID_SegmentationStorage
             && DcmSegTypes::OFString2Segtype(segType) == DcmSegTypes::ST_FRACTIONAL)
    {
        DCMSEG_DEBUG("Segmentation object for conversion is a fractional segmentation");
        result = SG_EC_CannotConvertFractionalToLabelmap;
    }
    else
    {
        DCMSEG_ERROR("SOP Class UID " << sopClassUID << " does not match Segmentation Type " << segType);
        result = IOD_EC_InvalidObject;
    }
    return result;
}

template <typename T>
OFCondition DcmBinToLabelConverter::copyComponent(T* src, T* dest)
{
    OFCondition result;
    if ((src && dest) && (src != dest))
    {
        DcmItem item;
        DCMSEG_DEBUG("Writing component into temporary item");
        result = src->write(item);
        if (result.good())
        {

            DCMSEG_DEBUG("Reading temporary item component into destination");
            result = dest->read(item);
        }
    }
    else
    {
        result = EC_IllegalParameter;
    }
    return result;
}

OFCondition DcmBinToLabelConverter::copyCommonModules(DcmSegmentation* src, DcmSegmentation* dest)
{
    OFCondition result;

    if (src && dest)
    {
        // Copy all components except pixel data:

        // Start with modules from IODImage:
        // Patient Module, General Study Module, General Equipment Module,
        // General Series Module, Frame of Reference Module.
        // This skips Image Pixel Module and SOP Common
        DCMSEG_DEBUG("Copying Patient Module from input to output segmentation");
        result = copyComponent(&(src->getPatient()), &dest->getPatient());
        if (result.good())
        {
            DCMSEG_DEBUG("Copying General Study Module from input to output segmentation");
            result = copyComponent(&(src->getStudy()), &dest->getStudy());
        }
        if (result.good())
        {
            DCMSEG_DEBUG("Copying General Equipment Module from input to output segmentation");
            result = copyComponent(&(src->getEquipment()), &dest->getEquipment());
        }
        if (result.good())
        {
            // TODO fix series instance UID and Series Number?
            DCMSEG_DEBUG("Copying General Series Module from input to output segmentation");
            result = copyComponent(&(src->getSeries()), &dest->getSeries());
        }
        if (result.good())
        {
            DCMSEG_DEBUG("Copying Segmentation Series Module from input to output segmentation");
            result = copyComponent(&(src->getSegmentationSeriesModule()), &dest->getSegmentationSeriesModule());
        }
        if (result.good())
        {
            DCMSEG_DEBUG("Copying Frame of Reference Module from input to output segmentation");
            result = copyComponent(&(src->getFrameOfReference()), &dest->getFrameOfReference());
        }
        if (result.good())
        {
            DCMSEG_DEBUG("Copying General Image Module from input to output segmentation");
            result = copyComponent(&(src->getGeneralImage()), &dest->getGeneralImage());
        }
        if (result.bad())
            return result;

        // Copy Common Instance Reference Module
        // TODO: Shall we copy Common Instance Reference module with its original references or leave it empty?
        if (result.good())
        {
            DCMSEG_DEBUG("Copying Common Instance Reference Module from input to output segmentation");
            result = copyComponent(&(src->getCommonInstanceReference()), &dest->getCommonInstanceReference());
        }

        // Skipping:
        // - Palette Color LUT Module (not set in binary segmentations)
        // - Segmentation Image Module (rewritten for labelmaps)
        // - Multi-Frame Dimension Module (probably invalid)

        // Set Instance Number (General Image / Multi-Frame FG Module)
        if (result.good())
        {
            result = dest->getGeneralImage().setInstanceNumber("1");
        }
        if (result.bad())
            return result;
    }
    else
    {
        result = EC_IllegalParameter;
    }
    return result;
}

OFCondition DcmBinToLabelConverter::createFramesWithMetadata(DcmSegmentation* src)
{
    OFCondition result;

    if (src)
    {
        // Walk through segments, and for each segments, get all the related frames
        // and construct a new destination frame if we don't have a corresponding one
        // at the same position in space. So input frames at the same position will
        // result in a new destination frame being created.
        OverlapUtil::DistinctFramePositions framesAtPositions;
        result = m_overlapUtil.getFramesByPosition(framesAtPositions);
        Uint32 outputFrameNum = 1; // for log output
        if (result.good())
        {
            // Iterate over all positions, and create a new destination frame
            DCMSEG_DEBUG("Creating new destination frames for each input frame position");
            OFVector<OverlapUtil::LogicalFrame>::iterator it = framesAtPositions.begin();
            while (result.good()&& (it != framesAtPositions.end()))
            {
                // Create per-frame functional groups.
                // Re-use Plane Position (Patient) FG from first frame at this position.
                // Create Frame Content FG for the frame
                FGBase* planePos = src->getFunctionalGroups().get(it->at(0), DcmFGTypes::EFG_PLANEPOSPATIENT);
                if (!planePos)
                {
                    DCMSEG_DEBUG("No Plane Position (Patient) FG found for frame #" << it->at(0));
                    result = SG_EC_MissingPlanePositionPatient;
                    break;
                }
                // Create Frame Content FG for the frame
                FGFrameContent* frameContent = OFnullptr;
                result = createFrameContentFG(outputFrameNum, it, frameContent);
                if (result.bad()) break;

                OFVector<FGBase*> perFrameInfo;
                if (planePos) perFrameInfo.push_back(planePos);
                // addFrame() will copy functional groups. Memory is still handled by source object, so
                // no need to delete them.
                if (m_use16Bit)
                {
                    DCMSEG_DEBUG("Creating new 16 bit destination frame #" << outputFrameNum << "/" << framesAtPositions.size());
                    // create a new destination frame
                    Uint16* newFrame = new Uint16[src->getRows() * src->getColumns()];
                    if (newFrame)
                    {
                        // Initialize new frame with zeros
                        memset(newFrame, 0, src->getRows() * src->getColumns() * sizeof(Uint16));
                        result = setPixelDataForFrame(src, outputFrameNum-1 /* aka current position */, newFrame, src->getRows() * src->getColumns());
                        if (result.good())
                        {
                            result = m_outputSeg->addFrame(newFrame, 0 /* ignored for labelmaps */, perFrameInfo);
                        }
                        delete[] newFrame;
                    }
                    else result = EC_MemoryExhausted;
                }
                else // 8 bit
                {
                    DCMSEG_DEBUG("Creating new 8 bit destination frame #" << outputFrameNum << "/" << framesAtPositions.size());
                    Uint8* newFrame = new Uint8[src->getRows() * src->getColumns()];
                    if (newFrame)
                    {
                        // Initialize new frame with zeros
                        memset(newFrame, 0, src->getRows() * src->getColumns() * sizeof(Uint8));
                        result = setPixelDataForFrame(src, outputFrameNum-1 /* aka current position */, newFrame, src->getRows() * src->getColumns());
                        if (result.good())
                        {
                            result = m_outputSeg->addFrame(newFrame, 0 /* ignored for labelmaps */, perFrameInfo);
                        }
                        delete[] newFrame;
                    }
                    else result = EC_MemoryExhausted;
                }
                it++;
                outputFrameNum++;
            }
        }
    }
    else
    {
        result = EC_IllegalParameter;
    }
    return result;
}


E_TransferSyntax DcmBinToLabelConverter::getInputTransferSyntax() const
{
    return m_inputXfer;
}


OFBool DcmBinToLabelConverter::checkCIELabColorsPresent()
{
    size_t numSegments = m_inputSeg->getNumberOfSegments();
    OFBool result = OFTrue;
    OFCondition cond;
    if (m_convFlags.m_outputColorModel == DcmSegTypes::SLCM_PALETTE)
    {
        if (!m_cielabColors.resize(numSegments))
        {
            DCMSEG_ERROR("Cannot allocate memory for CIELab colors of " << numSegments << " segments");
            return OFFalse;
        }

        // Check whether all segments have Recommended Display CIELab Value Macro
        size_t idx = 0;
        OFMap<Uint16, DcmSegment*>::const_iterator segIt = m_inputSeg->getSegments().begin();
        while (segIt != m_inputSeg->getSegments().end())
        {
            if (segIt->second)
            {
                Uint16 L,a,b;
                L = 0; a = 0; b = 0;
                cond = segIt->second->getRecommendedDisplayCIELabValue(L,a,b);
                if (cond.good())
                {
                    DCMSEG_DEBUG("Segment #" << segIt->first << " has CIELab color: L=" << L << ", a=" << a << ", b=" << b);
                    m_cielabColors.m_L[idx] = L;
                    m_cielabColors.m_a[idx] = a;
                    m_cielabColors.m_b[idx] = b;
                }
                else
                {
                    if (m_convFlags.m_forcePalette)
                    {
                        // Create random color, L, a and b are still in DICOM range 0..65535
                        L = OFstatic_cast(Uint16, (rand() % 65535) + 1);
                        a = OFstatic_cast(Uint16, (rand() % 65535) + 1);
                        b = OFstatic_cast(Uint16, (rand() % 65535) + 1);
                        m_cielabColors.m_L[idx] = L;
                        m_cielabColors.m_a[idx] = a;
                        m_cielabColors.m_b[idx] = b;
                        DCMSEG_DEBUG("Segment #" << segIt->first << " has no CIELab color, using random color: L=" << L << ", a=" << a << ", b=" << b);
                    }
                    else
                    {
                        DCMSEG_ERROR("No Display Recommended Display CIELab Value in segment #" << segIt->first);
                        result = OFFalse;
                        m_cielabColors.clear();
                        break;
                    }
                }
            }
            segIt++;
            idx++;
        }
    }
    return result;
}


OFCondition DcmBinToLabelConverter::loadInput()
{
    OFCondition result;
    // Check whether we have a segmentation object as input
    if (m_inputSeg == OFnullptr)
    {
        // If not, check if we have a dataset to load from
        if (!m_inputDataset)
        {
            // If not, check if we can load it from file
            if (m_inputFileName.isEmpty())
            {
                return EC_InvalidFilename;
            }
            else
            {
                // Load file into dataset
                DCMSEG_DEBUG("Loading potential segmentation file into a dataset");
                DcmFileFormat dcmff;
                result = dcmff.loadFile(m_inputFileName);
                if (result.good())
                {
                    // make sure dataset pointer is not freed by DcmFileFormat
                    m_inputDataset = dcmff.getAndRemoveDataset();
                    m_inputDatasetOwned = OFTrue;
                }
                else
                {
                    return result;
                }
            }
        }
        // At this point we have an input dataset, load it into segmentation
        DCMSEG_DEBUG("Loading dataset into DcmSegmentation object");
        DcmSegmentation *loaded = OFnullptr;
        result = DcmSegmentation::loadDataset(*m_inputDataset, loaded, m_loadFlags);
        if (result.good())
        {
            m_inputSeg = loaded;
        }
        else
        {
            delete loaded;
            m_inputDatasetOwned = OFFalse;
            return result;
        }
    }

    OFString sop, segtype;
    m_inputDataset->findAndGetOFString(DCM_SOPClassUID, sop);
    m_inputDataset->findAndGetOFString(DCM_SegmentationType, segtype);
    return checkSOPClassAndSegtype(sop, segtype);
}


OFCondition DcmBinToLabelConverter::getOutputSegmentation(OFshared_ptr<DcmSegmentation>& outputSeg)
{
   outputSeg = m_outputSeg;
   return outputSeg ? EC_Normal : EC_IllegalParameter;
}


OFCondition DcmBinToLabelConverter::getOutputDataset(DcmItem& outputDataset)
{
    outputDataset.clear();
    if (m_outputSeg)
    {
        DCMSEG_DEBUG("checkExportFG: " << (m_convFlags.m_checkExportFG ? "enabled" : "disabled"));
        DCMSEG_DEBUG("checkExportValues: " << (m_convFlags.m_checkExportValues ? "enabled" : "disabled"));
        m_outputSeg->getFunctionalGroups().setUseThreads(m_convFlags.m_numThreads);
        m_outputSeg->getFunctionalGroups().setCheckOnWrite(m_convFlags.m_checkExportFG);
        m_outputSeg->setValueCheckOnWrite(m_convFlags.m_checkExportValues);
        OFCondition result = m_outputSeg->writeDataset(outputDataset);
        if (result.bad())
        {
            outputDataset.clear();
            return result;
        }
    }
    return EC_Normal;
}


OFCondition DcmBinToLabelConverter::createPaletteColorLUT()
{
    DCMSEG_DEBUG("Creating palette color lookup table for output segmentation");
    IODPaletteColorLUTModule& lutModule = m_outputSeg->getPaletteColorLUT();
    OFCondition result;
    // Create LUT from CIELab colors stored during segment copying
    size_t numSegments = m_outputSeg->getNumberOfSegments();
    if (m_cielabColors.m_numSegments == numSegments)
    {
        result = lutModule.setRedPaletteColorLookupTableDescriptor(numSegments, 1, (m_use16Bit ? 16 : 8));
        if (result.good()) result = lutModule.setGreenPaletteColorLookupTableDescriptor(numSegments, 1, (m_use16Bit ? 16 : 8));
        if (result.good()) result = lutModule.setBluePaletteColorLookupTableDescriptor(numSegments, 1, (m_use16Bit ? 16 : 8));
        if (result.good())
        {
            Uint16 maxRange = (m_use16Bit ? 65535 : 255);
            for (size_t idx = 0; idx < m_cielabColors.m_numSegments; idx++)
            {
                // Scale to full 16 bit range
                double R, G, B;
                IODCIELabUtil::dicomLab2RGB(R, G, B, m_cielabColors.m_L[idx], m_cielabColors.m_a[idx], m_cielabColors.m_b[idx]);
                IODCIELabUtil::dicomLab2RGB(R, G, B, m_cielabColors.m_L[idx], m_cielabColors.m_a[idx], m_cielabColors.m_b[idx]);
                IODCIELabUtil::dicomLab2RGB(R, G, B, m_cielabColors.m_L[idx], m_cielabColors.m_a[idx], m_cielabColors.m_b[idx]);
                R = R*maxRange;
                G = G*maxRange;
                B = B*maxRange;
                if (R < 0) R = 0;
                if (R > maxRange) R = maxRange;
                if (G < 0) G = 0;
                if (G > maxRange) G = maxRange;
                if (B < 0) B = 0;
                if (B > maxRange) B = maxRange;
                m_cielabColors.m_L[idx] = OFstatic_cast(Uint16, R);
                m_cielabColors.m_a[idx] = OFstatic_cast(Uint16, G);
                m_cielabColors.m_b[idx] = OFstatic_cast(Uint16, B);
                // Print RGB values
                DCMSEG_DEBUG("Segment #" << (idx+1) << " uses RGB color: R=" << m_cielabColors.m_L[idx] << ", G=" << m_cielabColors.m_a[idx] << ", B=" << m_cielabColors.m_b[idx]);
            }
            if (m_use16Bit)
            {
                DCMSEG_DEBUG("Using 16 bit palette color lookup table data");
                result = lutModule.setRedPaletteColorLookupTableData(m_cielabColors.m_L, numSegments);
                if (result.good()) result = lutModule.setGreenPaletteColorLookupTableData(m_cielabColors.m_a, numSegments);
                if (result.good()) result = lutModule.setBluePaletteColorLookupTableData(m_cielabColors.m_b, numSegments);
            }
            // data is already in the range 0..255, so we just need to convert the arrays
            else
            {
                DCMSEG_DEBUG("Using 8 bit palette color lookup table data");
                Uint8* redData = new Uint8[numSegments];
                Uint8* greenData = new Uint8[numSegments];
                Uint8* blueData = new Uint8[numSegments];
                if (redData && greenData && blueData)
                {
                    for (size_t idx = 0; idx < numSegments; idx++)
                    {
                        redData[idx]   = OFstatic_cast(Uint8, m_cielabColors.m_L[idx]);
                        greenData[idx] = OFstatic_cast(Uint8, m_cielabColors.m_a[idx]);
                        blueData[idx]  = OFstatic_cast(Uint8, m_cielabColors.m_b[idx]);
                    }
                    result = lutModule.setRedPaletteColorLookupTableData(redData, numSegments);
                    if (result.good()) result = lutModule.setGreenPaletteColorLookupTableData(greenData, numSegments);
                    if (result.good()) result = lutModule.setBluePaletteColorLookupTableData(blueData, numSegments);
                    delete[] redData;
                    delete[] greenData;
                    delete[] blueData;
                }
                else
                {
                    DCMSEG_ERROR("Cannot allocate memory for temporary 8 bit palette color lookup table data");
                    result = EC_MemoryExhausted;
                }
            }

        }
        else {
            DCMSEG_ERROR("Cannot set palette color lookup table descriptor: "  << result.text());
        }
    }
    else
    {
        DCMSEG_ERROR("Cannot create palette color lookup table: Number of CIELab colors does not match number of segments");
        result = SG_EC_CannotConvertMissingCIELab;
    }
    if (result.good())
    {
        // Set default profile to our sample ICC profile
        result = m_outputSeg->getICCProfile().setDefaultProfile(OFTrue /* also set color space description */);
        // Some debug info, if debug logger is enabled
        if (result.good() && DCM_dcmsegLogger.isEnabledFor(OFLogger::DEBUG_LOG_LEVEL))
        {
            // m_cieLabColors now contains RGB values, dump then out
            for (size_t idx = 0; idx < m_cielabColors.m_numSegments; idx++)
            {
                DCMSEG_DEBUG("Segment #" << (idx+1) << " uses RGB color: R=" << m_cielabColors.m_L[idx] << ", G=" << m_cielabColors.m_a[idx] << ", B=" << m_cielabColors.m_b[idx]);
            }
        }
        else if (result.bad())
        {
            DCMSEG_ERROR("Cannot set default ICC profile for output segmentation: " << result.text());
        }
    }
    if (result.good())
    {
        DCMSEG_DEBUG("Successfully created palette color lookup table and ICC profile for output segmentation");
    }
    return result;
}


OFCondition DcmBinToLabelConverter::createFrameContentFG(Uint32 outputFrameNum /* will start with 1 */, OFVector<OverlapUtil::LogicalFrame>::iterator logicalFrame, FGFrameContent*& frameContent)
{
    OFCondition result;
    frameContent = OFstatic_cast(FGFrameContent*, FGFactory::instance().create(DcmFGTypes::EFG_FRAMECONTENT));
    if (frameContent)
    {
        // Set Stack ID and In Stack Position Number:
        // Single Stack "Frame Position". All frames on the stack are sorted by their position
        // and will increasingly start from 1 to number of frames.
        frameContent->setStackID("Frame Position");
        frameContent->setInStackPositionNumber(outputFrameNum);
        result = frameContent->setDimensionIndexValues(1, 0);
        if (result.good())
        {
            result = frameContent->setDimensionIndexValues(outputFrameNum, 1);
        }

        // Frame Comments: Create list of source frames that has been used for this frame and insert their label
        // in the form "XXX, YYY ..." in the new frame's Frame Comment attribute
        if (result.good())
        {
            OverlapUtil::SegmentsByPosition segmentsAtPos;
            result = m_overlapUtil.getSegmentsByPosition(segmentsAtPos);
            if (result.good() && (segmentsAtPos.size() >= outputFrameNum) && !segmentsAtPos[outputFrameNum - 1].empty())
            {
                OFVector<OverlapUtil::SegNumAndFrameNum>::iterator seg = segmentsAtPos[outputFrameNum - 1].begin();
                OFString frameComments;
                while (seg != segmentsAtPos[outputFrameNum - 1].end())
                {
                    OFString label;
                    m_inputSeg->getSegment(seg->m_segmentNumber)->getSegmentLabel(label);
                    // unlikely, but: max length for frame comments is 10240 characters
                    if (frameComments.length() + label.length() + 4 > 10240)
                    {
                        frameComments += "...";
                        break;
                    }
                    else
                    {
                        frameComments += label;
                        frameComments += "; ";
                        ++seg;
                    }
                }
                // cut off last comma, if applicable
                if (frameComments.length() > 2) frameComments = frameComments.substr(0, frameComments.length() - 2);
                frameContent->setFrameComments(frameComments);
                result = m_outputSeg->getFunctionalGroups().addPerFrame(OFstatic_cast(Uint32, outputFrameNum - 1), *frameContent);
            }
        }
        delete frameContent;
    }
    else
    {
        result = EC_MemoryExhausted;
    }
    return result;
}


OFCondition DcmBinToLabelConverter::addSourceSegmentationToDerivationImageFG(DcmSegmentation* src, DcmSegmentation* dest)
{

    DCMSEG_DEBUG("Adding Derivation Image Functional Group to output segmentation");
    OFCondition result;
    OFBool isPerFrame = OFFalse;
    OFBool newlyCreated = OFFalse;
    FGDerivationImage* derivFG = OFstatic_cast(FGDerivationImage*, dest->getFunctionalGroups().get(1, DcmFGTypes::EFG_DERIVATIONIMAGE, isPerFrame));
    // If it is per frame, we cannot add source segmentation info since it applies to all frames
    if (isPerFrame)
    {
        DCMSEG_DEBUG("Derivation Image Functional Group already exists as per-frame group, will not add source segmentation to converted object");
        return EC_Normal;
    }
    // If not found, create new one
    if (!derivFG)
    {
        derivFG = new FGDerivationImage();
        if (!derivFG)
        {
            DCMSEG_ERROR("Cannot allocate memory for Derivation Image Functional Group");
            return EC_MemoryExhausted;
        }
        result = dest->getFunctionalGroups().addShared(*derivFG);
        if (result.good())
        {
            // since created FG is copied, retrieve the copied one for further processing
            derivFG = OFstatic_cast(FGDerivationImage*, dest->getFunctionalGroups().get(1, DcmFGTypes::EFG_DERIVATIONIMAGE, isPerFrame));
            newlyCreated = OFTrue;
        }
        delete derivFG; // delete temporary one
    }
    // Shared Derivation Image FG found or newly created, extend it
    DerivationImageItem* dItem = NULL;
    // Add new Derivation Image Item with empty Derivation Code Sequence and Description
    result = derivFG->addDerivationImageItem(CodeSequenceMacro("113076", "DCM", "Segmentation"), "Converted from binary segmentation to label map segmentation", dItem);
    if (result.good())
    {
        // Add Source Image Item pointing to source segmentation
        SourceImageItem* srcItem = NULL;
        result = dItem->addSourceImageItem(&(src->getSOPCommon().getData()), CodeSequenceMacro("121322", "DCM", "Source Image for Image Processing Operation"), srcItem);
        if (result.good())
        {
            DCMSEG_DEBUG("Successfully added Derivation Image Functional Group with source segmentation to output segmentation");
        }
    }
    else
    {
        DCMSEG_ERROR("Cannot extend Derivation Image Functional Group with source segmentation: " << result.text());
        if (newlyCreated)
        {
            // remove the newly created Derivation Image FG since we failed to extend it
            dest->getFunctionalGroups().deleteShared(DcmFGTypes::EFG_DERIVATIONIMAGE);
        }
        result = EC_IllegalParameter;
    }
    return result;
}

} // end namespace dcmqi