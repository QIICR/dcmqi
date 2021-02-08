
// ITK includes
#include <itkImageDuplicator.h>
#include <itkCastImageFilter.h>

// DCMQI includes
#include "dcmqi/ParaMapConverter.h"
#include "dcmqi/ImageSEGConverter.h"

// DCMTK includes
#include <dcmtk/dcmsr/codes/dcm.h>
#include <dcmtk/dcmsr/codes/sct.h>
#include <dcmtk/dcmsr/codes/ucum.h>

using namespace std;

namespace dcmqi {

  DcmDataset* ParaMapConverter::itkimage2paramap(const FloatImageType::Pointer &parametricMapImage, vector<DcmDataset*> dcmDatasets,
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

    FloatImageType::SizeType inputSize = parametricMapImage->GetBufferedRegion().GetSize();
    cout << "Input image size: " << inputSize << endl;

    OFvariant<OFCondition,DPMParametricMapIOD> obj =
        DPMParametricMapIOD::create<IODFloatingPointImagePixelModule>(modality, metaInfo.getSeriesNumber().c_str(),
                                                                      metaInfo.getInstanceNumber().c_str(),
                                                                      inputSize[1], inputSize[0], eq, contentID,
                                                                      imageFlavor, pixContrast, DPMTypes::CQ_RESEARCH);
    if (OFget<OFCondition>(&obj)){
      return NULL;
    }

    DPMParametricMapIOD* pMapDoc = OFget<DPMParametricMapIOD>(&obj);

    DcmDataset* srcDataset = NULL;
    if(dcmDatasets.size()){
      srcDataset = dcmDatasets[0];
    }
    if (srcDataset)
      // import Patient, Study and Frame of Reference; do not import Series
      // attributes
      CHECK_COND(pMapDoc->importHierarchy(*srcDataset, OFTrue, OFTrue, OFTrue, OFFalse));

    /* Initialize dimension module */
    char dimUID[128];
    dcmGenerateUniqueIdentifier(dimUID, QIICR_UID_ROOT);
    IODMultiframeDimensionModule &mfdim = pMapDoc->getIODMultiframeDimensionModule();
    OFCondition result = mfdim.addDimensionIndex(DCM_ImagePositionPatient, dimUID,
                                                 DCM_RealWorldValueMappingSequence, "Frame position");

    // Shared FGs: PixelMeasuresSequence
    {
      FGPixelMeasures *pixmsr = new FGPixelMeasures();

      FloatImageType::SpacingType labelSpacing = parametricMapImage->GetSpacing();
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

      FloatImageType::DirectionType labelDirMatrix = parametricMapImage->GetDirection();

      cout << "Directions: " << labelDirMatrix << endl;

      FGPlaneOrientationPatient *planor =
          FGPlaneOrientationPatient::createMinimal(
              Helper::floatToStr(labelDirMatrix[0][0]).c_str(),
              Helper::floatToStr(labelDirMatrix[1][0]).c_str(),
              Helper::floatToStr(labelDirMatrix[2][0]).c_str(),
              Helper::floatToStr(labelDirMatrix[0][1]).c_str(),
              Helper::floatToStr(labelDirMatrix[1][1]).c_str(),
              Helper::floatToStr(labelDirMatrix[2][1]).c_str());

      //CHECK_COND(planor->setImageOrientationPatient(imageOrientationPatientStr));
      CHECK_COND(pMapDoc->addForAllFrames(*planor));
    }

    FGFrameAnatomy frameAnaFG;
    frameAnaFG.setLaterality(FGFrameAnatomy::str2Laterality(metaInfo.getFrameLaterality().c_str()));
    if(metaInfo.metaInfoRoot.isMember("AnatomicRegionSequence")){
      frameAnaFG.getAnatomy().getAnatomicRegion().set(
          metaInfo.metaInfoRoot["AnatomicRegionSequence"]["CodeValue"].asCString(),
          metaInfo.metaInfoRoot["AnatomicRegionSequence"]["CodingSchemeDesignator"].asCString(),
          metaInfo.metaInfoRoot["AnatomicRegionSequence"]["CodeMeaning"].asCString());
    } else {
      frameAnaFG.getAnatomy().getAnatomicRegion().set("85756007", "SCT", "Tissue");
    }
    CHECK_COND(pMapDoc->addForAllFrames(frameAnaFG));

    FGPixelValueTransformation idTransFG;
    // Rescale Intercept, Rescale Slope, Rescale Type are missing here
    CHECK_COND(pMapDoc->addForAllFrames(idTransFG));

    FGParametricMapFrameType frameTypeFG;
    std::string frameTypeStr = "DERIVED\\PRIMARY\\VOLUME\\QUANTITY";
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

    realWorldValueMappingItem->setRealWorldValueFirstValueMappedSigned(metaInfo.getFirstValueMapped());
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
	CodeSequenceMacro* qCodeName = new CodeSequenceMacro("246205007", "SCT", "Quantity");
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

      CodeSequenceMacro* qCodeName = new CodeSequenceMacro("370129005", "SCT", "Measurement Method");
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
	  DSRBasicCodedEntry code_mfm = CODE_DCM_ModelFittingMethod;
      CodeSequenceMacro* qCodeName = new CodeSequenceMacro(code_mfm.CodeValue, code_mfm.CodingSchemeDesignator,
		  code_mfm.CodeMeaning);
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
      for(size_t bvalId=0;bvalId<metaInfo.metaInfoRoot["SourceImageDiffusionBValues"].size();bvalId++){
        ContentItemMacro* bval = new ContentItemMacro;
        CodeSequenceMacro* bvalUnits = new CodeSequenceMacro("s/mm2", "UCUM", "seconds per square millimeter");
		DSRBasicCodedEntry code = CODE_DCM_SourceImageDiffusionBValue;
        CodeSequenceMacro* qCodeName = new CodeSequenceMacro(code.CodeValue, code.CodingSchemeDesignator,
			code.CodeMeaning);

        if (!bval || !bvalUnits || !qCodeName)
        {
          return NULL;
        }

        bval->setValueType(ContentItemMacro::VT_NUMERIC);
        bval->getEntireConceptNameCodeSequence().push_back(qCodeName);
        bval->getEntireMeasurementUnitsCodeSequence().push_back(bvalUnits);
        if(bval->setNumericValue(metaInfo.metaInfoRoot["SourceImageDiffusionBValues"][static_cast<int>(bvalId)].asCString()).bad())
          cout << "ERROR: Failed to insert the value!" << endl;;
        realWorldValueMappingItem->getEntireQuantityDefinitionSequence().push_back(bval);
        cout << bval->toString() << endl;
      }
    }

