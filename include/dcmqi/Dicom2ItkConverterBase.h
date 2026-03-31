#ifndef DCMQI_SEGMENTATION_CONVERTER_BASE_H
#define DCMQI_SEGMENTATION_CONVERTER_BASE_H

// DCMTK includes
#include <dcmtk/dcmdata/dcrledrg.h>
#include <dcmtk/dcmfg/fgderimg.h>
#include <dcmtk/dcmfg/fgseg.h>
#include <dcmtk/dcmseg/segdoc.h>
#include <dcmtk/dcmseg/segment.h>
#include <dcmtk/dcmseg/segutils.h>
#include <dcmtk/dcmseg/overlaputil.h>

// ITK includes
#include <itkBinaryThresholdImageFilter.h>
#include <itkChangeInformationImageFilter.h>
#include <itkImageDuplicator.h>
#include <itkImageRegionConstIterator.h>
#include <itkImportImageFilter.h>
#include <itkLabelStatisticsImageFilter.h>
#include <itkSmartPointer.h>

// DCMQI includes
#include "dcmqi/ConverterBase.h"
#include "dcmqi/JSONSegmentationMetaInformationHandler.h"

using namespace std;

namespace dcmqi
{

/**
 * @brief Provides methods to convert from DICOM Segmentation objects to ITK images.
 *
 * Usage:
 * - Call dcmSegmentation2itkimage() to start the conversion of a DICOM Segmentation object into a map of ITK images. This will return the JSON meta information of the conversion.
 * - Use bytesPerPixel() to determine whether the resulting ITK images are 8 or 16 bit.
 * - Call begin8Bit() or begin16Bit() to get the first ITK image result of the conversion.
 * - Call next8Bit() or next16Bit() to get the next ITK image result of the conversion, until it returns a null pointer.
 */
class Dicom2ItkConverterBase : public ConverterBase
{

public:

    /**
     * @brief Default constructor.
     */
    Dicom2ItkConverterBase();

    /**
     * @brief Default destructor.
     */
    virtual ~Dicom2ItkConverterBase() = default;

    /**
     * @brief Converts a DICOM Segmentation object into a map of ITK images and metadata.
     * @param segDataset Pointer to the DICOM Segmentation object to be converted.
     * @param metaInfo Output string containing the resulting metadata.
     * @param mergeSegments Boolean indicating whether to merge segments during the conversion (default: false).
         ignored for labelmaps since all segments are already guaranteed to be non-overlapping).
     * @return EC_Normal if successful, error otherwise.
     */
    virtual OFCondition dcmSegmentation2itkimage(DcmDataset* segDataset, std::string& metaInfo, const bool mergeSegments = false);

    /**
     * @brief Get first 16-bit ITK image result of conversion, or null pointer if conversion failed.
     * @return Shared pointer to first ITK image resulting from the conversion.
     */
    virtual itk::SmartPointer<ShortImageType> begin16Bit() =0;

    /**
     * @brief Get first 8-bit ITK image result of conversion, or null pointer if conversion failed.
     * @return Shared pointer to first ITK image resulting from the conversion.
     */
    virtual itk::SmartPointer<CharImageType> begin8Bit() =0;

    /**
     * @brief Get next 16-bit ITK image result of conversion, or null pointer if no more results.
     * @return Shared pointer to next ITK image resulting from the conversion, or null.
     */
    virtual itk::SmartPointer<ShortImageType> next16Bit() =0;

    /**
     * @brief Get next 8-bit ITK image result of conversion, or null pointer if no more results.
     * @return Shared pointer to next ITK image resulting from the conversion, or null.
     */
    virtual itk::SmartPointer<CharImageType> next8Bit() =0;

    /**
     * @brief Get the number of bytes per pixel for the resulting ITK images.
     * @return Number of bytes per pixel (1 or 2).
     */
    Uint8 bytesPerPixel();

    /**
     * @brief Check if the segmentation is a labelmap.
     * @return True if the segmentation is a labelmap, false if it is a binary segmentation.
     */
    bool isLabelmap();

    /**
     * @brief Get the JSON metadata of the conversion.
     * @return Metadata of the conversion.
     */
    JSONSegmentationMetaInformationHandler getMetaInformation();

protected:

