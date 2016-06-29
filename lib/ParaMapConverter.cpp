#include <itkImageRegionConstIteratorWithIndex.h>
#include "ParaMapConverter.h"


using namespace std;

namespace dcmqi {

    int ParaMapConverter::itkimage2dcmParaMap(const string &inputFileName, const string &metaDataFileName,
                                              const string &outputFileName) {

        ReaderType::Pointer reader = ReaderType::New();
        reader->SetFileName(inputFileName.c_str());
        reader->Update();
        ImageType::Pointer parametricMapImage = reader->GetOutput();

        ImageType::SizeType inputSize = parametricMapImage->GetBufferedRegion().GetSize();
        cout << "Input image size: " << inputSize << endl;

        JSONParametricMapMetaInformationHandler metaInfo(metaDataFileName);
        metaInfo.read();

        IODEnhGeneralEquipmentModule::EquipmentInfo eq = getEnhEquipmentInfo();
        ContentIdentificationMacro contentID = createContentIdentificationInformation();
        CHECK_COND(contentID.setInstanceNumber(metaInfo.seriesAttributes->getInstanceNumber().c_str()));

        DcmDataset pMapDocDataset;
        DPMParametricMapFloat *pMapDoc = NULL;

        // TODO: following should maybe be moved to meta info
        OFString imageFlavor = "VOLUME";
        OFString pixContrast = "MTT";
        DPMTypes::ContentQualification contQual = DPMTypes::CQ_RESEARCH;
        OFString modality = "MR";


        CHECK_COND(DPMParametricMapFloat::create(pMapDoc, modality, metaInfo.seriesAttributes->getSeriesNumber().c_str(),
                                                 metaInfo.seriesAttributes->getInstanceNumber().c_str(),
                                                 inputSize[0], inputSize[1], eq, contentID,
                                                 imageFlavor, pixContrast, contQual));

        /* Import patient and study from existing file */
//        CHECK_COND(segdoc->importPatientStudyFoR(dicomImageFileNames[0].c_str(), OFTrue, OFTrue, OFFalse, OFTrue));

        /* Initialize dimension module */
        char dimUID[128];
        dcmGenerateUniqueIdentifier(dimUID, QIICR_UID_ROOT);
        IODMultiframeDimensionModule &mfdim = pMapDoc->getIODMultiframeDimensionModule();
        OFCondition result = mfdim.addDimensionIndex(DCM_ImagePositionPatient, dimUID,
                                                     DCM_RealWorldValueMappingSequence, "Frame position");

        // Shared FGs: PixelMeasuresSequence
        {
            FGPixelMeasures *pixmsr = new FGPixelMeasures();

            ImageType::SpacingType labelSpacing = parametricMapImage->GetSpacing();
            ostringstream spacingSStream;
            spacingSStream << scientific << labelSpacing[0] << "\\" << labelSpacing[1];
            CHECK_COND(pixmsr->setPixelSpacing(spacingSStream.str().c_str()));

            spacingSStream.clear(); spacingSStream.str("");
            spacingSStream << scientific << labelSpacing[2];
            CHECK_COND(pixmsr->setSpacingBetweenSlices(spacingSStream.str().c_str()));
            CHECK_COND(pixmsr->setSliceThickness(spacingSStream.str().c_str()));
            CHECK_COND(pMapDoc->addForAllFrames(*pixmsr));
        }

        // Shared FGs: PlaneOrientationPatientSequence
        {
            OFString imageOrientationPatientStr;

            ImageType::DirectionType labelDirMatrix = parametricMapImage->GetDirection();

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
            CHECK_COND(pMapDoc->addForAllFrames(*planor));
        }

        FGFrameAnatomy frameAnaFG;
        frameAnaFG.setLaterality(FGFrameAnatomy::LATERALITY_UNPAIRED);
        frameAnaFG.getAnatomy().getAnatomicRegion().set("T-A0100", "SRT", "Brain");
        CHECK_COND(pMapDoc->addForAllFrames(frameAnaFG));

        FGIdentityPixelValueTransformation idTransFG;
        CHECK_COND(pMapDoc->addForAllFrames(idTransFG));

        FGParametricMapFrameType frameTypeFG;
        frameTypeFG.setFrameType("DERIVED\\PRIMARY\\VOLUME\\MTT");
        CHECK_COND(pMapDoc->addForAllFrames(frameTypeFG));

        for (unsigned long f = 0; result.good() && (f < inputSize[2]); f++) {
            result = addFrame(pMapDoc, parametricMapImage, f);
        }

    //cout << "found:" << uidfound << " not: " << uidnotfound << endl;

    COUT << "Successfully created parametric map document" << OFendl;

    // Set reader/session/timepoint information
    CHECK_COND(pMapDocDataset.putAndInsertString(DCM_ContentCreatorName, metaInfo.seriesAttributes->getReaderID().c_str()));
    CHECK_COND(pMapDocDataset.putAndInsertString(DCM_ClinicalTrialSeriesID, metaInfo.seriesAttributes->getSessionID().c_str()));
    CHECK_COND(pMapDocDataset.putAndInsertString(DCM_ClinicalTrialTimePointID, metaInfo.seriesAttributes->getTimePointID().c_str()));
    CHECK_COND(pMapDocDataset.putAndInsertString(DCM_ClinicalTrialCoordinatingCenterName, "UIowa"));

    // populate BodyPartExamined
    {
        DcmFileFormat sliceFF;
        DcmDataset *sliceDataset = NULL;
        OFString bodyPartStr;
        string bodyPartAssigned = metaInfo.seriesAttributes->getBodyPartExamined();

//        TODO: add the following only if source dicom file was parameterized
//        CHECK_COND(sliceFF.loadFile(dicomImageFileNames[0].c_str()));
//        sliceDataset = sliceFF.getDataset();
//        // inherit BodyPartExamined from the source image dataset, if available
//        if(sliceDataset->findAndGetOFString(DCM_BodyPartExamined, bodyPartStr).good())
//
//        if(string(bodyPartStr.c_str()).size())
//            bodyPartAssigned = bodyPartStr.c_str();

        if(bodyPartAssigned.size())
            CHECK_COND(pMapDocDataset.putAndInsertString(DCM_BodyPartExamined, bodyPartAssigned.c_str()));
    }

    // SeriesDate/Time should be of when parametric map was taken; initialize to when it was saved
    {
        OFString contentDate, contentTime;
        DcmDate::getCurrentDate(contentDate);
        DcmTime::getCurrentTime(contentTime);

        pMapDocDataset.putAndInsertString(DCM_ContentDate, contentDate.c_str());
        pMapDocDataset.putAndInsertString(DCM_ContentTime, contentTime.c_str());
        pMapDocDataset.putAndInsertString(DCM_SeriesDate, contentDate.c_str());
        pMapDocDataset.putAndInsertString(DCM_SeriesTime, contentTime.c_str());

        pMapDocDataset.putAndInsertString(DCM_SeriesDescription, metaInfo.seriesAttributes->getSeriesDescription().c_str());
        pMapDocDataset.putAndInsertString(DCM_SeriesNumber, metaInfo.seriesAttributes->getSeriesNumber().c_str());
    }

    CHECK_COND(pMapDoc->writeDataset(pMapDocDataset));
    CHECK_COND(pMapDoc->saveFile(outputFileName.c_str()));

    COUT << "Saved parametric map as " << outputFileName << endl;
        return EXIT_SUCCESS;
    }

