#include "ImageSEGConverter.h"
#include <iostream>


namespace dcmqi {

    bool ImageSEGConverter::itkimage2dcmSegmentation(vector<string> dicomImageFileNames, vector<string> segmentationFileNames,
                                             const char *metaDataFileName, const char *outputFileName) {

        ReaderType::Pointer reader = ReaderType::New();
        reader->SetFileName(segmentationFileNames[0].c_str());
        reader->Update();
        ImageType::Pointer labelImage = reader->GetOutput();

        ImageType::SizeType inputSize = labelImage->GetBufferedRegion().GetSize();
        cout << "Input image size: " << inputSize << endl;

        JSONMetaInformationHandler metaInfo(metaDataFileName);

        IODGeneralEquipmentModule::EquipmentInfo eq = getEquipmentInfo();
        ContentIdentificationMacro ident = createContentIdentificationInformation();
        CHECK_COND(ident.setInstanceNumber(metaInfo.instanceNumber.c_str()));


        /* Create new segmentation document */
        DcmDataset segdocDataset;
        DcmSegmentation *segdoc = NULL;

        CHECK_COND(DcmSegmentation::createBinarySegmentation(
                segdoc,   // resulting segmentation
                inputSize[1],      // rows
                inputSize[0],      // columns
                eq,       // equipment
                ident));   // content identification

        /* Import patient and study from existing file */
        CHECK_COND(segdoc->importPatientStudyFoR(dicomImageFileNames[0].c_str(), OFTrue, OFTrue, OFFalse, OFTrue));

        /* Initialize dimension module */
        char dimUID[128];
        dcmGenerateUniqueIdentifier(dimUID, QIICR_UID_ROOT);
        IODMultiframeDimensionModule &mfdim = segdoc->getDimensions();
        CHECK_COND(mfdim.addDimensionIndex(DCM_ReferencedSegmentNumber, dimUID, DCM_SegmentIdentificationSequence,
                                           DcmTag(DCM_ReferencedSegmentNumber).getTagName()));
        CHECK_COND(mfdim.addDimensionIndex(DCM_ImagePositionPatient, dimUID, DCM_PlanePositionSequence,
                                           DcmTag(DCM_ImagePositionPatient).getTagName()));

        /* Initialize shared functional groups */
        FGInterface &segFGInt = segdoc->getFunctionalGroups();
        vector<int> slice2derimg = getSliceMapForSegmentation2DerivationImage(dicomImageFileNames, labelImage);

        const unsigned frameSize = inputSize[0] * inputSize[1];


        // Shared FGs: PlaneOrientationPatientSequence
        {
            OFString imageOrientationPatientStr;

            ImageType::DirectionType labelDirMatrix = labelImage->GetDirection();

            cout << "Directions: " << labelDirMatrix << endl;

            FGPlaneOrientationPatient *planor =
                    FGPlaneOrientationPatient::createMinimal(
                            Helper::floatToStrScientific(labelDirMatrix[0][0]).c_str(),
                            Helper::floatToStrScientific(labelDirMatrix[1][0]).c_str(),
                            Helper::floatToStrScientific(labelDirMatrix[2][0]).c_str(),
                            Helper::floatToStrScientific(labelDirMatrix[0][1]).c_str(),
                            Helper::floatToStrScientific(labelDirMatrix[1][1]).c_str(),
                            Helper::floatToStrScientific(labelDirMatrix[2][1]).c_str());


            //CHECK_COND(planor->setImageOrientationPatient(imageOrientationPatientStr));
            CHECK_COND(segdoc->addForAllFrames(*planor));
        }

        // Shared FGs: PixelMeasuresSequence
        {
            FGPixelMeasures *pixmsr = new FGPixelMeasures();

            ImageType::SpacingType labelSpacing = labelImage->GetSpacing();
            ostringstream spacingSStream;
            spacingSStream << scientific << labelSpacing[0] << "\\" << labelSpacing[1];
            CHECK_COND(pixmsr->setPixelSpacing(spacingSStream.str().c_str()));

            spacingSStream.clear(); spacingSStream.str("");
            spacingSStream << scientific << labelSpacing[2];
            CHECK_COND(pixmsr->setSpacingBetweenSlices(spacingSStream.str().c_str()));
            CHECK_COND(pixmsr->setSliceThickness(spacingSStream.str().c_str()));
            CHECK_COND(segdoc->addForAllFrames(*pixmsr));
        }

        FGPlanePosPatient* fgppp = FGPlanePosPatient::createMinimal("1","1","1");
        FGFrameContent* fgfc = new FGFrameContent();
        FGDerivationImage* fgder = new FGDerivationImage();
        OFVector<FGBase*> perFrameFGs;

        perFrameFGs.push_back(fgppp);
        perFrameFGs.push_back(fgfc);
        perFrameFGs.push_back(fgder);

        // Iterate over the files and labels available in each file, create a segment for each label,
        //  initialize segment frames and add to the document

        OFString seriesInstanceUID, classUID;
        set<OFString> instanceUIDs;

        IODCommonInstanceReferenceModule &commref = segdoc->getCommonInstanceReference();
        OFVector<IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*> &refseries = commref.getReferencedSeriesItems();

        IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem refseriesItem;
        refseries.push_back(&refseriesItem);

        OFVector<SOPInstanceReferenceMacro*> &refinstances = refseriesItem.getReferencedInstanceItems();

        DcmFileFormat ff;
        CHECK_COND(ff.loadFile(dicomImageFileNames[slice2derimg[0]].c_str()));
        DcmDataset *dcm = ff.getDataset();
        CHECK_COND(dcm->findAndGetOFString(DCM_SeriesInstanceUID, seriesInstanceUID));
        CHECK_COND(refseriesItem.setSeriesInstanceUID(seriesInstanceUID));

        int uidfound = 0, uidnotfound = 0;

        Uint8 *frameData = new Uint8[frameSize];
        for(int segFileNumber=0; segFileNumber<segmentationFileNames.size(); segFileNumber++){

            cout << "Processing input label " << segmentationFileNames[segFileNumber] << endl;

            LabelToLabelMapFilterType::Pointer l2lm = LabelToLabelMapFilterType::New();
            reader->SetFileName(segmentationFileNames[segFileNumber]);
            reader->Update();
            ImageType::Pointer labelImage = reader->GetOutput();

            l2lm->SetInput(labelImage);
            l2lm->Update();

            typedef LabelToLabelMapFilterType::OutputImageType::LabelObjectType LabelType;
            typedef itk::LabelStatisticsImageFilter<ImageType,ImageType> LabelStatisticsType;

            LabelStatisticsType::Pointer labelStats = LabelStatisticsType::New();

            cout << "Found " << l2lm->GetOutput()->GetNumberOfLabelObjects() << " label(s)" << endl;
            labelStats->SetInput(reader->GetOutput());
            labelStats->SetLabelInput(reader->GetOutput());
            labelStats->Update();

            bool cropSegmentsBBox = false;
            if(cropSegmentsBBox){
                cout << "WARNING: Crop operation enabled - WIP" << endl;
                typedef itk::BinaryThresholdImageFilter<ImageType,ImageType> ThresholdType;
                ThresholdType::Pointer thresh = ThresholdType::New();
                thresh->SetInput(reader->GetOutput());
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
                return -1;//abort();
            }

            for(int segLabelNumber=0 ; segLabelNumber<l2lm->GetOutput()->GetNumberOfLabelObjects();segLabelNumber++){
                LabelType* labelObject = l2lm->GetOutput()->GetNthLabelObject(segLabelNumber);
                short label = labelObject->GetLabel();

                if(!label){
                    cout << "Skipping label 0" << endl;
                    continue;
                }

                cout << "Processing label " << label << endl;

                LabelStatisticsType::BoundingBoxType bbox = labelStats->GetBoundingBox(label);
                unsigned firstSlice, lastSlice;
                bool skipEmptySlices = true; // TODO: what to do with that line?
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
                string segFileName = segmentationFileNames[segFileNumber];

                SegmentAttributes* segmentAttributes = metaInfo.segmentsAttributes[segFileNumber];

                DcmSegTypes::E_SegmentAlgoType algoType;
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
                        return -1;
                    }
                }

                CodeSequenceMacro* typeCode = segmentAttributes->getSegmentedPropertyType();
                CodeSequenceMacro* categoryCode = segmentAttributes->getSegmentedPropertyCategoryCode();
                assert(typeCode != NULL && categoryCode!= NULL);
                OFString segmentLabel;
                CHECK_COND(typeCode->getCodeMeaning(segmentLabel));
                CHECK_COND(DcmSegment::create(segment, segmentLabel, *categoryCode, *typeCode, algoType, algoName.c_str()));

                if(segmentAttributes->getSegmentDescription().length() > 0)
                    segment->setSegmentDescription(segmentAttributes->getSegmentDescription().c_str());

                CodeSequenceMacro* typeModifierCode = segmentAttributes->getSegmentedPropertyTypeModifier();
                if (typeModifierCode != NULL) {
                    OFVector<CodeSequenceMacro*>& modifiersVector = segment->getSegmentedPropertyTypeModifierCode();
                    modifiersVector.push_back(typeModifierCode);
                }

                GeneralAnatomyMacro &anatomyMacro = segment->getGeneralAnatomyCode();
                if (segmentAttributes->getAnatomicRegion() != NULL){
                    OFVector<CodeSequenceMacro*>& anatomyMacroModifiersVector = anatomyMacro.getAnatomicRegionModifier();
                    CodeSequenceMacro& anatomicRegion = anatomyMacro.getAnatomicRegion();
                    anatomicRegion = *segmentAttributes->getAnatomicRegion();

                    if(segmentAttributes->getAnatomicRegionModifier() != NULL){
                        CodeSequenceMacro* anatomicRegionModifier = segmentAttributes->getAnatomicRegionModifier();
                        anatomyMacroModifiersVector.push_back(anatomicRegionModifier);
                    }
                }

                // TODO: Maybe implement for PrimaryAnatomicStructure and PrimaryAnatomicStructureModifier

                unsigned* rgb = segmentAttributes->getRecommendedDisplayRGBValue();
                unsigned cielabScaled[3];
                float cielab[3], ciexyz[3];

                Helper::getCIEXYZFromRGB(&rgb[0],&ciexyz[0]);
                Helper::getCIELabFromCIEXYZ(&ciexyz[0],&cielab[0]);
                Helper::getIntegerScaledCIELabFromCIELab(&cielab[0],&cielabScaled[0]);
                CHECK_COND(segment->setRecommendedDisplayCIELabValue(cielabScaled[0],cielabScaled[1],cielabScaled[2]));

                Uint16 segmentNumber;
                CHECK_COND(segdoc->addSegment(segment, segmentNumber /* returns logical segment number */));

                // TODO: make it possible to skip empty frames (optional)
                // iterate over slices for an individual label and populate output frames
                for(int sliceNumber=firstSlice;sliceNumber<lastSlice;sliceNumber++){

                    // segments are numbered starting from 1
                    Uint32 frameNumber = (segmentNumber-1)*inputSize[2]+sliceNumber;

                    OFString imagePositionPatientStr;

                    // PerFrame FG: FrameContentSequence
                    //fracon->setStackID("1"); // all frames go into the same stack
                    CHECK_COND(fgfc->setDimensionIndexValues(segmentNumber, 0));
                    CHECK_COND(fgfc->setDimensionIndexValues(sliceNumber-firstSlice+1, 1));
                    //ostringstream inStackPosSStream; // StackID is not present/needed
                    //inStackPosSStream << s+1;
                    //fracon->setInStackPositionNumber(s+1);

                    // PerFrame FG: PlanePositionSequence
                    {
                        ImageType::PointType sliceOriginPoint;
                        ImageType::IndexType sliceOriginIndex;
                        sliceOriginIndex.Fill(0);
                        sliceOriginIndex[2] = sliceNumber;
                        labelImage->TransformIndexToPhysicalPoint(sliceOriginIndex, sliceOriginPoint);
                        ostringstream pppSStream;
                        if(sliceNumber>0){
                            ImageType::PointType prevOrigin;
                            ImageType::IndexType prevIndex;
                            prevIndex.Fill(0);
                            prevIndex[2] = sliceNumber-1;
                            labelImage->TransformIndexToPhysicalPoint(prevIndex, prevOrigin);
                        }
                        fgppp->setImagePositionPatient(
                                Helper::floatToStrScientific(sliceOriginPoint[0]).c_str(),
                                Helper::floatToStrScientific(sliceOriginPoint[1]).c_str(),
                                Helper::floatToStrScientific(sliceOriginPoint[2]).c_str());
                    }

                    /* Add frame that references this segment */
                    {
                        ImageType::RegionType sliceRegion;
                        ImageType::IndexType sliceIndex;
                        ImageType::SizeType sliceSize;

                        sliceIndex[0] = 0;
                        sliceIndex[1] = 0;
                        sliceIndex[2] = sliceNumber;

                        sliceSize[0] = inputSize[0];
                        sliceSize[1] = inputSize[1];
                        sliceSize[2] = 1;

                        sliceRegion.SetIndex(sliceIndex);
                        sliceRegion.SetSize(sliceSize);

                        unsigned framePixelCnt = 0;
                        itk::ImageRegionConstIteratorWithIndex<ImageType> sliceIterator(labelImage, sliceRegion);
                        for(sliceIterator.GoToBegin();!sliceIterator.IsAtEnd();++sliceIterator,++framePixelCnt){
                            if(sliceIterator.Get() == label){
                                frameData[framePixelCnt] = 1;
                                ImageType::IndexType idx = sliceIterator.GetIndex();
                                //cout << framePixelCnt << " " << idx[1] << "," << idx[0] << endl;
                            } else
                                frameData[framePixelCnt] = 0;
                        }

                        if(sliceNumber!=firstSlice)
                            Helper::checkValidityOfFirstSrcImage(segdoc);

                        FGDerivationImage* fgder = new FGDerivationImage();
                        perFrameFGs[2] = fgder;
                        //fgder->clearData();

                        if(sliceNumber!=firstSlice)
                            Helper::checkValidityOfFirstSrcImage(segdoc);

                        DerivationImageItem *derimgItem;
                        CHECK_COND(fgder->addDerivationImageItem(CodeSequenceMacro("113076","DCM","Segmentation"),"",derimgItem));

                        OFVector<OFString> siVector;

                        if(sliceNumber>=dicomImageFileNames.size()){
                            cerr << "ERROR: trying to access missing DICOM Slice! And sorry, multi-frame not supported at the moment..." << endl;
                            return -1;
                        }

                        siVector.push_back(OFString(dicomImageFileNames[slice2derimg[sliceNumber]].c_str()));

                        OFVector<SourceImageItem*> srcimgItems;
                        CHECK_COND(derimgItem->addSourceImageItems(siVector,
                                                                   CodeSequenceMacro("121322","DCM","Source image for image processing operation"),
                                                                   srcimgItems));

                        CHECK_COND(segdoc->addFrame(frameData, segmentNumber, perFrameFGs));

                        // check if frame 0 still has what we expect
                        Helper::checkValidityOfFirstSrcImage(segdoc);

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
                }
            }
        }

