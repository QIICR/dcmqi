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
     * @param sortByLabelID A boolean indicating whether to sort the segments by label ID.
     *        In this case, the label IDs are used as DICOM segment numbers. This only works
     *        if the label IDs start at 1 and numbered monotonically without gaps. The processing
     *        order of label IDs are not relevant, i.e. they can occur in any order int he input.
     *        If n labels are not assigned uniquely to label IDs 1..n in the input, the
     *        conversion will fail.
     *        If this is set to false (default), the segment numbers are assigned in the order of the
     *        labels that are being converted, i.e. the first label will receive the Segment
     *        Number 1, the second label will receive the Segment Number 2, etc.
     * @return A pointer to the resulting DICOM Segmentation object.
     */
    static DcmDataset* itkimage2dcmSegmentation(vector<DcmDataset*> dcmDatasets,
                          vector<ShortImageType::Pointer> segmentations,
                          const string &metaData,
                          bool skipEmptySlices=true,
                          bool sortByLabelID=false);

  protected:

    static void sortByLabel(DcmDataset* dset, map<Uint16, Uint16> segNum2Label);

  };

}

#endif //DCMQI_ITK2DICOM_CONVERTER_H
