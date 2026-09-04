
// DCMQI includes
#include "dcmqi/Itk2DicomConverter.h"
#include "dcmqi/ColorUtilities.h"
#include "dcmqi/JSONSegmentationMetaInformationHandler.h"
#include "dcmqi/SourceImageIndex.h"

// DCMTK includes
#include <dcmtk/config/osconfig.h>
#if defined(PACKAGE_VERSION_NUMBER) && PACKAGE_VERSION_NUMBER > 366
  #include <dcmtk/dcmdata/dcuid.h>
  #include <dcmtk/dcmdata/dcvrda.h>
  #include <dcmtk/dcmdata/dcvrtm.h>
  #include <dcmtk/dcmfg/fgfracon.h>
#endif
#include <dcmtk/dcmsr/codes/dcm.h>

#include <itkVectorImageToImageAdaptor.h>

#include <memory>


namespace dcmqi {

  namespace {

    // Code describing how the output was derived from the source images (CID 7203)
    CodeSequenceMacro segmentationDerivationCode() {
      DSRBasicCodedEntry code_seg = CODE_DCM_Segmentation_113076;
      return CodeSequenceMacro(code_seg.CodeValue, code_seg.CodingSchemeDesignator, code_seg.CodeMeaning);
    }

    // Purpose of reference for the source images (CID 7202)
    CodeSequenceMacro sourceImagePurposeOfReferenceCode() {
      DSRBasicCodedEntry code = CODE_DCM_SourceImageForImageProcessingOperation;
      return CodeSequenceMacro(code.CodeValue, code.CodingSchemeDesignator, code.CodeMeaning);
    }

    /** Assigns the Segment Numbers that are written to DICOM and keeps the
     *  mappings derived from them.
     *
     *  Two numbering schemes exist: sequential (1..N, in the order the labels
     *  are processed) and the label IDs of the input images used directly.
     *  The rules of the latter differ between the two output flavors, and the
     *  resulting numbers are needed in three more places - to size the pixel
     *  bit depth of a labelmap, to translate input labels into pixel values
     *  while composing labelmap frames, and to re-map the numbers back to
     *  label IDs after a binary segmentation was written. Keeping the scheme
     *  and its three mappings together avoids restating the rules at each of
     *  those places.
     */
    class SegmentNumberAssigner {

    public:

      SegmentNumberAssigner(const bool useLabelIDAsSegmentNumber,
                            const bool outputLabelMap,
                            const size_t numSegmentationFiles)
        : m_useLabelIDAsSegmentNumber(useLabelIDAsSegmentNumber),
          m_outputLabelMap(outputLabelMap),
          m_nextSegmentNumber(1),
          m_fileLabelToSegmentNumber(numSegmentationFiles)
      {
      }

      /** Determine whether labelmap pixel data needs 16 instead of 8 bit.
       *  Pixel values equal the segment numbers, so the bit depth follows from
       *  the largest number that will be assigned: the highest label ID if
       *  label IDs are used (they may contain gaps), the number of segments
       *  otherwise.
       *  @param  metaInfo    Metadata describing the segments to be created.
       *  @param  use16Bit    Set to true if 16 bit pixel data is required.
       *  @return true on success, false if the required number of segments
       *          exceeds the 16 bit pixel value range.
       */
      bool determineLabelmapBitDepth(JSONSegmentationMetaInformationHandler& metaInfo, bool& use16Bit) const {
        size_t maxSegmentNumber = 0;
        size_t numSegmentsMetadata = 0;
        for (size_t segFileNumber = 0; segFileNumber < metaInfo.segmentsAttributesMappingList.size(); segFileNumber++)
        {
          const map<unsigned, SegmentAttributes*>& fileAttributes = metaInfo.segmentsAttributesMappingList[segFileNumber];
          numSegmentsMetadata += fileAttributes.size();
          if (m_useLabelIDAsSegmentNumber && !fileAttributes.empty())
          {
            // map is ordered by key, so the last entry holds the highest label ID
            maxSegmentNumber = std::max(maxSegmentNumber, static_cast<size_t>(fileAttributes.rbegin()->first));
          }
        }
        if (!m_useLabelIDAsSegmentNumber)
          maxSegmentNumber = numSegmentsMetadata;
        if (maxSegmentNumber > 65535)
        {
          cerr << "ERROR: Cannot create labelmap SEG: maximum segment number " << maxSegmentNumber
               << " exceeds the 16 bit pixel value range!" << endl;
          return false;
        }
        use16Bit = maxSegmentNumber > 255;
        return true;
      }

      /** Propose the Segment Number to be used for the given label. The number
       *  is only recorded once the segment has actually been added, see
       *  recordSegment().
       *  @param  label          Label ID from the input image.
       *  @param  segmentNumber  Set to the proposed Segment Number.
       *  @return true on success, false (with an error message) if the label ID
       *          cannot serve as a Segment Number.
       */
      bool proposeSegmentNumber(const short label, Uint16& segmentNumber) {
        if (!m_useLabelIDAsSegmentNumber)
        {
          segmentNumber = m_nextSegmentNumber++;
          return true;
        }
        if (label < 0)
        {
          cerr << "ERROR: Cannot use label ID " << label << " as segment number: label IDs must be positive!" << endl;
          return false;
        }
        segmentNumber = static_cast<Uint16>(label);
        // For labelmap output the label IDs become segment numbers directly, and
        // DcmSegmentation::addSegment() replaces an existing labelmap segment with
        // the same number (upsert), so a collision across input files would
        // silently drop a segment. Reject it here. (The binary path detects
        // collisions later via checkLabelNumbering().)
        if (m_outputLabelMap && m_segmentToLabel.find(segmentNumber) != m_segmentToLabel.end())
        {
          cerr << "ERROR: Label ID " << label << " is used by more than one input segment; "
               << "cannot use label IDs as segment numbers!" << endl;
          return false;
        }
        return true;
      }

      /** Record the Segment Number a segment was actually added with (which may
       *  differ from the proposed one). */
      void recordSegment(const size_t segFileNumber, const short label, const Uint16 segmentNumber) {
        m_segmentToLabel.insert(make_pair(segmentNumber, label));
        if (m_outputLabelMap)
          m_fileLabelToSegmentNumber[segFileNumber][label] = segmentNumber;
      }

      /** Segment Number to use as the labelmap pixel value for the given input label.
       *  @return true if the label belongs to a recorded segment. */
      bool pixelValueFor(const size_t segFileNumber, const short label, Uint16& segmentNumber) const {
        map<short, Uint16>::const_iterator it = m_fileLabelToSegmentNumber[segFileNumber].find(label);
        if (it == m_fileLabelToSegmentNumber[segFileNumber].end())
          return false;
        segmentNumber = it->second;
        return true;
      }

      /** Mapping from assigned Segment Number to the label ID it came from */
      const map<Uint16, Uint16>& segmentToLabel() const { return m_segmentToLabel; }

    private:

