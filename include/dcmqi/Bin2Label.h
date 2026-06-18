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
class DcmBinToLabelConverter
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

        /// Force 16-bit pixel data even if the number of segments would fit in 8 bits
        /// (default: OFFalse)
        OFBool m_force16Bit;

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
            m_force16Bit = OFFalse;
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

    /** Collect Recommended Display CIELab Value of each input segment into
     *  m_cielabColors so it can later be used to build the Palette Color LUT.
     *  Only relevant when the conversion target color model is PALETTE; for
     *  MONOCHROME2 output the call is a no-op and returns OFTrue.
     *
     *  In PALETTE mode every input segment must either provide a Recommended
     *  Display CIELab Value Macro or m_convFlags.m_forcePalette must be set so
     *  that random colors are generated for missing segments. Otherwise this
     *  method clears m_cielabColors and returns OFFalse.
     *
     *  @return OFTrue on success or no-op (non-PALETTE mode), OFFalse if a
     *          required color is missing and m_forcePalette is not set, or on
     *          memory allocation failure.
     */
    OFBool ensureCIELabColorsPresent();
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

    /** Check whether any frame in the output segmentation contains pixel value 0.
     *  If so, designate pixel value 0 as the labelmap background: a background
     *  segment with Segment Number 0 using Property Type Code
     *  (DCM, 125040, "Background") is inserted, and Pixel Padding Value
     *  (0028,0120) is written accordingly, which per DICOM standard marks
     *  that segment as background.
     *
     *  Implementation delegates the frame scan and background designation to
     *  ConverterBase::addBackgroundSegmentIfNeeded(); this wrapper additionally
     *  prepends the matching black entry to m_cielabColors when the output
     *  color model is PALETTE so that pixel value 0 maps to black in the
     *  Palette Color LUT (the per-segment CIELab macro is not written in
     *  PALETTE mode, per Sup 243).
     *
     *  @return EC_Normal if successful or no background designation needed, error otherwise
     */
    OFCondition addBackgroundSegmentIfNeeded();

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

        /** Prepend a color entry at index 0, shifting existing entries right.
         *  After this call m_numSegments is incremented by 1 and the values
         *  (L, a, b) occupy index 0; previous index i is now at i+1. Used to
         *  align the Palette Color LUT with a newly added background segment
         *  (Segment Number 0) so that pixel value 0 maps to the prepended
         *  color in the LUT.
         *  @param  L  L* component, DICOM-encoded (0..65535 maps to 0..100).
         *  @param  a  a* component, DICOM-encoded (0..65535 maps to -128..127).
         *  @param  b  b* component, DICOM-encoded (0..65535 maps to -128..127).
         *  @return OFTrue on success, OFFalse on memory allocation failure.
         */
        OFBool prepend(Uint16 L, Uint16 a, Uint16 b)
        {
            size_t newSize = m_numSegments + 1;
            Uint16* newL = new Uint16[newSize];
            Uint16* newA = new Uint16[newSize];
            Uint16* newB = new Uint16[newSize];
            if (!newL || !newA || !newB)
            {
                delete[] newL; delete[] newA; delete[] newB;
                return OFFalse;
            }
            newL[0] = L; newA[0] = a; newB[0] = b;
            for (size_t i = 0; i < m_numSegments; i++)
            {
                newL[i + 1] = m_L[i];
                newA[i + 1] = m_a[i];
                newB[i + 1] = m_b[i];
            }
            delete[] m_L; delete[] m_a; delete[] m_b;
            m_L = newL; m_a = newA; m_b = newB;
            m_numSegments = newSize;
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
