//
// Created by Andrey Fedorov on 3/11/17.
//

#include "dcmqi/SegmentationImageObject.h"

int SegmentationImageObject::initializeFromDICOM(DcmDataset* sourceDataset) {

//  TODO: add SegmentationImageObject to namespace dcmqi
  using namespace dcmqi;

  DcmRLEDecoderRegistration::registerCodecs();

  OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");
  dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);

  DcmSegmentation *segmentation = NULL;
  OFCondition cond = DcmSegmentation::loadDataset(*sourceDataset, segmentation);
  if(!segmentation){
    cerr << "Failed to load seg! " << cond.text() << endl;
    throw -1;
  }

  initializeVolumeGeometryFromDICOM(segmentation, sourceDataset);

//  imageSize[2] = ceil(computedVolumeExtent/imageSpacing[2])+1;

  // Initialize the image
  itkImage = volumeGeometry.getITKRepresentation<ShortImageType>();

  itkImage->Allocate();
  itkImage->FillBuffer(0);

  // Iterate over frames, find the matching slice for each of the frames based on
  // ImagePositionPatient, set non-zero pixels to the segment number. Notify
  // about pixels that are initialized more than once.

  FGInterface &fgInterface = segmentation->getFunctionalGroups();

  DcmIODTypes::Frame *unpackedFrame = NULL;

  for(size_t frameId=0;frameId<fgInterface.getNumberOfFrames();frameId++){
    const DcmIODTypes::Frame *frame = segmentation->getFrame(frameId);
    bool isPerFrame;

    FGPlanePosPatient *planposfg =
      OFstatic_cast(FGPlanePosPatient*,fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
    assert(planposfg);

#ifndef NDEBUG
    FGFrameContent *fracon =
      OFstatic_cast(FGFrameContent*,fgInterface.get(frameId, DcmFGTypes::EFG_FRAMECONTENT, isPerFrame));
    assert(fracon);
#endif

    FGSegmentation *fgseg =
      OFstatic_cast(FGSegmentation*,fgInterface.get(frameId, DcmFGTypes::EFG_SEGMENTATION, isPerFrame));
    assert(fgseg);

    Uint16 segmentId = -1;
    if(fgseg->getReferencedSegmentNumber(segmentId).bad()){
      cerr << "Failed to get seg number!";
      throw -1;
    }

    // WARNING: this is needed only for David's example, which numbers
    // (incorrectly!) segments starting from 0, should start from 1
    if(segmentId == 0){
      cerr << "Segment numbers should start from 1!" << endl;
      throw -1;
    }

    if(segment2image.find(segmentId) == segment2image.end()){
      typedef itk::ImageDuplicator<ShortImageType> DuplicatorType;
      DuplicatorType::Pointer dup = DuplicatorType::New();
      dup->SetInputImage(itkImage);
      dup->Update();
      ShortImageType::Pointer newSegmentImage = dup->GetOutput();
      newSegmentImage->FillBuffer(0);
      segment2image[segmentId] = newSegmentImage;
    }

    // populate meta information needed for Slicer ScalarVolumeNode initialization
    //  (for example)
    {
      // NOTE: according to the standard, segment numbering should start from 1,
      //  not clear if this is intentional behavior or a bug in DCMTK expecting
      //  it to start from 0

      DcmSegment *segment = segmentation->getSegment(segmentId);
      if (segment == NULL) {
        cerr << "Failed to get segment for segment ID " << segmentId << endl;
        continue;
      }

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

      // TODO: factor out SegmentAttributes
      SegmentAttributes *segmentAttributes = createAndGetNewSegment(segmentId);

      if (segmentAttributes) {
        segmentAttributes->setLabelID(segmentId);
        DcmSegTypes::E_SegmentAlgoType algorithmType = segment->getSegmentAlgorithmType();
        string readableAlgorithmType = DcmSegTypes::algoType2OFString(algorithmType).c_str();
        segmentAttributes->setSegmentAlgorithmType(readableAlgorithmType);

        if (algorithmType == DcmSegTypes::SAT_UNKNOWN) {
          cerr << "AlgorithmType is not valid with value " << readableAlgorithmType << endl;
          throw -1;
        }
        if (algorithmType != DcmSegTypes::SAT_MANUAL) {
          OFString segmentAlgorithmName;
          segment->getSegmentAlgorithmName(segmentAlgorithmName);
          if (segmentAlgorithmName.length() > 0)
            segmentAttributes->setSegmentAlgorithmName(segmentAlgorithmName.c_str());
        }

        OFString segmentDescription;
        segment->getSegmentDescription(segmentDescription);
        segmentAttributes->setSegmentDescription(segmentDescription.c_str());

        segmentAttributes->setRecommendedDisplayRGBValue(rgb[0], rgb[1], rgb[2]);
        segmentAttributes->setSegmentedPropertyCategoryCodeSequence(segment->getSegmentedPropertyCategoryCode());
        segmentAttributes->setSegmentedPropertyTypeCodeSequence(segment->getSegmentedPropertyTypeCode());

        if (segment->getSegmentedPropertyTypeModifierCode().size() > 0) {
          segmentAttributes->setSegmentedPropertyTypeModifierCodeSequence(
            segment->getSegmentedPropertyTypeModifierCode()[0]);
        }

        GeneralAnatomyMacro &anatomyMacro = segment->getGeneralAnatomyCode();
        CodeSequenceMacro &anatomicRegionSequence = anatomyMacro.getAnatomicRegion();
        if (anatomicRegionSequence.check(true).good()) {
          segmentAttributes->setAnatomicRegionSequence(anatomyMacro.getAnatomicRegion());
        }
        if (anatomyMacro.getAnatomicRegionModifier().size() > 0) {
          segmentAttributes->setAnatomicRegionModifierSequence(anatomyMacro.getAnatomicRegionModifier()[0]);
        }
      }
    }

    // get string representation of the frame origin
    ShortImageType::PointType frameOriginPoint;
    ShortImageType::IndexType frameOriginIndex;
    for(int j=0;j<3;j++){
      OFString planposStr;
      if(planposfg->getImagePositionPatient(planposStr, j).good()){
        frameOriginPoint[j] = atof(planposStr.c_str());
      }
    }

    if(!segment2image[segmentId]->TransformPhysicalPointToIndex(frameOriginPoint, frameOriginIndex)){
      cerr << "ERROR: Frame " << frameId << " origin " << frameOriginPoint <<
           " is outside image geometry!" << frameOriginIndex << endl;
      cerr << "Image size: " << segment2image[segmentId]->GetBufferedRegion().GetSize() << endl;
      throw -1;
    }

    unsigned slice = frameOriginIndex[2];

    if(segmentation->getSegmentationType() == DcmSegTypes::ST_BINARY)
      unpackedFrame = DcmSegUtils::unpackBinaryFrame(frame,
                                                     volumeGeometry.extent[1], // Rows
                                                     volumeGeometry.extent[0]); // Cols
    else
      unpackedFrame = new DcmIODTypes::Frame(*frame);

    // initialize slice with the frame content
    for(unsigned row=0;row<volumeGeometry.extent[1];row++){
      for(unsigned col=0;col<volumeGeometry.extent[0];col++){
        ShortImageType::PixelType pixel;
        unsigned bitCnt = row*volumeGeometry.extent[0]+col;
        pixel = unpackedFrame->pixData[bitCnt];

        if(pixel!=0){
          ShortImageType::IndexType index;
          index[0] = col;
          index[1] = row;
          index[2] = slice;
          segment2image[segmentId]->SetPixel(index, segmentId);
        }
      }
    }

    if(unpackedFrame != NULL)
      delete unpackedFrame;
  }

  initializeMetaDataFromDICOM(sourceDataset);

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

  return EXIT_SUCCESS;
}

dcmqi::SegmentAttributes *SegmentationImageObject::createAndGetNewSegment(unsigned labelID) {
  using namespace dcmqi;
  for (vector<map<unsigned,SegmentAttributes*> >::const_iterator vIt = segmentsAttributesMappingList.begin();
       vIt != segmentsAttributesMappingList.end(); ++vIt) {
    for(map<unsigned,SegmentAttributes*>::const_iterator mIt = vIt->begin();mIt!=vIt->end();++mIt){
      SegmentAttributes *segmentAttributes = mIt->second;
      if (segmentAttributes->getLabelID() == labelID)
        return NULL;
    }
  }

  SegmentAttributes *segment = new SegmentAttributes(labelID);
  map<unsigned,SegmentAttributes*> tempMap;
  tempMap[labelID] = segment;
  segmentsAttributesMappingList.push_back(tempMap);
  return segment;
}