    //cout << "found:" << uidfound << " not: " << uidnotfound << endl;

    COUT << "Successfully created segmentation document" << OFendl;

    /* Store to disk */
    COUT << "Saving the result to " << outputFileName << OFendl;
    //segdoc->saveFile(outputFileName.c_str(), EXS_LittleEndianExplicit);

    CHECK_COND(segdoc->writeDataset(segdocDataset));

    // Set reader/session/timepoint information
    CHECK_COND(segdocDataset.putAndInsertString(DCM_ContentCreatorName, metaInfo.readerID.c_str()));
    CHECK_COND(segdocDataset.putAndInsertString(DCM_ClinicalTrialSeriesID, metaInfo.sessionID.c_str()));
    CHECK_COND(segdocDataset.putAndInsertString(DCM_ClinicalTrialTimePointID, metaInfo.timePointID.c_str()));
    CHECK_COND(segdocDataset.putAndInsertString(DCM_ClinicalTrialCoordinatingCenterName, "UIowa"));

    // populate BodyPartExamined
    {
        DcmFileFormat sliceFF;
        DcmDataset *sliceDataset = NULL;
        OFString bodyPartStr;
        string bodyPartAssigned = metaInfo.bodyPartExamined;

        CHECK_COND(sliceFF.loadFile(dicomImageFileNames[0].c_str()));

        sliceDataset = sliceFF.getDataset();

        // inherit BodyPartExamined from the source image dataset, if available
        if(sliceDataset->findAndGetOFString(DCM_BodyPartExamined, bodyPartStr).good())
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

        segdocDataset.putAndInsertString(DCM_ContentDate, contentDate.c_str());
        segdocDataset.putAndInsertString(DCM_ContentTime, contentTime.c_str());
        segdocDataset.putAndInsertString(DCM_SeriesDate, contentDate.c_str());
        segdocDataset.putAndInsertString(DCM_SeriesTime, contentTime.c_str());

        segdocDataset.putAndInsertString(DCM_SeriesDescription, metaInfo.seriesDescription.c_str());
        segdocDataset.putAndInsertString(DCM_SeriesNumber, metaInfo.seriesNumber.c_str());
    }

