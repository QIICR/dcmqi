#ifndef DCMQI_SEGMENTATION_CONVERTER_LABEL_H
#define DCMQI_SEGMENTATION_CONVERTER_LABEL_H

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

namespace dcmqi
{

/**
 * @brief Converts DICOM Segmentation objects (labelmaps) to ITK images.
 *
 * This class handles DICOM segmentations that are encoded as labelmaps,
 * where each voxel value corresponds to a segment label.
 */
class Dicom2ItkConverterLabel : public Dicom2ItkConverterBase
{

   // Only Dicom2ItkConverter can create instances of this class, so we make the constructor
   // protected and declare the factory class as friend.
  friend class Dicom2ItkConverter;

public:

    /**
     * @brief Destructor.
     */
    ~Dicom2ItkConverterLabel() override;

    /**
     * @brief Converts the DICOM segmentation to ITK images and metadata.
     * @param mergeSegments Ignored for labelmaps (segments are non-overlapping).
     * @return EC_Normal if successful, error otherwise.
     */
    OFCondition dcmSegmentation2itkimage(const bool mergeSegments) override;

    /**
     * @brief Get the first 16-bit ITK image result of the conversion.
     * @return Shared pointer to the first ITK image, or null if none.
     */
    itk::SmartPointer<ShortImageType> begin16Bit() override;

    /**
     * @brief Get the first 8-bit ITK image result of the conversion.
     * @return Shared pointer to the first ITK image, or null if none.
     */
    itk::SmartPointer<CharImageType> begin8Bit() override;

    /**
     * @brief Get the next 16-bit ITK image result of the conversion.
     * @return Shared pointer to the next ITK image, or null if none.
     */
    itk::SmartPointer<ShortImageType> next16Bit() override;

    /**
     * @brief Get the next 8-bit ITK image result of the conversion.
     * @return Shared pointer to the next ITK image, or null if none.
     */
    itk::SmartPointer<CharImageType> next8Bit() override;

protected:

    /**
     * @brief Default constructor.
     */
    Dicom2ItkConverterLabel();

    /**
     * @brief Internal implementation for iterating over labelmap frames.
     * Reads pixel data with the given TPixelType (Uint8 or Uint16) and writes
     * it into TImageType (CharImageType or ShortImageType).
     * @tparam TImageType The ITK image type to produce.
     * @tparam TPixelType The pixel data type to read from the DICOM frame.
     * @return Shared pointer to the next ITK image, or null if no more frames.
     */
    template<typename TImageType, typename TPixelType>
    itk::SmartPointer<TImageType> nextImpl();

    /// Current frame
    size_t m_frameIterator;
};

} // namespace dcmqi

#endif // DCMQI_SEGMENTATION_CONVERTER_LABEL_H
