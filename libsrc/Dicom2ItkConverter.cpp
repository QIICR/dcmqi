
// DCMQI includes
#include "dcmqi/Dicom2ItkConverter.h"
#include "dcmqi/ColorUtilities.h"
#include "dcmqi/OverlapUtil.h"


//DCMTK includes
#include <dcmtk/dcmiod/cielabutil.h>
#include <dcmtk/dcmsr/codes/dcm.h>
#include <dcmtk/ofstd/ofmem.h>


namespace dcmqi {


  Dicom2ItkConverter::Dicom2ItkConverter()
  : m_segDoc()
  , m_direction()
  , m_computedSliceSpacing()
  , m_computedVolumeExtent()
  , m_sliceDirection(3)
  , m_imageOrigin()
  , m_imageSpacing()
  , m_imageSize()
  , m_imageRegion()
  , m_metaInfo()
  , m_overlapUtil()
  {
  };

  // -------------------------------------------------------------------------------------

  pair <map<unsigned,ShortImageType::Pointer>, string> Dicom2ItkConverter::dcmSegmentation2itkimage(DcmDataset *segDataset, const bool mergeSegments) {
    DcmSegmentation *segdoc = NULL;


    // Make sure RLE-compressed images can be decompressed
    DcmRLEDecoderRegistration::registerCodecs();

    // Load the DICOM segmentation dataset into DcmSegmentation member
    OFCondition cond = DcmSegmentation::loadDataset(*segDataset, segdoc);
    if(!segdoc){
      cerr << "ERROR: Failed to load segmentation dataset! " << cond.text() << endl;
      throw -1;
    }
    m_segDoc.reset(segdoc);

    // Populate DICOM series information into accompanying JSON metainfo member
    populateMetaInformationFromDICOM(segDataset);

    const std::map<unsigned, ShortImageType::Pointer> segment2image = dcmSegmentation2itkimage(mergeSegments);
    return pair<map<unsigned, ShortImageType::Pointer>, string>(
        segment2image, m_metaInfo.getJSONOutputAsString());
  }

  // -------------------------------------------------------------------------------------

  std::map<unsigned, ShortImageType::Pointer> Dicom2ItkConverter::dcmSegmentation2itkimage(const bool mergeSegments)
  {
    // Setup logging (not used so far?)
    OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");
    dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);

    // The result to be returned: ITK images corresponding to the individual segments
    map<unsigned,ShortImageType::Pointer> segment2image;

    // Extract directions, origin, spacing and image region
    OFCondition result = extractBasicSegmentationInfo();

    // Find groups of segments that can go into the same ITK image (i.e. that are non-overlapping)
    // TODO: Use dcemfinfLogger DEBUG logger: cout << "Computing groups of non-overlapping segments" << std::endl;
    m_overlapUtil.setSegmentationObject(m_segDoc.get());
    OverlapUtil::SegmentGroups segmentGroups;
    getNonOverlappingSegmentGroups(mergeSegments, segmentGroups);
    // TODO: Use dcemfinfLogger DEBUG logger: cout << "Found " << segmentGroups.size() << " groups of non-overlapping segments" << std::endl;

    // Create ITK image template, will be to create actual ITK images through
    // ImageDuplicator later.
    ShortImageType::Pointer imageTemplate = allocateITKImageTemplate();