    rwvmFG.getRealWorldValueMapping().push_back(realWorldValueMappingItem);
    CHECK_COND(pMapDoc->addForAllFrames(rwvmFG));

    /* Map referenced instances to the ITK parametric map slices */
    // this is a hack - the function below needs to be factored out
    vector<vector<int> > slice2derimg;
    bool hasDerivationImages = false;
    {

      typedef itk::CastImageFilter<FloatImageType,ShortImageType> CastFilterType;
      CastFilterType::Pointer cast = CastFilterType::New();
      cast->SetInput(parametricMapImage);
      cast->Update();
      slice2derimg = getSliceMapForSegmentation2DerivationImage(dcmDatasets, cast->GetOutput());
      cout << "Mapping from the ITK image slices to the DICOM instances in the input list" << endl;
      for(size_t i=0;i<slice2derimg.size();i++){
        cout << "  Slice " << i << ": ";
        for(size_t j=0;j<slice2derimg[i].size();j++){
          cout << slice2derimg[i][j] << " ";
          hasDerivationImages = true;
        }
        cout << endl;
      }
    }

    // TODO: factor initialization of the referenced instances out into Common class
    IODCommonInstanceReferenceModule &commref = pMapDoc->getCommonInstanceReference();
    OFVector<IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*> &refseries = commref.getReferencedSeriesItems();

    IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem* refseriesItem = new IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem;

    OFVector<SOPInstanceReferenceMacro*> &refinstances = refseriesItem->getReferencedInstanceItems();

    OFString seriesInstanceUID, classUID;

