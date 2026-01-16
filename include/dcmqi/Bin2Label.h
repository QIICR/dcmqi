/*
 *
 *  Copyright (C) 2015-2025, Open Connections GmbH
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation are maintained by
 *
 *    OFFIS e.V.
 *    R&D Division Health
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *
 *  Module:  dcmseg
 *
 *  Author:  Michael Onken
 *
 *  Purpose: Class for converting binary to label map segmentations
 *
 */

#ifndef BIN2LABEL
#define BIN2LABEL

#include "dcmtk/config/osconfig.h" // include OS configuration first

#include "dcmtk/dcmseg/overlaputil.h"
#include "dcmtk/dcmseg/segdoc.h" // for DcmSegmentation
#include "dcmtk/dcmfg/fgfracon.h"
#include "dcmtk/dcmseg/segtypes.h"
#include "dcmtk/dcmseg/segutils.h"

#include "dcmqi/ConverterBase.h"

/** Converter class for transforming binary DICOM segmentation (Segmentation SOP Class)
 *  objects into labelmap segmentations.
 */

namespace dcmqi
{
class DCMTK_DCMSEG_EXPORT DcmBinToLabelConverter
{

public:
    struct LoadingFlags;
    struct ConversionFlags;

    // -------------------- construction / destruction -------------------------------

    // Default constructor
    DcmBinToLabelConverter();

    /** Destructor, frees memory
     */
    virtual ~DcmBinToLabelConverter();

    // -------------------- input handling -------------------------------

    void setInput(DcmSegmentation* m_inputSeg);

    void setInput(DcmDataset* m_inputSeg, const DcmSegmentation::LoadingFlags& loadFlags = DcmSegmentation::LoadingFlags());

    void setInput(OFFilename filename, const DcmSegmentation::LoadingFlags& loadFlags = DcmSegmentation::LoadingFlags());

    // -------------------- processing  ----------------------------------

    E_TransferSyntax getInputTransferSyntax() const;

    /** Converts the currently set input segmentation from binary representation
     *  to a label map segmentation.
     *  The input segmentation must have been provided beforehand using one of the
     *  setInput(...) methods. After successful conversion, the result can be
     *  retrieved via getOutputSegmentation(...) or getOutputDataset(...).
     *  @param  convFlags Flags to configure the conversion process. If omitted,
     *          default conversion settings are used.
     *  @return EC_Normal if conversion was successful, an error code otherwise.
     */
    OFCondition convert(const ConversionFlags& convFlags = ConversionFlags());

    /// Flags for converting binary segmentation objects to label map segmentations
    struct ConversionFlags
    {
        /// Return error if object is already a label map object
        OFBool m_errorIfAlreadyLabelMap;

        /// Number of threads maximally used (for reading/writing segmentation objects)
        Uint32 m_numThreads;

        /// Enables/disables checking of functional groups on dataset export (default: on)
        OFBool m_checkExportFG;

        /// Enables/disables checking of values on dataset export (default: on)
        OFBool m_checkExportValues;

        /// Output color model for label map segmentations (default: MONOCHROME2)
        DcmSegTypes::E_SegmentationLabelmapColorModel m_outputColorModel;

        /// Force conversion to PALETTE color model even if not all segments contain CIELab colors
        /// (default: OFFalse)
        OFBool m_forcePalette;

        /// Constructor to initialize the flags
        ConversionFlags()
        {
            clear();
        }

        /** Clear all flags to their default values */
        void clear()
        {
            m_errorIfAlreadyLabelMap = OFFalse;
            m_numThreads = 1;
            m_checkExportFG = OFTrue;
            m_checkExportValues = OFTrue;
            m_outputColorModel = DcmSegTypes::SLCM_MONOCHROME2;
            m_forcePalette = OFFalse;

        }
    };

    // -------------------- output handling ----------------------------------

    /** Provides output as label map segmentation object. This is already computed
     *  if convert(...) has been called.
     *  @param  outputSeg Pointer to the output segmentation or NULL if not available
     *  @return EC_Normal if successful, error otherwise
     */
    OFCondition getOutputSegmentation(OFshared_ptr<DcmSegmentation>& outputSeg);

    /** Returns a pointer to the output dataset. This is an expensive operation
     *  since it requires producing the dataset from the internal output segmentation.
     *  The caller must provide an existing DcmItem. It is cleared before use.
     *  @param  outputDataset Output dataset.
     *  @return EC_Normal if successful, error otherwise
     */
    OFCondition getOutputDataset(DcmItem& outputDataset);

    /** Clear all internal data to get ready for processing another segmentation
     *  object.
     */
    void clear();

protected:

    template <typename T>
    static OFCondition copyComponent(T* src, T* dest);
    static OFCondition copyCommonModules(DcmSegmentation* src, DcmSegmentation* dest);
    OFCondition createFramesWithMetadata(DcmSegmentation* src);
    OFCondition copySegments(DcmSegmentation* src, DcmSegmentation* dest);
    OFBool checkCIELabColorsPresent();
    OFCondition createPaletteColorLUT();
    OFCondition loadInput();
    OFCondition addSourceSegmentationToDerivationImageFG(DcmSegmentation* src, DcmSegmentation* dest);