      const bool m_useLabelIDAsSegmentNumber;
      const bool m_outputLabelMap;
      Uint16 m_nextSegmentNumber;
      map<Uint16, Uint16> m_segmentToLabel;
      /// per input file, the Segment Number each label was assigned (labelmap output)
      vector<map<short, Uint16> > m_fileLabelToSegmentNumber;
    };

    /** Create the DICOM Segmentation document (binary or labelmap).
     *  Returns NULL if the document cannot be created.
     */
    DcmSegmentation* createSegmentationDocument(const Uint16 rows, const Uint16 columns,
                                                const bool outputLabelMap,
                                                const bool labelmapUse16Bit,
                                                IODGeneralEquipmentModule::EquipmentInfo& eq,
                                                ContentIdentificationMacro& ident) {
      DcmSegmentation* segdoc = NULL;

      if (outputLabelMap)
      {
        CHECK_COND(DcmSegmentation::createLabelmapSegmentation(
            segdoc,
            rows,
            columns,
            eq,
            ident,
            labelmapUse16Bit,
            DcmSegTypes::SLCM_MONOCHROME2));
      }
      else
      {
        CHECK_COND(DcmSegmentation::createBinarySegmentation(
            segdoc,   // resulting segmentation
            rows,
            columns,
            eq,       // equipment
            ident));  // content identification
      }
      return segdoc;
    }

    /** Initialize the Multi-frame Dimension module of the segmentation document */
    void initializeDimensions(DcmSegmentation& segdoc, const bool outputLabelMap) {
      char dimUID[128];
      dcmGenerateUniqueIdentifier(dimUID, QIICR_UID_ROOT);
      IODMultiframeDimensionModule &mfdim = segdoc.getDimensions();
      if (outputLabelMap)
      {
        CHECK_COND(mfdim.addDimensionIndex(DCM_StackID, dimUID, DCM_FrameContentSequence, "Stack ID"));
        CHECK_COND(mfdim.addDimensionIndex(DCM_InStackPositionNumber, dimUID, DCM_FrameContentSequence, "In-Stack Position Number"));
      }
      else
      {
        CHECK_COND(mfdim.addDimensionIndex(DCM_ReferencedSegmentNumber, dimUID, DCM_SegmentIdentificationSequence,
                           DcmTag(DCM_ReferencedSegmentNumber).getTagName()));
        CHECK_COND(mfdim.addDimensionIndex(DCM_ImagePositionPatient, dimUID, DCM_PlanePositionSequence,
                           DcmTag(DCM_ImagePositionPatient).getTagName()));
      }
    }

    /** Initialize the shared functional groups (Plane Orientation, Pixel Measures)
     *  from the geometry of the image being converted */
    void addGeometrySharedFGs(DcmSegmentation& segdoc, const itk::ImageBase<3>& geometry) {
      // Shared FGs: PlaneOrientationPatientSequence
      {
        itk::ImageBase<3>::DirectionType labelDirMatrix = geometry.GetDirection();

        std::unique_ptr<FGPlaneOrientationPatient> planor(
            FGPlaneOrientationPatient::createMinimal(
                Helper::floatToStr(labelDirMatrix[0][0]).c_str(),
                Helper::floatToStr(labelDirMatrix[1][0]).c_str(),
                Helper::floatToStr(labelDirMatrix[2][0]).c_str(),
                Helper::floatToStr(labelDirMatrix[0][1]).c_str(),
                Helper::floatToStr(labelDirMatrix[1][1]).c_str(),
                Helper::floatToStr(labelDirMatrix[2][1]).c_str()));

        CHECK_COND(segdoc.addForAllFrames(*planor));
      }

      // Shared FGs: PixelMeasuresSequence
      {
        FGPixelMeasures pixmsr;

        itk::ImageBase<3>::SpacingType labelSpacing = geometry.GetSpacing();
        ostringstream spacingSStream;
        spacingSStream << scientific << labelSpacing[1] << "\\" << labelSpacing[0];
        CHECK_COND(pixmsr.setPixelSpacing(spacingSStream.str().c_str()));

        spacingSStream.clear(); spacingSStream.str("");
        spacingSStream << scientific << labelSpacing[2];
        CHECK_COND(pixmsr.setSpacingBetweenSlices(spacingSStream.str().c_str()));
        CHECK_COND(pixmsr.setSliceThickness(spacingSStream.str().c_str()));
        CHECK_COND(segdoc.addForAllFrames(pixmsr));
      }
    }

    /** Create a segment from the JSON-provided attributes.
     *  Returns NULL if required attributes are missing or invalid.
     */
    DcmSegment* createSegmentFromAttributes(SegmentAttributes& segmentAttributes) {
      DcmSegTypes::E_SegmentAlgoType algoType = DcmSegTypes::SAT_UNKNOWN;
      string algoName = "";
      string algoTypeStr = segmentAttributes.getSegmentAlgorithmType();
      if(algoTypeStr == "MANUAL"){
        algoType = DcmSegTypes::SAT_MANUAL;
      } else {
        if(algoTypeStr == "AUTOMATIC")
          algoType = DcmSegTypes::SAT_AUTOMATIC;
        if(algoTypeStr == "SEMIAUTOMATIC")
          algoType = DcmSegTypes::SAT_SEMIAUTOMATIC;

        algoName = segmentAttributes.getSegmentAlgorithmName();
        if(algoName == ""){
          cerr << "ERROR: Algorithm name must be specified for non-manual algorithm types!" << endl;
          return NULL;
        }
      }

      CodeSequenceMacro* typeCode = segmentAttributes.getSegmentedPropertyTypeCodeSequence();
      CodeSequenceMacro* categoryCode = segmentAttributes.getSegmentedPropertyCategoryCodeSequence();
      assert(typeCode != NULL && categoryCode!= NULL);
      OFString segmentLabel;

      if(segmentAttributes.getSegmentLabel().length() > 0){
        cout << "Populating segment label to " << segmentAttributes.getSegmentLabel() << endl;
        segmentLabel = segmentAttributes.getSegmentLabel().c_str();
      } else if(segmentAttributes.getSegmentDescription().length() > 0){
        cout << "Populating segment label from SegmentDescription to " << segmentAttributes.getSegmentDescription() << endl;
        segmentLabel = segmentAttributes.getSegmentDescription().c_str();
      } else
        CHECK_COND(typeCode->getCodeMeaning(segmentLabel));

      DcmSegment* segment = NULL;
      CHECK_COND(DcmSegment::create(segment, segmentLabel, *categoryCode, *typeCode, algoType, algoName.c_str()));

      if(segmentAttributes.getSegmentDescription().length() > 0)
        segment->setSegmentDescription(segmentAttributes.getSegmentDescription().c_str());

      if(segmentAttributes.getTrackingIdentifier().length() > 0)
        segment->setTrackingID(segmentAttributes.getTrackingIdentifier().c_str());

      if(segmentAttributes.getTrackingUniqueIdentifier().length() > 0)
        segment->setTrackingUID(segmentAttributes.getTrackingUniqueIdentifier().c_str());

      // note: codes are copied into the segment, which frees them on destruction,
      // while the attributes object keeps ownership of its own copies
      CodeSequenceMacro* typeModifierCode = segmentAttributes.getSegmentedPropertyTypeModifierCodeSequence();
      if (typeModifierCode != NULL) {
        OFVector<CodeSequenceMacro*>& modifiersVector = segment->getSegmentedPropertyTypeModifierCode();
        modifiersVector.push_back(new CodeSequenceMacro(*typeModifierCode));
      }

      GeneralAnatomyMacro &anatomyMacro = segment->getGeneralAnatomyCode();
      if (segmentAttributes.getAnatomicRegionSequence() != NULL){
        anatomyMacro.getAnatomicRegion() = *segmentAttributes.getAnatomicRegionSequence();

        if(segmentAttributes.getAnatomicRegionModifierSequence() != NULL){
          OFVector<CodeSequenceMacro*>& anatomyMacroModifiersVector = anatomyMacro.getAnatomicRegionModifier();
          anatomyMacroModifiersVector.push_back(new CodeSequenceMacro(*segmentAttributes.getAnatomicRegionModifierSequence()));
        }
      }

      unsigned* rgb = segmentAttributes.getRecommendedDisplayRGBValue();
      int cielab[3];

      ColorUtilities::getIntegerScaledCIELabPCSFromSRGB(cielab[0], cielab[1], cielab[2], rgb[0], rgb[1], rgb[2]);

      CHECK_COND(segment->setRecommendedDisplayCIELabValue(cielab[0],cielab[1],cielab[2]));

      return segment;
    }

