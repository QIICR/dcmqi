#include "ParaMapConverter.h"


using namespace std;

namespace dcmqi {

  int ParaMapConverter::itkimage2paramap(const string &inputFileName, const string &dicomImageFileName,
																				 const string &metaDataFileName, const string &outputFileName) {

    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(inputFileName.c_str());
    reader->Update();
    ImageType::Pointer parametricMapImage = reader->GetOutput();

    ImageType::SizeType inputSize = parametricMapImage->GetBufferedRegion().GetSize();
    cout << "Input image size: " << inputSize << endl;

    JSONParametricMapMetaInformationHandler metaInfo(metaDataFileName);
    metaInfo.read();

    MinMaxCalculatorType::Pointer calculator = MinMaxCalculatorType::New();
    calculator->SetImage(parametricMapImage);
    calculator->Compute();

    metaInfo.setFirstValueMapped(calculator->GetMinimum());
    metaInfo.setLastValueMapped(calculator->GetMaximum());

    IODEnhGeneralEquipmentModule::EquipmentInfo eq = getEnhEquipmentInfo();
    ContentIdentificationMacro contentID = createContentIdentificationInformation();
    CHECK_COND(contentID.setInstanceNumber(metaInfo.getInstanceNumber().c_str()));

    DcmDataset pMapDocDataset;
    DPMParametricMapFloat *pMapDoc = NULL;

    // TODO: following should maybe be moved to meta info
    OFString imageFlavor = "VOLUME";
    OFString pixContrast = "MTT";
    DPMTypes::ContentQualification contQual = DPMTypes::CQ_RESEARCH;
    OFString modality = "MR";

    CHECK_COND(DPMParametricMapFloat::create(pMapDoc, modality, metaInfo.getSeriesNumber().c_str(),
																						 metaInfo.getInstanceNumber().c_str(),
																						 inputSize[0], inputSize[1], eq, contentID,
																						 imageFlavor, pixContrast, contQual));

    if (!dicomImageFileName.empty())
      CHECK_COND(pMapDoc->import(dicomImageFileName.c_str(), OFTrue, OFTrue, OFFalse, OFTrue));

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
    // Rescale Intercept, Rescale Slope, Rescale Type are missing here
    CHECK_COND(pMapDoc->addForAllFrames(idTransFG));

    FGParametricMapFrameType frameTypeFG;
    frameTypeFG.setFrameType("DERIVED\\PRIMARY\\VOLUME\\MTT");
    CHECK_COND(pMapDoc->addForAllFrames(frameTypeFG));

    for (unsigned long f = 0; result.good() && (f < inputSize[2]); f++) {
      result = addFrame(pMapDoc, parametricMapImage, metaInfo, f);
    }

  {
    string bodyPartAssigned = metaInfo.getBodyPartExamined();
    if(!dicomImageFileName.empty() && bodyPartAssigned.empty()) {
      DcmFileFormat sliceFF;
      CHECK_COND(sliceFF.loadFile(dicomImageFileName.c_str()));
      OFString bodyPartStr;
      if(sliceFF.getDataset()->findAndGetOFString(DCM_BodyPartExamined, bodyPartStr).good()) {
        if (!bodyPartStr.empty())
          bodyPartAssigned = bodyPartStr.c_str();
      }
    }
    if(!bodyPartAssigned.empty())
      pMapDoc->getIODGeneralSeriesModule().setBodyPartExamined(bodyPartAssigned.c_str());
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

    pMapDocDataset.putAndInsertString(DCM_SeriesDescription, metaInfo.getSeriesDescription().c_str());
    pMapDocDataset.putAndInsertString(DCM_SeriesNumber, metaInfo.getSeriesNumber().c_str());
  }

  CHECK_COND(pMapDoc->writeDataset(pMapDocDataset));
  CHECK_COND(pMapDoc->saveFile(outputFileName.c_str()));

  COUT << "Saved parametric map as " << outputFileName << endl;
    return EXIT_SUCCESS;
  }

  int ParaMapConverter::paramap2itkimage(const string &inputParamapFileName, const string &outputDirName) {
    return EXIT_SUCCESS;
  }

  OFCondition ParaMapConverter::addFrame(DPMParametricMapFloat *map, const ImageType::Pointer &parametricMapImage,
																				 const JSONParametricMapMetaInformationHandler &metaInfo,
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
//      cout << framePixelCnt << " " << idx[1] << "," << idx[0] << endl;
    }

    OFVector<FGBase*> groups;
    OFunique_ptr<FGPlanePosPatient> fgPlanePos(new FGPlanePosPatient);
    OFunique_ptr<FGFrameContent > fgFracon(new FGFrameContent);
    OFunique_ptr<FGRealWorldValueMapping> realWorldValueMappingFG(new FGRealWorldValueMapping());
    FGRealWorldValueMapping::RWVMItem* realWorldValueMappingItem = new FGRealWorldValueMapping::RWVMItem();
    if (!fgPlanePos  || !fgFracon || !realWorldValueMappingFG || !realWorldValueMappingItem )
    {
      delete[] data;
      return EC_MemoryExhausted;
    }

    realWorldValueMappingItem->setRealWorldValueSlope(atof(metaInfo.getRealWorldValueSlope().c_str()));
    realWorldValueMappingItem->setRealWorldValueIntercept(atof(metaInfo.getRealWorldValueIntercept().c_str()));

    realWorldValueMappingItem->setRealWorldValueFirstValueMappeSigned(metaInfo.getFirstValueMapped());
    realWorldValueMappingItem->setRealWorldValueLastValueMappedSigned(metaInfo.getLastValueMapped());

    CodeSequenceMacro* measurementUnitCode = metaInfo.getMeasurementUnitsCode();
    if (measurementUnitCode != NULL) {
      realWorldValueMappingItem->getMeasurementUnitsCode().set(metaInfo.getCodeSequenceValue(measurementUnitCode).c_str(),
																										metaInfo.getCodeSequenceDesignator(measurementUnitCode).c_str(),
																										metaInfo.getCodeSequenceMeaning(measurementUnitCode).c_str());
    }

    // TODO: LutExplanation and LUTLabel should be added as Metainformation
    realWorldValueMappingItem->setLUTExplanation("We are mapping trash to junk.");
    realWorldValueMappingItem->setLUTLabel("Just testing");
    ContentItemMacro* quantity = new ContentItemMacro;
    CodeSequenceMacro* qCodeName = new CodeSequenceMacro("G-C1C6", "SRT", "Quantity");
    CodeSequenceMacro* qSpec = new CodeSequenceMacro("110805", "SRT", "T2 Weighted MR Signal Intensity");

    if (!quantity || !qSpec || !qCodeName)
    {
      delete[] data;
      return EC_MemoryExhausted;
    }

    quantity->getEntireConceptNameCodeSequence().push_back(qCodeName);
    quantity->getEntireConceptCodeSequence().push_back(qSpec);
    realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(quantity);
    quantity->setValueType(ContentItemMacro::VT_CODE);
    realWorldValueMappingFG->getRealWorldValueMapping().push_back(realWorldValueMappingItem);

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
      groups.push_back(fgPlanePos.get());
      groups.push_back(fgFracon.get());
      groups.push_back(realWorldValueMappingFG.get());
      groups.push_back(fgPlanePos.get());
      result = map->addFrame(data, frameSize, groups);
    }
    delete[] data;
    return result;
  }
}


