
// DCMQI includes
#include "dcmqi/ImageSEGConverter.h"
#include "dcmqi/ColorUtilities.h"


//DCMTK includes
#include <dcmtk/dcmiod/cielabutil.h>
#include <dcmtk/dcmsr/codes/dcm.h>
#include <dcmtk/ofstd/ofmem.h>


namespace dcmqi {

  DcmDataset* ImageSEGConverter::itkimage2dcmSegmentation(vector<DcmDataset*> dcmDatasets,
                                                          vector<ShortImageType::Pointer> segmentations,
                                                          const string &metaData,
                                                          bool skipEmptySlices) {

    ShortImageType::SizeType inputSize = segmentations[0]->GetBufferedRegion().GetSize();
    //cout << "Input image size: " << inputSize << endl;

    JSONSegmentationMetaInformationHandler metaInfo(metaData.c_str());
    metaInfo.read();

    if(metaInfo.segmentsAttributesMappingList.size() != segmentations.size()){
      cerr << "Mismatch between the number of input segmentation files and the size of metainfo list!" << endl;
      return NULL;
    };

    IODGeneralEquipmentModule::EquipmentInfo eq = getEquipmentInfo();
    ContentIdentificationMacro ident = createContentIdentificationInformation(metaInfo);
    CHECK_COND(ident.setInstanceNumber(metaInfo.getInstanceNumber().c_str()));

    /* Create new segmentation document */
    DcmDataset segdocDataset;
    DcmSegmentation *segdoc = NULL;

    DcmSegmentation::createBinarySegmentation(
        segdoc,   // resulting segmentation
        inputSize[1],    // rows
        inputSize[0],    // columns
        eq,     // equipment
        ident);   // content identification

    // import Patient, Study and Frame of Reference; do not import Series
    // attributes
    CHECK_COND(segdoc->importHierarchy(*dcmDatasets[0], OFTrue, OFTrue, OFTrue, OFFalse));

    /* Initialize dimension module */
    char dimUID[128];
    dcmGenerateUniqueIdentifier(dimUID, QIICR_UID_ROOT);
    IODMultiframeDimensionModule &mfdim = segdoc->getDimensions();
    CHECK_COND(mfdim.addDimensionIndex(DCM_ReferencedSegmentNumber, dimUID, DCM_SegmentIdentificationSequence,
                       DcmTag(DCM_ReferencedSegmentNumber).getTagName()));
    CHECK_COND(mfdim.addDimensionIndex(DCM_ImagePositionPatient, dimUID, DCM_PlanePositionSequence,
                       DcmTag(DCM_ImagePositionPatient).getTagName()));

    /* Initialize shared functional groups */
    const unsigned frameSize = inputSize[0] * inputSize[1];

    // Shared FGs: PlaneOrientationPatientSequence
    {
      ShortImageType::DirectionType labelDirMatrix = segmentations[0]->GetDirection();

      //cout << "Directions: " << labelDirMatrix << endl;

      FGPlaneOrientationPatient *planor =
          FGPlaneOrientationPatient::createMinimal(
              Helper::floatToStr(labelDirMatrix[0][0]).c_str(),
              Helper::floatToStr(labelDirMatrix[1][0]).c_str(),
              Helper::floatToStr(labelDirMatrix[2][0]).c_str(),
              Helper::floatToStr(labelDirMatrix[0][1]).c_str(),
              Helper::floatToStr(labelDirMatrix[1][1]).c_str(),
              Helper::floatToStr(labelDirMatrix[2][1]).c_str());

      CHECK_COND(segdoc->addForAllFrames(*planor));
    }

    // Shared FGs: PixelMeasuresSequence
    {
      FGPixelMeasures *pixmsr = new FGPixelMeasures();

      ShortImageType::SpacingType labelSpacing = segmentations[0]->GetSpacing();
      ostringstream spacingSStream;
      spacingSStream << scientific << labelSpacing[0] << "\\" << labelSpacing[1];
      CHECK_COND(pixmsr->setPixelSpacing(spacingSStream.str().c_str()));

      spacingSStream.clear(); spacingSStream.str("");
      spacingSStream << scientific << labelSpacing[2];
      CHECK_COND(pixmsr->setSpacingBetweenSlices(spacingSStream.str().c_str()));
      CHECK_COND(pixmsr->setSliceThickness(spacingSStream.str().c_str()));
      CHECK_COND(segdoc->addForAllFrames(*pixmsr));
      delete pixmsr;
    }


    // Iterate over the files and labels available in each file, create a segment for each label,
    //  initialize segment frames and add to the document

    OFString seriesInstanceUID, classUID;
    set<OFString> instanceUIDs;

    IODCommonInstanceReferenceModule &commref = segdoc->getCommonInstanceReference();
    OFVector<IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*> &refseries = commref.getReferencedSeriesItems();

    IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem* refseriesItem = new IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem;

    OFVector<SOPInstanceReferenceMacro*> &refinstances = refseriesItem->getReferencedInstanceItems();

    CHECK_COND(dcmDatasets[0]->findAndGetOFString(DCM_SeriesInstanceUID, seriesInstanceUID));
    CHECK_COND(refseriesItem->setSeriesInstanceUID(seriesInstanceUID));

    int uidfound = 0, uidnotfound = 0;
    Uint8 *frameData = new Uint8[frameSize];

    // NB this assumes all segmentation files have the same dimensions; alternatively, need to
    //   do this operation for each segmentation file
    vector<vector<int> > slice2derimg = getSliceMapForSegmentation2DerivationImage(dcmDatasets, segmentations[0]);

    bool hasDerivationImages = false;
    for(vector<vector<int> >::const_iterator vI=slice2derimg.begin();vI!=slice2derimg.end();++vI)
      if((*vI).size()>0)
        hasDerivationImages = true;


    FGPlanePosPatient* fgppp = FGPlanePosPatient::createMinimal("1","1","1");
    FGFrameContent* fgfc = new FGFrameContent();
    FGDerivationImage* fgder = new FGDerivationImage();
    OFVector<FGBase*> perFrameFGs;

    perFrameFGs.push_back(fgppp);
    perFrameFGs.push_back(fgfc);
    if(hasDerivationImages)
      perFrameFGs.push_back(fgder);

    for(size_t segFileNumber=0; segFileNumber<segmentations.size(); segFileNumber++){

      //cout << "Processing input label " << segmentations[segFileNumber] << endl;

      LabelToLabelMapFilterType::Pointer l2lm = LabelToLabelMapFilterType::New();
      l2lm->SetInput(segmentations[segFileNumber]);
      l2lm->Update();

      typedef LabelToLabelMapFilterType::OutputImageType::LabelObjectType LabelType;
      typedef itk::LabelStatisticsImageFilter<ShortImageType,ShortImageType> LabelStatisticsType;

      LabelStatisticsType::Pointer labelStats = LabelStatisticsType::New();

      cout << "Found " << l2lm->GetOutput()->GetNumberOfLabelObjects() << " label(s)" << endl;
      labelStats->SetInput(segmentations[segFileNumber]);
      labelStats->SetLabelInput(segmentations[segFileNumber]);
      labelStats->Update();

      bool cropSegmentsBBox = false;
      if(cropSegmentsBBox){
        cout << "WARNING: Crop operation enabled - WIP" << endl;
        typedef itk::BinaryThresholdImageFilter<ShortImageType,ShortImageType> ThresholdType;
        ThresholdType::Pointer thresh = ThresholdType::New();
        thresh->SetInput(segmentations[segFileNumber]);
        thresh->SetLowerThreshold(1);
        thresh->SetLowerThreshold(100);
        thresh->SetInsideValue(1);
        thresh->Update();

        LabelStatisticsType::Pointer threshLabelStats = LabelStatisticsType::New();

        threshLabelStats->SetInput(thresh->GetOutput());
        threshLabelStats->SetLabelInput(thresh->GetOutput());
        threshLabelStats->Update();

        LabelStatisticsType::BoundingBoxType threshBbox = threshLabelStats->GetBoundingBox(1);
        /*
        cout << "OVerall bounding box: " << threshBbox[0] << ", " << threshBbox[1]
               << threshBbox[2] << ", " << threshBbox[3]
               << threshBbox[4] << ", " << threshBbox[5]
               << endl;
               */
        return NULL;
      }

      for(unsigned segLabelNumber=0 ; segLabelNumber<l2lm->GetOutput()->GetNumberOfLabelObjects();segLabelNumber++){
        LabelType* labelObject = l2lm->GetOutput()->GetNthLabelObject(segLabelNumber);
        short label = labelObject->GetLabel();

        if(!label){
          cout << "Skipping label 0" << endl;
          continue;
        }

        cout << "Processing label " << label << endl;

        LabelStatisticsType::BoundingBoxType bbox = labelStats->GetBoundingBox(label);
        unsigned firstSlice, lastSlice;
        //bool skipEmptySlices = true; // TODO: what to do with that line?
        //bool skipEmptySlices = false; // TODO: what to do with that line?
        if(skipEmptySlices){
          firstSlice = bbox[4];
          lastSlice = bbox[5]+1;
        } else {
          firstSlice = 0;
          lastSlice = inputSize[2];
        }

        cout << "Total non-empty slices that will be encoded in SEG for label " <<
        label << " is " << lastSlice-firstSlice << endl <<
        " (inclusive from " << firstSlice << " to " <<
        lastSlice << ")" << endl;

        DcmSegment* segment = NULL;
        if(metaInfo.segmentsAttributesMappingList[segFileNumber].find(label) == metaInfo.segmentsAttributesMappingList[segFileNumber].end()){
          cerr << "ERROR: Failed to match label from image to the segment metadata!" << endl;
          return NULL;
        }

        SegmentAttributes* segmentAttributes = metaInfo.segmentsAttributesMappingList[segFileNumber][label];

        DcmSegTypes::E_SegmentAlgoType algoType = DcmSegTypes::SAT_UNKNOWN;
        string algoName = "";
        string algoTypeStr = segmentAttributes->getSegmentAlgorithmType();
        if(algoTypeStr == "MANUAL"){
          algoType = DcmSegTypes::SAT_MANUAL;
        } else {
          if(algoTypeStr == "AUTOMATIC")
            algoType = DcmSegTypes::SAT_AUTOMATIC;
          if(algoTypeStr == "SEMIAUTOMATIC")
            algoType = DcmSegTypes::SAT_SEMIAUTOMATIC;

          algoName = segmentAttributes->getSegmentAlgorithmName();
          if(algoName == ""){
            cerr << "ERROR: Algorithm name must be specified for non-manual algorithm types!" << endl;
            return NULL;
          }
        }

        CodeSequenceMacro* typeCode = segmentAttributes->getSegmentedPropertyTypeCodeSequence();
        CodeSequenceMacro* categoryCode = segmentAttributes->getSegmentedPropertyCategoryCodeSequence();
        assert(typeCode != NULL && categoryCode!= NULL);
        OFString segmentLabel;

        if(segmentAttributes->getSegmentLabel().length() > 0){
          cout << "Populating segment label to " << segmentAttributes->getSegmentLabel() << endl;
          segmentLabel = segmentAttributes->getSegmentLabel().c_str();
        } else
          CHECK_COND(typeCode->getCodeMeaning(segmentLabel));

        CHECK_COND(DcmSegment::create(segment, segmentLabel, *categoryCode, *typeCode, algoType, algoName.c_str()));

        if(segmentAttributes->getSegmentDescription().length() > 0)
          segment->setSegmentDescription(segmentAttributes->getSegmentDescription().c_str());

        if(segmentAttributes->getTrackingIdentifier().length() > 0)
          segment->setTrackingID(segmentAttributes->getTrackingIdentifier().c_str());

        if(segmentAttributes->getTrackingUniqueIdentifier().length() > 0)
          segment->setTrackingUID(segmentAttributes->getTrackingUniqueIdentifier().c_str());

        CodeSequenceMacro* typeModifierCode = segmentAttributes->getSegmentedPropertyTypeModifierCodeSequence();
        if (typeModifierCode != NULL) {
          OFVector<CodeSequenceMacro*>& modifiersVector = segment->getSegmentedPropertyTypeModifierCode();
          modifiersVector.push_back(typeModifierCode);
        }

        GeneralAnatomyMacro &anatomyMacro = segment->getGeneralAnatomyCode();
        if (segmentAttributes->getAnatomicRegionSequence() != NULL){
          OFVector<CodeSequenceMacro*>& anatomyMacroModifiersVector = anatomyMacro.getAnatomicRegionModifier();
          CodeSequenceMacro& anatomicRegionSequence = anatomyMacro.getAnatomicRegion();
          anatomicRegionSequence = *segmentAttributes->getAnatomicRegionSequence();

          if(segmentAttributes->getAnatomicRegionModifierSequence() != NULL){
            CodeSequenceMacro* anatomicRegionModifierSequence = segmentAttributes->getAnatomicRegionModifierSequence();
            anatomyMacroModifiersVector.push_back(anatomicRegionModifierSequence);
          }
        }

        unsigned* rgb = segmentAttributes->getRecommendedDisplayRGBValue();
        int cielab[3];

        ColorUtilities::getIntegerScaledCIELabPCSFromSRGB(cielab[0], cielab[1], cielab[2], rgb[0], rgb[1], rgb[2]);
        //IODCIELabUtil::rgb2DicomLab(cielab[0], cielab[1], cielab[2], rgb[0], rgb[1], rgb[2]);

        CHECK_COND(segment->setRecommendedDisplayCIELabValue(cielab[0],cielab[1],cielab[2]));

        Uint16 segmentNumber;
        CHECK_COND(segdoc->addSegment(segment, segmentNumber /* returns logical segment number */));

        // TODO: make it possible to skip empty frames (optional)
        // iterate over slices for an individual label and populate output frames
        for(unsigned sliceNumber=firstSlice;sliceNumber<lastSlice;sliceNumber++){

          // PerFrame FG: FrameContentSequence
          //fracon->setStackID("1"); // all frames go into the same stack
          CHECK_COND(fgfc->setDimensionIndexValues(segmentNumber, 0));
          CHECK_COND(fgfc->setDimensionIndexValues(sliceNumber-firstSlice+1, 1));
          //ostringstream inStackPosSStream; // StackID is not present/needed
          //inStackPosSStream << s+1;
          //fracon->setInStackPositionNumber(s+1);

          // PerFrame FG: PlanePositionSequence
          {
            ShortImageType::PointType sliceOriginPoint;
            ShortImageType::IndexType sliceOriginIndex;
            sliceOriginIndex.Fill(0);
            sliceOriginIndex[2] = sliceNumber;
            segmentations[segFileNumber]->TransformIndexToPhysicalPoint(sliceOriginIndex, sliceOriginPoint);
            ostringstream pppSStream;
            if(sliceNumber>0){
              ShortImageType::PointType prevOrigin;
              ShortImageType::IndexType prevIndex;
              prevIndex.Fill(0);
              prevIndex[2] = sliceNumber-1;
              segmentations[segFileNumber]->TransformIndexToPhysicalPoint(prevIndex, prevOrigin);
            }
            fgppp->setImagePositionPatient(
                Helper::floatToStr(sliceOriginPoint[0]).c_str(),
                Helper::floatToStr(sliceOriginPoint[1]).c_str(),
                Helper::floatToStr(sliceOriginPoint[2]).c_str());
          }

          /* Add frame that references this segment */
          {
            ShortImageType::RegionType sliceRegion;
            ShortImageType::IndexType sliceIndex;
            ShortImageType::SizeType sliceSize;

            sliceIndex[0] = 0;
            sliceIndex[1] = 0;
            sliceIndex[2] = sliceNumber;

            sliceSize[0] = inputSize[0];
            sliceSize[1] = inputSize[1];
            sliceSize[2] = 1;

            sliceRegion.SetIndex(sliceIndex);
            sliceRegion.SetSize(sliceSize);

            unsigned framePixelCnt = 0;
            itk::ImageRegionConstIteratorWithIndex<ShortImageType> sliceIterator(segmentations[segFileNumber], sliceRegion);
            for(sliceIterator.GoToBegin();!sliceIterator.IsAtEnd();++sliceIterator,++framePixelCnt){
              if(sliceIterator.Get() == label){
                frameData[framePixelCnt] = 1;
                ShortImageType::IndexType idx = sliceIterator.GetIndex();
                //cout << framePixelCnt << " " << idx[1] << "," << idx[0] << endl;
              } else
                frameData[framePixelCnt] = 0;
            }

            /*
            if(sliceNumber>=dcmDatasets.size()){
              cerr << "ERROR: trying to access missing DICOM Slice! And sorry, multi-frame not supported at the moment..." << endl;
              return NULL;
            }*/

            OFVector<DcmDataset*> siVector;
            for(size_t derImageInstanceNum=0;
                derImageInstanceNum<slice2derimg[sliceNumber].size();
                derImageInstanceNum++){
              siVector.push_back(dcmDatasets[slice2derimg[sliceNumber][derImageInstanceNum]]);
            }

            if(siVector.size()>0){

              DerivationImageItem *derimgItem;
			  DSRBasicCodedEntry code_seg=CODE_DCM_Segmentation_113076;
              CHECK_COND(fgder->addDerivationImageItem(CodeSequenceMacro(code_seg.CodeValue,code_seg.CodingSchemeDesignator,
				  code_seg.CodeMeaning),"",derimgItem));

              //cout << "Total of " << siVector.size() << " source image items will be added" << endl;
			  DSRBasicCodedEntry code = CODE_DCM_SourceImageForImageProcessingOperation;
              OFVector<SourceImageItem*> srcimgItems;
              CHECK_COND(derimgItem->addSourceImageItems(siVector,
                                                       CodeSequenceMacro(code.CodeValue, code.CodingSchemeDesignator,
														   code.CodeMeaning),
                                                       srcimgItems));

              if(1){
                // initialize class UID and series instance UID
                ImageSOPInstanceReferenceMacro &instRef = srcimgItems[0]->getImageSOPInstanceReference();
                OFString instanceUID;
                CHECK_COND(instRef.getReferencedSOPClassUID(classUID));
                CHECK_COND(instRef.getReferencedSOPInstanceUID(instanceUID));

                if(instanceUIDs.find(instanceUID) == instanceUIDs.end()){
                  SOPInstanceReferenceMacro *refinstancesItem = new SOPInstanceReferenceMacro();
                  CHECK_COND(refinstancesItem->setReferencedSOPClassUID(classUID));
                  CHECK_COND(refinstancesItem->setReferencedSOPInstanceUID(instanceUID));
                  refinstances.push_back(refinstancesItem);
                  instanceUIDs.insert(instanceUID);
                  uidnotfound++;
                } else {
                  uidfound++;
                }
              }
            }

            CHECK_COND(segdoc->addFrame(frameData, segmentNumber, perFrameFGs));

            // remove derivation image FG from the per-frame FGs, only if applicable!
            if(siVector.size()>0){
              // clean up for the next frame
              fgder->clearData();
            }

          }
        }
      }
    }

    // add ReferencedSeriesItem only if it is not empty
    if(refinstances.size())
      refseries.push_back(refseriesItem);

    delete fgfc;
    delete fgppp;
    delete fgder;

    segdoc->getSeries().setSeriesNumber(metaInfo.getSeriesNumber().c_str());

    OFString frameOfRefUID;
    if(!segdoc->getFrameOfReference().getFrameOfReferenceUID(frameOfRefUID).good()){
      // TODO: add FoR UID to the metadata JSON and check that before generating one!
      char frameOfRefUIDchar[128];
      dcmGenerateUniqueIdentifier(frameOfRefUIDchar, QIICR_UID_ROOT);
      CHECK_COND(segdoc->getFrameOfReference().setFrameOfReferenceUID(frameOfRefUIDchar));
    }

    OFCondition writeResult = segdoc->writeDataset(segdocDataset);
    if(writeResult.bad()){
      cerr << "FATAL ERROR: Writing of the SEG dataset failed!";
      if (writeResult.text()){
        cerr << " Error: " << writeResult.text() << ".";
      }
      cerr << " Please report the problem to the developers, ideally accompanied by a de-identified dataset allowing to reproduce the problem!" << endl;
      return NULL;
    }

    // Set reader/session/timepoint information
    CHECK_COND(segdocDataset.putAndInsertString(DCM_SeriesDescription, metaInfo.getSeriesDescription().c_str()));
    CHECK_COND(segdocDataset.putAndInsertString(DCM_ContentCreatorName, metaInfo.getContentCreatorName().c_str()));
    CHECK_COND(segdocDataset.putAndInsertString(DCM_ClinicalTrialSeriesID, metaInfo.getClinicalTrialSeriesID().c_str()));
    CHECK_COND(segdocDataset.putAndInsertString(DCM_ClinicalTrialTimePointID, metaInfo.getClinicalTrialTimePointID().c_str()));
    if (metaInfo.getClinicalTrialCoordinatingCenterName().size())
      CHECK_COND(segdocDataset.putAndInsertString(DCM_ClinicalTrialCoordinatingCenterName, metaInfo.getClinicalTrialCoordinatingCenterName().c_str()));

    // populate BodyPartExamined
    {
      OFString bodyPartStr;
      string bodyPartAssigned = metaInfo.getBodyPartExamined();

      // inherit BodyPartExamined from the source image dataset, if available
      if(dcmDatasets[0]->findAndGetOFString(DCM_BodyPartExamined, bodyPartStr).good())
      if(string(bodyPartStr.c_str()).size())
        bodyPartAssigned = bodyPartStr.c_str();

      if(bodyPartAssigned.size())
        CHECK_COND(segdocDataset.putAndInsertString(DCM_BodyPartExamined, bodyPartAssigned.c_str()));
    }

    // StudyDate/Time should be of the series segmented, not when segmentation was made - this is initialized by DCMTK

    // SeriesDate/Time should be of when segmentation was done; initialize to when it was saved
    {
      OFString contentDate, contentTime;
      DcmDate::getCurrentDate(contentDate);
      DcmTime::getCurrentTime(contentTime);

      CHECK_COND(segdocDataset.putAndInsertString(DCM_SeriesDate, contentDate.c_str()));
      CHECK_COND(segdocDataset.putAndInsertString(DCM_SeriesTime, contentTime.c_str()));
      segdoc->getGeneralImage().setContentDate(contentDate.c_str());
      segdoc->getGeneralImage().setContentTime(contentTime.c_str());
    }

    {
      string segmentsOverlap;
      if(segmentations.size() == 1)
        segmentsOverlap = "NO";
      else
        segmentsOverlap = "UNDEFINED";
      CHECK_COND(segdocDataset.putAndInsertString(DCM_SegmentsOverlap, segmentsOverlap.c_str()));
    }

    return new DcmDataset(segdocDataset);
  }


