//
// Created by Andrey Fedorov on 3/11/17.
//

#include <dcmqi/Helper.h>
#include "dcmqi/QIICRConstants.h"
#include "dcmqi/MultiframeObject.h"

int MultiframeObject::initializeFromDICOM(std::vector<DcmDataset *> sourceDataset) {
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeMetaDataFromString(const std::string &metaDataStr) {
  std::istringstream metaDataStream(metaDataStr);
  metaDataStream >> metaDataJson;
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeEquipmentInfo() {
  if(sourceRepresentationType == ITK_REPR){
    equipmentInfoModule.m_Manufacturer = QIICR_MANUFACTURER;
    equipmentInfoModule.m_DeviceSerialNumber = QIICR_DEVICE_SERIAL_NUMBER;
    equipmentInfoModule.m_ManufacturerModelName = QIICR_MANUFACTURER_MODEL_NAME;
    equipmentInfoModule.m_SoftwareVersions = QIICR_SOFTWARE_VERSIONS;
  } else { // DICOM_REPR
  }
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeContentIdentification() {
  if(sourceRepresentationType == ITK_REPR){
    CHECK_COND(contentIdentificationMacro.setContentCreatorName("dcmqi"));
    if(metaDataJson.isMember("ContentDescription")){
      CHECK_COND(contentIdentificationMacro.setContentDescription(metaDataJson["ContentDescription"].asCString()));
    } else {
      CHECK_COND(contentIdentificationMacro.setContentDescription("DCMQI"));
    }
    if(metaDataJson.isMember("ContentLabel")){
      CHECK_COND(contentIdentificationMacro.setContentLabel(metaDataJson["ContentLabel"].asCString()));
    } else {
      CHECK_COND(contentIdentificationMacro.setContentLabel("DCMQI"));
    }
    return EXIT_SUCCESS;
  } else { // DICOM_REPR
  }
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeVolumeGeometryFromITK(DummyImageType::Pointer image) {
  DummyImageType::SpacingType spacing;
  DummyImageType::PointType origin;
  DummyImageType::DirectionType directions;
  DummyImageType::SizeType extent;

  spacing = image->GetSpacing();
  directions = image->GetDirection();
  extent = image->GetLargestPossibleRegion().GetSize();

  volumeGeometry.setSpacing(spacing);
  volumeGeometry.setOrigin(origin);
  volumeGeometry.setExtent(extent);

  volumeGeometry.setDirections(directions);

  return EXIT_SUCCESS;
}

// for now, we do not support initialization from JSON only,
//  and we don't have a way to validate metadata completeness - TODO!
bool MultiframeObject::metaDataIsComplete() {
  return false;
}

// List of tags, and FGs they belong to, for initializing dimensions module
int MultiframeObject::initializeDimensions(std::vector<std::pair<DcmTag,DcmTag> > dimTagList){
  OFString dimUID;

  dimensionsModule.clearData();

  dimUID = dcmqi::Helper::generateUID();
  for(int i=0;i<dimTagList.size();i++){
    std::pair<DcmTag,DcmTag> dimTagPair = dimTagList[i];
    CHECK_COND(dimensionsModule.addDimensionIndex(dimTagPair.first, dimUID, dimTagPair.second,
    dimTagPair.first.getTagName()));
  }
  return EXIT_SUCCESS;
}

int MultiframeObject::initializePixelMeasuresFG(){
  string pixelSpacingStr, sliceSpacingStr;

  pixelSpacingStr = dcmqi::Helper::floatToStrScientific(volumeGeometry.spacing[0])+
      "\\"+dcmqi::Helper::floatToStrScientific(volumeGeometry.spacing[1]);
  sliceSpacingStr = dcmqi::Helper::floatToStrScientific(volumeGeometry.spacing[2]);

  CHECK_COND(pixelMeasuresFG.setPixelSpacing(pixelSpacingStr.c_str()));
  CHECK_COND(pixelMeasuresFG.setSpacingBetweenSlices(sliceSpacingStr.c_str()));
  CHECK_COND(pixelMeasuresFG.setSliceThickness(sliceSpacingStr.c_str()));

  return EXIT_SUCCESS;
}

int MultiframeObject::initializePlaneOrientationFG() {
  planeOrientationPatientFG.setImageOrientationPatient(
      dcmqi::Helper::floatToStrScientific(volumeGeometry.rowDirection[0]).c_str(),
      dcmqi::Helper::floatToStrScientific(volumeGeometry.rowDirection[1]).c_str(),
      dcmqi::Helper::floatToStrScientific(volumeGeometry.rowDirection[2]).c_str(),
      dcmqi::Helper::floatToStrScientific(volumeGeometry.columnDirection[0]).c_str(),
      dcmqi::Helper::floatToStrScientific(volumeGeometry.columnDirection[1]).c_str(),
      dcmqi::Helper::floatToStrScientific(volumeGeometry.columnDirection[2]).c_str()
  );
  return EXIT_SUCCESS;
}

ContentItemMacro* MultiframeObject::initializeContentItemMacro(CodeSequenceMacro conceptName,
                                                               CodeSequenceMacro conceptCode){
  ContentItemMacro* item = new ContentItemMacro();
  CodeSequenceMacro* concept = new CodeSequenceMacro(conceptName);
  CodeSequenceMacro* value = new CodeSequenceMacro(conceptCode);

  if (!item || !concept || !value)
  {
    return NULL;
  }

  item->getEntireConceptNameCodeSequence().push_back(concept);
  item->getEntireConceptCodeSequence().push_back(value);
  item->setValueType(ContentItemMacro::VT_CODE);

  return EXIT_SUCCESS;
}

// populates slice2frame vector that maps each of the volume slices to the set of frames that
// are considered as derivation dataset
int MultiframeObject::mapVolumeSlicesToDICOMFrames(ImageVolumeGeometry& volume,
                                                   const vector<DcmDataset*> dcmDatasets,
                                                   vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> > slice2frame){
  for(int d=0;d<dcmDatasets.size();d++){
    Uint32 numFrames;
    DcmDataset* dcm = dcmDatasets[d];
    if(dcm->findAndGetUint32(DCM_NumberOfFrames, numFrames).good()){
      // this is a multiframe object
      for(int f=0;f<numFrames;f++){
        dcmqi::DICOMFrame frame(dcm,f+1);
        vector<int> intersectingSlices = findIntersectingSlices(volume, frame);

        for(int s=0;s<intersectingSlices.size();s++)
          slice2frame[s].insert(frame);

      }
    } else {
      dcmqi::DICOMFrame frame(dcm);
      vector<int> intersectingSlices = findIntersectingSlices(volume, frame);

      for(int s=0;s<intersectingSlices.size();s++)
        slice2frame[s].insert(frame);

    }
  }

  return EXIT_SUCCESS;
}

std::vector<int> MultiframeObject::findIntersectingSlices(ImageVolumeGeometry &volume, dcmqi::DICOMFrame &frame) {
  std::vector<int> intersectingSlices;
  // for now, adopt a simple strategy that maps origin of the frame to index, and selects the slice corresponding
  //  to this index as the intersecting one
  ImageVolumeGeometry::DummyImageType::Pointer itkvolume = volume.getITKRepresentation();
  ImageVolumeGeometry::DummyImageType::PointType point;
  ImageVolumeGeometry::DummyImageType::IndexType index;
  vnl_vector<double> frameIPP = frame.getFrameIPP();
  point[0] = frameIPP[0];
  point[1] = frameIPP[1];
  point[2] = frameIPP[2];

  if(itkvolume->TransformPhysicalPointToIndex(point, index))
    intersectingSlices.push_back(index[2]);

  return intersectingSlices;
}