    /** Set pixel data for a specific frame in the output segmentation.
     *  @param  src        Source segmentation object
     *  @param  logicalPos Logical position of the frame (0 is first)
     *  @param  destFrame  Pointer to the destination frame
     *  @param  numPixels  Number of pixels in the frame
     *  @return EC_Normal if successful, error otherwise
     */
    template<typename PixelType>
    inline OFCondition setPixelDataForFrame(DcmSegmentation* src, Uint32 logicalPos, PixelType* destFrame, const size_t numPixels)
    {
        // For the current logical frame position we have one or more segments in the source data.
        // Get those segments, get the related original source frame they refer to, and
        // then loop over the pixels of the source frame. If the source frame pixel is > 0,
        // then set at the same coordinates in the destination frame the pixel to the
        // segment number.
        OverlapUtil::SegmentsByPosition segs;
        OFCondition result = m_overlapUtil.getSegmentsByPosition(segs);
        if (result.good())
        {
            // Get the segments for the current frame
            OFVector<OverlapUtil::SegNumAndFrameNum>::const_iterator seg = segs[logicalPos].begin();
            OFVector<OverlapUtil::SegNumAndFrameNum>::const_iterator endSeg = segs[logicalPos].end();
            while (result.good() && (seg != endSeg))
            {
                const DcmIODTypes::FrameBase* srcFrame = src->getFrame( (*seg).m_frameNumber);
                if (srcFrame)
                {
                    // unpack binary segmentation frame
                    DcmIODTypes::Frame<Uint8>* unpackedFrame = DcmSegUtils::unpackBinaryFrame(OFstatic_cast(const DcmIODTypes::Frame<Uint8>*, srcFrame), src->getRows(), src->getColumns());
                    if (unpackedFrame)
                    {
                        for (size_t p=0; (p < numPixels) && result.good(); p++)
                        {
                            Uint8 val;
                            result = unpackedFrame->getUint8AtIndex(val, p); // either 0 or 1
                            if (result.good())
                            {
                                // cast is safe in case of 8 bit since this is checked beforehand
                                if (val > 0) destFrame[p] = OFstatic_cast(PixelType, seg->m_segmentNumber);
                            }
                            else {
                                DCMSEG_ERROR("Cannot get pixel from source frame: " << result.text());
                            }
                        }
                    }
                    else
                    {
                        DCMSEG_ERROR("Cannot unpack binary source frame");
                        result = IOD_EC_InvalidPixelData;
                    }
                    delete unpackedFrame;
                    unpackedFrame = NULL;
                }
                else {
                    DCMSEG_ERROR("Cannot get pixel data for source frame");
                    result = IOD_EC_InvalidPixelData;
                }
                seg++;
            }
        }
        return result;
    }

    OFCondition createFrameContentFG(Uint32 outputFrameNum, OFVector<OverlapUtil::LogicalFrame>::iterator logicalFrame, FGFrameContent*& frameContent);

private:

    // Disable copy constructor and assignment operator
    DcmBinToLabelConverter(const DcmBinToLabelConverter&);
    DcmBinToLabelConverter& operator=(const DcmBinToLabelConverter&);

    OFCondition checkSOPClassAndSegtype(const OFString& sopClassUID,
                                        const OFString& segType);

    DcmSegmentation::LoadingFlags m_loadFlags;
    ConversionFlags m_convFlags;

    // Input sources
    DcmDataset* m_inputDataset;
    OFBool m_inputDatasetOwned;
    OFFilename m_inputFileName;
    DcmSegmentation* m_inputSeg;
    E_TransferSyntax m_inputXfer;
    // Remember output segmentation in converter but also return in parameter,
    // so the caller is responsible for destroying it
    OFshared_ptr<DcmSegmentation> m_outputSeg;
    OFBool m_use16Bit;
    OverlapUtil m_overlapUtil;
    struct CIELabColor
    {
        Uint16* m_L;
        Uint16* m_a;
        Uint16* m_b;
        size_t m_numSegments;

        CIELabColor() : m_L(OFnullptr), m_a(OFnullptr), m_b(OFnullptr) {}
        OFBool resize(size_t numSegments)
        {
            clear();
            m_L = new Uint16[numSegments];
            m_a = new Uint16[numSegments];
            m_b = new Uint16[numSegments];
            if ((numSegments > 0) && (!m_L || !m_a || !m_b))
            {
                DCMSEG_ERROR("Cannot resize CIELab color arrays");
                clear();
                return OFFalse;
            }
            m_numSegments = numSegments;
            return OFTrue;
        }

        ~CIELabColor()
        {
            clear();
        }

        void clear()
        {
            delete[] m_L; m_L = OFnullptr;
            delete[] m_a; m_a = OFnullptr;
            delete[] m_b; m_b = OFnullptr;
            m_numSegments = 0;
        }
    };
    // Remember CIELab colors for segments, as optionally provided in
    // Recommended Display CIELab Value Macro for each segment.
    // Each contained array has a size of numSegments. Segment number N
    // has its color at index N-1.
    CIELabColor m_cielabColors;
};

} // namespace dcmqi
#endif // BIN2LABEL
