#ifndef DCMQI_SEGMENTATION_CONVERTER_BIN_H
#define DCMQI_SEGMENTATION_CONVERTER_BIN_H

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

// DCMQI includes
#include "dcmqi/Dicom2ItkConverterBase.h"
#include "dcmqi/ConverterBase.h"
#include "dcmqi/JSONSegmentationMetaInformationHandler.h"

using namespace std;

typedef itk::LabelImageToLabelMapFilter<ShortImageType> LabelToLabelMapFilterType;

namespace dcmqi
{

/**
 * @brief Converts DICOM binary segmentation objects to ITK images.
 *
 * Usage:
 * - Call dcmSegmentation2itkimage() to start the conversion of a DICOM Segmentation object into a
 *   map of ITK images. This will return the JSON meta information of the conversion.
 * - Call begin16Bit() to get the first ITK image result of the conversion. If conversion fails, this
 *   can return a null pointer.
 * - Call next16Bit() to get the next ITK image result of the conversion, until it returns a null
 *   pointer.
 * - 8-bit output is not supported for binary segmentations.
 */
class Dicom2ItkConverterBin : public Dicom2ItkConverterBase
{

   // Only Dicom2ItkConverter can create instances of this class, so we make the constructor
   // protected and declare the factory class as friend.
  friend class Dicom2ItkConverter;

public:

    /**
     * @brief Destructor.
     */
    virtual ~Dicom2ItkConverterBin() = default;

    /**
     * @brief Get first ITK image result of conversion, or null pointer if conversion failed.
     *
     * One result represents one ITK image containing all segments of one (OverlapUtil) segment group.
     * @return Shared pointer to first ITK image resulting from the conversion.
     */
    itk::SmartPointer<ShortImageType> begin16Bit() override;

    /**
     * @brief Get next 16-bit ITK image result of conversion, or null pointer if no more results.
     * @return Shared pointer to next ITK image resulting from the conversion, or null.
     */
    itk::SmartPointer<ShortImageType> next16Bit() override;

    /**
     * @brief Get first 8-bit ITK image result of conversion (not supported for binary segmentations).
     * @return Always returns nullptr.
     */
    itk::SmartPointer<CharImageType> begin8Bit() override
    {
        cerr << "ERROR: 8-bit output is not supported for binary segmentations!" << endl;
        return nullptr;
    }

    /**
     * @brief Get next 8-bit ITK image result of conversion (not supported for binary segmentations).
     * @return Always returns nullptr.
     */
    itk::SmartPointer<CharImageType> next8Bit() override
    {
        cerr << "ERROR: 8-bit output is not supported for binary segmentations!" << endl;
        return nullptr;
    }

protected:

    /**
     * @brief Default constructor.
     */
    Dicom2ItkConverterBin();

    /**
     * @brief Find groups of non-overlapping segments for conversion.
     * @param mergeSegments Boolean indicating whether to merge segments.
     * @param segmentGroups Output segment groups.
     * @return EC_Normal if successful, error otherwise.
     */
    OFCondition getNonOverlappingSegmentGroups(
        const bool mergeSegments,
        OverlapUtil::SegmentGroups& segmentGroups);

    /**
     * @brief Converts a DICOM Segmentation object into a map of ITK images.
     * @param mergeSegments Boolean indicating whether to merge segments during the conversion
     *        (default: false).
     * @return EC_Normal if successful, error otherwise.
     */
    virtual OFCondition dcmSegmentation2itkimage(const bool mergeSegments) override;

    /// Internal iterator to "next" segment group to be returned,
    /// used while iterating over results using begin16Bit() and next16Bit()
    OverlapUtil::SegmentGroups::iterator m_groupIterator;

    /// OverlapUtil instance used by this class, used in DICOM segmentation to ITK conversion
    OverlapUtil m_overlapUtil;
};

} // namespace dcmqi

#endif // DCMQI_DICOM2ITK_CONVERTER_BIN_H
