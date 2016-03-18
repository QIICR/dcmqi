#include "dcmtk/config/osconfig.h"   // make sure OS specific configuration is included first
#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/oflog/oflog.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmseg/segdoc.h"
#include "dcmtk/dcmseg/segment.h"
#include "dcmtk/dcmfg/fginterface.h"
#include "dcmtk/dcmiod/iodutil.h"
#include "dcmtk/dcmiod/modmultiframedimension.h"
#include "dcmtk/dcmdata/dcsequen.h"

#include "dcmtk/dcmfg/fgderimg.h"
#include "dcmtk/dcmfg/fgplanor.h"
#include "dcmtk/dcmfg/fgpixmsr.h"
#include "dcmtk/dcmfg/fgfracon.h"
#include "dcmtk/dcmfg/fgplanpo.h"

#include "dcmtk/dcmiod/iodmacro.h"

#include "dcmtk/oflog/loglevel.h"

#define INCLUDE_CSTDLIB
#define INCLUDE_CSTRING
#include "dcmtk/ofstd/ofstdinc.h"

#include <sstream>
#include <fstream>

#ifdef WITH_ZLIB
#include <zlib.h>                     /* for zlibVersion() */
#endif

// ITK includes
#include <itkImageFileReader.h>
#include <itkLabelImageToLabelMapFilter.h>
#include <itkImageRegionConstIterator.h>
#include <itkLabelStatisticsImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>

// UIDs
#include "QIICRUIDs.h"
#include "SegmentAttributes.h"
//#include "../Common/conditionCheckMacros.h"

// versioning
#include "dcmqiVersionConfigure.h"

// CLP inclides
#include "itkimage2segimageCLP.h"

#define CHECK_COND(condition) \
  do { \
    if (condition.bad()) { \
      std::cerr << condition.text() << " in " __FILE__ << ":" << __LINE__  << std::endl; \
      return EXIT_FAILURE; \
    } \
  } while (0)

std::string FloatToStrScientific(float f) {
  std::ostringstream sstream;
  sstream << std::scientific << f;
  return sstream.str();
}

void checkValidityOfFirstSrcImage(DcmSegmentation *segdoc) {
  FGInterface &fgInterface = segdoc->getFunctionalGroups();
  bool isPerFrame = false;
  FGDerivationImage *derimgfg =
    OFstatic_cast(FGDerivationImage*,fgInterface.get(0,
          DcmFGTypes::EFG_DERIVATIONIMAGE, isPerFrame));
  assert(derimgfg);
  assert(isPerFrame);

  OFVector<DerivationImageItem*> &deritems = derimgfg->getDerivationImageItems();

  OFVector<SourceImageItem*> &srcitems = deritems[0]->getSourceImageItems();
  OFString codeValue;
  CodeSequenceMacro &code = srcitems[0]->getPurposeOfReferenceCode();
  if(!code.getCodeValue(codeValue).good()) {
    std::cout << "Failed to look up purpose of reference code" << std::endl;
    abort();
  }
}


