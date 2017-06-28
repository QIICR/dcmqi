//
// Created by Andrey Fedorov on 3/11/17.
//

#ifndef DCMQI_MULTIFRAMEOBJECT_H
#define DCMQI_MULTIFRAMEOBJECT_H

#include <dcmtk/dcmfg/fginterface.h>
#include <dcmtk/dcmfg/fgpixmsr.h>
#include <dcmtk/dcmfg/fgplanor.h>
#include <dcmtk/dcmfg/fgplanpo.h>
#include <dcmtk/dcmfg/fgfracon.h>
#include <dcmtk/dcmfg/fgderimg.h>
#include <dcmtk/dcmiod/modfloatingpointimagepixel.h>

#include <itkImage.h>
#include <json/json.h>

#include <vector>
#include "vnl/vnl_cross.h"

#include <dcmtk/dcmiod/modequipment.h>
#include <dcmtk/dcmiod/modenhequipment.h>
#include <dcmtk/dcmiod/modmultiframedimension.h>
#include <dcmtk/dcmiod/iodcontentitemmacro.h>
#include <dcmtk/dcmiod/modcommoninstanceref.h>

#include "dcmqi/Exceptions.h"
#include "dcmqi/ImageVolumeGeometry.h"
#include "dcmqi/DICOMFrame.h"

using namespace std;
/*
 * Class to support conversion between DICOM and ITK representations
 * of multiframe objects
 *
 *
 */
class MultiframeObject {

public:

  // initialization from DICOM is always from the dataset(s)
  //  that encode DICOM representation, optionally augmented by the
  //  derivation datasets that potentially can be helpful in some
  //  situations.
  int initializeFromDICOM(std::vector<DcmDataset*> sourceDataset);
  int initializeFromDICOM(std::vector<DcmDataset*> sourceDatasets,
                          std::vector<DcmDataset*> derivationDatasets);

  // initialization from ITK will be specific to the derived types,
  //  although there will be some common initialization of the metadata

  // Output is always a single DcmDataset, since this is a multiframe
  //  object
  DcmDataset* getDcmDataset() const;
  Json::Value getMetaDataJson() const {
    return metaDataJson;
  };

  // get ITK representation will be specific to the derived classes,
  //  since the type of the ITK image and the number of ITK images is
  //  different between PM and SEG

protected:

  // Helpers to convert to dummy image type to support common
  //  implementation of the image volume geometry
  typedef unsigned char DummyPixelType;
  typedef itk::Image<DummyPixelType, 3> DummyImageType;
  typedef DummyImageType::PointType PointType;
  typedef DummyImageType::DirectionType DirectionType;
  typedef DummyImageType::SpacingType SpacingType;
  typedef DummyImageType::SizeType SizeType;

  int initializeMetaDataFromString(const std::string&);
  // what this function does depends on whether we are coming from
  //  DICOM or from ITK. No parameters, since all it does is exchange
  //  between DICOM and MetaData
  int initializeEquipmentInfo();
  int initializeContentIdentification();

  // from ITK
  int initializeVolumeGeometryFromITK(DummyImageType::Pointer);

  template <typename T>
  int initializeVolumeGeometryFromDICOM(T iodImage, DcmDataset *dataset) {
    SpacingType spacing;
    PointType origin;
    DirectionType directions;
    SizeType extent;

    FGInterface &fgInterface = iodImage->getFunctionalGroups();

    if (getImageDirections(fgInterface, directions)) {
      cerr << "Failed to get image directions" << endl;
      throw -1;
    }

    cout << directions << endl;

    double computedSliceSpacing, computedVolumeExtent;
    vnl_vector<double> sliceDirection(3);
    sliceDirection[0] = *directions[0];
    sliceDirection[1] = *directions[1];
    sliceDirection[2] = *directions[2];
    if (computeVolumeExtent(fgInterface, sliceDirection, origin, computedSliceSpacing, computedVolumeExtent)) {
      cerr << "Failed to compute origin and/or slice spacing!" << endl;
      throw -1;
    }

    if (getDeclaredImageSpacing(fgInterface, spacing)) {
      cerr << "Failed to get image spacing from DICOM!" << endl;
      throw -1;
    }

    const double tolerance = 1e-5;
    if(!spacing[2]){
      spacing[2] = computedSliceSpacing;
    } else if(fabs(spacing[2]-computedSliceSpacing)>tolerance){
      cerr << "WARNING: Declared slice spacing is significantly different from the one declared in DICOM!" <<
           " Declared = " << spacing[2] << " Computed = " << computedSliceSpacing << endl;
    }

    // Region size
    {
      OFString str;
      if(dataset->findAndGetOFString(DCM_Rows, str).good())
        extent[1] = atoi(str.c_str());
      if(dataset->findAndGetOFString(DCM_Columns, str).good())
        extent[0] = atoi(str.c_str());
    }
    extent[2] = fgInterface.getNumberOfFrames();

    volumeGeometry.setSpacing(spacing);
    volumeGeometry.setOrigin(origin);
    volumeGeometry.setExtent(extent);
    volumeGeometry.setDirections(directions);

    return EXIT_SUCCESS;
  }