    int ParaMapConverter::paraMap2itkimage(const string &inputSEGFileName, const string &outputDirName) {
        return EXIT_SUCCESS;
    }

    OFCondition ParaMapConverter::addFrame(DPMParametricMapFloat *map, const ImageType::Pointer &parametricMapImage,
                                           const unsigned long frameNo)
    {
        ImageType::RegionType sliceRegion;
        ImageType::IndexType sliceIndex;
        ImageType::SizeType inputSize = parametricMapImage->GetBufferedRegion().GetSize();

        sliceIndex[0] = 0;
        sliceIndex[1] = 0;
        sliceIndex[2] = frameNo;

        inputSize[2] = 1;

        sliceRegion.SetIndex(sliceIndex);
        sliceRegion.SetSize(inputSize);

        const unsigned frameSize = inputSize[0] * inputSize[1];

        Float32 *data = new Float32[frameSize];

        if (data == NULL) {
            return EC_MemoryExhausted;
        }

        itk::ImageRegionConstIteratorWithIndex<ImageType> sliceIterator(parametricMapImage, sliceRegion);

        unsigned framePixelCnt = 0;
        for(sliceIterator.GoToBegin();!sliceIterator.IsAtEnd(); ++sliceIterator, ++framePixelCnt){
            data[framePixelCnt] = sliceIterator.Get();
            ImageType::IndexType idx = sliceIterator.GetIndex();
//            cout << framePixelCnt << " " << idx[1] << "," << idx[0] << endl;
        }

        // Create functional groups
        OFVector<FGBase*> groups;
        OFunique_ptr<FGPlanePosPatient> fgPlanePos(new FGPlanePosPatient);
        OFunique_ptr<FGFrameContent > fgFracon(new FGFrameContent);
        OFunique_ptr<FGRealWorldValueMapping> fgRVWM(new FGRealWorldValueMapping());
        FGRealWorldValueMapping::RWVMItem* rvwmItemSimple = new FGRealWorldValueMapping::RWVMItem();
        // FGRealWorldValueMapping::RWVMItem* rvwmItemLUT = new FGRealWorldValueMapping::RWVMItem();
        if (!fgPlanePos  || !fgFracon || !fgRVWM || !rvwmItemSimple )
        {
            delete[] data;
            return EC_MemoryExhausted;
        }
        // Fill in functional group values

        // Real World Value Mapping
        rvwmItemSimple->setRealWorldValueSlope(10);
        rvwmItemSimple->setRealWorldValueIntercept(0);

        // TODO: get from meta information
        rvwmItemSimple->getMeasurementUnitsCode().set("{Particles}/[100]g{Tissue}", "UCUM", "number particles per 100 gram of tissue");
        rvwmItemSimple->setLUTExplanation("We are mapping trash to junk.");
        rvwmItemSimple->setLUTLabel("Just testing");

        CodeSequenceMacro* qCodeName = new CodeSequenceMacro("G-C1C6", "SRT", "Quantity");
        CodeSequenceMacro* qSpec = new CodeSequenceMacro("110805", "SRT", "T2 Weighted MR Signal Intensity");
        ContentItemMacro* quantity = new ContentItemMacro;

        if (!quantity || !qSpec || !quantity)
        {
            delete[] data;
            return EC_MemoryExhausted;
        }

        quantity->getEntireConceptNameCodeSequence().push_back(qCodeName);
        quantity->getEntireConceptCodeSequence().push_back(qSpec);
        rvwmItemSimple->getEntireQuantityDefinitionSequence().push_back(quantity);
        quantity->setValueType(ContentItemMacro::VT_CODE);
        fgRVWM->getRealWorldValueMapping().push_back(rvwmItemSimple);

        // Plane Position
        OFStringStream ss;
        ss << frameNo;
        OFSTRINGSTREAM_GETOFSTRING(ss, framestr) // convert number to string
        fgPlanePos->setImagePositionPatient("0", "0", framestr);

        // Frame Content
        OFCondition result = fgFracon->setDimensionIndexValues(frameNo+1 /* value within dimension */, 0 /* first dimension */);

        // Add frame with related groups
        if (result.good())
        {
            // Add frame
            groups.push_back(fgPlanePos.get());
            groups.push_back(fgFracon.get());
            groups.push_back(fgRVWM.get());
            groups.push_back(fgPlanePos.get());
            result = map->addFrame(data, frameSize, groups);
        }
        delete[] data;
        return result;
    }
}