  pair <map<unsigned,ShortImageType::Pointer>, string> ImageSEGConverter::dcmSegmentation2itkimage(DcmDataset *segDataset) {
    DcmSegmentation *segdoc = NULL;
    OFCondition cond = DcmSegmentation::loadDataset(*segDataset, segdoc);
    if(!segdoc){
      cerr << "ERROR: Failed to load segmentation dataset! " << cond.text() << endl;
      throw -1;
    }
    OFunique_ptr<DcmSegmentation> segdocguard(segdoc);

    JSONSegmentationMetaInformationHandler metaInfo;
    populateMetaInformationFromDICOM(segDataset, segdoc, metaInfo);

    const std::map<unsigned, ShortImageType::Pointer> segment2image =
        dcmSegmentation2itkimage(segdoc, &metaInfo);
    return pair<map<unsigned, ShortImageType::Pointer>, string>(
        segment2image, metaInfo.getJSONOutputAsString());
  }

  std::map<unsigned, ShortImageType::Pointer>
  ImageSEGConverter::dcmSegmentation2itkimage(
      DcmSegmentation *segdoc,
      JSONSegmentationMetaInformationHandler *metaInfo) {
    DcmRLEDecoderRegistration::registerCodecs();

    OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");
    dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);

    // Directions
    FGInterface &fgInterface = segdoc->getFunctionalGroups();
    ShortImageType::DirectionType direction;
    if(getImageDirections(fgInterface, direction)){
      cerr << "ERROR: Failed to get image directions!" << endl;
      throw -1;
    }