    /** Owns the functional groups that are re-filled for every frame of the
     *  output, and enforces the protocol they have to be used with:
     *  beginFrame() before filling them, get() when handing them to
     *  DcmSegmentation::addFrame(), endFrame() afterwards.
     *
     *  Plane Position (Patient) and Frame Content apply to every frame, while
     *  the Derivation Image group is only part of a frame that actually
     *  references source images (an empty Derivation Image Sequence would be
     *  invalid). Its content is per-frame state that must be cleared once the
     *  frame has been added, otherwise the next frame inherits the references
     *  of its predecessor - endFrame() takes care of that.
     */
    class PerFrameFunctionalGroups {

    public:

      PerFrameFunctionalGroups()
        : m_planePosition(FGPlanePosPatient::createMinimal("1","1","1")),
          m_frameContent(new FGFrameContent()),
          m_derivationImage(new FGDerivationImage()),
          m_derivationImageUsed(false)
      {
      }

      /** Start a new frame: drop what the previous frame collected */
      void beginFrame() {
        m_functionalGroups.clear();
        m_functionalGroups.push_back(m_planePosition.get());
        m_functionalGroups.push_back(m_frameContent.get());
        m_derivationImageUsed = false;
      }

      FGPlanePosPatient& planePosition() { return *m_planePosition; }

      FGFrameContent& frameContent() { return *m_frameContent; }

      /** Access the Derivation Image group and add it to this frame. Adding it
       *  more than once per frame is a no-op, so several source image items can
       *  be collected into the same group. */
      FGDerivationImage& derivationImage() {
        if(!m_derivationImageUsed){
          m_functionalGroups.push_back(m_derivationImage.get());
          m_derivationImageUsed = true;
        }
        return *m_derivationImage;
      }

      /** The groups of the current frame, for DcmSegmentation::addFrame() */
      const OFVector<FGBase*>& get() const { return m_functionalGroups; }

      /** Finish the frame: release the per-frame content of the Derivation
       *  Image group so it does not leak into the next frame */
      void endFrame() {
        if(m_derivationImageUsed){
          m_derivationImage->clearData();
          m_derivationImageUsed = false;
        }
      }

    private:

      std::unique_ptr<FGPlanePosPatient> m_planePosition;
      std::unique_ptr<FGFrameContent> m_frameContent;
      std::unique_ptr<FGDerivationImage> m_derivationImage;
      OFVector<FGBase*> m_functionalGroups;
      bool m_derivationImageUsed;
    };

    /** Set the Image Position (Patient) of the given plane position FG to the
     *  origin of the given slice of the image */
    void setPlanePositionToSlice(FGPlanePosPatient& fgppp, const itk::ImageBase<3>& image, const unsigned sliceNumber) {
      itk::ImageBase<3>::PointType sliceOriginPoint;
      itk::ImageBase<3>::IndexType sliceOriginIndex;
      sliceOriginIndex.Fill(0);
      sliceOriginIndex[2] = sliceNumber;
      image.TransformIndexToPhysicalPoint(sliceOriginIndex, sliceOriginPoint);
      fgppp.setImagePositionPatient(
          Helper::floatToStr(sliceOriginPoint[0]).c_str(),
          Helper::floatToStr(sliceOriginPoint[1]).c_str(),
          Helper::floatToStr(sliceOriginPoint[2]).c_str());
    }

    /** Extract the binary frame of the given label from a slice of the segmentation image */
    template<class ImageSourceType>
    vector<Uint8> extractBinaryFrame(const ImageSourceType& segmentation, const short label,
                                     const unsigned sliceNumber, const unsigned frameSize) {
      typename ImageSourceType::RegionType sliceRegion;
      typename ImageSourceType::IndexType sliceIndex;
      typename ImageSourceType::SizeType sliceSize;

      auto inputSize = segmentation.GetBufferedRegion().GetSize();

      sliceIndex[0] = 0;
      sliceIndex[1] = 0;
      sliceIndex[2] = sliceNumber;

      sliceSize[0] = inputSize[0];
      sliceSize[1] = inputSize[1];
      sliceSize[2] = 1;

      sliceRegion.SetIndex(sliceIndex);
      sliceRegion.SetSize(sliceSize);

      vector<Uint8> frameData(frameSize, 0);
      unsigned framePixelCnt = 0;
      itk::ImageRegionConstIteratorWithIndex<ImageSourceType> sliceIterator(&segmentation, sliceRegion);
      for(sliceIterator.GoToBegin();!sliceIterator.IsAtEnd();++sliceIterator,++framePixelCnt){
        if(sliceIterator.Get() == label)
          frameData[framePixelCnt] = 1;
        else
          frameData[framePixelCnt] = 0;
      }
      return frameData;
    }

