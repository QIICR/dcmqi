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

  vector<set<dcmqi::DICOMFrame, dcmqi::DICOMFrame_compare> > slice2frame;
  mapVolumeSlicesToDICOMFrames(derivationDatasets, slice2frame);

  // NB: this assumes all segmentation files have the same dimensions; alternatively, need to
  //    do this operation for each segmentation file
  vector<vector<int> > slice2derimg = getSliceMapForSegmentation2DerivationImage<ShortImageType>(derivationDatasets,
                                                                                                 itkSegmentations[0]);

  initializeCommonInstanceReferenceModule(segmentation->getCommonInstanceReference(), slice2frame);

  // initial frames ...

  FGPlanePosPatient* fgppp = FGPlanePosPatient::createMinimal("1","1","1");
  FGFrameContent* fgfc = new FGFrameContent();
  FGDerivationImage* fgder = new FGDerivationImage();
  OFVector<FGBase*> perFrameFGs;

  perFrameFGs.push_back(fgppp);
  perFrameFGs.push_back(fgfc);
  if(hasDerivationImages(slice2derimg))
    perFrameFGs.push_back(fgder);

  int uidfound = 0, uidnotfound = 0;
  const unsigned frameSize = volumeGeometry.extent[0] * volumeGeometry.extent[1];
  Uint8 *frameData = new Uint8[frameSize];

