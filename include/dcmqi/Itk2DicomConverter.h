#ifndef DCMQI_ITK2DICOM_CONVERTER_H
#define DCMQI_ITK2DICOM_CONVERTER_H

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


using namespace std;

typedef itk::LabelImageToLabelMapFilter<ShortImageType> LabelToLabelMapFilterType;

namespace dcmqi {

  /**
   * @brief The Itk2DicomConverter class provides methods to convert from itk images to DICOM Segmentation objects.
   */
  class Itk2DicomConverter : public ConverterBase {

  public:

    Itk2DicomConverter();

    /**
     * @brief Converts itk images data into a DICOM Segmentation object.
     *
     * @param dcmDatasets A vector of DICOM datasets with the images that the segmentation is based on.
     * @param segmentations A vector of itk images to be converted.
     * @param metaData A string containing the metadata to be used for the DICOM Segmentation object.
     * @param skipEmptySlices A boolean indicating whether to skip empty slices during the conversion.
     * @param useLabelIDAsSegmentNumber A boolean indicating whether to use input label IDs as segment numbers.
     *        This only works if the label IDs start at 1 and are numbered monotonically without gaps.
     *        The processing order of label IDs is not relevant, i.e. they can occur in any order in the input.
     *        If n labels are not assigned uniquely to label IDs 1..n in the input, the
     *        conversion will fail.
     *        If this is set to false (default), the segment numbers are assigned in the order of the
     *        labels that are being converted, i.e. the first label will receive the Segment
     *        Number 1, the second label will receive the Segment Number 2, etc.
     *        Setting this flag to true is especially useful for track label IDs in the DICOM
     *        result, as done in the tests (e.g. roundtrip testing DICOM -> ITK -> DICOM).
     *        Note that neither the order of frames nor the order of display (Dimension Index Values)
     *        are updated by this flag, compared to the default behavior (false). Of course, the
     *        resulting DICOM object is still valid, dimensions are consistent and lead to meaningful
     *        display.
     * @param referencesGeometryCheck A boolean indicating whether the conversion process should attempt checking if the geometry of the referenced DICOM images is consistent with the corresponding slices of the segmentation. 
     *        By default, this check is enabled. If disabled, all of the references will be 
     *        added in the SharedFunctionalGroupsSequence without any geometry checks.
     * @return A pointer to the resulting DICOM Segmentation object.
     */
    static DcmDataset* itkimage2dcmSegmentation(vector<DcmDataset*> dcmDatasets,
                          vector<ShortImageType::ConstPointer> segmentations,
                          const string &metaData,
                          bool skipEmptySlices=true,
                          bool useLabelIDAsSegmentNumber=false,
                          bool referencesGeometryCheck=true);

  protected:

    /** This method takes an existing DICOM segmentation dataset and a mapping from
     *  (existing) segment number to new segment number (i.e. original label ID).
     *  Therefore it will go through all frames in the dataset, and for each frame
     *  maps Referenced Segment Number to the new value.
     *  Afterwards, it goes trough the Segment Sequence, arranging Segment (i.e. items)
     *  so that the  first Segment has the number 1, second number 2, and so on.
     *  There is no reordering of frames, i.e. only the meta data is adapted.
     *  @param  dset DICOM dataset to be modified, must be a DICOM Segmentation object
     *  @param  segNum2Label mapping from segment number (old) to label ID (new)
     *  @return true if successful, false otherwise
     */
    static bool mapLabelIDsToSegmentNumbers(DcmDataset* dset, map<Uint16, Uint16> segNum2Label);

    /** Check whether labels (values in given map) are unique and monotonically increasing by 1
     *  @param  segNum2Label mapping from segment number (old) to label ID (new)
     *  @return true if successful, false otherwise
     */
    static bool checkLabelNumbering(const map<Uint16, Uint16>& segNum2Label);

  };

}

#endif //DCMQI_ITK2DICOM_CONVERTER_H
