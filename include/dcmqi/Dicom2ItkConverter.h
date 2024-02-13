#ifndef DCMQI_SEGMENTATION_CONVERTER_H
#define DCMQI_SEGMENTATION_CONVERTER_H

// DCMTK includes
#include <dcmtk/dcmdata/dcrledrg.h>
#include <dcmtk/dcmfg/fgderimg.h>
#include <dcmtk/dcmfg/fgseg.h>
#include <dcmtk/dcmseg/segdoc.h>
#include <dcmtk/dcmseg/segment.h>
#include <dcmtk/dcmseg/segutils.h>

// ITK includes
#include <itkBinaryThresholdImageFilter.h>
#include <itkChangeInformationImageFilter.h>
#include <itkImageDuplicator.h>
#include <itkImageRegionConstIterator.h>
#include <itkImportImageFilter.h>
#include <itkLabelStatisticsImageFilter.h>

// DCMQI includes
#include "OverlapUtil.h"
#include "dcmqi/ConverterBase.h"
#include "dcmqi/JSONSegmentationMetaInformationHandler.h"
#include "dcmqi/OverlapUtil.h"

using namespace std;

typedef itk::LabelImageToLabelMapFilter<ShortImageType> LabelToLabelMapFilterType;

namespace dcmqi
{

/**
 * @brief The ImageSEGConverter class provides methods to convert from DICOM Segmentation objects to itk images.
 * It should be used as follows:
 * - Call dcmSegmentation2itkimage() to start the conversion of DICOM
 *   Segmentation object into a map of itk images. This will return to you
 *   the JSON meta information of the conversion.
 * - Call begin() to get the first ITK image result of the conversion.
 *   Note that if conversion fails, this can return a null pointer.
 * - Call next() to get the next ITK image result of the conversion, until
 *   it returns a null pointer.
 */
class Dicom2ItkConverter : public ConverterBase
{

public:
    Dicom2ItkConverter();

    /**
     * @brief Converts a DICOM Segmentation object into a map of itk images and metadata.
     *
     * @param segDataset A pointer to the DICOM Segmentation object to be converted.
     * @param mergeSegments A boolean indicating whether to merge segments during the conversion.
     *        Defaults to false.
     * @return A pair containing the resulting map of itk images and the metadata.
     */
    OFCondition dcmSegmentation2itkimage(DcmDataset* segDataset, std::string& metaInfo, const bool mergeSegments = false);

    /** Get first ITK image result of conversion, or null pointer if conversion failed.
     *  One result represents one ITK image containing all segments of one (OverlapUtil)
     *  segment group.
     *  @return Shared pointer to first ITK image resulting from the conversion
     */
    itk::SmartPointer<ShortImageType> begin();

    /** Get next ITK image result of conversion, or nuĺl pointer if no more results
     *  @return Shared pointer to next ITK image resulting from the conversion, or null
     */
    itk::SmartPointer<ShortImageType> next();

    /** Get JSON the metadata of the conversion
     *  @return Metadata of the conversion
     */
    JSONSegmentationMetaInformationHandler getMetaInformation();

protected:
    /** Internal result loop, produced one result at a time (or null).
     *  One result represents one ITK image containing all segments of one (OverlapUtil)
     *  segment group.
     *  @return Shared pointer to first/next ITK image resulting from the conversion
     */
    itk::SmartPointer<ShortImageType> nextResult();

    /**
     * @brief Converts a DICOM Segmentation object into a map of itk images.
     *
     * @param mergeSegments A boolean indicating whether to merge segments during the conversion.
     *        Defaults to false.
     * @return A map of itk images resulting from the conversion.
     */

    OFCondition dcmSegmentation2itkimage(const bool mergeSegments = false);

    /**
     * @brief Populates the metadata of a DICOM Segmentation object from a DICOM dataset.
     *
     * @param segDataset A pointer to the DICOM dataset containing the metadata.
     */
    void populateMetaInformationFromDICOM(DcmItem* segDataset);

    /**
     * Helper method that uses the OverlapUtil class to retrieve non-overlapping segment
     * groups (by their segment numbers) from the DICOM Segmentation object.
     * If mergeSegments is false, all segments are returned within a single group.
     *
     * @param mergeSegments Whether or not to merge overlapping segments into the same group.
     * @param segmentGroups The resulting segment groups.
     * @return EC_Normal if successful, error otherwise
     */
    OFCondition getNonOverlappingSegmentGroups(const bool mergeSegments, OverlapUtil::SegmentGroups& segmentGroups);

    /**
     *  Extract basic image info like directions, origin, spacing and image region.
     *  @return EC_Normal if successful, error otherwise
     */
    OFCondition extractBasicSegmentationInfo();

    /**
     *  Allocate an ITK image with the same size and spacing as the DICOM image.
     *  This is used for each slice that is to be inserted into the ITK space.
     *  @return EC_Normal if successful, error otherwise
     */
    ShortImageType::Pointer allocateITKImage();

    /**
     *  Allocate an ITK image based on the given template.
     *  @param  imageTemplate The template to use for the new image.
     *  @return EC_Normal if successful, error otherwise
     */
    ShortImageType::Pointer allocateITKImageDuplicate(ShortImageType::Pointer imageTemplate);

    /**
     *  Get the ITK image origin for the given frame.
     *  @param  frameNo The (DICOM) frame number to get the origin for.
     *  @param  origin The resulting origin.
     *  @return EC_Normal if successful, error otherwise
     */
    OFCondition getITKImageOrigin(const Uint32 frameNo, ShortImageType::PointType& origin);

    /**
     *  Collect segment metadata for the given frame that will later go into the
     *  accompanying JSON segmentation description.
     *  @param  segmentGroup The segment group number
     *          (i.e. uniquely identifying the NRRD output file in the end).
     *  @param  segmentNumber The DICOM segment number.
     *  @param  origin The resulting origin.
     *  @return EC_Normal if successful, error otherwise
     */
    OFCondition addSegmentMetadata(const size_t segmentGroup, const Uint16 segmentNumber);

    OFCondition createMetaInfo();

    /// The segmentation object, guarded by unique pointer
    OFunique_ptr<DcmSegmentation> m_segDoc;

    /// Image direction in ITK speak
    ShortImageType::DirectionType m_direction;
    /// Computed image slice spacing
    double m_computedSliceSpacing;
    /// Computed volume extent
    double m_computedVolumeExtent;
    /// Slice direction in ITK speak
    vnl_vector<double> m_sliceDirection;

    /// Image origin in ITK speak
    ShortImageType::PointType m_imageOrigin;
    /// Image slice spacing in ITK speak
    ShortImageType::SpacingType m_imageSpacing;
    /// Image size in ITK speak
    ShortImageType::SizeType m_imageSize;
    /// Image region in ITK speak
    ShortImageType::RegionType m_imageRegion;

    /// Segment meta information handler for the JSON segmentation description
    JSONSegmentationMetaInformationHandler m_metaInfo;

    /// Segment groups, used to iterate over results
    OverlapUtil::SegmentGroups m_segmentGroups;

    /// Internal iterator to "next" segment group to be returned,
    /// used while iterating over results using begin() and next()
    OverlapUtil::SegmentGroups::iterator m_groupIterator;

    /// OverlapUtil instance used by this class, used in DICOM segmentation
    /// to itk conversion
    OverlapUtil m_overlapUtil;
};

}

#endif // DCMQI_SEGMENTATION_CONVERTER_H