    /** Compose one labelmap output frame by merging the given slice of all
     *  segmentation files. Fails (returns false) if two foreground segments
     *  overlap at a pixel. frameHasForeground reports whether any non-zero
     *  pixel was written.
     */
    template<class ImageSourceType>
    bool composeLabelmapFrame(const vector<itk::SmartPointer<const ImageSourceType>>& segmentations,
                              const SegmentNumberAssigner& segmentNumbers,
                              const unsigned sliceNumber, const unsigned frameSize,
                              const bool use16Bit,
                              vector<Uint8>& frameData8, vector<Uint16>& frameData16,
                              bool& frameHasForeground) {
      if (use16Bit)
        frameData16.assign(frameSize, 0);
      else
        frameData8.assign(frameSize, 0);

      frameHasForeground = false;

      auto inputSize = segmentations[0]->GetBufferedRegion().GetSize();

      for (size_t segFileNumber = 0; segFileNumber < segmentations.size(); segFileNumber++)
      {
        typename ImageSourceType::RegionType sliceRegion;
        typename ImageSourceType::IndexType sliceIndex;
        typename ImageSourceType::SizeType sliceSize;

        sliceIndex[0] = 0;
        sliceIndex[1] = 0;
        sliceIndex[2] = sliceNumber;

        sliceSize[0] = inputSize[0];
        sliceSize[1] = inputSize[1];
        sliceSize[2] = 1;

        sliceRegion.SetIndex(sliceIndex);
        sliceRegion.SetSize(sliceSize);

        unsigned framePixelCnt = 0;
        itk::ImageRegionConstIteratorWithIndex<ImageSourceType> sliceIterator(segmentations[segFileNumber], sliceRegion);
        for (sliceIterator.GoToBegin(); !sliceIterator.IsAtEnd(); ++sliceIterator, ++framePixelCnt)
        {
          short inputLabel = sliceIterator.Get();
          if (inputLabel == 0)
            continue;

          Uint16 segmentValue = 0;
          if (!segmentNumbers.pixelValueFor(segFileNumber, inputLabel, segmentValue))
          {
            cerr << "ERROR: Failed to map input label " << inputLabel << " to output segment number!" << endl;
            return false;
          }

          if (use16Bit)
          {
            Uint16 currentValue = frameData16[framePixelCnt];
            if (currentValue != 0 && currentValue != segmentValue)
            {
              cerr << "ERROR: Cannot write labelmap SEG due to overlapping segments at slice " << sliceNumber << "!" << endl;
              return false;
            }
            frameData16[framePixelCnt] = segmentValue;
          }
          else
          {
            Uint8 segmentValue8 = static_cast<Uint8>(segmentValue);
            Uint8 currentValue = frameData8[framePixelCnt];
            if (currentValue != 0 && currentValue != segmentValue8)
            {
              cerr << "ERROR: Cannot write labelmap SEG due to overlapping segments at slice " << sliceNumber << "!" << endl;
              return false;
            }
            frameData8[framePixelCnt] = segmentValue8;
          }
          frameHasForeground = true;
        }
      }
      return true;
    }

    /** Patch attributes that are not covered by the DcmSegmentation API into the
     *  written dataset */
    void patchDerivedAttributes(DcmDataset& dset, DcmSegmentation& segdoc,
                                JSONSegmentationMetaInformationHandler& metaInfo,
                                DcmItem& firstSourceDataset, const bool outputLabelMap,
                                const size_t numSegmentationFiles) {
      // Set reader/session/timepoint information
      cout << "Patching in extra meta information into DICOM dataset" << endl;
      CHECK_COND(dset.putAndInsertString(DCM_SeriesDescription, metaInfo.getSeriesDescription().c_str()));
      CHECK_COND(dset.putAndInsertString(DCM_ContentCreatorName, metaInfo.getContentCreatorName().c_str()));
      CHECK_COND(dset.putAndInsertString(DCM_ClinicalTrialSeriesID, metaInfo.getClinicalTrialSeriesID().c_str()));
      CHECK_COND(dset.putAndInsertString(DCM_ClinicalTrialTimePointID, metaInfo.getClinicalTrialTimePointID().c_str()));
      if (metaInfo.getClinicalTrialCoordinatingCenterName().size())
        CHECK_COND(dset.putAndInsertString(DCM_ClinicalTrialCoordinatingCenterName, metaInfo.getClinicalTrialCoordinatingCenterName().c_str()));

      // populate BodyPartExamined
      {
        OFString bodyPartStr;
        string bodyPartAssigned = metaInfo.getBodyPartExamined();

        // inherit BodyPartExamined from the source image dataset, if available
        if(firstSourceDataset.findAndGetOFString(DCM_BodyPartExamined, bodyPartStr).good()
           && string(bodyPartStr.c_str()).size())
          bodyPartAssigned = bodyPartStr.c_str();

        if(bodyPartAssigned.size())
          CHECK_COND(dset.putAndInsertString(DCM_BodyPartExamined, bodyPartAssigned.c_str()));
      }

      // StudyDate/Time should be of the series segmented, not when segmentation was made - this is initialized by DCMTK

      // SeriesDate/Time should be of when segmentation was done; initialize to when it was saved
      {
        OFString contentDate, contentTime;
        DcmDate::getCurrentDate(contentDate);
        DcmTime::getCurrentTime(contentTime);

        CHECK_COND(dset.putAndInsertString(DCM_SeriesDate, contentDate.c_str()));
        CHECK_COND(dset.putAndInsertString(DCM_SeriesTime, contentTime.c_str()));
        segdoc.getGeneralImage().setContentDate(contentDate.c_str());
        segdoc.getGeneralImage().setContentTime(contentTime.c_str());
      }

      {
        string segmentsOverlap;
        if (outputLabelMap)
          segmentsOverlap = "NO";
        else if(numSegmentationFiles == 1)
          segmentsOverlap = "NO";
        else
          segmentsOverlap = "UNDEFINED";
        CHECK_COND(dset.putAndInsertString(DCM_SegmentsOverlap, segmentsOverlap.c_str()));
      }
    }

    /** Turns a set of ITK segmentation images into a DICOM Segmentation object.
     *
     *  Written as a class rather than a function because the individual steps
     *  of the conversion share a substantial amount of state - the document
     *  being built, the source image inventory and the mapping derived from it,
     *  the segment numbering, the functional groups reused for every frame -
     *  which as plain locals would have to be threaded through every step as a
     *  dozen or more parameters. As members, each step reduces to what it
     *  actually does.
     *
     *  Derives privately from ConverterBase only to reach its protected
     *  helpers for the equipment and content identification defaults.
     */
    template<class ImageSourceType>
    class SegmentationEncoder : private ConverterBase {

    public:

      /** The conversion flags of itkimage2dcmSegmentation(), see there for
       *  their documentation */
      struct Options {
        bool skipEmptySlices;
        bool useLabelIDAsSegmentNumber;
        bool referencesGeometryCheck;
        bool doDicomValueChecks;
        bool outputLabelMap;
      };

      /** @param dcmDatasets   Source images the segmentation is based on.
       *  @param segmentations The segmentation images to convert.
       *  @param metaInfo      Metadata describing the segments.
       *  @param options       Conversion flags.
       *  The referenced objects must outlive this encoder.
       */
      SegmentationEncoder(const vector<DcmItem*>& dcmDatasets,
                          const vector<itk::SmartPointer<const ImageSourceType> >& segmentations,
                          JSONSegmentationMetaInformationHandler& metaInfo,
                          const Options& options)
        : m_dcmDatasets(dcmDatasets),
          m_segmentations(segmentations),
          m_metaInfo(metaInfo),
          m_options(options),
          m_inputSize(segmentations[0]->GetBufferedRegion().GetSize()),
          m_frameSize(m_inputSize[0] * m_inputSize[1]),
          m_labelmapUse16Bit(false),
          // Inventory of the source images, used to map slices of the
          // segmentation to the source frames and to create the references
          // derived from that mapping
          m_sourceIndex(dcmDatasets),
          // Assigns the Segment Numbers written to DICOM and keeps the mappings
          // derived from them (label ID per segment number for the re-mapping
          // after writing, and the labelmap pixel value per input label).
          // Segment numbers always start at 1; pixel value 0 in labelmap output
          // is reserved for background per Sup 243, and a background segment
          // with number 0 (designated via Pixel Padding Value) is added later
          // if needed.
          m_segmentNumbers(options.useLabelIDAsSegmentNumber, options.outputLabelMap, segmentations.size()),
          m_hasDerivationImagesAny(false),
          m_framesAdded(0)
      {
      }

