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
    equipmentInfoModule = IODEnhGeneralEquipmentModule::EquipmentInfo(QIICR_MANUFACTURER, QIICR_DEVICE_SERIAL_NUMBER,
                                                                      QIICR_MANUFACTURER_MODEL_NAME, QIICR_SOFTWARE_VERSIONS);
    /*
    equipmentInfoModule.m_Manufacturer = QIICR_MANUFACTURER;
    equipmentInfoModule.m_DeviceSerialNumber = QIICR_DEVICE_SERIAL_NUMBER;
    equipmentInfoModule.m_ManufacturerModelName = QIICR_MANUFACTURER_MODEL_NAME;
    equipmentInfoModule.m_SoftwareVersions = QIICR_SOFTWARE_VERSIONS;
    */

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
    if(metaDataJson.isMember("InstanceNumber")){
      CHECK_COND(contentIdentificationMacro.setInstanceNumber(metaDataJson["InstanceNumber"].asCString()));
    } else {
      CHECK_COND(contentIdentificationMacro.setInstanceNumber("1"));
    }
    CHECK_COND(contentIdentificationMacro.check())
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
// TODO: this function assumes that all of the derivation datasets are images, which is probably ok
int MultiframeObject::mapVolumeSlicesToDICOMFrames(ImageVolumeGeometry& volume,
                                                   const vector<DcmDataset*> dcmDatasets,
                                                   vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> > &slice2frame){
  for(int d=0;d<dcmDatasets.size();d++){
    Uint32 numFrames;
    DcmDataset* dcm = dcmDatasets[d];
    if(dcm->findAndGetUint32(DCM_NumberOfFrames, numFrames).good()){
      // this is a multiframe object
      for(int f=0;f<numFrames;f++){
        dcmqi::DICOMFrame frame(dcm,f+1);
        vector<int> intersectingSlices = findIntersectingSlices(volume, frame);

        for(int s=0;s<intersectingSlices.size();s++) {
          slice2frame[s].insert(frame);
          if (!s)
            this->insertDerivationSeriesInstance(frame.getSeriesUID(), frame.getInstanceUID());
        }
      }
    } else {
      dcmqi::DICOMFrame frame(dcm);
      vector<int> intersectingSlices = findIntersectingSlices(volume, frame);

      for(int s=0;s<intersectingSlices.size();s++) {
        slice2frame[s].insert(frame);
        if (!s)
          this->insertDerivationSeriesInstance(frame.getSeriesUID(), frame.getInstanceUID());
      }
    }
  }

  return EXIT_SUCCESS;
}