    // Spacing and origin
    double computedSliceSpacing, computedVolumeExtent;
    vnl_vector<double> sliceDirection(3);
    sliceDirection[0] = direction[0][2];
    sliceDirection[1] = direction[1][2];
    sliceDirection[2] = direction[2][2];

    ShortImageType::PointType imageOrigin;
    if(computeVolumeExtent(fgInterface, sliceDirection, imageOrigin, computedSliceSpacing, computedVolumeExtent)){
      cerr << "ERROR: Failed to compute origin and/or slice spacing!" << endl;
      throw -1;
    }

    ShortImageType::SpacingType imageSpacing;
    imageSpacing.Fill(0);
    if(getDeclaredImageSpacing(fgInterface, imageSpacing)){
      cerr << "ERROR: Failed to get image spacing from DICOM!" << endl;
      throw -1;
    }

    if(!imageSpacing[2]){
      cerr << "ERROR: No sufficient information to derive slice spacing! Unable to interpret the data." << endl;
      throw -1;
    }

    // Region size
    ShortImageType::SizeType imageSize;
    {
      DcmIODImage<IODImagePixelModule<Uint8> > *ip =
          static_cast<DcmIODImage<IODImagePixelModule<Uint8> > *>(segdoc);
      Uint16 value;
      if (ip->getImagePixel().getRows(value).good()) {
        imageSize[1] = value;
      }
      if (ip->getImagePixel().getColumns(value).good()) {
        imageSize[0] = value;
      }
    }
    // number of slices should be computed, since segmentation may have empty frames
    imageSize[2] = round(computedVolumeExtent/imageSpacing[2])+1;

