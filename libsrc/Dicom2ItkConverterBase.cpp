// DCMQI includes
#include "dcmqi/Dicom2ItkConverterBase.h"
#include "dcmqi/Dicom2ItkConverterBin.h"
#include "dcmqi/Dicom2ItkConverterLabel.h"
#include "dcmqi/ColorUtilities.h"

// DCMTK includes
#include <cstddef>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmseg/overlaputil.h>
#include <dcmtk/dcmiod/cielabutil.h>
#include <dcmtk/dcmsr/codes/dcm.h>
#include <dcmtk/ofstd/ofmem.h>
#include <memory>

namespace dcmqi
{

Dicom2ItkConverterBase* Dicom2ItkConverter::getConverter(DcmItem* dataset)
{
    OFString sopClassUID;
    if (dataset->findAndGetOFString(DCM_SOPClassUID, sopClassUID).bad())
    {
        cerr << "ERROR: Failed to get SOP Class UID from dataset! Unable to interpret the data." << endl;
        return nullptr;
    }
    if (sopClassUID == UID_SegmentationStorage)
        return new Dicom2ItkConverterBin();
    else if (sopClassUID == UID_LabelMapSegmentationStorage)
        return new Dicom2ItkConverterLabel();
    else
    {
        cerr << "ERROR: Unsupported SOP Class UID " << sopClassUID << "! Unable to interpret the data." << endl;
        return nullptr;
    }
}

Dicom2ItkConverterBase::Dicom2ItkConverterBase()
    : m_segDoc()
    , m_direction()
    , m_computedSliceSpacing()
    , m_computedVolumeExtent()
    , m_sliceDirection(3)
    , m_imageOrigin()
    , m_imageSpacing()
    , m_imageSize()
    , m_imageRegion()
    , m_segmentGroups()
    , m_metaInfo()
    , m_isLabelmap(false)
{
    // Setup logging (not used so far?)
    OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");
    dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);
}


// -------------------------------------------------------------------------------------

OFCondition
Dicom2ItkConverterBase::dcmSegmentation2itkimage(DcmDataset* segDataset, std::string& metaInfo, const bool mergeSegments)
{
    DcmSegmentation* segdoc = NULL;

    // Load the DICOM segmentation dataset into DcmSegmentation member
    OFCondition cond = DcmSegmentation::loadDataset(*segDataset, segdoc);
    if (!segdoc)
    {
        cerr << "ERROR: Failed to load segmentation dataset! " << cond.text() << endl;
        throw -1;
    }
    m_segDoc.reset(segdoc);

    cond = extractBasicSegmentationInfo();
    if (cond.bad())
    {
        cerr << "ERROR: Failed to extract basic segmentation information! " << cond.text() << endl;
        throw -1;
    }

    // Populate DICOM series information into accompanying JSON metainfo member
    populateMetaInformationFromDICOM(segDataset);

    // Call the actual conversion, implemented in the subclasses.
    // The conversion will populate the meta information member with segment-specific metadata
    // as well, which we then convert to string and return as output.
    OFCondition result = dcmSegmentation2itkimage(mergeSegments);
    if (result.good())
    {
        metaInfo = m_metaInfo.getJSONOutputAsString();
    }
    return result;
}


// -------------------------------------------------------------------------------------

void Dicom2ItkConverterBase::populateMetaInformationFromDICOM(DcmDataset* segDataset)
{
    OFString creatorName, sessionID, timePointID, seriesDescription, seriesNumber, instanceNumber, bodyPartExamined,
        coordinatingCenter;

    segDataset->findAndGetOFString(DCM_InstanceNumber, instanceNumber);
    m_segDoc->getContentIdentification().getContentCreatorName(creatorName);

    segDataset->findAndGetOFString(DCM_ClinicalTrialTimePointID, timePointID);
    segDataset->findAndGetOFString(DCM_ClinicalTrialSeriesID, sessionID);
    segDataset->findAndGetOFString(DCM_ClinicalTrialCoordinatingCenterName, coordinatingCenter);

    m_segDoc->getSeries().getBodyPartExamined(bodyPartExamined);
    m_segDoc->getSeries().getSeriesNumber(seriesNumber);
    m_segDoc->getSeries().getSeriesDescription(seriesDescription);

    m_metaInfo.setClinicalTrialCoordinatingCenterName(coordinatingCenter.c_str());
    m_metaInfo.setContentCreatorName(creatorName.c_str());
    m_metaInfo.setClinicalTrialSeriesID(sessionID.c_str());
    m_metaInfo.setSeriesNumber(seriesNumber.c_str());
    m_metaInfo.setClinicalTrialTimePointID(timePointID.c_str());
    m_metaInfo.setSeriesDescription(seriesDescription.c_str());
    m_metaInfo.setInstanceNumber(instanceNumber.c_str());
    m_metaInfo.setBodyPartExamined(bodyPartExamined.c_str());
}

// -------------------------------------------------------------------------------------