      /** Run the conversion.
       *  @return The resulting dataset, which the caller owns, or NULL on failure.
       */
      DcmDataset* encode() {
        if(m_metaInfo.segmentsAttributesMappingList.size() != m_segmentations.size()){
          cerr << "Mismatch between the number of input segmentation files and the size of metainfo list!" << endl;
          return NULL;
        }

        if(!createDocument())
          return NULL;
        addSharedFunctionalGroups();
        mapSourceFrames();

        if(!encodeSegments())
          return NULL;
        if(m_options.outputLabelMap)
        {
          if(!encodeLabelmapFrames())
            return NULL;
          // designate pixel value 0 as background (Background segment plus
          // Pixel Padding Value) if it occurs in any frame
          CHECK_COND(addBackgroundSegmentIfNeeded(m_segdoc.get()));
        }

        if(m_framesAdded == 0){
          cerr << "FATAL ERROR: No input labels found - input segmentation is empty!" << endl;
          cerr << "If you would like to encode background label, please see https://github.com/QIICR/dcmqi/issues/490" << endl;
          return NULL;
        }

        return writeDocument();
      }

      /** Mapping from assigned Segment Number to the label ID it came from,
       *  for the caller-side re-mapping of binary segmentations */
      const map<Uint16, Uint16>& segmentToLabel() const { return m_segmentNumbers.segmentToLabel(); }

    private:

      /** Create the segmentation document and initialize what it inherits from
       *  the source images and the metadata */
      bool createDocument() {
        // Get equipment and content identification information from the metadata handler.
        // Contains constant dcmqi defaults for equipment and content identification.
        // These are used to create the DICOM Segmentation object.
        IODGeneralEquipmentModule::EquipmentInfo eq = getEquipmentInfo();
        ContentIdentificationMacro ident = createContentIdentificationInformation(m_metaInfo);
        CHECK_COND(ident.setInstanceNumber(m_metaInfo.getInstanceNumber().c_str()));

        if(m_options.outputLabelMap && !m_segmentNumbers.determineLabelmapBitDepth(m_metaInfo, m_labelmapUse16Bit))
          return false;
        m_segdoc.reset(createSegmentationDocument(m_inputSize[1], m_inputSize[0], m_options.outputLabelMap,
                                                  m_labelmapUse16Bit, eq, ident));
        if(!m_segdoc)
          return false;

        // import Patient, Study and Frame of Reference; do not import Series
        // attributes
        CHECK_COND(m_segdoc->importHierarchy(*m_dcmDatasets[0], OFTrue, OFTrue, OFTrue, OFFalse));

        initializeDimensions(*m_segdoc, m_options.outputLabelMap);
        return true;
      }

      /** Initialize the functional groups that apply to every frame */
      void addSharedFunctionalGroups() {
        addGeometrySharedFGs(*m_segdoc, *m_segmentations[0]);

        // Shared FGs: DerivationImageSequence
        // When geometry checks are disabled, all source images are referenced as
        // whole instances by all frames.
        if(!m_options.referencesGeometryCheck && m_dcmDatasets.size() > 1){
          FGDerivationImage fgderShared;
          CHECK_COND(m_sourceIndex.addWholeInstanceDerivationImageItem(fgderShared,
              segmentationDerivationCode(), "", segmentationDerivationCode()));
          CHECK_COND(m_segdoc->addForAllFrames(fgderShared));
        }
      }

      /** Map the slices of each segmentation file to the source frames located on them */
      void mapSourceFrames() {
        if(!m_options.referencesGeometryCheck)
          return;
        m_slice2framesPerFile.resize(m_segmentations.size());
        for (size_t segFileNumber = 0; segFileNumber < m_segmentations.size(); segFileNumber++)
        {
          m_slice2framesPerFile[segFileNumber] = m_sourceIndex.mapSlicesToFrames(*m_segmentations[segFileNumber]);
          for (vector<vector<size_t> >::const_iterator vI = m_slice2framesPerFile[segFileNumber].begin();
               vI != m_slice2framesPerFile[segFileNumber].end(); ++vI)
            if ((*vI).size() > 0)
              m_hasDerivationImagesAny = true;
        }
      }

      /** Iterate over the files and the labels available in each file and create
       *  a segment for every label. For binary output the frames of a segment
       *  are written right away; labelmap output composes its frames from all
       *  segments afterwards, see encodeLabelmapFrames().
       */
      bool encodeSegments() {
        for(size_t segFileNumber=0; segFileNumber<m_segmentations.size(); segFileNumber++){

          // note that labels are the same in the input and output image produced
          // by this filter, see https://itk.org/Doxygen/html/classitk_1_1LabelImageToLabelMapFilter.html
          using LabelToLabelMapFilterType2 = itk::LabelImageToLabelMapFilter<ImageSourceType>;
          typename LabelToLabelMapFilterType2::Pointer l2lm = LabelToLabelMapFilterType2::New();
          l2lm->SetInput(m_segmentations[segFileNumber]);
          l2lm->Update();

          typedef typename LabelToLabelMapFilterType2::OutputImageType::LabelObjectType LabelType;
          typedef itk::LabelStatisticsImageFilter<ImageSourceType,ImageSourceType> LabelStatisticsType;

          typename LabelStatisticsType::Pointer labelStats = LabelStatisticsType::New();

          cout << "Found " << l2lm->GetOutput()->GetNumberOfLabelObjects() << " label(s)" << endl;
          labelStats->SetInput(m_segmentations[segFileNumber]);
          labelStats->SetLabelInput(m_segmentations[segFileNumber]);
          labelStats->Update();

          for(unsigned segLabelNumber=0 ; segLabelNumber<l2lm->GetOutput()->GetNumberOfLabelObjects();segLabelNumber++){
            LabelType* labelObject = l2lm->GetOutput()->GetNthLabelObject(segLabelNumber);
            short label = labelObject->GetLabel();

            if(!label){
              continue;
            }

            cout << "Processing label " << label << endl;

            auto bbox = labelStats->GetBoundingBox(label);
            unsigned firstSlice, lastSlice;
            if(m_options.skipEmptySlices){
              firstSlice = bbox[4];
              lastSlice = bbox[5]+1;
            } else {
              firstSlice = 0;
              lastSlice = m_inputSize[2];
            }

            cout << "Total non-empty slices that will be encoded in SEG for label " <<
            label << " is " << lastSlice-firstSlice+1 << endl <<
            " (inclusive from " << firstSlice << " to " <<
            lastSlice << ")" << endl;

            if(m_metaInfo.segmentsAttributesMappingList[segFileNumber].find(label) == m_metaInfo.segmentsAttributesMappingList[segFileNumber].end()){
              cerr << "ERROR: Failed to match label from image to the segment metadata!" << endl;
              return false;
            }

            DcmSegment* segment = createSegmentFromAttributes(*m_metaInfo.segmentsAttributesMappingList[segFileNumber][label]);
            if(!segment)
              return false;

            Uint16 segmentNumber = 0;
            if (!m_segmentNumbers.proposeSegmentNumber(label, segmentNumber))
            {
              delete segment;
              return false;
            }
            CHECK_COND(m_segdoc->addSegment(segment, segmentNumber /* returns logical segment number */));
            m_segmentNumbers.recordSegment(segFileNumber, label, segmentNumber);

            if (!m_options.outputLabelMap)
              addBinaryFrames(segFileNumber, label, segmentNumber, firstSlice, lastSlice);
          }
        }
        return true;
      }