    CHECK_COND(dcmDatasets[0]->findAndGetOFString(DCM_SeriesInstanceUID, seriesInstanceUID));
    CHECK_COND(refseriesItem->setSeriesInstanceUID(seriesInstanceUID));

    FGPlanePosPatient* fgppp = FGPlanePosPatient::createMinimal("1","1","1");
    FGFrameContent* fgfc = new FGFrameContent();
    FGDerivationImage* fgder = new FGDerivationImage();
    OFVector<FGBase*> perFrameFGs;

    perFrameFGs.push_back(fgppp);
    perFrameFGs.push_back(fgfc);
    if(hasDerivationImages)
      perFrameFGs.push_back(fgder);

    for (unsigned long sliceNumber = 0; result.good() && (sliceNumber < inputSize[2]); sliceNumber++) {

      OFVector<DcmDataset*> siVector;
      for(size_t derImageInstanceNum=0;
          derImageInstanceNum<slice2derimg[sliceNumber].size();
          derImageInstanceNum++){
        siVector.push_back(dcmDatasets[slice2derimg[sliceNumber][derImageInstanceNum]]);
      }

      int uidfound = 0, uidnotfound = 0;

      if(siVector.size()>0){

        set<OFString> instanceUIDs;

        DerivationImageItem *derimgItem;

        // TODO: I know David will not like this ...
		DSRBasicCodedEntry code = CODE_DCM_ImageProcessing;
        CodeSequenceMacro derivationCode = CodeSequenceMacro(code.CodeValue, code.CodingSchemeDesignator,
			code.CodeMeaning);

        // Mandatory, defined in CID 7203
        // http://dicom.nema.org/medical/dicom/current/output/chtml/part16/sect_CID_7203.html
        if(metaInfo.getDerivationCode() != NULL) {
          CHECK_COND(fgder->addDerivationImageItem(*metaInfo.getDerivationCode(),
                                                   metaInfo.getDerivationDescription().c_str(),
                                                   derimgItem));
        } else {
          cerr << "ERROR: DerivationCode must be specified in the input metadata!" << endl;
          throw -1;
        }

        //cout << "Total of " << siVector.size() << " source image items will be added" << endl;

        OFVector<SourceImageItem*> srcimgItems;
		DSRBasicCodedEntry code_src_img = CODE_DCM_SourceImageForImageProcessingOperation;

        CHECK_COND(derimgItem->addSourceImageItems(siVector,
                                                 CodeSequenceMacro(code_src_img.CodeValue, code_src_img.CodingSchemeDesignator,code_src_img.CodeMeaning),
                                                 srcimgItems));

        {
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


      //result = addFrame(*pMapDoc, parametricMapImage, metaInfo, sliceNumber, perFrameFGs);

      // addFrame
      {
        FloatImageType::RegionType sliceRegion;
        FloatImageType::IndexType sliceIndex;
        FloatImageType::SizeType inputSize = parametricMapImage->GetBufferedRegion().GetSize();

        sliceIndex[0] = 0;
        sliceIndex[1] = 0;
        sliceIndex[2] = sliceNumber;

        inputSize[2] = 1;

        sliceRegion.SetIndex(sliceIndex);
        sliceRegion.SetSize(inputSize);

        const unsigned frameSize = inputSize[0] * inputSize[1];

        OFVector<IODFloatingPointImagePixelModule::value_type> data(frameSize);

        itk::ImageRegionConstIteratorWithIndex<FloatImageType> sliceIterator(parametricMapImage, sliceRegion);

        unsigned framePixelCnt = 0;
        for(sliceIterator.GoToBegin();!sliceIterator.IsAtEnd(); ++sliceIterator, ++framePixelCnt){
          data[framePixelCnt] = sliceIterator.Get();
          FloatImageType::IndexType idx = sliceIterator.GetIndex();
          //      cout << framePixelCnt << " " << idx[1] << "," << idx[0] << endl;
        }

        // Plane Position
        FloatImageType::PointType sliceOriginPoint;
        parametricMapImage->TransformIndexToPhysicalPoint(sliceIndex, sliceOriginPoint);
        fgppp->setImagePositionPatient(
            Helper::floatToStr(sliceOriginPoint[0]).c_str(),
            Helper::floatToStr(sliceOriginPoint[1]).c_str(),
            Helper::floatToStr(sliceOriginPoint[2]).c_str());

        // Frame Content
        OFCondition result = fgfc->setDimensionIndexValues(sliceNumber+1 /* value within dimension */, 0 /* first dimension */);

#if ADD_DERIMG
        // Already pushed above if siVector.size > 0
        // if(fgder)
          // perFrameFGs.push_back(fgder);
#endif

        DPMParametricMapIOD::FramesType frames = pMapDoc->getFrames();
        result = OFget<DPMParametricMapIOD::Frames<FloatPixelType> >(&frames)->addFrame(&*data.begin(), frameSize, perFrameFGs);

        cout << "Frame " << sliceNumber << " added" << endl;
      }

      // remove derivation image FG from the per-frame FGs, only if applicable!
      if(!siVector.empty()){
        // clean up for next frame
        fgder->clearData();
      }
    }

    // add ReferencedSeriesItem only if it is not empty
    if(refinstances.size())
      refseries.push_back(refseriesItem);

    delete fgppp;
    delete fgfc;
    delete fgder;

    string bodyPartAssigned = metaInfo.getBodyPartExamined();
    if(srcDataset != NULL && bodyPartAssigned.empty()) {
      OFString bodyPartStr;
      if(srcDataset->findAndGetOFString(DCM_BodyPartExamined, bodyPartStr).good()) {
        if (!bodyPartStr.empty())
          bodyPartAssigned = bodyPartStr.c_str();
      }
    }
    if(!bodyPartAssigned.empty())
      pMapDoc->getIODGeneralSeriesModule().setBodyPartExamined(bodyPartAssigned.c_str());


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

  pair <FloatImageType::Pointer, string> ParaMapConverter::paramap2itkimage(DcmDataset *pmapDataset) {

    DcmRLEDecoderRegistration::registerCodecs();

    OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");
    dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);

    OFvariant<OFCondition,DPMParametricMapIOD*> result = DPMParametricMapIOD::loadDataset(*pmapDataset);
    if (OFget<OFCondition>(&result)) {
      throw -1;
    }

    DPMParametricMapIOD* pMapDoc = *OFget<DPMParametricMapIOD*>(&result);

    // Directions
    FGInterface &fgInterface = pMapDoc->getFunctionalGroups();
    FloatImageType::DirectionType direction;
    if(getImageDirections(fgInterface, direction)){
      cerr << "ERROR: Failed to get image directions" << endl;
      throw -1;
    }

    // Spacing and origin
    double computedSliceSpacing, computedVolumeExtent;
    vnl_vector<double> sliceDirection(3);
    sliceDirection[0] = direction[0][2];
    sliceDirection[1] = direction[1][2];
    sliceDirection[2] = direction[2][2];

    FloatImageType::PointType imageOrigin;
    if(computeVolumeExtent(fgInterface, sliceDirection, imageOrigin, computedSliceSpacing, computedVolumeExtent)){
      cerr << "ERROR: Failed to compute origin and/or slice spacing!" << endl;
      throw -1;
    }

    FloatImageType::SpacingType imageSpacing;
    imageSpacing.Fill(0);
    if(getDeclaredImageSpacing(fgInterface, imageSpacing)){
      cerr << "ERROR: Failed to get image spacing from DICOM!" << endl;
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
    FloatImageType::SizeType imageSize;
    {
      OFString str;

      if(pmapDataset->findAndGetOFString(DCM_Rows, str).good())
        imageSize[1] = atoi(str.c_str());
      if(pmapDataset->findAndGetOFString(DCM_Columns, str).good())
        imageSize[0] = atoi(str.c_str());
    }
    imageSize[2] = fgInterface.getNumberOfFrames();

    FloatImageType::RegionType imageRegion;
    imageRegion.SetSize(imageSize);
    FloatImageType::Pointer pmImage = FloatImageType::New();
    pmImage->SetRegions(imageRegion);
    pmImage->SetOrigin(imageOrigin);
    pmImage->SetSpacing(imageSpacing);
    pmImage->SetDirection(direction);
    pmImage->Allocate();
    pmImage->FillBuffer(0);

    JSONParametricMapMetaInformationHandler metaInfo;
    populateMetaInformationFromDICOM(pmapDataset, metaInfo);

    DPMParametricMapIOD::FramesType obj = pMapDoc->getFrames();
    if (OFget<OFCondition>(&obj)) {
      throw -1;
    }

    DPMParametricMapIOD::Frames<FloatPixelType> frames = *OFget<DPMParametricMapIOD::Frames<FloatPixelType> >(&obj);

    for(unsigned int frameId=0;frameId<fgInterface.getNumberOfFrames();frameId++){

      FloatPixelType *frame = frames.getFrame(frameId);

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

      FloatImageType::IndexType index;
      // initialize slice with the frame content
      for(unsigned int row=0;row<imageSize[1];row++){
        index[1] = row;
        index[2] = frameId;
        for(unsigned int col=0;col<imageSize[0];col++){
          unsigned pixelPosition = row*imageSize[0] + col;
          index[0] = col;
          pmImage->SetPixel(index, frame[pixelPosition]);
        }
      }
    }

    return pair <FloatImageType::Pointer, string>(pmImage, metaInfo.getJSONOutputAsString());
  }

  OFCondition ParaMapConverter::addFrame(DPMParametricMapIOD &map, const FloatImageType::Pointer &parametricMapImage,
                                         const JSONParametricMapMetaInformationHandler &itkNotUsed(metaInfo),
                                         const unsigned long frameNo, OFVector<FGBase*> groups)
  {
    FloatImageType::RegionType sliceRegion;
    FloatImageType::IndexType sliceIndex;
    FloatImageType::SizeType inputSize = parametricMapImage->GetBufferedRegion().GetSize();

    sliceIndex[0] = 0;
    sliceIndex[1] = 0;
    sliceIndex[2] = frameNo;

    inputSize[2] = 1;

    sliceRegion.SetIndex(sliceIndex);
    sliceRegion.SetSize(inputSize);

    const unsigned frameSize = inputSize[0] * inputSize[1];

    OFVector<IODFloatingPointImagePixelModule::value_type> data(frameSize);

    itk::ImageRegionConstIteratorWithIndex<FloatImageType> sliceIterator(parametricMapImage, sliceRegion);

    unsigned framePixelCnt = 0;
    for(sliceIterator.GoToBegin();!sliceIterator.IsAtEnd(); ++sliceIterator, ++framePixelCnt){
      data[framePixelCnt] = sliceIterator.Get();
      FloatImageType::IndexType idx = sliceIterator.GetIndex();
//      cout << framePixelCnt << " " << idx[1] << "," << idx[0] << endl;
    }

    OFunique_ptr<FGPlanePosPatient> fgPlanePos(new FGPlanePosPatient);
    OFunique_ptr<FGFrameContent > fgFracon(new FGFrameContent);
    if (!fgPlanePos  || !fgFracon)
    {
      return EC_MemoryExhausted;
    }

    // Plane Position
    FloatImageType::PointType sliceOriginPoint;
    parametricMapImage->TransformIndexToPhysicalPoint(sliceIndex, sliceOriginPoint);
    fgPlanePos->setImagePositionPatient(
        Helper::floatToStr(sliceOriginPoint[0]).c_str(),
        Helper::floatToStr(sliceOriginPoint[1]).c_str(),
        Helper::floatToStr(sliceOriginPoint[2]).c_str());

    // Frame Content
    OFCondition result = fgFracon->setDimensionIndexValues(frameNo+1 /* value within dimension */, 0 /* first dimension */);

    // Add frame with related groups
    if (result.good())
    {
      groups.push_back(fgPlanePos.get());
      groups.push_back(fgFracon.get());
      groups.push_back(fgPlanePos.get());
      DPMParametricMapIOD::FramesType frames = map.getFrames();
      result = OFget<DPMParametricMapIOD::Frames<FloatPixelType> >(&frames)->addFrame(&*data.begin(), frameSize, groups);
    }
    return result;
  }

  void ParaMapConverter::populateMetaInformationFromDICOM(DcmDataset *pmapDataset,
                                                          JSONParametricMapMetaInformationHandler &metaInfo) {

    OFvariant<OFCondition,DPMParametricMapIOD*> result = DPMParametricMapIOD::loadDataset(*pmapDataset);
    if (OFCondition* pCondition = OFget<OFCondition>(&result)) {
      cerr << "ERROR: Failed to load parametric map! " << pCondition->text() << endl;
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
      if (rw->getRealWorldValueMapping().size() > 0) {
        FGRealWorldValueMapping::RWVMItem *item = rw->getRealWorldValueMapping()[0];
        metaInfo.setMeasurementUnitsCode(item->getMeasurementUnitsCode());

        OFString slope;
        // TODO: replace the following call by following getter once it is available
//        item->getRealWorldValueSlope(slope);
        item->getData().findAndGetOFString(DCM_RealWorldValueSlope, slope);
        metaInfo.setRealWorldValueSlope(slope.c_str());

        for(unsigned int quantIdx=0; quantIdx<item->getEntireQuantityDefinitionSequence().size(); quantIdx++) {
          ContentItemMacro* macro = item->getEntireQuantityDefinitionSequence()[quantIdx];
          CodeSequenceMacro* codeSequence= macro->getConceptNameCodeSequence();
          if (codeSequence != NULL) {
            OFString codeMeaning;
            codeSequence->getCodeMeaning(codeMeaning);
            OFString designator, meaning, value;

            if (codeMeaning == "Quantity") {
              CodeSequenceMacro* quantityValueCode = macro->getConceptCodeSequence();
              if (quantityValueCode != NULL) {
                quantityValueCode->getCodeValue(value);
                quantityValueCode->getCodeMeaning(meaning);
                quantityValueCode->getCodingSchemeDesignator(designator);
                metaInfo.setQuantityValueCode(value.c_str(), designator.c_str(), meaning.c_str());
              }
            } else if (codeMeaning == "Measurement Method") {
              CodeSequenceMacro* measurementMethodValueCode = macro->getConceptCodeSequence();
              if (measurementMethodValueCode != NULL) {
                measurementMethodValueCode->getCodeValue(value);
                measurementMethodValueCode->getCodeMeaning(meaning);
                measurementMethodValueCode->getCodingSchemeDesignator(designator);
                metaInfo.setMeasurementMethodCode(value.c_str(), designator.c_str(), meaning.c_str());
              }
            } else if (codeMeaning == "Source image diffusion b-value") {
              macro->getNumericValue(value);
              metaInfo.addSourceImageDiffusionBValue(value.c_str());
            }
          }
        }
      }

      FGDerivationImage* derivationImage = OFstatic_cast(FGDerivationImage*, fg.get(0, DcmFGTypes::EFG_DERIVATIONIMAGE));

      if(derivationImage){
        OFVector<DerivationImageItem*>& derivationImageItems = derivationImage->getDerivationImageItems();
        if(derivationImageItems.size()>0){
          DerivationImageItem* derivationImageItem = derivationImageItems[0];
          CodeSequenceMacro* derivationCode = derivationImageItem->getDerivationCodeItems()[0];
          if (derivationCode != NULL) {
            OFString designator, meaning, value;
            derivationCode->getCodeValue(value);
            derivationCode->getCodeMeaning(meaning);
            derivationCode->getCodingSchemeDesignator(designator);
            metaInfo.setDerivationCode(value.c_str(), designator.c_str(), meaning.c_str());
          }
        }
      }

      FGFrameAnatomy* fa = OFstatic_cast(FGFrameAnatomy*, fg.get(0, DcmFGTypes::EFG_FRAMEANATOMY));
        metaInfo.setAnatomicRegionSequence(fa->getAnatomy().getAnatomicRegion());
      FGFrameAnatomy::LATERALITY frameLaterality;
      fa->getLaterality(frameLaterality);
      metaInfo.setFrameLaterality(fa->laterality2Str(frameLaterality).c_str());
    }
    if(pMapDoc != NULL)
      delete pMapDoc;
  }
}