std::vector<int> MultiframeObject::findIntersectingSlices(ImageVolumeGeometry &volume, dcmqi::DICOMFrame &frame) {
  std::vector<int> intersectingSlices;
  // for now, adopt a simple strategy that maps origin of the frame to index, and selects the slice corresponding
  //  to this index as the intersecting one
  ImageVolumeGeometry::DummyImageType::Pointer itkvolume = volume.getITKRepresentation<ImageVolumeGeometry::DummyImageType>();
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

void MultiframeObject::insertDerivationSeriesInstance(string seriesUID, string instanceUID){
  if(derivationSeriesToInstanceUIDs.find(seriesUID) == derivationSeriesToInstanceUIDs.end())
    derivationSeriesToInstanceUIDs[seriesUID] = std::set<string>();
  derivationSeriesToInstanceUIDs[seriesUID].insert(instanceUID);
}

int MultiframeObject::initializeCommonInstanceReferenceModule(IODCommonInstanceReferenceModule &commref, vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> > &slice2frame){

  // map individual series UIDs to the list of instance UIDs - we need to have this organization
  // to populate Common instance reference module
  std::map<std::string, std::set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> > series2frame;
  for(int slice=0;slice!=slice2frame.size();slice++){
    for(set<dcmqi::DICOMFrame, dcmqi::DICOMFrame_compare>::const_iterator frameI=slice2frame[slice].begin();
        frameI!=slice2frame[slice].end();++frameI){
      dcmqi::DICOMFrame frame = *frameI;
      if(series2frame.find(frame.getSeriesUID()) == series2frame.end()){
        std::set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> setOfInstances;
        setOfInstances.insert(frame);
        series2frame[frame.getSeriesUID()] = setOfInstances;
      } else {
        series2frame[frame.getSeriesUID()].insert(frame);
      }
    }
  }

  // create a new ReferencedSeriesItem for each series, and populate with instances
  OFVector<IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*> &refseries = commref.getReferencedSeriesItems();
  for(std::map<std::string, std::set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> >::const_iterator mIt=series2frame.begin(); mIt!=series2frame.end();++mIt){
    IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem* refseriesItem = new IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem;
    refseriesItem->setSeriesInstanceUID(mIt->first.c_str());
    OFVector<SOPInstanceReferenceMacro*> &refinstances = refseriesItem->getReferencedInstanceItems();

    for(std::set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare>::const_iterator sIt=mIt->second.begin();sIt!=mIt->second.end();++sIt){
      dcmqi::DICOMFrame frame = *sIt;
      SOPInstanceReferenceMacro *refinstancesItem = new SOPInstanceReferenceMacro();
      CHECK_COND(refinstancesItem->setReferencedSOPClassUID(frame.getClassUID().c_str()));
      CHECK_COND(refinstancesItem->setReferencedSOPInstanceUID(frame.getInstanceUID().c_str()));
      refinstances.push_back(refinstancesItem);
    }
  }

  return EXIT_SUCCESS;
}

int MultiframeObject::getImageDirections(FGInterface& fgInterface, DirectionType &dir){
  // TODO: handle the situation when FoR is not initialized
  OFBool isPerFrame;
  vnl_vector<double> rowDirection(3), colDirection(3);

  FGPlaneOrientationPatient *planorfg = OFstatic_cast(FGPlaneOrientationPatient*,
                                                      fgInterface.get(0, DcmFGTypes::EFG_PLANEORIENTPATIENT, isPerFrame));
  if(!planorfg){
    cerr << "Plane Orientation (Patient) is missing, cannot parse input " << endl;
    return EXIT_FAILURE;
  }
  OFString orientStr;
  for(int i=0;i<3;i++){
    if(planorfg->getImageOrientationPatient(orientStr, i).good()){
      rowDirection[i] = atof(orientStr.c_str());
    } else {
      cerr << "Failed to get orientation " << i << endl;
      return EXIT_FAILURE;
    }
  }
  for(int i=3;i<6;i++){
    if(planorfg->getImageOrientationPatient(orientStr, i).good()){
      colDirection[i-3] = atof(orientStr.c_str());
    } else {
      cerr << "Failed to get orientation " << i << endl;
      return EXIT_FAILURE;
    }
  }
  vnl_vector<double> sliceDirection = vnl_cross_3d(rowDirection, colDirection);
  sliceDirection.normalize();

  cout << "Row direction: " << rowDirection << endl;
  cout << "Col direction: " << colDirection << endl;

  for(int i=0;i<3;i++){
    dir[i][0] = rowDirection[i];
    dir[i][1] = colDirection[i];
    dir[i][2] = sliceDirection[i];
  }

  cout << "Z direction: " << sliceDirection << endl;

  return 0;
}

int MultiframeObject::computeVolumeExtent(FGInterface& fgInterface, vnl_vector<double> &sliceDirection,
                                          PointType &imageOrigin, double &sliceSpacing, double &sliceExtent) {
  // Size
  // Rows/Columns can be read directly from the respective attributes
  // For number of slices, consider that all segments must have the same number of frames.
  //   If we have FoR UID initialized, this means every segment should also have Plane
  //   Position (Patient) initialized. So we can get the number of slices by looking
  //   how many per-frame functional groups a segment has.

  vector<double> originDistances;
  map<OFString, double> originStr2distance;
  map<OFString, unsigned> frame2overlap;
  double minDistance = 0.0;

  sliceSpacing = 0;

  unsigned numFrames = fgInterface.getNumberOfFrames();

  // Determine ordering of the frames, keep mapping from ImagePositionPatient string
  //   to the distance, and keep track (just out of curiosity) how many frames overlap
  vnl_vector<double> refOrigin(3);
  {
    OFBool isPerFrame;
    FGPlanePosPatient *planposfg = OFstatic_cast(FGPlanePosPatient*,
                                                 fgInterface.get(0, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
    for(int j=0;j<3;j++){
      OFString planposStr;
      if(planposfg->getImagePositionPatient(planposStr, j).good()){
        refOrigin[j] = atof(planposStr.c_str());
      } else {
        cerr << "Failed to read patient position" << endl;
      }
    }
  }

  for(size_t frameId=0;frameId<numFrames;frameId++){
    OFBool isPerFrame;
    FGPlanePosPatient *planposfg = OFstatic_cast(FGPlanePosPatient*,
                                                 fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));

    if(!planposfg){
      cerr << "PlanePositionPatient is missing" << endl;
      return EXIT_FAILURE;
    }

    if(!isPerFrame){
      cerr << "PlanePositionPatient is required for each frame!" << endl;
      return EXIT_FAILURE;
    }

    vnl_vector<double> sOrigin;
    OFString sOriginStr = "";
    sOrigin.set_size(3);
    for(int j=0;j<3;j++){
      OFString planposStr;
      if(planposfg->getImagePositionPatient(planposStr, j).good()){
        sOrigin[j] = atof(planposStr.c_str());
        sOriginStr += planposStr;
        if(j<2)
          sOriginStr+='/';
      } else {
        cerr << "Failed to read patient position" << endl;
        return EXIT_FAILURE;
      }
    }

    // check if this frame has already been encountered
    if(originStr2distance.find(sOriginStr) == originStr2distance.end()){
      vnl_vector<double> difference;
      difference.set_size(3);
      difference[0] = sOrigin[0]-refOrigin[0];
      difference[1] = sOrigin[1]-refOrigin[1];
      difference[2] = sOrigin[2]-refOrigin[2];
      double dist = dot_product(difference,sliceDirection);
      frame2overlap[sOriginStr] = 1;
      originStr2distance[sOriginStr] = dist;
      assert(originStr2distance.find(sOriginStr) != originStr2distance.end());
      originDistances.push_back(dist);

      if(frameId==0){
        minDistance = dist;
        imageOrigin[0] = sOrigin[0];
        imageOrigin[1] = sOrigin[1];
        imageOrigin[2] = sOrigin[2];
      }
      else if(dist<minDistance){
        imageOrigin[0] = sOrigin[0];
        imageOrigin[1] = sOrigin[1];
        imageOrigin[2] = sOrigin[2];
        minDistance = dist;
      }
    } else {
      frame2overlap[sOriginStr]++;
    }
  }

  // it IS possible to have a segmentation object containing just one frame!
  if(numFrames>1){
    // WARNING: this should be improved further. Spacing should be calculated for
    //  consecutive frames of the individual segment. Right now, all frames are considered
    //  indiscriminately. Question is whether it should be computed at all, considering we do
    //  not have any information about whether the 2 frames are adjacent or not, so perhaps we should
    //  always rely on the declared spacing, and not even try to compute it?
    // TODO: discuss this with the QIICR team!

    // sort all unique distances, this will be used to check consistency of
    //  slice spacing, and also to locate the slice position from ImagePositionPatient
    //  later when we read the segments
    sort(originDistances.begin(), originDistances.end());

    sliceSpacing = fabs(originDistances[0]-originDistances[1]);

    for(size_t i=1;i<originDistances.size(); i++){
      float dist1 = fabs(originDistances[i-1]-originDistances[i]);
      float delta = sliceSpacing-dist1;
    }

    sliceExtent = fabs(originDistances[0]-originDistances[originDistances.size()-1]);
    unsigned overlappingFramesCnt = 0;
    for(map<OFString, unsigned>::const_iterator it=frame2overlap.begin();
        it!=frame2overlap.end();++it){
      if(it->second>1)
        overlappingFramesCnt++;
    }

    cout << "Total frames: " << numFrames << endl;
    cout << "Total frames with unique IPP: " << originDistances.size() << endl;
    cout << "Total overlapping frames: " << overlappingFramesCnt << endl;
    cout << "Origin: " << imageOrigin << endl;
  }

  return EXIT_SUCCESS;
}

int MultiframeObject::getDeclaredImageSpacing(FGInterface &fgInterface, SpacingType &spacing){
  OFBool isPerFrame;
  FGPixelMeasures *pixelMeasures = OFstatic_cast(FGPixelMeasures*,
                                                 fgInterface.get(0, DcmFGTypes::EFG_PIXELMEASURES, isPerFrame));
  if(!pixelMeasures){
    cerr << "Pixel measures FG is missing!" << endl;
    return EXIT_FAILURE;
  }

  pixelMeasures->getPixelSpacing(spacing[0], 0);
  pixelMeasures->getPixelSpacing(spacing[1], 1);

  Float64 spacingFloat;
  if(pixelMeasures->getSpacingBetweenSlices(spacingFloat,0).good() && spacingFloat != 0){
    spacing[2] = spacingFloat;
  } else if(pixelMeasures->getSliceThickness(spacingFloat,0).good() && spacingFloat != 0){
    // SliceThickness can be carried forward from the source images, and may not be what we need
    // As an example, this ePAD example has 1.25 carried from CT, but true computed thickness is 1!
    cerr << "WARNING: SliceThickness is present and is " << spacingFloat << ". using it!" << endl;
    spacing[2] = spacingFloat;
  }
  return EXIT_SUCCESS;
}