//  for(size_t segFileNumber=0; segFileNumber<itkImages.size(); segFileNumber++) {
//
//    cout << "Processing input label " << itkImages[segFileNumber] << endl;
//
//    LabelToLabelMapFilterType::Pointer l2lm = LabelToLabelMapFilterType::New();
//    l2lm->SetInput(itkImages[segFileNumber]);
//    l2lm->Update();
//
//    typedef LabelToLabelMapFilterType::OutputImageType::LabelObjectType LabelType;
//    typedef itk::LabelStatisticsImageFilter<ShortImageType,ShortImageType> LabelStatisticsType;
//
//    LabelStatisticsType::Pointer labelStats = LabelStatisticsType::New();
//
//    cout << "Found " << l2lm->GetOutput()->GetNumberOfLabelObjects() << " label(s)" << endl;
//    labelStats->SetInput(itkImages[segFileNumber]);
//    labelStats->SetLabelInput(itkImages[segFileNumber]);
//    labelStats->Update();
//
//    // TODO: implement crop segments box
//
//    for(unsigned segLabelNumber=0 ; segLabelNumber<l2lm->GetOutput()->GetNumberOfLabelObjects();segLabelNumber++){
//      LabelType* labelObject = l2lm->GetOutput()->GetNthLabelObject(segLabelNumber);
//      short label = labelObject->GetLabel();
//
//      if(!label){
//        cout << "Skipping label 0" << endl;
//        continue;
//      }
//
//      cout << "Processing label " << label << endl;
//
//      LabelStatisticsType::BoundingBoxType bbox = labelStats->GetBoundingBox(label);
//      unsigned firstSlice, lastSlice;
//      //bool skipEmptySlices = true; // TODO: what to do with that line?
//      //bool skipEmptySlices = false; // TODO: what to do with that line?
//      if(skipEmptySlices){
//        firstSlice = bbox[4];
//        lastSlice = bbox[5]+1;
//      } else {
//        firstSlice = 0;
//        lastSlice = inputSize[2];
//      }
//
//      cout << "Total non-empty slices that will be encoded in SEG for label " <<
//           label << " is " << lastSlice-firstSlice << endl <<
//           " (inclusive from " << firstSlice << " to " <<
//           lastSlice << ")" << endl;
//
//      DcmSegment* segment = NULL;
//      if(metaInfo.segmentsAttributesMappingList[segFileNumber].find(label) == metaInfo.segmentsAttributesMappingList[segFileNumber].end()){
//        cerr << "ERROR: Failed to match label from image to the segment metadata!" << endl;
//        return NULL;
//      }
//
//      SegmentAttributes* segmentAttributes = metaInfo.segmentsAttributesMappingList[segFileNumber][label];
//
//      DcmSegTypes::E_SegmentAlgoType algoType = DcmSegTypes::SAT_UNKNOWN;
//      string algoName = "";
//      string algoTypeStr = segmentAttributes->getSegmentAlgorithmType();
//      if(algoTypeStr == "MANUAL"){
//        algoType = DcmSegTypes::SAT_MANUAL;
//      } else {
//        if(algoTypeStr == "AUTOMATIC")
//          algoType = DcmSegTypes::SAT_AUTOMATIC;
//        if(algoTypeStr == "SEMIAUTOMATIC")
//          algoType = DcmSegTypes::SAT_SEMIAUTOMATIC;
//
//        algoName = segmentAttributes->getSegmentAlgorithmName();
//        if(algoName == ""){
//          cerr << "ERROR: Algorithm name must be specified for non-manual algorithm types!" << endl;
//          return NULL;
//        }
//      }
//
//      CodeSequenceMacro* typeCode = segmentAttributes->getSegmentedPropertyTypeCodeSequence();
//      CodeSequenceMacro* categoryCode = segmentAttributes->getSegmentedPropertyCategoryCodeSequence();
//      assert(typeCode != NULL && categoryCode!= NULL);
//      OFString segmentLabel;
//      CHECK_COND(typeCode->getCodeMeaning(segmentLabel));
//      CHECK_COND(DcmSegment::create(segment, segmentLabel, *categoryCode, *typeCode, algoType, algoName.c_str()));
//
//      if(segmentAttributes->getSegmentDescription().length() > 0)
//        segment->setSegmentDescription(segmentAttributes->getSegmentDescription().c_str());
//
//      CodeSequenceMacro* typeModifierCode = segmentAttributes->getSegmentedPropertyTypeModifierCodeSequence();
//      if (typeModifierCode != NULL) {
//        OFVector<CodeSequenceMacro*>& modifiersVector = segment->getSegmentedPropertyTypeModifierCode();
//        modifiersVector.push_back(typeModifierCode);
//      }
//
//      GeneralAnatomyMacro &anatomyMacro = segment->getGeneralAnatomyCode();
//      if (segmentAttributes->getAnatomicRegionSequence() != NULL){
//        OFVector<CodeSequenceMacro*>& anatomyMacroModifiersVector = anatomyMacro.getAnatomicRegionModifier();
//        CodeSequenceMacro& anatomicRegionSequence = anatomyMacro.getAnatomicRegion();
//        anatomicRegionSequence = *segmentAttributes->getAnatomicRegionSequence();
//
//        if(segmentAttributes->getAnatomicRegionModifierSequence() != NULL){
//          CodeSequenceMacro* anatomicRegionModifierSequence = segmentAttributes->getAnatomicRegionModifierSequence();
//          anatomyMacroModifiersVector.push_back(anatomicRegionModifierSequence);
//        }
//      }
//
//      unsigned* rgb = segmentAttributes->getRecommendedDisplayRGBValue();
//      unsigned cielabScaled[3];
//      float cielab[3], ciexyz[3];
//
//      Helper::getCIEXYZFromRGB(&rgb[0],&ciexyz[0]);
//      Helper::getCIELabFromCIEXYZ(&ciexyz[0],&cielab[0]);
//      Helper::getIntegerScaledCIELabFromCIELab(&cielab[0],&cielabScaled[0]);
//      CHECK_COND(segment->setRecommendedDisplayCIELabValue(cielabScaled[0],cielabScaled[1],cielabScaled[2]));
//
//      Uint16 segmentNumber;
//      CHECK_COND(segdoc->addSegment(segment, segmentNumber /* returns logical segment number */));
//
//      // TODO: make it possible to skip empty frames (optional)
//      // iterate over slices for an individual label and populate output frames
//      for(unsigned sliceNumber=firstSlice;sliceNumber<lastSlice;sliceNumber++){
//
//        // PerFrame FG: FrameContentSequence
//        //fracon->setStackID("1"); // all frames go into the same stack
//        CHECK_COND(fgfc->setDimensionIndexValues(segmentNumber, 0));
//        CHECK_COND(fgfc->setDimensionIndexValues(sliceNumber-firstSlice+1, 1));
//        //ostringstream inStackPosSStream; // StackID is not present/needed
//        //inStackPosSStream << s+1;
//        //fracon->setInStackPositionNumber(s+1);
//
//        // PerFrame FG: PlanePositionSequence
//        {
//          ShortImageType::PointType sliceOriginPoint;
//          ShortImageType::IndexType sliceOriginIndex;
//          sliceOriginIndex.Fill(0);
//          sliceOriginIndex[2] = sliceNumber;
//          itkImages[segFileNumber]->TransformIndexToPhysicalPoint(sliceOriginIndex, sliceOriginPoint);
//          ostringstream pppSStream;
//          if(sliceNumber>0){
//            ShortImageType::PointType prevOrigin;
//            ShortImageType::IndexType prevIndex;
//            prevIndex.Fill(0);
//            prevIndex[2] = sliceNumber-1;
//            itkImages[segFileNumber]->TransformIndexToPhysicalPoint(prevIndex, prevOrigin);
//          }
//          fgppp->setImagePositionPatient(
//            Helper::floatToStrScientific(sliceOriginPoint[0]).c_str(),
//            Helper::floatToStrScientific(sliceOriginPoint[1]).c_str(),
//            Helper::floatToStrScientific(sliceOriginPoint[2]).c_str());
//        }
//
//        /* Add frame that references this segment */
//        {
//          ShortImageType::RegionType sliceRegion;
//          ShortImageType::IndexType sliceIndex;
//          ShortImageType::SizeType sliceSize;
//
//          sliceIndex[0] = 0;
//          sliceIndex[1] = 0;
//          sliceIndex[2] = sliceNumber;
//
//          sliceSize[0] = volumeGeometry.extent[0];
//          sliceSize[1] = volumeGeometry.extent[1];
//          sliceSize[2] = 1;
//
//          sliceRegion.SetIndex(sliceIndex);
//          sliceRegion.SetSize(sliceSize);
//
//          unsigned framePixelCnt = 0;
//          itk::ImageRegionConstIteratorWithIndex<ShortImageType> sliceIterator(itkImages[segFileNumber], sliceRegion);
//          for(sliceIterator.GoToBegin();!sliceIterator.IsAtEnd();++sliceIterator,++framePixelCnt){
//            if(sliceIterator.Get() == label){
//              frameData[framePixelCnt] = 1;
//              ShortImageType::IndexType idx = sliceIterator.GetIndex();
//              //cout << framePixelCnt << " " << idx[1] << "," << idx[0] << endl;
//            } else
//              frameData[framePixelCnt] = 0;
//          }
//
//          /*
//          if(sliceNumber>=dcmDatasets.size()){
//            cerr << "ERROR: trying to access missing DICOM Slice! And sorry, multi-frame not supported at the moment..." << endl;
//            return NULL;
//          }*/
//
//          OFVector<DcmDataset*> siVector;
//          for(size_t derImageInstanceNum=0;
//              derImageInstanceNum<slice2derimg[sliceNumber].size();
//              derImageInstanceNum++){
//            siVector.push_back(dcmDatasets[slice2derimg[sliceNumber][derImageInstanceNum]]);
//          }
//
//          if(siVector.size()>0){
//
//            DerivationImageItem *derimgItem;
//            CHECK_COND(fgder->addDerivationImageItem(CodeSequenceMacro("113076","DCM","Segmentation"),"",derimgItem));
//
//            cout << "Total of " << siVector.size() << " source image items will be added" << endl;
//
//            OFVector<SourceImageItem*> srcimgItems;
//            CHECK_COND(derimgItem->addSourceImageItems(siVector,
//                                                       CodeSequenceMacro("121322","DCM","Source image for image processing operation"),
//                                                       srcimgItems));
//
//            if(1){
//              // initialize class UID and series instance UID
//              ImageSOPInstanceReferenceMacro &instRef = srcimgItems[0]->getImageSOPInstanceReference();
//              OFString instanceUID;
//              CHECK_COND(instRef.getReferencedSOPClassUID(classUID));
//              CHECK_COND(instRef.getReferencedSOPInstanceUID(instanceUID));
//
//              if(instanceUIDs.find(instanceUID) == instanceUIDs.end()){
//                SOPInstanceReferenceMacro *refinstancesItem = new SOPInstanceReferenceMacro();
//                CHECK_COND(refinstancesItem->setReferencedSOPClassUID(classUID));
//                CHECK_COND(refinstancesItem->setReferencedSOPInstanceUID(instanceUID));
//                refinstances.push_back(refinstancesItem);
//                instanceUIDs.insert(instanceUID);
//                uidnotfound++;
//              } else {
//                uidfound++;
//              }
//            }
//          }
//
//          CHECK_COND(segdoc->addFrame(frameData, segmentNumber, perFrameFGs));
//
//          // remove derivation image FG from the per-frame FGs, only if applicable!
//          if(siVector.size()>0){
//            // clean up for the next frame
//            fgder->clearData();
//          }
//
//        }
//      }
//    }
//  }
//
//  delete fgfc;
//  delete fgppp;
//  delete fgder;

  return EXIT_SUCCESS;

}

bool SegmentationImageObject::hasDerivationImages(vector<vector<int> > &slice2derimg) const {
  for (vector<vector<int> >::const_iterator vI = slice2derimg.begin(); vI != slice2derimg.end(); ++vI) {
    if ((*vI).size() > 0) {
      return true;
    }
  }
  return false;
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

template <typename ImageType, typename ImageTypePointer>
vector<vector<int> > SegmentationImageObject::getSliceMapForSegmentation2DerivationImage(const vector<DcmDataset*> dcmDatasets,
                                                                                         const ImageTypePointer &labelImage) {
  // Find mapping from the segmentation slice number to the derivation image
  // Assume that orientation of the segmentation is the same as the source series
  unsigned numLabelSlices = labelImage->GetLargestPossibleRegion().GetSize()[2];
  vector<vector<int> > slice2derimg(numLabelSlices);
  for(size_t i=0;i<dcmDatasets.size();i++){
    OFString ippStr;
    typename ImageType::PointType ippPoint;
    typename ImageType::IndexType ippIndex;
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