      /** Write the frames of one segment of a binary segmentation, one per slice
       *  of the given range */
      void addBinaryFrames(const size_t segFileNumber, const short label, const Uint16 segmentNumber,
                           const unsigned firstSlice, const unsigned lastSlice) {
        for(unsigned sliceNumber=firstSlice;sliceNumber<lastSlice;sliceNumber++){

          m_perFrameFGs.beginFrame();

          // PerFrame FG: FrameContentSequence
          CHECK_COND(m_perFrameFGs.frameContent().setDimensionIndexValues(segmentNumber, 0));
          CHECK_COND(m_perFrameFGs.frameContent().setDimensionIndexValues(sliceNumber-firstSlice+1, 1));

          // PerFrame FG: PlanePositionSequence
          setPlanePositionToSlice(m_perFrameFGs.planePosition(), *m_segmentations[segFileNumber], sliceNumber);

          // PerFrame FG: DerivationImageSequence, references the source frames
          // this slice was derived from
          if(m_options.referencesGeometryCheck && !m_slice2framesPerFile[segFileNumber][sliceNumber].empty()){
            CHECK_COND(m_sourceIndex.addDerivationImageItem(m_perFrameFGs.derivationImage(),
                m_slice2framesPerFile[segFileNumber][sliceNumber],
                segmentationDerivationCode(), "", sourceImagePurposeOfReferenceCode()));
          }

          /* Add frame that references this segment */
          vector<Uint8> frameData = extractBinaryFrame(*m_segmentations[segFileNumber], label, sliceNumber, m_frameSize);
          OFCondition frameAdded = m_segdoc->addFrame(frameData.data(), segmentNumber, m_perFrameFGs.get());
          if(frameAdded.good()){
            m_framesAdded++;
          }
          m_perFrameFGs.endFrame();
        }
      }

      /** Compose and write the frames of a labelmap segmentation, one per slice,
       *  merging the segments of all input files into a single frame */
      bool encodeLabelmapFrames() {
        unsigned outputFrameNumber = 1;

        // Stack ID is invariant across all labelmap frames; set it once on the
        // reused FGFrameContent instance.
        CHECK_COND(m_perFrameFGs.frameContent().setStackID("Frame Position"));

        for (unsigned sliceNumber = 0; sliceNumber < m_inputSize[2]; sliceNumber++)
        {
          std::vector<Uint8> frameData8;
          std::vector<Uint16> frameData16;
          bool frameHasForeground = false;
          if (!composeLabelmapFrame(m_segmentations, m_segmentNumbers, sliceNumber, m_frameSize,
                                    m_labelmapUse16Bit, frameData8, frameData16, frameHasForeground))
            return false;

          if (m_options.skipEmptySlices && !frameHasForeground)
            continue;

          m_perFrameFGs.beginFrame();

          CHECK_COND(m_perFrameFGs.frameContent().setInStackPositionNumber(outputFrameNumber));
          CHECK_COND(m_perFrameFGs.frameContent().setDimensionIndexValues(1, 0));
          CHECK_COND(m_perFrameFGs.frameContent().setDimensionIndexValues(outputFrameNumber, 1));

          setPlanePositionToSlice(m_perFrameFGs.planePosition(), *m_segmentations[0], sliceNumber);

          addLabelmapFrameReferences(sliceNumber);

          OFCondition frameAdded = m_labelmapUse16Bit
              ? m_segdoc->addFrame(frameData16.data(), 0, m_perFrameFGs.get())
              : m_segdoc->addFrame(frameData8.data(), 0, m_perFrameFGs.get());
          if (frameAdded.good())
          {
            m_framesAdded++;
            outputFrameNumber++;
          }
          m_perFrameFGs.endFrame();
        }
        return true;
      }

      /** PerFrame FG: DerivationImageSequence of a labelmap frame, referencing
       *  the source frames of this slice across all segmentation files */
      void addLabelmapFrameReferences(const unsigned sliceNumber) {
        if (!m_options.referencesGeometryCheck || !m_hasDerivationImagesAny || m_slice2framesPerFile.empty())
          return;

        std::set<size_t> referencedFrameIds;
        for (size_t segFileNumber = 0; segFileNumber < m_slice2framesPerFile.size(); segFileNumber++)
        {
          if (sliceNumber >= m_slice2framesPerFile[segFileNumber].size())
            continue;
          referencedFrameIds.insert(m_slice2framesPerFile[segFileNumber][sliceNumber].begin(),
                                    m_slice2framesPerFile[segFileNumber][sliceNumber].end());
        }
        if (referencedFrameIds.empty())
          return;

        vector<size_t> frameIds(referencedFrameIds.begin(), referencedFrameIds.end());
        CHECK_COND(m_sourceIndex.addDerivationImageItem(m_perFrameFGs.derivationImage(), frameIds,
            segmentationDerivationCode(), "", sourceImagePurposeOfReferenceCode()));
      }