int main(int argc, char *argv[])
{
  PARSE_ARGS;

  //dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);

  typedef short PixelType;
  typedef itk::Image<PixelType,3> ImageType;
  typedef itk::ImageFileReader<ImageType> ReaderType;
  typedef itk::LabelImageToLabelMapFilter<ImageType> LabelToLabelMapFilterType;

  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(inputSegmentationsFileNames[0].c_str());
  reader->Update();
  ImageType::Pointer labelImage = reader->GetOutput();

  ImageType::SizeType inputSize = labelImage->GetBufferedRegion().GetSize();
  std::cout << "Input image size: " << inputSize << std::endl;

  const unsigned frameSize = inputSize[0]*inputSize[1];

  //OFLog::configure(OFLogger::DEBUG_LOG_LEVEL);

  /* Construct Equipment information */
  IODGeneralEquipmentModule::EquipmentInfo eq;
  eq.m_Manufacturer = "QIICR";
  eq.m_DeviceSerialNumber = "0";
  eq.m_ManufacturerModelName = dcmqi_WC_URL;
  eq.m_SoftwareVersions = dcmqi_WC_REVISION;

  /* Construct Content identification information */
  ContentIdentificationMacro ident;
  CHECK_COND(ident.setContentCreatorName("QIICR"));
  CHECK_COND(ident.setContentDescription("Iowa QIN segmentation result"));
  CHECK_COND(ident.setContentLabel("QIICR QIN IOWA"));
  CHECK_COND(ident.setInstanceNumber(instanceNumber.c_str()));

  /* Create new segementation document */
  DcmDataset segdocDataset;
  DcmSegmentation *segdoc = NULL;

  CHECK_COND(DcmSegmentation::createBinarySegmentation(
    segdoc,   // resulting segmentation
    inputSize[1],      // rows
    inputSize[0],      // columns
    eq,       // equipment
    ident));   // content identification

  /* Import patient and study from existing file */
  CHECK_COND(segdoc->importPatientStudyFoR(inputDICOMImageFileNames[0].c_str(), OFTrue, OFTrue, OFFalse, OFTrue));

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

  // Find mapping from the segmentation slice number to the derivation image
  // Assume that orientation of the segmentation is the same as the source series
  std::vector<int> slice2derimg(inputDICOMImageFileNames.size());
  for(int i=0;i<inputDICOMImageFileNames.size();i++){
    OFString ippStr;
    DcmFileFormat sliceFF;
    DcmDataset *sliceDataset = NULL;
    ImageType::PointType ippPoint;
    ImageType::IndexType ippIndex;
    CHECK_COND(sliceFF.loadFile(inputDICOMImageFileNames[i].c_str()));
    sliceDataset = sliceFF.getDataset();
    for(int j=0;j<3;j++){
      CHECK_COND(sliceDataset->findAndGetOFString(DCM_ImagePositionPatient, ippStr, j));
      ippPoint[j] = atof(ippStr.c_str());
    }
    if(!labelImage->TransformPhysicalPointToIndex(ippPoint, ippIndex)){
      std::cerr << "ImagePositionPatient maps outside the ITK image!" << std::endl;
      std::cout << "image position: " << ippPoint << std::endl;
      std::cerr << "ippIndex: " << ippIndex << std::endl;
      return -1;
    }
    slice2derimg[ippIndex[2]] = i;
  }

  // Shared FGs: PlaneOrientationPatientSequence
  {
    OFString imageOrientationPatientStr;

    ImageType::DirectionType labelDirMatrix = labelImage->GetDirection();

    std::cout << "Directions: " << labelDirMatrix << std::endl;

    FGPlaneOrientationPatient *planor =
        FGPlaneOrientationPatient::createMinimal(
            FloatToStrScientific(labelDirMatrix[0][0]).c_str(),
            FloatToStrScientific(labelDirMatrix[1][0]).c_str(),
            FloatToStrScientific(labelDirMatrix[2][0]).c_str(),
            FloatToStrScientific(labelDirMatrix[0][1]).c_str(),
            FloatToStrScientific(labelDirMatrix[1][1]).c_str(),
            FloatToStrScientific(labelDirMatrix[2][1]).c_str());


    //CHECK_COND(planor->setImageOrientationPatient(imageOrientationPatientStr));
    CHECK_COND(segdoc->addForAllFrames(*planor));
  }

  // Shared FGs: PixelMeasuresSequence
  {
    FGPixelMeasures *pixmsr = new FGPixelMeasures();

    ImageType::SpacingType labelSpacing = labelImage->GetSpacing();
    std::ostringstream spacingSStream;
    spacingSStream << std::scientific << labelSpacing[0] << "\\" << labelSpacing[1];
    CHECK_COND(pixmsr->setPixelSpacing(spacingSStream.str().c_str()));

    spacingSStream.clear(); spacingSStream.str("");
    spacingSStream << std::scientific << labelSpacing[2];
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
  std::set<OFString> instanceUIDs;

  IODCommonInstanceReferenceModule &commref = segdoc->getCommonInstanceReference();
  OFVector<IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*> &refseries = commref.getReferencedSeriesItems();

  IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem refseriesItem;
  refseries.push_back(&refseriesItem);

  OFVector<SOPInstanceReferenceMacro*> &refinstances = refseriesItem.getReferencedInstanceItems();

  DcmFileFormat ff;
  CHECK_COND(ff.loadFile(inputDICOMImageFileNames[slice2derimg[0]].c_str()));
  DcmDataset *dcm = ff.getDataset();
  CHECK_COND(dcm->findAndGetOFString(DCM_SeriesInstanceUID, seriesInstanceUID));
  CHECK_COND(refseriesItem.setSeriesInstanceUID(seriesInstanceUID));

  int uidfound = 0, uidnotfound = 0;

  Uint8 *frameData = new Uint8[frameSize];
  for(int segFileNumber=0;segFileNumber<inputSegmentationsFileNames.size();segFileNumber++){

    std::cout << "Processing input label " << inputSegmentationsFileNames[segFileNumber] << std::endl;

    // read meta-information for the segmentation file
    std::map<unsigned,SegmentAttributes> label2attributes;
    {
      std::ifstream attrStream(inputLabelAttributesFileNames[segFileNumber].c_str());
      while(!attrStream.eof()){
        std::string attrString;
        getline(attrStream,attrString);
        std::string labelStr, attributesStr;
        SplitString(attrString,labelStr,attributesStr,";");
        unsigned labelId = atoi(labelStr.c_str());
        label2attributes[labelId] = SegmentAttributes(labelId);
        label2attributes[labelId].populateAttributesFromString(attributesStr);
        //label2attributes[labelId].PrintSelf();
      }
    }

    LabelToLabelMapFilterType::Pointer l2lm = LabelToLabelMapFilterType::New();
    reader->SetFileName(inputSegmentationsFileNames[segFileNumber]);
    reader->Update();
    ImageType::Pointer labelImage = reader->GetOutput();

    l2lm->SetInput(labelImage);

    l2lm->Update();

    typedef LabelToLabelMapFilterType::OutputImageType::LabelObjectType LabelType;
    typedef itk::LabelStatisticsImageFilter<ImageType,ImageType> LabelStatisticsType;

    LabelStatisticsType::Pointer labelStats = LabelStatisticsType::New();

    std::cout << "Found " << l2lm->GetOutput()->GetNumberOfLabelObjects() << " label(s)" << std::endl;
    labelStats->SetInput(reader->GetOutput());
    labelStats->SetLabelInput(reader->GetOutput());
    labelStats->Update();

    bool cropSegmentsBBox = false;
    if(cropSegmentsBBox){
      std::cout << "WARNING: Crop operation enabled - WIP" << std::endl;
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
      std::cout << "OVerall bounding box: " << threshBbox[0] << ", " << threshBbox[1]
                   << threshBbox[2] << ", " << threshBbox[3]
                   << threshBbox[4] << ", " << threshBbox[5]
                   << std::endl;
                   */
      return -1;//abort();
    }

    for(int segLabelNumber=0;segLabelNumber<l2lm->GetOutput()->GetNumberOfLabelObjects();segLabelNumber++){
      LabelType* labelObject = l2lm->GetOutput()->GetNthLabelObject(segLabelNumber);
      short label = labelObject->GetLabel();

      if(!label){
        std::cout << "Skipping label 0" << std::endl;
        continue;
      }

      std::cout << "Processing label " << label << std::endl;

      LabelStatisticsType::BoundingBoxType bbox = labelStats->GetBoundingBox(label);
      unsigned firstSlice, lastSlice;
      if(skipEmptySlices){
        firstSlice = bbox[4];
        lastSlice = bbox[5]+1;
      } else {
        firstSlice = 0;
        lastSlice = inputSize[2];
      }

      std::cout << "Total non-empty slices that will be encoded in SEG for label " <<
        label << " is " << lastSlice-firstSlice << std::endl <<
        " (inclusive from " << firstSlice << " to " <<
        lastSlice << ")" << std::endl;

      DcmSegment* segment = NULL;

      std::string segmentName;
      std::string segFileName = inputSegmentationsFileNames[segFileNumber];
      CodeSequenceMacro categoryCode, typeCode;

      // these are required
      categoryCode = StringToCodeSequenceMacro(label2attributes[label].lookupAttribute("SegmentedPropertyCategory"));
      typeCode = StringToCodeSequenceMacro(label2attributes[label].lookupAttribute("SegmentedPropertyType"));

      // these ones are optional
      std::string anatomicRegionStr = label2attributes[label].lookupAttribute("AnatomicRegion");
      std::string anatomicRegionModifierStr = label2attributes[label].lookupAttribute("AnatomicRegionModifer");

      DcmSegTypes::E_SegmentAlgoType algoType;
      std::string algoName = "";
      std::string algoTypeStr = label2attributes[label].lookupAttribute("SegmentAlgorithmType");
      if(algoTypeStr == "MANUAL"){
        algoType = DcmSegTypes::SAT_MANUAL;
      } else {
        if(algoTypeStr == "AUTOMATIC")
          algoType = DcmSegTypes::SAT_AUTOMATIC;
        if(algoTypeStr == "SEMIAUTOMATIC")
          algoType = DcmSegTypes::SAT_SEMIAUTOMATIC;
        algoName = label2attributes[label].lookupAttribute("SegmentAlgorithmName");
        if(algoName == ""){
          std::cerr << "ERROR: Algorithm name must be specified for non-manual algorithm types!" << std::endl;
          return -1;
        }
      }

      OFString segmentLabel;
      CHECK_COND(typeCode.getCodeMeaning(segmentLabel));
      CHECK_COND(DcmSegment::create(segment,
                              segmentLabel,
                              categoryCode, typeCode,
                              algoType,
                              algoName.c_str()));


      if(anatomicRegionStr != ""){
        GeneralAnatomyMacro &anatomyMacro = segment->getGeneralAnatomyCode();
        CodeSequenceMacro &anatomicRegion = anatomyMacro.getAnatomicRegion();
        OFVector<CodeSequenceMacro*>& modifiersVector = anatomyMacro.getAnatomicRegionModifier();

        anatomicRegion = StringToCodeSequenceMacro(anatomicRegionStr);
        if(anatomicRegionModifierStr != ""){
          CodeSequenceMacro anatomicRegionModifier = StringToCodeSequenceMacro(anatomicRegionModifierStr);
          modifiersVector.push_back(&anatomicRegionModifier);
        }
      }

      std::string rgbStr = label2attributes[label].lookupAttribute("RecommendedDisplayRGBValue");
      if(rgbStr != ""){
        unsigned rgb[3];
        std::vector<std::string> rgbStrVector;
        TokenizeString(rgbStr,rgbStrVector,",");
        for(int rgb_i=0;rgb_i<3;rgb_i++)
          rgb[rgb_i] = atoi(rgbStrVector[rgb_i].c_str());

        unsigned cielabScaled[3];
        float cielab[3], ciexyz[3];

        getCIEXYZFromRGB(&rgb[0],&ciexyz[0]);
        getCIELabFromCIEXYZ(&ciexyz[0],&cielab[0]);
        getIntegerScaledCIELabFromCIELab(&cielab[0],&cielabScaled[0]);
        CHECK_COND(segment->setRecommendedDisplayCIELabValue(cielabScaled[0],cielabScaled[1],cielabScaled[2]));
      }

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
        //std::ostringstream inStackPosSStream; // StackID is not present/needed
        //inStackPosSStream << s+1;
        //fracon->setInStackPositionNumber(s+1);

        // PerFrame FG: PlanePositionSequence
        {
          ImageType::PointType sliceOriginPoint;
          ImageType::IndexType sliceOriginIndex;
          sliceOriginIndex.Fill(0);
          sliceOriginIndex[2] = sliceNumber;
          labelImage->TransformIndexToPhysicalPoint(sliceOriginIndex, sliceOriginPoint);
          std::ostringstream pppSStream;
          if(sliceNumber>0){
            ImageType::PointType prevOrigin;
            ImageType::IndexType prevIndex;
            prevIndex.Fill(0);
            prevIndex[2] = sliceNumber-1;
            labelImage->TransformIndexToPhysicalPoint(prevIndex, prevOrigin);
          }
          fgppp->setImagePositionPatient(
            FloatToStrScientific(sliceOriginPoint[0]).c_str(),
            FloatToStrScientific(sliceOriginPoint[1]).c_str(),
            FloatToStrScientific(sliceOriginPoint[2]).c_str());
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
              //std::cout << framePixelCnt << " " << idx[1] << "," << idx[0] << std::endl;
            } else
              frameData[framePixelCnt] = 0;
          }

          if(sliceNumber!=firstSlice)
            checkValidityOfFirstSrcImage(segdoc);

          FGDerivationImage* fgder = new FGDerivationImage();
          perFrameFGs[2] = fgder;
          //fgder->clearData();

          if(sliceNumber!=firstSlice)
            checkValidityOfFirstSrcImage(segdoc);

          DerivationImageItem *derimgItem;
          CHECK_COND(fgder->addDerivationImageItem(CodeSequenceMacro("113076","DCM","Segmentation"),"",derimgItem));

          OFVector<OFString> siVector;

          if(sliceNumber>=inputDICOMImageFileNames.size()){
            std::cerr << "ERROR: trying to access missing DICOM Slice! And sorry, multi-frame not supported at the moment..." << std::endl;
            return -1;
          }

          siVector.push_back(OFString(inputDICOMImageFileNames[slice2derimg[sliceNumber]].c_str()));

          OFVector<SourceImageItem*> srcimgItems;
          CHECK_COND(derimgItem->addSourceImageItems(siVector,
              CodeSequenceMacro("121322","DCM","Source image for image processing operation"),
              srcimgItems));

          CHECK_COND(segdoc->addFrame(frameData, segmentNumber, perFrameFGs));

          // check if frame 0 still has what we expect
          checkValidityOfFirstSrcImage(segdoc);

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

  //std::cout << "found:" << uidfound << " not: " << uidnotfound << std::endl;

  COUT << "Successfully created segmentation document" << OFendl;

  /* Store to disk */
  COUT << "Saving the result to " << outputSEGFileName << OFendl;
  //segdoc->saveFile(outputSEGFileName.c_str(), EXS_LittleEndianExplicit);

  CHECK_COND(segdoc->writeDataset(segdocDataset));

  // Set reader/session/timepoint information
  CHECK_COND(segdocDataset.putAndInsertString(DCM_ContentCreatorName, readerId.c_str()));
  CHECK_COND(segdocDataset.putAndInsertString(DCM_ClinicalTrialSeriesID, sessionId.c_str()));
  CHECK_COND(segdocDataset.putAndInsertString(DCM_ClinicalTrialTimePointID, timePointId.c_str()));
  CHECK_COND(segdocDataset.putAndInsertString(DCM_ClinicalTrialCoordinatingCenterName, "UIowa"));

  // populate BodyPartExamined
  {
    DcmFileFormat sliceFF;
    DcmDataset *sliceDataset = NULL;
    OFString bodyPartStr;
    std::string bodyPartAssigned = bodyPart;

    CHECK_COND(sliceFF.loadFile(inputDICOMImageFileNames[0].c_str()));

    sliceDataset = sliceFF.getDataset();

    // inherit BodyPartExamined from the source image dataset, if available
    if(sliceDataset->findAndGetOFString(DCM_BodyPartExamined, bodyPartStr).good())
      if(std::string(bodyPartStr.c_str()).size())
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

    segdocDataset.putAndInsertString(DCM_SeriesDescription, seriesDescription.c_str());
    segdocDataset.putAndInsertString(DCM_SeriesNumber, seriesNumber.c_str());
  }

  DcmFileFormat segdocFF(&segdocDataset);
  if(compress){
    CHECK_COND(segdocFF.saveFile(outputSEGFileName.c_str(), EXS_DeflatedLittleEndianExplicit));
  } else {
    CHECK_COND(segdocFF.saveFile(outputSEGFileName.c_str(), EXS_LittleEndianExplicit));
  }

  COUT << "Saved segmentation as " << outputSEGFileName << std::endl;

  return 0;
}