    // Initialize the image
    ShortImageType::RegionType imageRegion;
    imageRegion.SetSize(imageSize);
    ShortImageType::Pointer segImage = ShortImageType::New();
    segImage->SetRegions(imageRegion);
    segImage->SetOrigin(imageOrigin);
    segImage->SetSpacing(imageSpacing);
    segImage->SetDirection(direction);
    segImage->Allocate();
    segImage->FillBuffer(0);

    // ITK images corresponding to the individual segments
    map<unsigned,ShortImageType::Pointer> segment2image;

    // Iterate over frames, find the matching slice for each of the frames based on
    // ImagePositionPatient, set non-zero pixels to the segment number. Notify
    // about pixels that are initialized more than once.

    const DcmIODTypes::Frame *unpackedFrame = NULL;

    for(size_t frameId=0;frameId<fgInterface.getNumberOfFrames();frameId++){
      const DcmIODTypes::Frame *frame = segdoc->getFrame(frameId);
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
        cerr << "ERROR: Failed to get ReferencedSegmentNumber!";
        throw -1;
      }

      // WARNING: this is needed only for David's example, which numbers
      // (incorrectly!) segments starting from 0, should start from 1
      if(segmentId == 0){
        cerr << "ERROR: ReferencedSegmentNumber value of 0 was encountered. Segment numbers and references to segment numbers should both start from 1!" << endl;
        throw -1;
      }

