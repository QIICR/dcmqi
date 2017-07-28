//
// Created by Andrey Fedorov on 3/11/17.
//

#include "dcmqi/SegmentationImageObject.h"


int SegmentationImageObject::initializeFromITK(vector<ShortImageType::Pointer> itkSegmentations,
                                               const string &metaDataStr, vector<DcmDataset *> derivationDatasets,
                                               bool skipEmptySlices) {

  setDerivationDatasets(derivationDatasets);

  sourceRepresentationType = ITK_REPR;

  // TODO: think about the following code. Do all input segmentations have the same extent?
  itkImages = itkSegmentations;

  initializeMetaDataFromString(metaDataStr);

//  if(!metaDataIsComplete()){
//    updateMetaDataFromDICOM(derivationDatasets);
//  }

  initializeVolumeGeometry();

  // NB: the sequence of steps initializing different components of the object parallels that
  //  in the original converter function. It probably makes sense to revisit the sequence
  //  of these steps. It does not necessarily need to happen in this order.
  initializeEquipmentInfo();
  initializeContentIdentification();

//  OFString frameOfRefUID;
//  if(!segdoc->getFrameOfReference().getFrameOfReferenceUID(frameOfRefUID).good()){
//    // TODO: add FoR UID to the metadata JSON and check that before generating one!
//    char frameOfRefUIDchar[128];
//    dcmGenerateUniqueIdentifier(frameOfRefUIDchar, QIICR_UID_ROOT);
//    CHECK_COND(segdoc->getFrameOfReference().setFrameOfReferenceUID(frameOfRefUIDchar));
//  }
//

  createDICOMSegmentation();

  // populate metadata about patient/study, from derivation
  //  datasets or from metadata
  initializeCompositeContext();

  initializeSeriesSpecificAttributes(segmentation->getSeries(),
                                     segmentation->getGeneralImage());

  initializePixelMeasuresFG();
  CHECK_COND(segmentation->addForAllFrames(pixelMeasuresFG));

  initializePlaneOrientationFG();
  CHECK_COND(segmentation->addForAllFrames(planeOrientationPatientFG));

  // Mapping from parametric map volume slices to the DICOM frames
  vector<set<dcmqi::DICOMFrame, dcmqi::DICOMFrame_compare> > slice2frame;

  // TODO: not sure about volumeGeometry since it was created from the first segmentation volume only
  mapVolumeSlicesToDICOMFrames(volumeGeometry, derivationDatasets, slice2frame);

  initializeCommonInstanceReferenceModule(segmentation->getCommonInstanceReference(), slice2frame);

  // NB this assumes all segmentation files have the same dimensions; alternatively, need to
  //   do this operation for each segmentation file
  vector<vector<int> > slice2derimg = getSliceMapForSegmentation2DerivationImage(derivationDatasets,
                                                                                 itkSegmentations[0]);

  //  initializeFrames(slice2derimg);

  return EXIT_SUCCESS;

}

int SegmentationImageObject::initializeEquipmentInfo() {
  if(sourceRepresentationType == ITK_REPR){
    generalEquipmentInfoModule = IODGeneralEquipmentModule::EquipmentInfo(QIICR_MANUFACTURER,
                                                                          QIICR_DEVICE_SERIAL_NUMBER,
                                                                          QIICR_MANUFACTURER_MODEL_NAME,
                                                                          QIICR_SOFTWARE_VERSIONS);
  } else { // DICOM_REPR
  }
  return EXIT_SUCCESS;
}

int SegmentationImageObject::initializeVolumeGeometry() {
  if(sourceRepresentationType == ITK_REPR){

    ShortToDummyCasterType::Pointer caster =
      ShortToDummyCasterType::New();
    // right now assuming that all input segmentations have the same volume extent
    // TODO: make this work when input segmentation have different geometry
    caster->SetInput(itkImages[0]);
    caster->Update();

    MultiframeObject::initializeVolumeGeometryFromITK(caster->GetOutput());
  } else {
    MultiframeObject::initializeVolumeGeometryFromDICOM(segmentation->getFunctionalGroups());
  }
  return EXIT_SUCCESS;
}