      /** Complete the document and write it out, including the attributes that
       *  have to be patched into the written dataset */
      DcmDataset* writeDocument() {
        // populate the Common Instance Reference module with the references
        // accumulated while creating the derivation image items; the study of
        // this object decides which of its two sequences a reference goes into
        OFString objectStudyInstanceUID;
        m_segdoc->getStudy().getStudyInstanceUID(objectStudyInstanceUID);
        CHECK_COND(m_sourceIndex.populateCommonInstanceReference(m_segdoc->getCommonInstanceReference(),
                                                                 objectStudyInstanceUID));

        m_segdoc->getSeries().setSeriesNumber(m_metaInfo.getSeriesNumber().c_str());

        OFString frameOfRefUID;
        if(!m_segdoc->getFrameOfReference().getFrameOfReferenceUID(frameOfRefUID).good()){
          // TODO: add FoR UID to the metadata JSON and check that before generating one!
          char frameOfRefUIDchar[128];
          dcmGenerateUniqueIdentifier(frameOfRefUIDchar, QIICR_UID_ROOT);
          CHECK_COND(m_segdoc->getFrameOfReference().setFrameOfReferenceUID(frameOfRefUIDchar));
        }

        std::cout << "Writing DICOM segmentation dataset" << std::endl;
        // Don't check functional groups since its very time consuming and we trust
        // ourselves to put together valid datasets
        m_segdoc->setCheckFGOnWrite(OFFalse);

        // Ensure dataset memory is cleaned up on exit, and avoid the copy on
        // return that was performed before
        std::unique_ptr<DcmDataset> segdocDataset(new DcmDataset());
        std::cout << "Checking DICOM attribute values before writing: "
                  << (m_options.doDicomValueChecks ? "enabled" : "disabled") << std::endl;
        m_segdoc->setValueCheckOnWrite(m_options.doDicomValueChecks);
        OFCondition writeResult = m_segdoc->writeDataset(*segdocDataset);
        if(writeResult.bad()){
          cerr << "FATAL ERROR: Writing of the SEG dataset failed!";
          if (writeResult.text()){
            cerr << " Error: " << writeResult.text() << ".";
          }
          cerr << " Please report the problem to the developers, ideally accompanied by a de-identified dataset allowing to reproduce the problem!" << endl;
          return NULL;
        }

        patchDerivedAttributes(*segdocDataset, *m_segdoc, m_metaInfo, *m_dcmDatasets[0],
                               m_options.outputLabelMap, m_segmentations.size());
        return segdocDataset.release();
      }

      // inputs
      const vector<DcmItem*>& m_dcmDatasets;
      const vector<itk::SmartPointer<const ImageSourceType> >& m_segmentations;
      JSONSegmentationMetaInformationHandler& m_metaInfo;
      const Options m_options;
      const itk::ImageBase<3>::SizeType m_inputSize;
      const unsigned m_frameSize;

      // conversion state
      std::unique_ptr<DcmSegmentation> m_segdoc;
      bool m_labelmapUse16Bit;
      SourceImageIndex m_sourceIndex;
      SegmentNumberAssigner m_segmentNumbers;
      /// per input file and slice, the source frames located on that slice
      vector<vector<vector<size_t> > > m_slice2framesPerFile;
      bool m_hasDerivationImagesAny;
      PerFrameFunctionalGroups m_perFrameFGs;
      unsigned m_framesAdded;
    };

  } // anonymous namespace


  Itk2DicomConverter::Itk2DicomConverter()
  {
  };

  // -------------------------------------------------------------------------------------

  template<class ImageSourceType, std::enable_if_t<std::is_same_v<short, typename ImageSourceType::PixelType>, bool>>
  DcmDataset* Itk2DicomConverter::itkimage2dcmSegmentation(vector<DcmItem*> dcmDatasets,
                                                          vector<itk::SmartPointer<const ImageSourceType>> segmentations,
                                                          const string &metaData,
                                                          bool skipEmptySlices,
                                                          bool useLabelIDAsSegmentNumber,
                                                          bool referencesGeometryCheck,
                                                          bool doDicomValueChecks,
                                                          bool outputLabelMap) {
    // Thin wrapper around the handler-based overload: parse the JSON string into a
    // handler and delegate. Keeps the legacy file-driven call sites working while
    // the handler overload is the load-bearing implementation.
    JSONSegmentationMetaInformationHandler metaInfo(metaData);
    metaInfo.read();
    return itkimage2dcmSegmentation(dcmDatasets, segmentations, metaInfo,
                                    skipEmptySlices, useLabelIDAsSegmentNumber,
                                    referencesGeometryCheck, doDicomValueChecks,
                                    outputLabelMap);
  }

  // -------------------------------------------------------------------------------------

  template<class ImageSourceType, std::enable_if_t<std::is_same_v<short, typename ImageSourceType::PixelType>, bool>>
  DcmDataset* Itk2DicomConverter::itkimage2dcmSegmentation(vector<DcmItem*> dcmDatasets,
                                                          vector<itk::SmartPointer<const ImageSourceType>> segmentations,
                                                          JSONSegmentationMetaInformationHandler& metaInfo,
                                                          bool skipEmptySlices,
                                                          bool useLabelIDAsSegmentNumber,
                                                          bool referencesGeometryCheck,
                                                          bool doDicomValueChecks,
                                                          bool outputLabelMap) {

    const typename SegmentationEncoder<ImageSourceType>::Options options =
        { skipEmptySlices, useLabelIDAsSegmentNumber, referencesGeometryCheck, doDicomValueChecks, outputLabelMap };
    SegmentationEncoder<ImageSourceType> encoder(dcmDatasets, segmentations, metaInfo, options);

    std::unique_ptr<DcmDataset> segdocDataset(encoder.encode());
    if(!segdocDataset)
      return NULL;

    if (useLabelIDAsSegmentNumber && !outputLabelMap)
    {
      // Binary segmentations require Segment Numbers to start at 1 and increase
      // monotonically by 1, so re-mapping to label IDs only works for label IDs
      // that satisfy the same constraint.
      if (!checkLabelNumbering(encoder.segmentToLabel()))
      {
        return NULL;
      }
      mapLabelIDsToSegmentNumbers(segdocDataset.get(), encoder.segmentToLabel());
    }
    // For labelmap output the label IDs were used as segment numbers directly when
    // the segments were added. LABELMAP only requires segment numbers to be unique
    // (enforced at insertion), not consecutive, so gaps in the label IDs are
    // allowed (https://github.com/QIICR/dcmqi/issues/537).

    return segdocDataset.release();
  }