  int getImageDirections(FGInterface& fgInterface, DirectionType &dir);

  int computeVolumeExtent(FGInterface& fgInterface, vnl_vector<double> &sliceDirection, PointType &imageOrigin,
                          double &sliceSpacing, double &sliceExtent);
  int getDeclaredImageSpacing(FGInterface &fgInterface, SpacingType &spacing);

  // initialize attributes of the composite context that are common for all multiframe objects
  //virtual int initializeCompositeContext();
  // check whether all of the attributes required for initialization of the object are present in the
  //   input metadata
  virtual bool metaDataIsComplete();

  ContentItemMacro* initializeContentItemMacro(CodeSequenceMacro, CodeSequenceMacro);

  // List of tags, and FGs they belong to, for initializing dimensions module
  int initializeDimensions(std::vector<std::pair<DcmTag, DcmTag> >);
  int initializePixelMeasuresFG();
  int initializePlaneOrientationFG();
  int initializeCommonInstanceReferenceModule(IODCommonInstanceReferenceModule &, vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> >&);

  int mapVolumeSlicesToDICOMFrames(ImageVolumeGeometry&, const vector<DcmDataset*>,
                                          vector<set<dcmqi::DICOMFrame, dcmqi::DICOMFrame_compare> >&);

  static std::vector<int> findIntersectingSlices(ImageVolumeGeometry& volume,
                                    dcmqi::DICOMFrame& frame);

  void insertDerivationSeriesInstance(string seriesUID, string instanceUID);

  // constants to describe original representation of the data being converted
  enum {
    DICOM_REPR = 0,
    ITK_REPR
  };
  int sourceRepresentationType;

  // TODO: abstract this into a different class, which would help with:
  //  - checking for presence of attributes
  //  - handling of defaults (in the future, initialized from the schema?)
  //  - simplifying common access patterns (access to the Code tuples)
  Json::Value metaDataJson;

  // Multiframe DICOM object representation
  // probably not needed, since need object-specific DCMTK class in
  // derived classes
  DcmDataset* dcmRepresentation;

  // probably not needed at this level, since for SEG each segment will
  // have separate geometry definition
  ImageVolumeGeometry volumeGeometry;

  // DcmDataset(s) that hold the original representation of the
  //  object, when the sourceRepresentationType == DICOM_REPR
  OFVector<DcmDataset*> sourceDcmDatasets;

  // Common components present in the derived classes
  // TODO: check whether both PM and SEG use Enh module or not, refactor based on that
  IODEnhGeneralEquipmentModule::EquipmentInfo equipmentInfoModule;
  ContentIdentificationMacro contentIdentificationMacro;
  IODMultiframeDimensionModule dimensionsModule;

  // DcmDataset(s) that were used to derive the object
  //  Probably will only be populated when sourceRepresentationType == ITK_REPR
  // Purpose of those:
  //  1) initialize derivation derivationImageFG (ITK->)
  //  2) initialize CommonInstanceReferenceModule (ITK->)
  //  3) initialize common attributes (ITK->)
  OFVector<DcmDataset*> derivationDcmDatasets;

  // Functional groups common to all MF objects:
  //  - Shared
  FGPixelMeasures pixelMeasuresFG;
  FGPlaneOrientationPatient planeOrientationPatientFG;
  //  - Per-frame
  OFVector<FGPlanePosPatient> planePosPatientFGList;
  OFVector<FGFrameContent> frameContentFGList;
  OFVector<FGDerivationImage> derivationImageFGList;

  // Mapping from the derivation items SeriesUIDs to InstanceUIDs
  std::map<std::string, std::set<std::string> > derivationSeriesToInstanceUIDs;

};


#endif //DCMQI_MULTIFRAMEOBJECT_H