int SegmentationImageObject::createDICOMSegmentation() {

  DcmSegmentation::createBinarySegmentation(segmentation,
                                            volumeGeometry.extent[1],
                                            volumeGeometry.extent[0],
                                            generalEquipmentInfoModule,
                                            contentIdentificationMacro);   // content identification

  /* Initialize dimension module */
  IODMultiframeDimensionModule &mfdim = segmentation->getDimensions();
  OFString dimUID = dcmqi::Helper::generateUID();
  CHECK_COND(mfdim.addDimensionIndex(DCM_ReferencedSegmentNumber, dimUID, DCM_SegmentIdentificationSequence,
                                     DcmTag(DCM_ReferencedSegmentNumber).getTagName()));
  CHECK_COND(mfdim.addDimensionIndex(DCM_ImagePositionPatient, dimUID,
                                     DCM_PlanePositionSequence, "ImagePositionPatient"));

  return EXIT_SUCCESS;
}

int SegmentationImageObject::initializeCompositeContext() {
  if(derivationDcmDatasets.size()){
    CHECK_COND(segmentation->import(*derivationDcmDatasets[0],
                                    OFTrue, // Patient
                                    OFTrue, // Study
                                    OFTrue, // Frame of reference
                                    OFFalse)); // Series
  } else {
    // TODO: once we support passing of composite context in metadata, propagate it into segmentation here
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int SegmentationImageObject::initializeFromDICOM(DcmDataset* sourceDataset) {

  dcmRepresentation = sourceDataset;

//  TODO: add SegmentationImageObject to namespace dcmqi
  using namespace dcmqi;

  DcmRLEDecoderRegistration::registerCodecs();

  OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");
  dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);

  OFCondition cond = DcmSegmentation::loadDataset(*sourceDataset, segmentation);
  if(!segmentation){
    cerr << "Failed to load seg! " << cond.text() << endl;
    throw -1;
  }

  initializeVolumeGeometryFromDICOM(segmentation->getFunctionalGroups());
  itkImage = volumeGeometry.getITKRepresentation<ShortImageType>();
  vector< pair<Uint16 , long> > matchedFramesWithSlices = matchFramesWithSegmentIdAndSliceNumber(
    segmentation->getFunctionalGroups());
  unpackFramesAndWriteSegmentImage(matchedFramesWithSlices);
  initializeMetaDataFromDICOM(sourceDataset);

  return EXIT_SUCCESS;
}

vector< pair<Uint16, long> > SegmentationImageObject::matchFramesWithSegmentIdAndSliceNumber(FGInterface &fgInterface) {

  vector< pair<Uint16, long> > matchedFramesWithSlices;

  for(size_t frameId=0;frameId<fgInterface.getNumberOfFrames();frameId++){
    bool isPerFrame;

#ifndef NDEBUG
    FGFrameContent *frameContent =
      OFstatic_cast(FGFrameContent*,fgInterface.get(frameId, DcmFGTypes::EFG_FRAMECONTENT, isPerFrame));
    assert(frameContent);
#endif

    Uint16 segmentId = getSegmentId(fgInterface, frameId);

    // WARNING: this is needed only for David's example, which numbers
    // (incorrectly!) segments starting from 0, should start from 1
    if(segmentId == 0){
      cerr << "Segment numbers should start from 1!" << endl;
      throw -1;
    }

    if(segment2image.find(segmentId) == segment2image.end()) {
      createNewSegmentImage(segmentId);
    }

    FGPlanePosPatient *planposfg =
      OFstatic_cast(FGPlanePosPatient*,fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
    assert(planposfg);

    ShortImageType::PointType frameOriginPoint;
    ShortImageType::IndexType frameOriginIndex;
    for(int j=0;j<3;j++){
      OFString planposStr;
      if(planposfg->getImagePositionPatient(planposStr, j).good()){
        frameOriginPoint[j] = atof(planposStr.c_str());
      }
    }
    clog << "Image size: " << segment2image[segmentId]->GetBufferedRegion().GetSize() << endl;

    if(!segment2image[segmentId]->TransformPhysicalPointToIndex(frameOriginPoint, frameOriginIndex)){
      cerr << "ERROR: Frame " << frameId << " origin " << frameOriginPoint <<
           " is outside image geometry!" << frameOriginIndex << endl;
      cerr << "Image size: " << segment2image[segmentId]->GetBufferedRegion().GetSize() << endl;
      throw -1;
    }
    matchedFramesWithSlices.push_back(make_pair(segmentId, frameOriginIndex[2]));
  }
  return matchedFramesWithSlices;
}

int SegmentationImageObject::unpackFramesAndWriteSegmentImage(
  vector< pair<Uint16 , long> > matchingSegmentIDsAndSliceNumbers) {

  for (std::vector< pair<Uint16 , long> >::iterator it = matchingSegmentIDsAndSliceNumbers.begin();
       it != matchingSegmentIDsAndSliceNumbers.end(); ++it) {

    unsigned frameId = it - matchingSegmentIDsAndSliceNumbers.begin();
    Uint16 segmentId = (*it).first;
    long sliceNumber = (*it).second;

    const DcmIODTypes::Frame *frame = segmentation->getFrame(frameId);

    DcmIODTypes::Frame *unpackedFrame = NULL;

    if (segmentation->getSegmentationType() == DcmSegTypes::ST_BINARY) {
      unpackedFrame = DcmSegUtils::unpackBinaryFrame(frame,
                                                     volumeGeometry.extent[1], // Rows
                                                     volumeGeometry.extent[0]); // Cols
    } else {
      unpackedFrame = new DcmIODTypes::Frame(*frame);
    }

    for (unsigned row = 0; row < volumeGeometry.extent[1]; row++) {
      for (unsigned col = 0; col < volumeGeometry.extent[0]; col++) {
        ShortImageType::PixelType pixel;
        unsigned bitCnt = row * volumeGeometry.extent[0] + col;
        pixel = unpackedFrame->pixData[bitCnt];

        if (pixel != 0) {
          ShortImageType::IndexType index;
          index[0] = col;
          index[1] = row;
          index[2] = sliceNumber;
          segment2image[segmentId]->SetPixel(index, segmentId);
        }
      }
    }

    if (unpackedFrame != NULL)
      delete unpackedFrame;
  }

  return EXIT_SUCCESS;
}

int SegmentationImageObject::createNewSegmentImage(Uint16 segmentId) {
  typedef itk::ImageDuplicator<ShortImageType> DuplicatorType;
  DuplicatorType::Pointer dup = DuplicatorType::New();
  dup->SetInputImage(itkImage);
  dup->Update();
  ShortImageType::Pointer newSegmentImage = dup->GetOutput();
  newSegmentImage->FillBuffer(0);
  segment2image[segmentId] = newSegmentImage;
  return EXIT_SUCCESS;
}

int SegmentationImageObject::initializeMetaDataFromDICOM(DcmDataset *segDataset) {

  OFString temp;
  segmentation->getSeries().getSeriesDescription(temp);
  metaDataJson["SeriesDescription"] = temp.c_str();

  segmentation->getSeries().getSeriesNumber(temp);
  metaDataJson["SeriesNumber"] = temp.c_str();

  segDataset->findAndGetOFString(DCM_InstanceNumber, temp);
  metaDataJson["InstanceNumber"] = temp.c_str();

  segmentation->getSeries().getBodyPartExamined(temp);
  metaDataJson["BodyPartExamined"] = temp.c_str();

  segmentation->getContentIdentification().getContentCreatorName(temp);
  metaDataJson["ContentCreatorName"] = temp.c_str();

  segDataset->findAndGetOFString(DCM_ClinicalTrialTimePointID, temp);
  metaDataJson["ClinicalTrialTimePointID"] = temp.c_str();

  segDataset->findAndGetOFString(DCM_ClinicalTrialSeriesID, temp);
  metaDataJson["ClinicalTrialSeriesID"] = temp.c_str();

  segDataset->findAndGetOFString(DCM_ClinicalTrialCoordinatingCenterName, temp);
  if (temp.size())
    metaDataJson["ClinicalTrialCoordinatingCenterName"] = temp.c_str();

  metaDataJson["segmentAttributes"] = getSegmentAttributesMetadata();

  return EXIT_SUCCESS;
}

Json::Value SegmentationImageObject::getSegmentAttributesMetadata() {

  using namespace dcmqi;

  FGInterface &fgInterface = segmentation->getFunctionalGroups();

  Json::Value values(Json::arrayValue);
  vector<Uint16> processedSegmentIDs;

  for(size_t frameId=0;frameId<fgInterface.getNumberOfFrames();frameId++) {
    Uint16 segmentId = getSegmentId(fgInterface, frameId);

    // populate meta information needed for Slicer ScalarVolumeNode initialization
    //  (for example)

    // NOTE: according to the standard, segment numbering should start from 1,
    //  not clear if this is intentional behavior or a bug in DCMTK expecting
    //  it to start from 0
    DcmSegment *segment = segmentation->getSegment(segmentId);
    if (segment == NULL) {
      cerr << "Failed to get segment for segment ID " << segmentId << endl;
      continue;
    }

    if(std::find(processedSegmentIDs.begin(), processedSegmentIDs.end(), segmentId) != processedSegmentIDs.end()) {
      continue;
    }
    processedSegmentIDs.push_back(segmentId);

    // get CIELab color for the segment
    Uint16 ciedcm[3];
    unsigned cielabScaled[3];
    float cielab[3], ciexyz[3];
    unsigned rgb[3];
    if (segment->getRecommendedDisplayCIELabValue(
      ciedcm[0], ciedcm[1], ciedcm[2]
    ).bad()) {
      // NOTE: if the call above fails, it overwrites the values anyway,
      //  not sure if this is a dcmtk bug or not
      ciedcm[0] = 43803;
      ciedcm[1] = 26565;
      ciedcm[2] = 37722;
      cerr << "Failed to get CIELab values - initializing to default " <<
           ciedcm[0] << "," << ciedcm[1] << "," << ciedcm[2] << endl;
    }
    cielabScaled[0] = unsigned(ciedcm[0]);
    cielabScaled[1] = unsigned(ciedcm[1]);
    cielabScaled[2] = unsigned(ciedcm[2]);

    Helper::getCIELabFromIntegerScaledCIELab(&cielabScaled[0], &cielab[0]);
    Helper::getCIEXYZFromCIELab(&cielab[0], &ciexyz[0]);
    Helper::getRGBFromCIEXYZ(&ciexyz[0], &rgb[0]);

    Json::Value segmentEntry;

    OFString temp;

    segmentEntry["labelID"] = segmentId;

    segment->getSegmentDescription(temp);
    segmentEntry["SegmentDescription"] = temp.c_str();


    DcmSegTypes::E_SegmentAlgoType algorithmType = segment->getSegmentAlgorithmType();
    string readableAlgorithmType = DcmSegTypes::algoType2OFString(algorithmType).c_str();
    segmentEntry["SegmentAlgorithmType"] = readableAlgorithmType;

    if (algorithmType == DcmSegTypes::SAT_UNKNOWN) {
      cerr << "AlgorithmType is not valid with value " << readableAlgorithmType << endl;
      throw -1;
    }
    if (algorithmType != DcmSegTypes::SAT_MANUAL) {
      segment->getSegmentAlgorithmName(temp);
      if (temp.length() > 0)
        segmentEntry["SegmentAlgorithmName"] = temp.c_str();
    }

    Json::Value rgbArray(Json::arrayValue);
    rgbArray.append(rgb[0]);
    rgbArray.append(rgb[1]);
    rgbArray.append(rgb[2]);
    segmentEntry["recommendedDisplayRGBValue"] = rgbArray;

    segmentEntry["SegmentedPropertyCategoryCodeSequence"] =
      Helper::codeSequence2Json(segment->getSegmentedPropertyCategoryCode());

    segmentEntry["SegmentedPropertyTypeCodeSequence"] =
      Helper::codeSequence2Json(segment->getSegmentedPropertyTypeCode());

    if (segment->getSegmentedPropertyTypeModifierCode().size() > 0) {
      segmentEntry["SegmentedPropertyTypeModifierCodeSequence"] =
        Helper::codeSequence2Json(*(segment->getSegmentedPropertyTypeModifierCode())[0]);
    }

    GeneralAnatomyMacro &anatomyMacro = segment->getGeneralAnatomyCode();
    CodeSequenceMacro &anatomicRegionSequence = anatomyMacro.getAnatomicRegion();
    if (anatomicRegionSequence.check(true).good()) {
      segmentEntry["AnatomicRegionSequence"] = Helper::codeSequence2Json(anatomyMacro.getAnatomicRegion());
    }
    if (anatomyMacro.getAnatomicRegionModifier().size() > 0) {
      segmentEntry["AnatomicRegionModifierSequence"] =
        Helper::codeSequence2Json(*(anatomyMacro.getAnatomicRegionModifier()[0]));
    }

    Json::Value innerList(Json::arrayValue);
    innerList.append(segmentEntry);
    values.append(innerList);
  }
  return values;
}

Uint16 SegmentationImageObject::getSegmentId(FGInterface &fgInterface, size_t frameId) const {
  bool isPerFrame;
  FGSegmentation *fgseg =
      OFstatic_cast(FGSegmentation*,fgInterface.get(frameId, DcmFGTypes::EFG_SEGMENTATION, isPerFrame));
    assert(fgseg);

  Uint16 segmentId = -1;
  if(fgseg->getReferencedSegmentNumber(segmentId).bad()){
      cerr << "Failed to get seg number!";
      throw -1;
    }
  return segmentId;
}

// AF: I could not quickly figure out how to template this function over image type - suggestions are welcomed!
vector<vector<int> > SegmentationImageObject::getSliceMapForSegmentation2DerivationImage(const vector<DcmDataset*> dcmDatasets,
                                                                                         const ShortImageType::Pointer &labelImage) {
  // Find mapping from the segmentation slice number to the derivation image
  // Assume that orientation of the segmentation is the same as the source series
  unsigned numLabelSlices = labelImage->GetLargestPossibleRegion().GetSize()[2];
  vector<vector<int> > slice2derimg(numLabelSlices);
  for(size_t i=0;i<dcmDatasets.size();i++){
    OFString ippStr;
    ShortImageType::PointType ippPoint;
    ShortImageType::IndexType ippIndex;
    for(int j=0;j<3;j++){
      CHECK_COND(dcmDatasets[i]->findAndGetOFString(DCM_ImagePositionPatient, ippStr, j));
      ippPoint[j] = atof(ippStr.c_str());
    }
    // NB: this will map slice origin to index without failure, unless the point is out
    //   of FOV bounds!
    // TODO: do a better job matching volume slices by considering comparison of the origin
    //   and orientation of the slice within tolerance
    if(!labelImage->TransformPhysicalPointToIndex(ippPoint, ippIndex)){
      //cout << "image position: " << ippPoint << endl;
      //cerr << "ippIndex: " << ippIndex << endl;
      // if certain DICOM instance does not map to a label slice, just skip it
      continue;
    }
    slice2derimg[ippIndex[2]].push_back(i);
  }
  return slice2derimg;
}