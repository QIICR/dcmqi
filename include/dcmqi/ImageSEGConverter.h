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
#include "dcmqi/ConverterBase.h"
#include "dcmqi/JSONSegmentationMetaInformationHandler.h"

using namespace std;


typedef itk::LabelImageToLabelMapFilter<ShortImageType> LabelToLabelMapFilterType;

namespace dcmqi {

  /**
   * @brief The ImageSEGConverter class provides methods to convert between itk images and DICOM Segmentation objects.
   */
  class ImageSEGConverter : public ConverterBase {

  public:

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
    static pair<map<unsigned, ShortImageType::Pointer>, string> dcmSegmentation2itkimage(DcmDataset *segDataset,
      bool mergeSegments);

    /**
     * @brief Converts a DICOM Segmentation object into a map of itk images.
     *
     * @param segdoc A pointer to the DICOM Segmentation object to be converted.
     * @param metaInfo A pointer to the JSONSegmentationMetaInformationHandler object containing the metadata.
     * @param mergeSegments A boolean indicating whether to merge segments during the conversion.
     * @return A map of itk images resulting from the conversion.
     */
    static map<unsigned, ShortImageType::Pointer> dcmSegmentation2itkimage(
      DcmSegmentation *segdoc,
      JSONSegmentationMetaInformationHandler *metaInfo=NULL,
      bool mergeSegments = false);

    /**
     * @brief Populates the metadata of a DICOM Segmentation object from a DICOM dataset.
     *
     * @param segDataset A pointer to the DICOM dataset containing the metadata.
     * @param segdoc A pointer to the DICOM Segmentation object to be populated.
     * @param metaInfo A reference to the JSONSegmentationMetaInformationHandler object to be populated.
     */
    static void populateMetaInformationFromDICOM(DcmDataset *segDataset, DcmSegmentation *segdoc,
                           JSONSegmentationMetaInformationHandler &metaInfo);
  };

}

#endif //DCMQI_SEGMENTATION_CONVERTER_H