  bool Itk2DicomConverter::mapLabelIDsToSegmentNumbers(DcmDataset* dset, map<Uint16,Uint16> segNum2Label)
  {
    cout << "Mapping Label IDs to Segment Numbers" << endl;
    DcmSequenceOfItems* seq = NULL;
    CHECK_COND(dset->findAndGetSequence(DCM_PerFrameFunctionalGroupsSequence, seq));
    if(!seq){
      cerr << "ERROR: Mapping Label IDs to Segment Numbers: Per-frame functional groups sequence not found!" << endl;
      return false;
    }

    // Check whether the provided label numbers (values in segNum2Label) are unique
    // and monotonically increasing from 1.
    if (!checkLabelNumbering(segNum2Label))
    {
      return false;
    }

    // 0. Get Segment Sequence and remember number of segments
    // 1. Loop over all items in per-frame FG sequence
    // 2. Get segmentation FG for that frame
    // 3. Get the referenced segment number from it
    // 4. Replace the referenced segment number with the original label value
    // 5. From the Segment Sequence, find the item with the matching segment number
    // 6. Replace the segment number in the item of the Segment Sequence to make it match the replacement (label) value
    // 7. Sort items in Segment Sequence by newly assigned Segment Numbers

    // 0. Get Segment Sequence
    DcmSequenceOfItems* segmentSequence = NULL;
    CHECK_COND(dset->findAndGetSequence(DCM_SegmentSequence, segmentSequence));
    if(!segmentSequence){
      cerr << "ERROR: Mapping Label IDs to Segment Numbers: Segment sequence not found!" << endl;
      return false;
    }
    // This tells us how many segments we have, and therefore also how many segment numbers we must expect
    Uint16 numSegments = segmentSequence->card();
    // Vector that holds all input segments (from Segment Sequence) that have been handled. This is
    // used to check whether the Segmentation Sequence has already been updated with the new segment number.
    // Since multiple frames can reference the same segment and we are looping over frames,
    // we only want to update the segment sequence once per segment to save time.
    // The index is the sequence number, the value is the item from the Segment Sequence.
    vector<DcmItem*> segmentHandled(numSegments, NULL);

    // 1.Get per-frame FG sequence and loop over all items (frames)
    DcmSequenceOfItems* fgSeq = NULL;
    CHECK_COND(dset->findAndGetSequence(DCM_PerFrameFunctionalGroupsSequence, fgSeq));
    for(unsigned i=0;i<fgSeq->card();i++){
      DcmItem* frameItem = seq->getItem(i);
      if(!frameItem){
        cerr << "ERROR: Mapping Label IDs to Segment Numbers: Failed to get item " << i << " from the per-frame FG sequence!" << endl;
        return false;
      }

      // 2. Get segmentation FG for that frame
      DcmItem* segItem = NULL;
      CHECK_COND(frameItem->findAndGetSequenceItem(DCM_SegmentIdentificationSequence, segItem, 0));

      // 3. Get the referenced segment number from it
      Uint16 oldSegNum;
      CHECK_COND(segItem->findAndGetUint16(DCM_ReferencedSegmentNumber, oldSegNum));

      // 4. Replace the referenced segment number with the original label value
      map<Uint16,Uint16>::iterator segNum2LabelIt = segNum2Label.find(oldSegNum);
      if(segNum2LabelIt == segNum2Label.end()){
        cerr << "ERROR: Mapping Label IDs to Segment Numbers: Failed to find segment number " << oldSegNum << " in the mapping!" << endl;
        return false;
      }
      Uint16 newSegNum = segNum2LabelIt->second;
      CHECK_COND(segItem->putAndInsertUint16(DCM_ReferencedSegmentNumber, newSegNum));
      // Replace the referenced segment number with the original label value if not already done.
      // Range check is not necessary, because we already checked sanity of segmentSequence.
      if (!segmentHandled[newSegNum-1])
      {
        // 5. From the Segment Sequence, find the item with the matching old segment number
        DcmItem* segmentItem = NULL;
        for(unsigned j=0;j<segmentSequence->card();j++){
          segmentItem = segmentSequence->getItem(j);
          if(!segmentItem){
            cerr << "ERROR: Mapping Label IDs to Segment Numbers: Failed to get item " << j << " from the segment sequence!" << endl;
            return false;
          }
          Uint16 segmentNumber;
          CHECK_COND(segmentItem->findAndGetUint16(DCM_SegmentNumber, segmentNumber));
          // 6. Replace the segment number in the item of the Segment Sequence to make it match the replacement (label) value
          if(segmentNumber == oldSegNum)
          {
            segmentHandled[newSegNum-1] = new DcmItem(*segmentItem);
            CHECK_COND(segmentHandled[newSegNum-1]->putAndInsertUint16(DCM_SegmentNumber,newSegNum));
            break;
          }
        }
        if(!segmentItem){
          cerr << "ERROR: Mapping Label IDs to Segment Numbers: Failed to find segment number " << oldSegNum << " in the segment sequence!" << endl;
          return false;
        }
      }
    }
    // 7. Sort items in Segment Sequence by newly assigned Segment Numbers
    std::unique_ptr<DcmSequenceOfItems> newSegSeq(new DcmSequenceOfItems(DCM_SegmentSequence));
    for (size_t z=0; z<segmentHandled.size(); z++)
    {
      if (!segmentHandled[z])
      {
        cerr << "ERROR: Mapping Label IDs to Segment Numbers: Segment number " << z+1 << " is missing!" << endl;
        return false;
      }
      newSegSeq->insert(segmentHandled[z]);
    }
    dset->insert(newSegSeq.release(), OFTrue /* replace old */);
    return true;
  }

  bool Itk2DicomConverter::checkLabelNumbering(const map<Uint16, Uint16>& segNum2Label)
  {
    // Check whether the provided label numbers (values in segNum2Label) are unique
    // and monotonically increasing from 1.
    // If not, we cannot use this method to sort the segments.
    std::vector<Uint16> labels;
    for (auto it = segNum2Label.begin(); it != segNum2Label.end(); ++it)
    {
      labels.push_back(it->second);
    }
    std::sort(labels.begin(), labels.end());
    if (labels[0] != 1)
    {
      cerr << "ERROR: Cannot sort segments by label, because the label numbers are not monotonically increasing from 1!" << endl;
      return false;
    }
    for (size_t i=0; i<labels.size(); i++)
    {
      if (labels[i] != i+1)
      {
        cerr << "ERROR: Cannot sort segments by label, because the label numbers are not monotonically increasing from 1!" << endl;
        return false;
      }
    }
    return true;
  }

  template DcmDataset* Itk2DicomConverter::itkimage2dcmSegmentation<ShortImageType>(
      vector<DcmItem*> dcmDatasets,
      vector<ShortImageType::ConstPointer> segmentations,
      const string& metaData,
      bool skipEmptySlices,
      bool useLabelIDAsSegmentNumber,
      bool referencesGeometryCheck,
      bool doDicomValueChecks,
      bool outputLabelMap);

  template DcmDataset* Itk2DicomConverter::itkimage2dcmSegmentation<ShortImageType>(
      vector<DcmItem*> dcmDatasets,
      vector<ShortImageType::ConstPointer> segmentations,
      JSONSegmentationMetaInformationHandler& metaInfo,
      bool skipEmptySlices,
      bool useLabelIDAsSegmentNumber,
      bool referencesGeometryCheck,
      bool doDicomValueChecks,
      bool outputLabelMap);

  using VectorImageAdapter = itk::VectorImageToImageAdaptor<short, 3U>;
  template DcmDataset* Itk2DicomConverter::itkimage2dcmSegmentation<VectorImageAdapter>(
      vector<DcmItem*> dcmDatasets,
      vector<VectorImageAdapter::ConstPointer> segmentations,
      const string& metaData,
      bool skipEmptySlices,
      bool useLabelIDAsSegmentNumber,
      bool referencesGeometryCheck,
      bool doDicomValueChecks,
      bool outputLabelMap);

  template DcmDataset* Itk2DicomConverter::itkimage2dcmSegmentation<VectorImageAdapter>(
      vector<DcmItem*> dcmDatasets,
      vector<VectorImageAdapter::ConstPointer> segmentations,
      JSONSegmentationMetaInformationHandler& metaInfo,
      bool skipEmptySlices,
      bool useLabelIDAsSegmentNumber,
      bool referencesGeometryCheck,
      bool doDicomValueChecks,
      bool outputLabelMap);
}
