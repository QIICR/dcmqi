#include <itkImageDuplicator.h>
#include "ParaMapConverter.h"


using namespace std;

namespace dcmqi {

  DcmDataset* ParaMapConverter::itkimage2paramap(const ImageType::Pointer &parametricMapImage, DcmDataset* dcmDataset,
                                         const string &metaData) {

    MinMaxCalculatorType::Pointer calculator = MinMaxCalculatorType::New();
    calculator->SetImage(parametricMapImage);
    calculator->Compute();

    JSONParametricMapMetaInformationHandler metaInfo(metaData);
    metaInfo.read();

    metaInfo.setFirstValueMapped(calculator->GetMinimum());
    metaInfo.setLastValueMapped(calculator->GetMaximum());

    IODEnhGeneralEquipmentModule::EquipmentInfo eq = getEnhEquipmentInfo();
    ContentIdentificationMacro contentID = createContentIdentificationInformation(metaInfo);
    CHECK_COND(contentID.setInstanceNumber(metaInfo.getInstanceNumber().c_str()));

    // TODO: following should maybe be moved to meta info
    OFString imageFlavor = "VOLUME";
    OFString pixContrast = "NONE";
    if(metaInfo.metaInfoRoot.isMember("DerivedPixelContrast")){
      pixContrast = metaInfo.metaInfoRoot["DerivedPixelContrast"].asCString();
    }

    // TODO: initialize modality from the source / add to schema?
    OFString modality = "MR";

    ImageType::SizeType inputSize = parametricMapImage->GetBufferedRegion().GetSize();
    cout << "Input image size: " << inputSize << endl;

    OFvariant<OFCondition,DPMParametricMapIOD> obj =
        DPMParametricMapIOD::create<IODFloatingPointImagePixelModule>(modality, metaInfo.getSeriesNumber().c_str(),
                                                                      metaInfo.getInstanceNumber().c_str(),
                                                                      inputSize[0], inputSize[1], eq, contentID,
                                                                      imageFlavor, pixContrast, DPMTypes::CQ_RESEARCH);
    if (OFCondition* pCondition = OFget<OFCondition>(&obj))
      return NULL;

    DPMParametricMapIOD* pMapDoc = OFget<DPMParametricMapIOD>(&obj);

    if (dcmDataset != NULL)
      CHECK_COND(pMapDoc->import(*dcmDataset, OFTrue, OFTrue, OFFalse, OFTrue));

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
    frameAnaFG.setLaterality(FGFrameAnatomy::str2Laterality(metaInfo.getFrameLaterality().c_str()));
    if(metaInfo.metaInfoRoot.isMember("AnatomicRegionCode")){
      frameAnaFG.getAnatomy().getAnatomicRegion().set(
          metaInfo.metaInfoRoot["AnatomicRegionCode"]["CodeValue"].asCString(),
          metaInfo.metaInfoRoot["AnatomicRegionCode"]["CodingSchemeDesignator"].asCString(),
          metaInfo.metaInfoRoot["AnatomicRegionCode"]["CodeMeaning"].asCString());
    } else {
      frameAnaFG.getAnatomy().getAnatomicRegion().set("T-D0050", "SRT", "Tissue");
    }
    CHECK_COND(pMapDoc->addForAllFrames(frameAnaFG));

    FGIdentityPixelValueTransformation idTransFG;
    // Rescale Intercept, Rescale Slope, Rescale Type are missing here
    CHECK_COND(pMapDoc->addForAllFrames(idTransFG));

    FGParametricMapFrameType frameTypeFG;
    std::string frameTypeStr = "DERIVED\\PRIMARY\\VOLUME\\";
    if(metaInfo.metaInfoRoot.isMember("DerivedPixelContrast")){
      frameTypeStr = frameTypeStr + metaInfo.metaInfoRoot["DerivedPixelContrast"].asCString();
    } else {
      frameTypeStr = frameTypeStr + "NONE";
    }
    frameTypeFG.setFrameType(frameTypeStr.c_str());
    CHECK_COND(pMapDoc->addForAllFrames(frameTypeFG));

    FGRealWorldValueMapping rwvmFG;
    FGRealWorldValueMapping::RWVMItem* realWorldValueMappingItem = new FGRealWorldValueMapping::RWVMItem();
    if (!realWorldValueMappingItem )
    {
      return NULL;
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
    realWorldValueMappingItem->setLUTExplanation(metaInfo.metaInfoRoot["MeasurementUnitsCode"]["CodeMeaning"].asCString());
    realWorldValueMappingItem->setLUTLabel(metaInfo.metaInfoRoot["MeasurementUnitsCode"]["CodeValue"].asCString());
    ContentItemMacro* quantity = new ContentItemMacro;
    CodeSequenceMacro* qCodeName = new CodeSequenceMacro("G-C1C6", "SRT", "Quantity");
    CodeSequenceMacro* qSpec = new CodeSequenceMacro(
      metaInfo.metaInfoRoot["QuantityValueCode"]["CodeValue"].asCString(),
      metaInfo.metaInfoRoot["QuantityValueCode"]["CodingSchemeDesignator"].asCString(),
      metaInfo.metaInfoRoot["QuantityValueCode"]["CodeMeaning"].asCString());

    if (!quantity || !qSpec || !qCodeName)
    {
      return NULL;
    }

    quantity->getEntireConceptNameCodeSequence().push_back(qCodeName);
    quantity->getEntireConceptCodeSequence().push_back(qSpec);
    realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(quantity);
    quantity->setValueType(ContentItemMacro::VT_CODE);

    // initialize optional items, if available
    if(metaInfo.metaInfoRoot.isMember("MeasurementMethodCode")){
      ContentItemMacro* measureMethod = new ContentItemMacro;
      CodeSequenceMacro* qCodeName = new CodeSequenceMacro("G-C306", "SRT", "Measurement Method");
      CodeSequenceMacro* qSpec = new CodeSequenceMacro(
        metaInfo.metaInfoRoot["MeasurementMethodCode"]["CodeValue"].asCString(),
        metaInfo.metaInfoRoot["MeasurementMethodCode"]["CodingSchemeDesignator"].asCString(),
        metaInfo.metaInfoRoot["MeasurementMethodCode"]["CodeMeaning"].asCString());

      if (!measureMethod || !qSpec || !qCodeName)
      {
        return NULL;
      }

      measureMethod->getEntireConceptNameCodeSequence().push_back(qCodeName);
      measureMethod->getEntireConceptCodeSequence().push_back(qSpec);
      realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(measureMethod);
      measureMethod->setValueType(ContentItemMacro::VT_CODE);
    }

    if(metaInfo.metaInfoRoot.isMember("ModelFittingMethodCode")){
      ContentItemMacro* fittingMethod = new ContentItemMacro;
      CodeSequenceMacro* qCodeName = new CodeSequenceMacro("DWMPxxxxx2", "99QIICR", "Model fitting method");
      CodeSequenceMacro* qSpec = new CodeSequenceMacro(
        metaInfo.metaInfoRoot["ModelFittingMethodCode"]["CodeValue"].asCString(),
        metaInfo.metaInfoRoot["ModelFittingMethodCode"]["CodingSchemeDesignator"].asCString(),
        metaInfo.metaInfoRoot["ModelFittingMethodCode"]["CodeMeaning"].asCString());

      if (!fittingMethod || !qSpec || !qCodeName)
      {
        return NULL;
      }

      fittingMethod->getEntireConceptNameCodeSequence().push_back(qCodeName);
      fittingMethod->getEntireConceptCodeSequence().push_back(qSpec);
      realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(fittingMethod);
      fittingMethod->setValueType(ContentItemMacro::VT_CODE);
    }

    if(metaInfo.metaInfoRoot.isMember("SourceImageDiffusionBValues")){
      for(int bvalId=0;bvalId<metaInfo.metaInfoRoot["SourceImageDiffusionBValues"].size();bvalId++){
        ContentItemMacro* bval = new ContentItemMacro;
        CodeSequenceMacro* bvalUnits = new CodeSequenceMacro("s/mm2", "UCUM", "seconds per square millimeter");
        CodeSequenceMacro* qCodeName = new CodeSequenceMacro("DWMPxxxxx1", "99QIICR", "Source image diffusion b-value");

        if (!bval || !bvalUnits || !qCodeName)
        {
          return NULL;
        }

        bval->setValueType(ContentItemMacro::VT_NUMERIC);
        bval->getEntireConceptNameCodeSequence().push_back(qCodeName);
        bval->getEntireMeasurementUnitsCodeSequence().push_back(bvalUnits);
        if(bval->setNumericValue(metaInfo.metaInfoRoot["SourceImageDiffusionBValues"][bvalId].asCString()).bad())
          cout << "Failed to insert the value!" << endl;;
        realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(bval);
        cout << bval->toString() << endl;
      }
    }

    rwvmFG.getRealWorldValueMapping().push_back(realWorldValueMappingItem);
    CHECK_COND(pMapDoc->addForAllFrames(rwvmFG));

    for (unsigned long f = 0; result.good() && (f < inputSize[2]); f++) {
      result = addFrame(*pMapDoc, parametricMapImage, metaInfo, f);
    }

  {
    string bodyPartAssigned = metaInfo.getBodyPartExamined();
    if(dcmDataset != NULL && bodyPartAssigned.empty()) {
      OFString bodyPartStr;
      if(dcmDataset->findAndGetOFString(DCM_BodyPartExamined, bodyPartStr).good()) {
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

    pMapDoc->getSeries().setSeriesDate(contentDate.c_str());
    pMapDoc->getSeries().setSeriesTime(contentTime.c_str());
    pMapDoc->getGeneralImage().setContentDate(contentDate.c_str());
    pMapDoc->getGeneralImage().setContentTime(contentTime.c_str());
  }
  pMapDoc->getSeries().setSeriesDescription(metaInfo.getSeriesDescription().c_str());
  pMapDoc->getSeries().setSeriesNumber(metaInfo.getSeriesNumber().c_str());

  DcmDataset* output = new DcmDataset();
  CHECK_COND(pMapDoc->writeDataset(*output));
  return output;
  }

  pair <ImageType::Pointer, string> ParaMapConverter::paramap2itkimage(DcmDataset *pmapDataset) {

    DcmRLEDecoderRegistration::registerCodecs();

    dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);

    OFvariant<OFCondition,DPMParametricMapIOD*> result = DPMParametricMapIOD::loadDataset(*pmapDataset);
    if (OFCondition* pCondition = OFget<OFCondition>(&result)) {
      throw -1;
    }

    DPMParametricMapIOD* pMapDoc = *OFget<DPMParametricMapIOD*>(&result);

    // Directions
    FGInterface &fgInterface = pMapDoc->getFunctionalGroups();
    ImageType::DirectionType direction;
    if(getImageDirections(fgInterface, direction)){
      cerr << "Failed to get image directions" << endl;
      throw -1;
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
      throw -1;
    }

    ImageType::SpacingType imageSpacing;
    imageSpacing.Fill(0);
    if(getDeclaredImageSpacing(fgInterface, imageSpacing)){
      cerr << "Failed to get image spacing from DICOM!" << endl;
      throw -1;
    }

    const double tolerance = 1e-5;
    if(!imageSpacing[2]){
      imageSpacing[2] = computedSliceSpacing;
    } else if(fabs(imageSpacing[2]-computedSliceSpacing)>tolerance){
      cerr << "WARNING: Declared slice spacing is significantly different from the one declared in DICOM!" <<
           " Declared = " << imageSpacing[2] << " Computed = " << computedSliceSpacing << endl;
    }

    // Region size
    ImageType::SizeType imageSize;
    {
      OFString str;

      if(pmapDataset->findAndGetOFString(DCM_Rows, str).good())
        imageSize[1] = atoi(str.c_str());
      if(pmapDataset->findAndGetOFString(DCM_Columns, str).good())
        imageSize[0] = atoi(str.c_str());
    }
    imageSize[2] = fgInterface.getNumberOfFrames();

    ImageType::RegionType imageRegion;
    imageRegion.SetSize(imageSize);
    ImageType::Pointer pmImage = ImageType::New();
    pmImage->SetRegions(imageRegion);
    pmImage->SetOrigin(imageOrigin);
    pmImage->SetSpacing(imageSpacing);
    pmImage->SetDirection(direction);
    pmImage->Allocate();
    pmImage->FillBuffer(0);

    JSONParametricMapMetaInformationHandler metaInfo;
    populateMetaInformationFromDICOM(pmapDataset, metaInfo);

    DPMParametricMapIOD::FramesType obj = pMapDoc->getFrames();
    if (OFCondition* pCondition = OFget<OFCondition>(&obj)) {
      throw -1;
    }

    DPMParametricMapIOD::Frames<PixelType> frames = *OFget<DPMParametricMapIOD::Frames<PixelType> >(&obj);

    for(int frameId=0;frameId<fgInterface.getNumberOfFrames();frameId++){

      PixelType *frame = frames.getFrame(frameId);

      bool isPerFrame;

      FGPlanePosPatient *planposfg =
          OFstatic_cast(FGPlanePosPatient*,fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
      assert(planposfg);

      FGFrameContent *fracon =
          OFstatic_cast(FGFrameContent*,fgInterface.get(frameId, DcmFGTypes::EFG_FRAMECONTENT, isPerFrame));
      assert(fracon);

      // populate meta information needed for Slicer ScalarVolumeNode initialization
      {
      }

      ImageType::IndexType index;
      // initialize slice with the frame content
      for(int row=0;row<imageSize[1];row++){
        index[1] = row;
        index[2] = frameId;
        for(int col=0;col<imageSize[0];col++){
          unsigned pixelPosition = row*imageSize[0] + col;
          index[0] = col;
          pmImage->SetPixel(index, frame[pixelPosition]);
        }
      }
    }

    return pair <ImageType::Pointer, string>(pmImage, metaInfo.getJSONOutputAsString());
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

  void ParaMapConverter::populateMetaInformationFromDICOM(DcmDataset *pmapDataset,
                                                          JSONParametricMapMetaInformationHandler &metaInfo) {

    OFvariant<OFCondition,DPMParametricMapIOD*> result = DPMParametricMapIOD::loadDataset(*pmapDataset);
    if (OFCondition* pCondition = OFget<OFCondition>(&result)) {
      cerr << "Failed to load parametric map! " << pCondition->text() << endl;
      throw -1;
    }
    DPMParametricMapIOD* pMapDoc = *OFget<DPMParametricMapIOD*>(&result);

    OFString temp;

    pMapDoc->getSeries().getSeriesDescription(temp);
    metaInfo.setSeriesDescription(temp.c_str());

    pMapDoc->getSeries().getSeriesNumber(temp);
    metaInfo.setSeriesNumber(temp.c_str());

    if(pmapDataset->findAndGetOFString(DCM_InstanceNumber, temp).good())
      metaInfo.setInstanceNumber(temp.c_str());

    pMapDoc->getSeries().getBodyPartExamined(temp);
    metaInfo.setBodyPartExamined(temp.c_str());

    pMapDoc->getDPMParametricMapImageModule().getImageType(temp, 3);
    metaInfo.setDerivedPixelContrast(temp.c_str());

    if (pMapDoc->getNumberOfFrames() > 0) {
      FGInterface& fg = pMapDoc->getFunctionalGroups();
      FGRealWorldValueMapping* rw = OFstatic_cast(FGRealWorldValueMapping*,
                                                  fg.get(0, DcmFGTypes::EFG_REALWORLDVALUEMAPPING));
      size_t numMappings = rw->getRealWorldValueMapping().size();
      if (numMappings > 0) {
        FGRealWorldValueMapping::RWVMItem *item = rw->getRealWorldValueMapping()[0];
        metaInfo.setMeasurementUnitsCode(item->getMeasurementUnitsCode());

        Float64 slope;
        // TODO: replace the following call by following getter once it is available
//        item->getRealWorldValueSlope(slope);
        item->getData().findAndGetFloat64(DCM_RealWorldValueSlope, slope);
        metaInfo.setRealWorldValueSlope(Helper::floatToStrScientific(slope));

        size_t numQuant = item->getEntireQuantityDefinitionSequence().size();
        if (numQuant > 0) {
          ContentItemMacro *macro = item->getEntireQuantityDefinitionSequence()[0];
          size_t numEntireConcept = macro->getEntireConceptCodeSequence().size();
          if (numEntireConcept > 0) {
//            BUG??? For any reason metaInfo.setQuantityValueCode(macro->getConceptCodeSequence()); results in an empty code sequence
            CodeSequenceMacro* quantityValueCode = macro->getConceptCodeSequence();
            if (quantityValueCode != NULL) {
              OFString designator, meaning, value;
              quantityValueCode->getCodeValue(value);
              quantityValueCode->getCodeMeaning(meaning);
              quantityValueCode->getCodingSchemeDesignator(designator);
              metaInfo.setQuantityValueCode(value.c_str(), designator.c_str(), meaning.c_str());
            }
          }
        }
      }
      FGFrameAnatomy* fa = OFstatic_cast(FGFrameAnatomy*, fg.get(0, DcmFGTypes::EFG_FRAMEANATOMY));
      metaInfo.setAnatomicRegion(fa->getAnatomy().getAnatomicRegion());
      FGFrameAnatomy::LATERALITY frameLaterality;
      fa->getLaterality(frameLaterality);
      metaInfo.setFrameLaterality(fa->laterality2Str(frameLaterality).c_str());
    }
    if(pMapDoc != NULL)
      delete pMapDoc;
  }
}