      if(segment2image.find(segmentId) == segment2image.end()){
        typedef itk::ImageDuplicator<ShortImageType> DuplicatorType;
        DuplicatorType::Pointer dup = DuplicatorType::New();
        dup->SetInputImage(segImage);
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

        DcmSegment* segment = segdoc->getSegment(segmentId);
        if(segment == NULL){
          cerr << "Failed to get segment for segment ID " << segmentId << endl;
          continue;
        }

        // get CIELab color for the segment
        Uint16 ciedcm[3];
        int rgb[3];
        if(segment->getRecommendedDisplayCIELabValue(
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

        /* Debug CIELab 2 RGB conversion
        double lab[3];

        cout << "DICOM Lab " << ciedcm[0] << ", " << ciedcm[1] << ", " << ciedcm[2] << endl;

        IODCIELabUtil::dicomlab2Lab(lab[0], lab[1], lab[2], ciedcm[0], ciedcm[1], ciedcm[2]);

        cout << "Lab " << ciedcm[0] << ", " << ciedcm[1] << ", " << ciedcm[2] << endl;

        IODCIELabUtil::lab2Rgb(rgb[0], rgb[1], rgb[2], lab[0], lab[1], lab[2]);

        cout << "RGB " << unsigned(rgb[0]*256) << ", " << unsigned(rgb[1]*256) << ", " << unsigned(rgb[2]*256) << endl;
        */

        ColorUtilities::getSRGBFromIntegerScaledCIELabPCS(rgb[0], rgb[1], rgb[2], ciedcm[0], ciedcm[1], ciedcm[2]);
        //IODCIELabUtil::dicomLab2RGB(rgb[0], rgb[1], rgb[2], ciedcm[0], ciedcm[1], ciedcm[2]);

        //rgb[0] = unsigned(rgb[0]*256);
        //rgb[1] = unsigned(rgb[1]*256);
        //rgb[2] = unsigned(rgb[2]*256);

        SegmentAttributes* segmentAttributes = metaInfo?metaInfo->createAndGetNewSegment(segmentId):NULL;

        if (segmentAttributes) {
          segmentAttributes->setLabelID(segmentId);
          DcmSegTypes::E_SegmentAlgoType algorithmType = segment->getSegmentAlgorithmType();
          string readableAlgorithmType = DcmSegTypes::algoType2OFString(algorithmType).c_str();
          segmentAttributes->setSegmentAlgorithmType(readableAlgorithmType);

          if (algorithmType == DcmSegTypes::SAT_UNKNOWN) {
            cerr << "ERROR: AlgorithmType is not valid with value " << readableAlgorithmType << endl;
            throw -1;
          }
          if (algorithmType != DcmSegTypes::SAT_MANUAL) {
            OFString segmentAlgorithmName;
            segment->getSegmentAlgorithmName(segmentAlgorithmName);
            if(segmentAlgorithmName.length() > 0)
              segmentAttributes->setSegmentAlgorithmName(segmentAlgorithmName.c_str());
          }

          OFString segmentDescription, segmentLabel, trackingIdentifier, trackingUniqueIdentifier;

          segment->getSegmentDescription(segmentDescription);
          segmentAttributes->setSegmentDescription(segmentDescription.c_str());

          segment->getSegmentLabel(segmentLabel);
          segmentAttributes->setSegmentLabel(segmentLabel.c_str());

          segment->getTrackingID(trackingIdentifier);
          segment->getTrackingUID(trackingUniqueIdentifier);

          if (trackingIdentifier.length() > 0) {
              segmentAttributes->setTrackingIdentifier(trackingIdentifier.c_str());
          }
          if (trackingUniqueIdentifier.length() > 0) {
              segmentAttributes->setTrackingUniqueIdentifier(trackingUniqueIdentifier.c_str());
          }

          segmentAttributes->setRecommendedDisplayRGBValue(rgb[0], rgb[1], rgb[2]);
          segmentAttributes->setSegmentedPropertyCategoryCodeSequence(segment->getSegmentedPropertyCategoryCode());
            segmentAttributes->setSegmentedPropertyTypeCodeSequence(segment->getSegmentedPropertyTypeCode());

          if (segment->getSegmentedPropertyTypeModifierCode().size() > 0) {
              segmentAttributes->setSegmentedPropertyTypeModifierCodeSequence(
                      segment->getSegmentedPropertyTypeModifierCode()[0]);
          }

          GeneralAnatomyMacro &anatomyMacro = segment->getGeneralAnatomyCode();
          CodeSequenceMacro& anatomicRegionSequence = anatomyMacro.getAnatomicRegion();
          if (anatomicRegionSequence.check(true).good()) {
              segmentAttributes->setAnatomicRegionSequence(anatomyMacro.getAnatomicRegion());
          }
          if (anatomyMacro.getAnatomicRegionModifier().size() > 0) {
              segmentAttributes->setAnatomicRegionModifierSequence(*(anatomyMacro.getAnatomicRegionModifier()[0]));
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

      bool deleteFrame(true);
      if(segdoc->getSegmentationType() == DcmSegTypes::ST_BINARY)
        unpackedFrame = DcmSegUtils::unpackBinaryFrame(frame,
                                 imageSize[1], // Rows
                                 imageSize[0]); // Cols
      else {
        unpackedFrame = frame;
        deleteFrame = false;
      }

      // initialize slice with the frame content
      for(unsigned row=0;row<imageSize[1];row++){
        for(unsigned col=0;col<imageSize[0];col++){
          ShortImageType::PixelType pixel;
          unsigned bitCnt = row*imageSize[0]+col;
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

      if(deleteFrame)
        delete unpackedFrame;
    }

    return segment2image;
  }

  void ImageSEGConverter::populateMetaInformationFromDICOM(DcmDataset *segDataset, DcmSegmentation *segdoc,
                               JSONSegmentationMetaInformationHandler &metaInfo) {
    OFString creatorName, sessionID, timePointID, seriesDescription, seriesNumber, instanceNumber, bodyPartExamined, coordinatingCenter;

    segDataset->findAndGetOFString(DCM_InstanceNumber, instanceNumber);
    segdoc->getContentIdentification().getContentCreatorName(creatorName);

    segDataset->findAndGetOFString(DCM_ClinicalTrialTimePointID, timePointID);
    segDataset->findAndGetOFString(DCM_ClinicalTrialSeriesID, sessionID);
    segDataset->findAndGetOFString(DCM_ClinicalTrialCoordinatingCenterName, coordinatingCenter);

    segdoc->getSeries().getBodyPartExamined(bodyPartExamined);
    segdoc->getSeries().getSeriesNumber(seriesNumber);
    segdoc->getSeries().getSeriesDescription(seriesDescription);

    metaInfo.setClinicalTrialCoordinatingCenterName(coordinatingCenter.c_str());
    metaInfo.setContentCreatorName(creatorName.c_str());
    metaInfo.setClinicalTrialSeriesID(sessionID.c_str());
    metaInfo.setSeriesNumber(seriesNumber.c_str());
    metaInfo.setClinicalTrialTimePointID(timePointID.c_str());
    metaInfo.setSeriesDescription(seriesDescription.c_str());
    metaInfo.setInstanceNumber(instanceNumber.c_str());
    metaInfo.setBodyPartExamined(bodyPartExamined.c_str());
  }

}
