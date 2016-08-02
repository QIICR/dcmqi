#include "ParaMapConverter.h"


using namespace std;

namespace dcmqi {

  int ParaMapConverter::itkimage2paramap(const string &inputFileName, const string &dicomImageFileName,
																				 const string &metaDataFileName, const string &outputFileName) {

    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(inputFileName.c_str());
    reader->Update();
    ImageType::Pointer parametricMapImage = reader->GetOutput();

    MinMaxCalculatorType::Pointer calculator = MinMaxCalculatorType::New();
    calculator->SetImage(parametricMapImage);
    calculator->Compute();

    JSONParametricMapMetaInformationHandler metaInfo(metaDataFileName);
    metaInfo.read();

    metaInfo.setFirstValueMapped(calculator->GetMinimum());
    metaInfo.setLastValueMapped(calculator->GetMaximum());

    IODEnhGeneralEquipmentModule::EquipmentInfo eq = getEnhEquipmentInfo();
    ContentIdentificationMacro contentID = createContentIdentificationInformation(metaInfo);
    CHECK_COND(contentID.setInstanceNumber(metaInfo.getInstanceNumber().c_str()));

    // TODO: following should maybe be moved to meta info
    OFString imageFlavor = "VOLUME";
    OFString pixContrast = "MTT";
    OFString modality = "MR";

    ImageType::SizeType inputSize = parametricMapImage->GetBufferedRegion().GetSize();
    cout << "Input image size: " << inputSize << endl;

    OFvariant<OFCondition,DPMParametricMapIOD> obj =
        DPMParametricMapIOD::create<IODFloatingPointImagePixelModule>(modality, metaInfo.getSeriesNumber().c_str(),
                                                                      metaInfo.getInstanceNumber().c_str(),
                                                                      inputSize[0], inputSize[1], eq, contentID,
                                                                      imageFlavor, pixContrast, DPMTypes::CQ_RESEARCH);
    if (OFCondition* pCondition = OFget<OFCondition>(&obj))
      return EXIT_FAILURE;

    DPMParametricMapIOD& pMapDoc = *OFget<DPMParametricMapIOD>(&obj);

    if (!dicomImageFileName.empty())
      CHECK_COND(pMapDoc.import(dicomImageFileName.c_str(), OFTrue, OFTrue, OFFalse, OFTrue));

    /* Initialize dimension module */
    char dimUID[128];
    dcmGenerateUniqueIdentifier(dimUID, QIICR_UID_ROOT);
    IODMultiframeDimensionModule &mfdim = pMapDoc.getIODMultiframeDimensionModule();
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
      CHECK_COND(pMapDoc.addForAllFrames(*pixmsr));
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
      CHECK_COND(pMapDoc.addForAllFrames(*planor));
    }

    FGFrameAnatomy frameAnaFG;
    frameAnaFG.setLaterality(FGFrameAnatomy::LATERALITY_UNPAIRED);
    frameAnaFG.getAnatomy().getAnatomicRegion().set("T-A0100", "SRT", "Brain");
    CHECK_COND(pMapDoc.addForAllFrames(frameAnaFG));

    FGIdentityPixelValueTransformation idTransFG;
    // Rescale Intercept, Rescale Slope, Rescale Type are missing here
    CHECK_COND(pMapDoc.addForAllFrames(idTransFG));

    FGParametricMapFrameType frameTypeFG;
    std::string frameTypeStr = "DERIVED\\PRIMARY\\VOLUME\\";
    if(metaInfo.metaInfoRoot.isMember("DerivedPixelContrast")){
      frameTypeStr = frameTypeStr + metaInfo.metaInfoRoot["DerivedPixelContrast"].asCString();
    } else {
      frameTypeStr = frameTypeStr + "NONE";
    }
    frameTypeFG.setFrameType(frameTypeStr.c_str());
    CHECK_COND(pMapDoc.addForAllFrames(frameTypeFG));

    FGRealWorldValueMapping rwvmFG;
    FGRealWorldValueMapping::RWVMItem* realWorldValueMappingItem = new FGRealWorldValueMapping::RWVMItem();
    if (!realWorldValueMappingItem )
    {
      return -1;
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
    realWorldValueMappingItem->setLUTExplanation(metaInfo.metaInfoRoot["MeasurementUnitsCode"]["codeMeaning"].asCString());
    realWorldValueMappingItem->setLUTLabel(metaInfo.metaInfoRoot["MeasurementUnitsCode"]["codeValue"].asCString());
    ContentItemMacro* quantity = new ContentItemMacro;
    CodeSequenceMacro* qCodeName = new CodeSequenceMacro("G-C1C6", "SRT", "Quantity");
    CodeSequenceMacro* qSpec = new CodeSequenceMacro(
      metaInfo.metaInfoRoot["QuantityValueCode"]["codeValue"].asCString(),
      metaInfo.metaInfoRoot["QuantityValueCode"]["codingSchemeDesignator"].asCString(),
      metaInfo.metaInfoRoot["QuantityValueCode"]["codeMeaning"].asCString());

    if (!quantity || !qSpec || !qCodeName)
    {
      return -1;
    }

    quantity->getEntireConceptNameCodeSequence().push_back(qCodeName);
    quantity->getEntireConceptCodeSequence().push_back(qSpec);
    realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(quantity);
    quantity->setValueType(ContentItemMacro::VT_CODE);
    rwvmFG.getRealWorldValueMapping().push_back(realWorldValueMappingItem);
    CHECK_COND(pMapDoc.addForAllFrames(rwvmFG));

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
      pMapDoc.getIODGeneralSeriesModule().setBodyPartExamined(bodyPartAssigned.c_str());
  }

  // SeriesDate/Time should be of when parametric map was taken; initialize to when it was saved
  {
    OFString contentDate, contentTime;
    DcmDate::getCurrentDate(contentDate);
    DcmTime::getCurrentTime(contentTime);

    pMapDoc.getSeries().setSeriesDate(contentDate.c_str());
    pMapDoc.getSeries().setSeriesTime(contentTime.c_str());
    pMapDoc.getGeneralImage().setContentDate(contentDate.c_str());
    pMapDoc.getGeneralImage().setContentTime(contentTime.c_str());
  }
  pMapDoc.getSeries().setSeriesDescription(metaInfo.getSeriesDescription().c_str());
  pMapDoc.getSeries().setSeriesNumber(metaInfo.getSeriesNumber().c_str());

  CHECK_COND(pMapDoc.saveFile(outputFileName.c_str()));

  COUT << "Saved parametric map as " << outputFileName << endl;
    return EXIT_SUCCESS;
  }

  int ParaMapConverter::paramap2itkimage(const string &inputParamapFileName, const string &outputDirName) {
    return EXIT_SUCCESS;
  }

  OFCondition ParaMapConverter::addFrame(DPMParametricMapIOD &map, const ImageType::Pointer &parametricMapImage,
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

    OFVector<IODFloatingPointImagePixelModule::value_type> data(frameSize);

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
    if (!fgPlanePos  || !fgFracon)
    {
      return EC_MemoryExhausted;
    }

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
      groups.push_back(fgPlanePos.get());
      DPMParametricMapIOD::FramesType frames = map.getFrames();
      result = OFget<DPMParametricMapIOD::Frames<PixelType> >(&frames)->addFrame(&*data.begin(), frameSize, groups);
    }
    return result;
  }
}