    /**
     * @brief Internal conversion method that must be implemented by subclasses.
     * @param mergeSegments Boolean indicating whether to merge segments during the conversion.
     * Note: For labelmaps, this parameter is ignored since all segments are already guaranteed to be non-overlapping.
     * @return EC_Normal if successful, error otherwise.
     */
    virtual OFCondition dcmSegmentation2itkimage(const bool mergeSegments) =0;

    /**
     * @brief Populates the metadata of a DICOM Segmentation object from a DICOM dataset.
     * @param segDataset Pointer to the DICOM dataset containing the metadata.
     */
    void populateMetaInformationFromDICOM(DcmDataset* segDataset);

    /**
     * @brief Extract basic image info like directions, origin, spacing and image region.
     * @return EC_Normal if successful, error otherwise.
     */
    OFCondition extractBasicSegmentationInfo();

    /**
     * @brief Allocate an ITK image with the same size and spacing as the DICOM image.
     * This is used for each slice that is to be inserted into the ITK space.
     * The template parameter is meant to be used for types CharImageType and ShortImageType,
     * which are the two types of ITK images that we return from this converter.
     * @tparam TImageType The ITK image type.
     * @return Pointer to the allocated ITK image.
     */
    template<typename TImageType>
    typename TImageType::Pointer allocateITKImage()
    {
        // Initialize the image
        typename TImageType::Pointer itkImage = TImageType::New();
        itkImage->SetRegions(m_imageRegion);
        itkImage->SetOrigin(m_imageOrigin);
        itkImage->SetSpacing(m_imageSpacing);
        itkImage->SetDirection(m_direction);
        itkImage->Allocate();
        itkImage->FillBuffer(0);
        return itkImage;
    }

    /**
     * @brief Allocate an ITK image based on the given template.
     * @param imageTemplate The template to use for the new image.
     * @return Pointer to the allocated ITK image.
     */
    ShortImageType::Pointer allocateITKImageDuplicate(ShortImageType::Pointer imageTemplate);

    /**
     * @brief Get the ITK image origin for the given frame.
     * @param frameNo The (DICOM) frame number to get the origin for.
     * @param origin Output parameter for the resulting origin.
     * @return EC_Normal if successful, error otherwise.
     */
    OFCondition getITKImageOrigin(const Uint32 frameNo, ShortImageType::PointType& origin);

    /**
     * @brief Collect segment metadata for the given frame that will later go into the accompanying JSON segmentation description.
     * @param segmentGroup The segment group number (uniquely identifying the NRRD output file in the end).
     * @param segmentNumber The DICOM segment number.
     * @return EC_Normal if successful, error otherwise.
     */
    OFCondition addSegmentMetadata(const size_t segmentGroup, const Uint16 segmentNumber);

    /**
     * @brief Create meta info (meta.json) from DICOM segment descriptions.
     * @return EC_Normal if successful, error otherwise.
     */
    virtual OFCondition createMetaInfo();

    // Some protected members that are needed for the conversion and/or the
    // metadata population, which can be used by the subclasses as well.

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

    /// Segment groups, used to iterate over results
    OverlapUtil::SegmentGroups m_segmentGroups;

    /// Segment meta information handler for the JSON segmentation description
    JSONSegmentationMetaInformationHandler m_metaInfo;

    /// Whether the DICOM input segmentation is a labelmap or a binary segmentation
    bool m_isLabelmap;

    /// Number of bytes per pixel required for the output ITK images,
    /// determined from the DICOM input and needed to correctly interpret the pixel data
    /// when calling begin8/16() and next8/16() to get the resulting ITK images.
    Uint8 m_bytesPerPixel;
};

// -------------------------------------------------------------------------------------

/**
 * @brief Factory class to create the appropriate converter for a given DICOM dataset.
 *
 * It mainly checks the SOP Class UID to decide which converter to create,
 * then calls the constructor of that converter and returns it.
 */
class Dicom2ItkConverter
{

public:

  /**
   * @brief Get the appropriate converter for a given DICOM dataset.
   * @param dataset The DICOM dataset to create the converter for.
   * @return A pointer to the created converter, or nullptr if the dataset is not supported.
   */
  static Dicom2ItkConverterBase* getConverter(DcmItem* dataset);

};

} // namespace dcmqi

#endif // DCMQI_SEGMENTATION_CONVERTER_BASE_H