OFCondition Dicom2ItkConverterBase::extractBasicSegmentationInfo()
{
    // TODO: Better error handling
    OFCondition result;

    // Directions
    FGInterface& fgInterface = m_segDoc->getFunctionalGroups();
    if (getImageDirections(fgInterface, m_direction))
    {
        cerr << "ERROR: Failed to get image directions!" << endl;
        throw -1;
    }

    // Origin
    m_sliceDirection[0] = m_direction[0][2];
    m_sliceDirection[1] = m_direction[1][2];
    m_sliceDirection[2] = m_direction[2][2];
    if (computeVolumeExtent(
            fgInterface, m_sliceDirection, m_imageOrigin, m_computedSliceSpacing, m_computedVolumeExtent))
    {
        cerr << "ERROR: Failed to compute origin and/or slice spacing!" << endl;
        throw -1;
    }

    // Spacing
    m_imageSpacing.Fill(0);
    if (getDeclaredImageSpacing(fgInterface, m_imageSpacing))
    {
        cerr << "ERROR: Failed to get image spacing from DICOM!" << endl;
        throw -1;
    }

    if (!m_imageSpacing[2])
    {
        cerr << "ERROR: No sufficient information to derive slice spacing! Unable to interpret the data." << endl;
        throw -1;
    }

    // Region, defined by DICOM rows and columns
    {
        Uint16 rows, cols;
        rows = m_segDoc->getRows();
        cols = m_segDoc->getColumns();
        if (rows == 0 || cols == 0)
        {
            cerr << "ERROR: Failed to get image rows and/or columns from DICOM!" << endl;
            throw -1;
        }
        m_imageSize[1] = rows;
        m_imageSize[0] = cols;
    }
    // Number of slices should be computed, since segmentation may have empty frames
    m_imageSize[2] = round(m_computedVolumeExtent / m_imageSpacing[2]) + 1;

    // Initialize the image template. This will only serve as a template for the individual
    // frames, which will be created via ImageDuplicator later on.
    m_imageRegion.SetSize(m_imageSize);

    // Check SOP Class and remember whether this is a labelmap or not, this will be needed later when we read the pixel data
    OFString sopClassUID;
    m_segDoc->getSOPCommon().getSOPClassUID(sopClassUID);
    if (sopClassUID == UID_SegmentationStorage)
        m_isLabelmap = false;
    else if (sopClassUID == UID_LabelMapSegmentationStorage)
        m_isLabelmap = true;
    else
    {
        cerr << "ERROR: Unsupported SOP Class UID " << sopClassUID << "! Unable to interpret the data." << endl;
        throw -1;
    }

    // Determine bytes per pixel from the first frame, this will be needed to correctly interpret the pixel data later on
    const DcmIODTypes::FrameBase* firstFrame = m_segDoc->getFrame(0);
    if (!firstFrame)
        return EC_IllegalParameter;
    m_bytesPerPixel = firstFrame->bytesPerPixel();
    std::cout << "Bytes per pixel: " << static_cast<int>(m_bytesPerPixel) << std::endl;
    return result;
}

// -------------------------------------------------------------------------------------

ShortImageType::Pointer Dicom2ItkConverterBase::allocateITKImageDuplicate(ShortImageType::Pointer imageTemplate)
{
    typedef itk::ImageDuplicator<ShortImageType> DuplicatorType;
    DuplicatorType::Pointer dup = DuplicatorType::New();
    dup->SetInputImage(imageTemplate);
    dup->Update();
    ShortImageType::Pointer newSegmentImage = dup->GetOutput();
    newSegmentImage->FillBuffer(0);
    return newSegmentImage;
}

// -------------------------------------------------------------------------------------

OFCondition Dicom2ItkConverterBase::getITKImageOrigin(const Uint32 frameNo, ShortImageType::PointType& origin)
{
    FGInterface& fgInterface = m_segDoc->getFunctionalGroups();

    FGPlanePosPatient* planposfg
        = OFstatic_cast(FGPlanePosPatient*, fgInterface.get(frameNo, DcmFGTypes::EFG_PLANEPOSPATIENT));
    assert(planposfg);

    for (int j = 0; j < 3; j++)
    {
        OFString planposStr;
        if (planposfg->getImagePositionPatient(planposStr, j).good())
        {
            origin[j] = atof(planposStr.c_str());
        }
    }
    // Dump the origin for debugging
    std::cout << "Extracted image origin for frame " << frameNo << ": " << origin << std::endl;
    return EC_Normal;
}


// -------------------------------------------------------------------------------------