    DcmFileFormat segdocFF(&segdocDataset);
    bool compress = false; // TODO: remove hardcoded
    if(compress){
        CHECK_COND(segdocFF.saveFile(outputFileName, EXS_DeflatedLittleEndianExplicit));
    } else {
        CHECK_COND(segdocFF.saveFile(outputFileName, EXS_LittleEndianExplicit));
    }

    COUT << "Saved segmentation as " << outputFileName << endl;
        return true;
    }

    bool ImageSEGConverter::dcmSegmentation2itkimage(const char *inputSEGFileName, const char *outputDirName) {

        DcmRLEDecoderRegistration::registerCodecs();

        dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);

        DcmFileFormat segFF;
        DcmDataset *segDataset = NULL;
        if(segFF.loadFile(inputSEGFileName).good()){
            segDataset = segFF.getDataset();
        } else {
            cerr << "Failed to read input " << endl;
            return EXIT_FAILURE;
        }

        DcmSegmentation *segdoc = NULL;
        OFCondition cond = DcmSegmentation::loadFile(inputSEGFileName, segdoc);
        if(!segdoc){
            cerr << "Failed to load seg! " << cond.text() << endl;
            return EXIT_FAILURE;
        }

        // Directions
        FGInterface &fgInterface = segdoc->getFunctionalGroups();
        ImageType::DirectionType direction;
        if(getImageDirections(fgInterface, direction)){
            cerr << "Failed to get image directions" << endl;
            return EXIT_FAILURE;
        }

        // Spacing and origin
        double computedSliceSpacing, computedVolumeExtent;
        vnl_vector<double> sliceDirection(3);
        sliceDirection[0] = direction[0][2];
        sliceDirection[1] = direction[1][2];
        sliceDirection[2] = direction[2][2];

        ImageType::PointType imageOrigin;
        if(computeVolumeExtent(fgInterface, sliceDirection, imageOrigin, computedSliceSpacing, computedVolumeExtent)){
            cerr << "Failed to compute origin and/or slice spacing!" << endl;
            return EXIT_FAILURE;
        }

        ImageType::SpacingType imageSpacing;
        imageSpacing.Fill(0);
        if(getDeclaredImageSpacing(fgInterface, imageSpacing)){
            cerr << "Failed to get image spacing from DICOM!" << endl;
            return EXIT_FAILURE;
        }

        const double tolerance = 1e-5;
        if(!imageSpacing[2]){
            imageSpacing[2] = computedSliceSpacing;
        }
        else if(fabs(imageSpacing[2]-computedSliceSpacing)>tolerance){
            cerr << "WARNING: Declared slice spacing is significantly different from the one declared in DICOM!" <<
            " Declared = " << imageSpacing[2] << " Computed = " << computedSliceSpacing << endl;
        }

        // Region size
        ImageType::SizeType imageSize;
        {
            OFString str;
            if(segDataset->findAndGetOFString(DCM_Rows, str).good()){
                imageSize[1] = atoi(str.c_str());
            }
            if(segDataset->findAndGetOFString(DCM_Columns, str).good()){
                imageSize[0] = atoi(str.c_str());
            }
        }
        // number of slices should be computed, since segmentation may have empty frames
        // Small differences in image spacing could lead to computing number of slices incorrectly
        // (it is always floored), round it with a tolerance instead.
        imageSize[2] = int(computedVolumeExtent/imageSpacing[2]/tolerance+0.5)*tolerance+1;

        // Initialize the image
        ImageType::RegionType imageRegion;
        imageRegion.SetSize(imageSize);
        ImageType::Pointer segImage = ImageType::New();
        segImage->SetRegions(imageRegion);
        segImage->SetOrigin(imageOrigin);
        segImage->SetSpacing(imageSpacing);
        segImage->SetDirection(direction);
        segImage->Allocate();
        segImage->FillBuffer(0);

        // ITK images corresponding to the individual segments
        map<unsigned,ImageType::Pointer> segment2image;
        // list of strings that
        map<unsigned,string> segment2meta;

        // Iterate over frames, find the matching slice for each of the frames based on
        // ImagePositionPatient, set non-zero pixels to the segment number. Notify
        // about pixels that are initialized more than once.

        DcmIODTypes::Frame *unpackedFrame = NULL;

        JSONMetaInformationHandler metaInfo;

        populateMetaInformationFromDICOM(segDataset, segdoc, metaInfo);

        for(int frameId=0;frameId<fgInterface.getNumberOfFrames();frameId++){
            const DcmIODTypes::Frame *frame = segdoc->getFrame(frameId);
            bool isPerFrame;

            FGPlanePosPatient *planposfg =
                    OFstatic_cast(FGPlanePosPatient*,fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
            assert(planposfg);

            FGFrameContent *fracon =
                    OFstatic_cast(FGFrameContent*,fgInterface.get(frameId, DcmFGTypes::EFG_FRAMECONTENT, isPerFrame));
            assert(fracon);

            FGSegmentation *fgseg =
                    OFstatic_cast(FGSegmentation*,fgInterface.get(frameId, DcmFGTypes::EFG_SEGMENTATION, isPerFrame));
            assert(fgseg);

            Uint16 segmentId = -1;
            if(fgseg->getReferencedSegmentNumber(segmentId).bad()){
                cerr << "Failed to get seg number!";
                return EXIT_FAILURE;
            }

            // WARNING: this is needed only for David's example, which numbers
            // (incorrectly!) segments starting from 0, should start from 1
            if(segmentId == 0){
                cerr << "Segment numbers should start from 1!" << endl;
                return EXIT_FAILURE;
            }

            if(segment2image.find(segmentId) == segment2image.end()){
                typedef itk::ImageDuplicator<ImageType> DuplicatorType;
                DuplicatorType::Pointer dup = DuplicatorType::New();
                dup->SetInputImage(segImage);
                dup->Update();
                ImageType::Pointer newSegmentImage = dup->GetOutput();
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
                    return EXIT_FAILURE;
                }

                // get CIELab color for the segment
                Uint16 ciedcm[3];
                unsigned cielabScaled[3];
                float cielab[3], ciexyz[3];
                unsigned rgb[3];
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
                cielabScaled[0] = unsigned(ciedcm[0]);
                cielabScaled[1] = unsigned(ciedcm[1]);
                cielabScaled[2] = unsigned(ciedcm[2]);

                dcmqi::Helper::getCIELabFromIntegerScaledCIELab(&cielabScaled[0],&cielab[0]);
                dcmqi::Helper::getCIEXYZFromCIELab(&cielab[0],&ciexyz[0]);
                dcmqi::Helper::getRGBFromCIEXYZ(&ciexyz[0],&rgb[0]);

                SegmentAttributes* segmentAttributes = metaInfo.createAndGetNewSegment(segmentId);

                if (segmentAttributes) {
                    switch(segment->getSegmentAlgorithmType()) {
                        case DcmSegTypes::SAT_MANUAL:
                            segmentAttributes->setSegmentAlgorithmType("MANUAL");
                        case DcmSegTypes::SAT_SEMIAUTOMATIC:
                            segmentAttributes->setSegmentAlgorithmType("SEMIAUTOMATIC");
                        case DcmSegTypes::SAT_AUTOMATIC:
                            segmentAttributes->setSegmentAlgorithmType("AUTOMATIC");
                        default:
                            segmentAttributes->setSegmentAlgorithmType("SEMIAUTOMATIC");
                    }

                    OFString segmentDescription;
                    segment->getSegmentDescription(segmentDescription);
                    segmentAttributes->setSegmentDescription(segmentDescription.c_str());

//                  TODO: dcmtk method for getting algorithm name is missing???

                    segmentAttributes->setRecommendedDisplayRGBValue(rgb[0], rgb[1], rgb[2]);
                    segmentAttributes->setSegmentedPropertyCategoryCode(segment->getSegmentedPropertyCategoryCode());
                    segmentAttributes->setSegmentedPropertyType(segment->getSegmentedPropertyTypeCode());

                    if (segment->getSegmentedPropertyTypeModifierCode().size() > 0) {
                        segmentAttributes->setSegmentedPropertyTypeModifier(
                                segment->getSegmentedPropertyTypeModifierCode()[0]);
                    }

                    GeneralAnatomyMacro &anatomyMacro = segment->getGeneralAnatomyCode();
                    segmentAttributes->setAnatomicRegion(anatomyMacro.getAnatomicRegion());
                    if (anatomyMacro.getAnatomicRegionModifier().size() > 0) {
                        segmentAttributes->setAnatomicRegionModifier(anatomyMacro.getAnatomicRegionModifier()[0]);
                    }
                }
            }

            // get string representation of the frame origin
            ImageType::PointType frameOriginPoint;
            ImageType::IndexType frameOriginIndex;
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
                return EXIT_FAILURE;
            }

            unsigned slice = frameOriginIndex[2];

            if(segdoc->getSegmentationType() == DcmSegTypes::ST_BINARY)
                unpackedFrame = DcmSegUtils::unpackBinaryFrame(frame,
                                                               imageSize[1], // Rows
                                                               imageSize[0]); // Cols
            else
                unpackedFrame = new DcmIODTypes::Frame(*frame);

            // initialize slice with the frame content
            for(int row=0;row<imageSize[1];row++){
                for(int col=0;col<imageSize[0];col++){
                    ImageType::PixelType pixel;
                    unsigned bitCnt = row*imageSize[0]+col;
                    pixel = unpackedFrame->pixData[bitCnt];

                    if(pixel!=0){
                        ImageType::IndexType index;
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

        for(map<unsigned,ImageType::Pointer>::const_iterator sI=segment2image.begin();sI!=segment2image.end();++sI){
            typedef itk::ImageFileWriter<ImageType> WriterType;
            stringstream imageFileNameSStream;
            imageFileNameSStream << outputDirName << "/" << sI->first << ".nrrd";

            WriterType::Pointer writer = WriterType::New();
            writer->SetFileName(imageFileNameSStream.str().c_str());
            writer->SetInput(sI->second);
            writer->SetUseCompression(1);
            writer->Update();
        }

        stringstream jsonOutput;
        jsonOutput << outputDirName << "/" << "meta.json";
        metaInfo.write(jsonOutput.str().c_str());

        return true;
    }

    IODGeneralEquipmentModule::EquipmentInfo ImageSEGConverter::getEquipmentInfo() {
        IODGeneralEquipmentModule::EquipmentInfo eq;
        eq.m_Manufacturer = "QIICR";
        eq.m_DeviceSerialNumber = "0";
        eq.m_ManufacturerModelName = dcmqi_WC_URL;
        eq.m_SoftwareVersions = dcmqi_WC_REVISION;
        return eq;
    }

    ContentIdentificationMacro ImageSEGConverter::createContentIdentificationInformation() {
        ContentIdentificationMacro ident;
        CHECK_COND(ident.setContentCreatorName("QIICR"));
        CHECK_COND(ident.setContentDescription("Iowa QIN segmentation result"));
        CHECK_COND(ident.setContentLabel("QIICR QIN IOWA"));
        return ident;
    }

    vector<int> ImageSEGConverter::getSliceMapForSegmentation2DerivationImage(const vector<string> &dicomImageFileNames,
                                                                              const itk::Image<short, 3>::Pointer &labelImage) {
        // Find mapping from the segmentation slice number to the derivation image
        // Assume that orientation of the segmentation is the same as the source series
        vector<int> slice2derimg(dicomImageFileNames.size());
        for(int i=0;i<dicomImageFileNames.size();i++){
            OFString ippStr;
            DcmFileFormat sliceFF;
            DcmDataset *sliceDataset = NULL;
            ImageType::PointType ippPoint;
            ImageType::IndexType ippIndex;
            CHECK_COND(sliceFF.loadFile(dicomImageFileNames[i].c_str()));
            sliceDataset = sliceFF.getDataset();
            for(int j=0;j<3;j++){
                CHECK_COND(sliceDataset->findAndGetOFString(DCM_ImagePositionPatient, ippStr, j));
                ippPoint[j] = atof(ippStr.c_str());
            }
            if(!labelImage->TransformPhysicalPointToIndex(ippPoint, ippIndex)){
                cout << "image position: " << ippPoint << endl;
                cerr << "ippIndex: " << ippIndex << endl;
                throw DCMQIImagePositionPatientMapsOutsideITKException("ImagePositionPatient maps outside the ITK image!");
            }
            slice2derimg[ippIndex[2]] = i;
        }
        return slice2derimg;
    }

    int ImageSEGConverter::CHECK_COND(const OFCondition& condition) {
        if (condition.bad()) {
            cerr << condition.text() << " in " __FILE__ << ":" << __LINE__  << endl;
            throw OFConditionBadException();
        }
        return 0;
    }

    int ImageSEGConverter::getImageDirections(FGInterface &fgInterface, ImageType::DirectionType &dir){
        // For directions, we can only handle segments that have patient orientation
        //  identical for all frames, so either find it in shared FG, or fail
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

        for(int i=0;i<3;i++){
            dir[i][0] = rowDirection[i];
            dir[i][1] = colDirection[i];
            dir[i][2] = sliceDirection[i];
        }

        return 0;
    }

    int ImageSEGConverter::computeVolumeExtent(FGInterface &fgInterface, vnl_vector<double> &sliceDirection,
                                               ImageType::PointType &imageOrigin, double &sliceSpacing,
                                               double &sliceExtent){
        // Size
        // Rows/Columns can be read directly from the respective attributes
        // For number of slices, consider that all segments must have the same number of frames.
        //   If we have FoR UID initialized, this means every segment should also have Plane
        //   Position (Patient) initialized. So we can get the number of slices by looking
        //   how many per-frame functional groups a segment has.

        vector<double> originDistances;
        map<OFString, double> originStr2distance;
        map<OFString, unsigned> frame2overlap;
        double minDistance;

        unsigned numFrames = fgInterface.getNumberOfFrames();

        /* Framesorter is to be moved to DCMTK at some point
         * in the future. For now it is causing build issues on windows

        FrameSorterIPP fsIPP;
        FrameSorterIPP::Results sortResults;
        fsIPP.setSorterInput(&fgInterface);
        fsIPP.sort(sortResults);

        */

        // Determine ordering of the frames, keep mapping from ImagePositionPatient string
        //   to the distance, and keep track (just out of curiousity) how many frames overlap
        vnl_vector<double> refOrigin(3);
        {
            OFBool isPerFrame;
            FGPlanePosPatient *planposfg =
                    OFstatic_cast(FGPlanePosPatient*,fgInterface.get(0, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
            for(int j=0;j<3;j++){
                OFString planposStr;
                if(planposfg->getImagePositionPatient(planposStr, j).good()){
                    refOrigin[j] = atof(planposStr.c_str());
                } else {
                    cerr << "Failed to read patient position" << endl;
                }
            }
        }

        for(int frameId=0;frameId<numFrames;frameId++){
            OFBool isPerFrame;
            FGPlanePosPatient *planposfg =
                    OFstatic_cast(FGPlanePosPatient*,fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));

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
                else
                if(dist<minDistance){
                    imageOrigin[0] = sOrigin[0];
                    imageOrigin[1] = sOrigin[1];
                    imageOrigin[2] = sOrigin[2];
                    minDistance = dist;
                }
            } else {
                frame2overlap[sOriginStr]++;
            }
        }

        // sort all unique distances, this will be used to check consistency of
        //  slice spacing, and also to locate the slice position from ImagePositionPatient
        //  later when we read the segments
        sort(originDistances.begin(), originDistances.end());

        sliceSpacing = fabs(originDistances[0]-originDistances[1]);

        for(int i=1;i<originDistances.size();i++){
            float dist1 = fabs(originDistances[i-1]-originDistances[i]);
            float delta = sliceSpacing-dist1;
            if(delta > 0.001){
                cerr << "WARNING: Inter-slice distance " << originDistances[i] <<
                " difference exceeded threshold: " << delta << endl;
            }
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

        return 0;
    }

    int ImageSEGConverter::getDeclaredImageSpacing(FGInterface &fgInterface, ImageType::SpacingType &spacing){
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
            cerr << "WARNING: SliceThickness is present and is " << spacingFloat << ". NOT using it!" << endl;
        }
        return 0;
    }

    void ImageSEGConverter::populateMetaInformationFromDICOM(DcmDataset *segDataset, DcmSegmentation *segdoc,
                                                             JSONMetaInformationHandler &metaInfo) {
        OFString readerID, sessionID, timePointID, seriesDescription, seriesNumber, instanceNumber, bodyPartExamined;

        ContentIdentificationMacro& contentIdentificationMacro = segdoc->getContentIdentification();
        contentIdentificationMacro.getInstanceNumber(instanceNumber);
        contentIdentificationMacro.getContentCreatorName(readerID);

        segDataset->findAndGetOFString(DCM_ClinicalTrialTimePointID, timePointID);
        segDataset->findAndGetOFString(DCM_ClinicalTrialSeriesID, sessionID);

        IODGeneralSeriesModule seriesModule = segdoc->getSeries();
        seriesModule.getBodyPartExamined(bodyPartExamined);
        seriesModule.getSeriesNumber(seriesNumber);
        seriesModule.getSeriesDescription(seriesDescription);

        metaInfo.readerID = readerID.c_str();
        metaInfo.sessionID = sessionID.c_str();
        metaInfo.seriesNumber = seriesNumber.c_str();
        metaInfo.timePointID = timePointID.c_str();
        metaInfo.seriesDescription = seriesDescription.c_str();
        metaInfo.instanceNumber = instanceNumber.c_str();
        metaInfo.bodyPartExamined = bodyPartExamined.c_str();
    }

}


