#ifndef DCMQI_SEGMENTATION_CONVERTER_H
#define DCMQI_SEGMENTATION_CONVERTER_H

// DCMTK includes
#include <dcmtk/dcmfg/fgderimg.h>
#include <dcmtk/dcmfg/fgseg.h>
#include <dcmtk/dcmseg/segdoc.h>
#include <dcmtk/dcmseg/segment.h>
#include <dcmtk/dcmseg/segutils.h>
#include <dcmtk/dcmdata/dcrledrg.h>

// ITK includes
#include <itkImageDuplicator.h>
#include <itkImageRegionConstIterator.h>
#include <itkLabelStatisticsImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkChangeInformationImageFilter.h>
#include <itkImportImageFilter.h>

// DCMQI includes
//#include "dcmqi/OverlapUtil.h"
#include "OverlapUtil.h"
#include "SegmentAttributes.h"
#include "dcmqi/ConverterBase.h"
#include "dcmqi/OverlapUtil.h"
#include "dcmqi/JSONSegmentationMetaInformationHandler.h"


using namespace std;

typedef itk::LabelImageToLabelMapFilter<ShortImageType> LabelToLabelMapFilterType;

namespace dcmqi {

  /**
   * @brief The ImageSEGConverter class provides methods to convert between itk images and DICOM Segmentation objects.
   */
  class ImageSEGConverter : public ConverterBase {

  public:

    ImageSEGConverter();

    /**
     * @brief Converts itk images data into a DICOM Segmentation object.
     *
     * @param dcmDatasets A vector of DICOM datasets with the images that the segmentation is based on.
     * @param segmentations A vector of itk images to be converted.
     * @param metaData A string containing the metadata to be used for the DICOM Segmentation object.
     * @param skipEmptySlices A boolean indicating whether to skip empty slices during the conversion.
     * @return A pointer to the resulting DICOM Segmentation object.
     */
    static DcmDataset* itkimage2dcmSegmentation(vector<DcmDataset*> dcmDatasets,
                          vector<ShortImageType::Pointer> segmentations,
                          const string &metaData,
                          bool skipEmptySlices=true);

    /**
     * @brief Converts a DICOM Segmentation object into a map of itk images and metadata.
     *
     * @param segDataset A pointer to the DICOM Segmentation object to be converted.
     * @param mergeSegments A boolean indicating whether to merge segments during the conversion.
     * @return A pair containing the resulting map of itk images and the metadata.
     */
    pair<map<unsigned, ShortImageType::Pointer>, string> dcmSegmentation2itkimage(DcmDataset *segDataset, const bool mergeSegments);

    /**
     * @brief Converts a DICOM Segmentation object into a map of itk images.
     *
     * @param mergeSegments A boolean indicating whether to merge segments during the conversion.
     * @return A map of itk images resulting from the conversion.
     */

    map<unsigned, ShortImageType::Pointer> dcmSegmentation2itkimage(const bool mergeSegments = false);

    /**
     * @brief Populates the metadata of a DICOM Segmentation object from a DICOM dataset.
     *
     * @param segDataset A pointer to the DICOM dataset containing the metadata.
     */
    void populateMetaInformationFromDICOM(DcmDataset* segDataset);

  protected:

    /**
     * Helper method that uses the OverlapUtil class to retrieve non-overlapping segment
     * groups (by their segment numbers) from the DICOM Segmentation object.
     * If mergeSegments is false, all segments are returned within a single group.
     *
     * @param mergeSegments Whether or not to merge overlapping segments into the same group.
     * @param segmentGroups The resulting segment groups.
     * @return EC_Normal if successful, error otherwise
     */
    OFCondition getNonOverlappingSegmentGroups(const bool mergeSegments,
                                               OverlapUtil::SegmentGroups& segmentGroups);

    OFCondition extractBasicSegmentationInfo();

    ShortImageType::Pointer allocateITKImageTemplate();

    ShortImageType::Pointer allocateITKImageDuplicate(ShortImageType::Pointer imageTemplate);

    OFCondition getITKImageOrigin(const Uint32 frameNo, ShortImageType::PointType& origin);

    OFCondition addSegmentMetadata(const size_t segmentGroup,
                                   const Uint16 segmentNumber);


    OFunique_ptr<DcmSegmentation> m_segDoc;

    ShortImageType::DirectionType m_direction;
    // Spacing and origin
    double m_computedSliceSpacing;
    double m_computedVolumeExtent;
    vnl_vector<double> m_sliceDirection;

    ShortImageType::PointType m_imageOrigin;
    ShortImageType::SpacingType m_imageSpacing;
    ShortImageType::SizeType m_imageSize;
    ShortImageType::RegionType m_imageRegion;

    JSONSegmentationMetaInformationHandler m_metaInfo;

    OverlapUtil m_overlapUtil;

  };

}

#endif //DCMQI_SEGMENTATION_CONVERTER_H