OFCondition Dicom2ItkConverterBase::addSegmentMetadata(const size_t segmentGroup, const Uint16 segmentNumber)
{
    SegmentAttributes* segmentAttributes = m_metaInfo.createOrGetSegment(segmentGroup, segmentNumber);
    // NOTE: Segment numbers in DICOM start with 1
    DcmSegment* segment = m_segDoc->getSegment(segmentNumber);
    if (segment == NULL)
    {
        cerr << "Failed to get segment for segment ID " << segmentNumber << endl;
        return EC_IllegalParameter;
    }

    // get CIELab color for the segment
    Uint16 ciedcm[3];
    int rgb[3];
    if (segment->getRecommendedDisplayCIELabValue(ciedcm[0], ciedcm[1], ciedcm[2]).bad())
    {
        // NOTE: if the call above fails, it overwrites the values anyway,
        //  not sure if this is a dcmtk bug or not
        ciedcm[0] = 43803;
        ciedcm[1] = 26565;
        ciedcm[2] = 37722;
        cerr << "Failed to get CIELab values - initializing to default " << ciedcm[0] << "," << ciedcm[1] << ","
             << ciedcm[2] << endl;
    }

    ColorUtilities::getSRGBFromIntegerScaledCIELabPCS(rgb[0], rgb[1], rgb[2], ciedcm[0], ciedcm[1], ciedcm[2]);

    if (segmentAttributes)
    {
        segmentAttributes->setLabelID(segmentNumber);
        DcmSegTypes::E_SegmentAlgoType algorithmType = segment->getSegmentAlgorithmType();
        string readableAlgorithmType                 = DcmSegTypes::algoType2OFString(algorithmType).c_str();
        segmentAttributes->setSegmentAlgorithmType(readableAlgorithmType);

        if (algorithmType == DcmSegTypes::SAT_UNKNOWN)
        {
            cerr << "ERROR: AlgorithmType is not valid with value " << readableAlgorithmType << endl;
            throw -1;
        }
        if (algorithmType != DcmSegTypes::SAT_MANUAL)
        {
            OFString segmentAlgorithmName;
            segment->getSegmentAlgorithmName(segmentAlgorithmName);
            if (segmentAlgorithmName.length() > 0)
                segmentAttributes->setSegmentAlgorithmName(segmentAlgorithmName.c_str());
        }

        OFString segmentDescription, segmentLabel, trackingIdentifier, trackingUniqueIdentifier;

        segment->getSegmentDescription(segmentDescription);
        segmentAttributes->setSegmentDescription(segmentDescription.c_str());

        segment->getSegmentLabel(segmentLabel);
        segmentAttributes->setSegmentLabel(segmentLabel.c_str());

        segment->getTrackingID(trackingIdentifier);
        segment->getTrackingUID(trackingUniqueIdentifier);

        if (trackingIdentifier.length() > 0)
        {
            segmentAttributes->setTrackingIdentifier(trackingIdentifier.c_str());
        }
        if (trackingUniqueIdentifier.length() > 0)
        {
            segmentAttributes->setTrackingUniqueIdentifier(trackingUniqueIdentifier.c_str());
        }

        segmentAttributes->setRecommendedDisplayRGBValue(rgb[0], rgb[1], rgb[2]);
        segmentAttributes->setSegmentedPropertyCategoryCodeSequence(segment->getSegmentedPropertyCategoryCode());
        segmentAttributes->setSegmentedPropertyTypeCodeSequence(segment->getSegmentedPropertyTypeCode());

        if (segment->getSegmentedPropertyTypeModifierCode().size() > 0)
        {
            segmentAttributes->setSegmentedPropertyTypeModifierCodeSequence(
                segment->getSegmentedPropertyTypeModifierCode()[0]);
        }

        GeneralAnatomyMacro& anatomyMacro         = segment->getGeneralAnatomyCode();
        CodeSequenceMacro& anatomicRegionSequence = anatomyMacro.getAnatomicRegion();
        if (anatomicRegionSequence.check(true).good())
        {
            segmentAttributes->setAnatomicRegionSequence(anatomyMacro.getAnatomicRegion());
        }
        if (anatomyMacro.getAnatomicRegionModifier().size() > 0)
        {
            segmentAttributes->setAnatomicRegionModifierSequence(*(anatomyMacro.getAnatomicRegionModifier()[0]));
        }
    }
    return EC_Normal;
}

// -------------------------------------------------------------------------------------

Uint8 Dicom2ItkConverterBase::bytesPerPixel()
{
    return m_bytesPerPixel;
}

// -------------------------------------------------------------------------------------

bool Dicom2ItkConverterBase::isLabelmap()
{
    return m_isLabelmap;
}

// -------------------------------------------------------------------------------------

OFCondition Dicom2ItkConverterBase::createMetaInfo()
{
    OFCondition result;
    OverlapUtil::SegmentGroups::const_iterator group = m_segmentGroups.begin();
    size_t groupNumber                               = 1;
    while (group != m_segmentGroups.end())
    {
        auto segNum = group->begin();
        while (result.good() && (segNum != group->end()))
        {
            result = addSegmentMetadata(groupNumber, *segNum);
            segNum++;
        }
        group++;
        groupNumber++;
    }
    if (result.bad())
    {
        cerr << "ERROR: Failed to create segment metadata: " << result.text() << endl;
    }
    return result;
}


}  // namespace dcmqi