    // Iterate over segment groups. For each group, create an ITK image and
    // populate it with the segments' data from this group. Use the segment
    // number as a pixel value. The group number is not directly relevant; we
    // use it as index to store results (i.e. one file per group) into
    // segment2image map.
    // Along the way, we also create JSON metadata for each segment,
    // to be returned in metaInfo parameter.
    OverlapUtil::SegmentGroups::iterator group = segmentGroups.begin();
    size_t groupNumber = 0;
    while (group != segmentGroups.end())
    {
      // Create target ITK image for this group
      ShortImageType::Pointer itkImage = allocateITKImageDuplicate(imageTemplate);
      segment2image[groupNumber] = itkImage;

      // Loop over segments belonging to this segment group
      auto segNum = group->begin();
      while (segNum != group->end())
      {
        // Iterate over frames for this segment, and copy the data into the ITK image.
        // Afterwards, the ITK image will have the complete data belonging to that segment
        OverlapUtil::FramesForSegment::value_type framesForSegment;
        m_overlapUtil.getFramesForSegment(*segNum, framesForSegment);
        for (size_t frameIndex = 1 /* vector uses 1 as starting index*/; frameIndex < framesForSegment.size(); frameIndex++)
        {
          cout << "Writing group " << groupNumber << " segment " << *segNum << " frame " << frameIndex << endl;
          // Copy the data from the frame into the ITK image
          ShortImageType::PointType frameOriginPoint;
          ShortImageType::IndexType frameOriginIndex;
          result = getITKImageOrigin(framesForSegment[frameIndex], frameOriginPoint);
          if (result.bad())
          {
            cerr << "ERROR: Failed to get origin for frame " << framesForSegment[frameIndex] << " of segment " << *segNum << endl;
            throw -1;
          }
          if(!itkImage->TransformPhysicalPointToIndex(frameOriginPoint, frameOriginIndex))
          {
            cerr << "ERROR: Frame " << framesForSegment[frameIndex] << " origin " << frameOriginPoint <<
            " is outside image geometry!" << frameOriginIndex << endl;
            cerr << "Image size: " << itkImage->GetBufferedRegion().GetSize() << endl;
            throw -1;
          }
          // Handling differs depending on whether the segmentation is binary or fractional
          // (we have to unpack binary frames before copying them into the ITK image)
          const DcmIODTypes::Frame *rawFrame = m_segDoc->getFrame(framesForSegment[frameIndex]);
          const DcmIODTypes::Frame *unpackedFrame = NULL;
          if(m_segDoc->getSegmentationType() == DcmSegTypes::ST_BINARY)
          {
            unpackedFrame = DcmSegUtils::unpackBinaryFrame(rawFrame,
                                    m_imageSize[1],  // Rows
                                    m_imageSize[0]); // Cols
          }
          else
          {
            // fractional segmentation frames can be used as is
            unpackedFrame = rawFrame;
          }
          unsigned slice = frameOriginIndex[2];

    {
     /* WIP */
        // use ConstIterator, as in this example:
        // https://github.com/InsightSoftwareConsortium/ITK/blob/633f84548311600845d54ab2463d3412194690a8/Examples/Iterators/ImageRegionIterator.cxx#L201-L212
      //   CharImageType::RegionType frameRegion;
      //   CharImageType::RegionType::IndexType regionStart;
      //   CharImageType::RegionType::SizeType regionSize;

      //   regionStart[0] = 0;
      //   regionStart[1] = 0;
      //   regionStart[2] = slice;

      //   regionSize[0] = imageSize[0];
      //   regionSize[1] = imageSize[1];
      //   regionSize[2] = 1;

      //   frameRegion.SetSize(regionSize);
      //   frameRegion.SetIndex(regionStart);

      //   const bool importImageFilterWillOwnTheBuffer = false;

      //   // itkImportImageFilter example:
      //   // https://itk.org/Doxygen/html/SphinxExamples_2src_2Core_2Common_2ConvertArrayToImage_2Code_8cxx-example.html#_a1
      //   const unsigned int numberOfPixels = imageSize[0]*imageSize[1];
      //   using ImportFilterType = itk::ImportImageFilter<CharPixelType, 3>;
      //   ImportFilterType::Pointer importFilter = ImportFilterType::New();
      //   importFilter->SetImportPointer((unsigned char*)unpackedFrame->pixData, numberOfPixels, importImageFilterWillOwnTheBuffer);
      //   importFilter->SetRegion(frameRegion);
      //   importFilter->Update();


      //   using ConstIteratorType = itk::ImageRegionConstIterator<CharImageType>;
      //   using IteratorType = itk::ImageRegionIterator<ShortImageType>;
      //   ConstIteratorType inputIt(importFilter->GetOutput(), frameRegion);
      //   IteratorType outputIt(segment2image[targetSegmentsImageIndex], frameRegion);

      //   inputIt.GoToBegin();
      //   outputIt.GoToBegin();

      //   while(!inputIt.IsAtEnd()){
      //     if(inputIt.Get() != 0)
      //       outputIt.Set(segmentIdLabel);
      //     ++outputIt;
      //     ++inputIt;
      // }
      /* WIP */
        }

          for(unsigned row=0;row<m_imageSize[1];row++)
          {
            for(unsigned col=0;col<m_imageSize[0];col++)
            {
              ShortImageType::PixelType pixel;
              unsigned bitCnt = row*m_imageSize[0]+col;
              pixel = unpackedFrame->pixData[bitCnt];
              ShortImageType::IndexType index;
              if(pixel!=0)
              {
                index[0] = col;
                index[1] = row;
                index[2] = slice;
                // Use the segment number as the pixel value in case of binary segmentations.
                // Otherwise, just copy the pixel value (fractional value) from the frame.
                if (m_segDoc->getSegmentationType() == DcmSegTypes::ST_BINARY)
                {
                  itkImage->SetPixel(index, *segNum);
                }
                else
                {
                  itkImage->SetPixel(index, pixel);
                }
              }
            }
          }
          if (m_segDoc->getSegmentationType() == DcmSegTypes::ST_BINARY)
          {
            delete unpackedFrame;
            unpackedFrame = NULL;
          }
        }
        // Create/register JSON metadata for this segment
        result = addSegmentMetadata(groupNumber, *segNum);
        segNum++;
      }
      group++; groupNumber++;
    }
    return segment2image;
  }

  // -------------------------------------------------------------------------------------

  void Dicom2ItkConverter::populateMetaInformationFromDICOM(DcmDataset *segDataset)
  {
    OFString creatorName, sessionID, timePointID, seriesDescription, seriesNumber, instanceNumber, bodyPartExamined, coordinatingCenter;

    segDataset->findAndGetOFString(DCM_InstanceNumber, instanceNumber);
    m_segDoc->getContentIdentification().getContentCreatorName(creatorName);

    segDataset->findAndGetOFString(DCM_ClinicalTrialTimePointID, timePointID);
    segDataset->findAndGetOFString(DCM_ClinicalTrialSeriesID, sessionID);
    segDataset->findAndGetOFString(DCM_ClinicalTrialCoordinatingCenterName, coordinatingCenter);

    m_segDoc->getSeries().getBodyPartExamined(bodyPartExamined);
    m_segDoc->getSeries().getSeriesNumber(seriesNumber);
    m_segDoc->getSeries().getSeriesDescription(seriesDescription);

    m_metaInfo.setClinicalTrialCoordinatingCenterName(coordinatingCenter.c_str());
    m_metaInfo.setContentCreatorName(creatorName.c_str());
    m_metaInfo.setClinicalTrialSeriesID(sessionID.c_str());
    m_metaInfo.setSeriesNumber(seriesNumber.c_str());
    m_metaInfo.setClinicalTrialTimePointID(timePointID.c_str());
    m_metaInfo.setSeriesDescription(seriesDescription.c_str());
    m_metaInfo.setInstanceNumber(instanceNumber.c_str());
    m_metaInfo.setBodyPartExamined(bodyPartExamined.c_str());
  }

  // -------------------------------------------------------------------------------------

  OFCondition Dicom2ItkConverter::extractBasicSegmentationInfo()
  {
    // TODO: Better error handling
    OFCondition result;

    // Directions
    FGInterface &fgInterface = m_segDoc->getFunctionalGroups();
    if(getImageDirections(fgInterface, m_direction)){
      cerr << "ERROR: Failed to get image directions!" << endl;
      throw -1;
    }

    // Origin
    m_sliceDirection[0] = m_direction[0][2];
    m_sliceDirection[1] = m_direction[1][2];
    m_sliceDirection[2] = m_direction[2][2];
    if(computeVolumeExtent(fgInterface, m_sliceDirection, m_imageOrigin, m_computedSliceSpacing, m_computedVolumeExtent)){
      cerr << "ERROR: Failed to compute origin and/or slice spacing!" << endl;
      throw -1;
    }

    // Spacing
    m_imageSpacing.Fill(0);
    if(getDeclaredImageSpacing(fgInterface, m_imageSpacing)){
      cerr << "ERROR: Failed to get image spacing from DICOM!" << endl;
      throw -1;
    }

    if(!m_imageSpacing[2]){
      cerr << "ERROR: No sufficient information to derive slice spacing! Unable to interpret the data." << endl;
      throw -1;
    }

    // Region, defined by DICOM rows and columns
    {
      DcmIODImage<IODImagePixelModule<Uint8> > *ip =
          static_cast<DcmIODImage<IODImagePixelModule<Uint8> > *>(m_segDoc.get());
      Uint16 value;
      if (ip->getImagePixel().getRows(value).good()) {
        m_imageSize[1] = value;
      }
      if (ip->getImagePixel().getColumns(value).good()) {
        m_imageSize[0] = value;
      }
    }
    // Number of slices should be computed, since segmentation may have empty frames
    m_imageSize[2] = round(m_computedVolumeExtent/m_imageSpacing[2])+1;

    // Initialize the image template. This will only serve as a template for the individual
    // frames, which will be created via ImageDuplicator later on.
    m_imageRegion.SetSize(m_imageSize);
    return result;
  }


  // -------------------------------------------------------------------------------------

  OFCondition Dicom2ItkConverter::getNonOverlappingSegmentGroups(const bool mergeSegments,
                                                                OverlapUtil::SegmentGroups& segmentGroups)
  {
    OFCondition result;
    if (mergeSegments)
    {
      result = m_overlapUtil.getNonOverlappingSegments(segmentGroups);
      if (result.bad())
      {
        cout << "WARNING: Failed to compute non-overlapping segments (Error: " << result.text() << "), "
              << "falling back to one group per segment instead." << endl;
      }
      cout << "Identified " << segmentGroups.size() << " groups of non-overlapping segments" << endl;
    }
    // Otherwise, use single group containing all segments (which might overlap)
    if (!mergeSegments || result.bad())
    {
      size_t numSegs = m_segDoc->getNumberOfSegments();
      for (size_t i = 1; i <= numSegs; ++i)
      {
        OFVector<Uint32> segs;
        segs.push_back(i);
        segmentGroups.push_back(segs);
      }
      cout << "Will not merge segments: Splitting segments into " << segmentGroups.size() << " groups" << endl;
    }
    return result;;
  }

  // -------------------------------------------------------------------------------------

  ShortImageType::Pointer Dicom2ItkConverter::allocateITKImageTemplate()
  {
    // Initialize the image
    ShortImageType::Pointer segImage = ShortImageType::New();
    segImage->SetRegions(m_imageRegion);
    segImage->SetOrigin(m_imageOrigin);
    segImage->SetSpacing(m_imageSpacing);
    segImage->SetDirection(m_direction);
    segImage->Allocate();
    segImage->FillBuffer(0);
    return segImage;
  }

  ShortImageType::Pointer Dicom2ItkConverter::allocateITKImageDuplicate(ShortImageType::Pointer imageTemplate)
  {
      typedef itk::ImageDuplicator<ShortImageType> DuplicatorType;
      DuplicatorType::Pointer dup = DuplicatorType::New();
      dup->SetInputImage(imageTemplate);
      dup->Update();
      ShortImageType::Pointer newSegmentImage = dup->GetOutput();
      newSegmentImage->FillBuffer(0);
      return newSegmentImage;
  }

  // -------------------------------------------------------------------------------------

  OFCondition Dicom2ItkConverter::getITKImageOrigin(const Uint32 frameNo, ShortImageType::PointType& origin)
  {
    FGInterface& fgInterface = m_segDoc->getFunctionalGroups();

    FGPlanePosPatient *planposfg =
        OFstatic_cast(FGPlanePosPatient*,fgInterface.get(frameNo, DcmFGTypes::EFG_PLANEPOSPATIENT));
    assert(planposfg);

    for(int j=0;j<3;j++)
    {
      OFString planposStr;
      if(planposfg->getImagePositionPatient(planposStr, j).good())
      {
        origin[j] = atof(planposStr.c_str());
      }
    }
    return EC_Normal;
  }

  // -------------------------------------------------------------------------------------

  OFCondition Dicom2ItkConverter::addSegmentMetadata(const size_t segmentGroup,
                                                    const Uint16 segmentNumber)
  {
    SegmentAttributes* segmentAttributes = m_metaInfo.createOrGetSegment(segmentGroup, segmentNumber);
    // NOTE: Segment numbers in DICOM start with 1
    DcmSegment* segment = m_segDoc->getSegment(segmentNumber);
    if(segment == NULL){
      cerr << "Failed to get segment for segment ID " << segmentNumber << endl;
      return EC_IllegalParameter;
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

    ColorUtilities::getSRGBFromIntegerScaledCIELabPCS(rgb[0], rgb[1], rgb[2], ciedcm[0], ciedcm[1], ciedcm[2]);

    if (segmentAttributes) {
      segmentAttributes->setLabelID(segmentNumber);
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
    return EC_Normal;
  }



}
