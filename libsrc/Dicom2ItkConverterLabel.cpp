// DCMQI includes
#include "dcmqi/Dicom2ItkConverterLabel.h"
#include "dcmqi/ColorUtilities.h"

// DCMTK includes
#include <cstddef>
#include <dcmtk/dcmiod/cielabutil.h>
#include <dcmtk/dcmseg/overlaputil.h>
#include <dcmtk/dcmsr/codes/dcm.h>
#include <dcmtk/ofstd/ofmem.h>
#include <itkSmartPointer.h>
#include <memory>

namespace dcmqi
{

Dicom2ItkConverterLabel::Dicom2ItkConverterLabel()
    : Dicom2ItkConverterBase()
    , m_frameIterator(0)
    , m_fillValue(0)
{
}

// -------------------------------------------------------------------------------------

Dicom2ItkConverterLabel::~Dicom2ItkConverterLabel()
{
}

// -------------------------------------------------------------------------------------

OFCondition Dicom2ItkConverterLabel::dcmSegmentation2itkimage(const bool mergeSegments)
{
    if (!m_isLabelmap)
    {
        cerr << "ERROR: Dicom2ItkConverterLabel can only be used for labelmap segmentations!" << endl;
        return EC_IllegalParameter;
    }
    OFCondition result;

    // Identify the background segment: per DICOM standard the (only) indicator that
    // a segment is to be treated as background is Pixel Padding Value (0028,0120),
    // which DcmSegmentation interprets on reading. The background segment does
    // not produce an entry in the JSON segment metadata (its pixel values are
    // still exported unchanged to the ITK image). A segment that is merely
    // *typed* as Background (DCM,125040) without being designated via Pixel
    // Padding Value is treated as a regular segment.
    Uint16 bgSegmentNumber      = 0;
    const OFBool hasBgSegment   = m_segDoc->getBackgroundSegmentNumber(bgSegmentNumber);
    if (hasBgSegment && (bgSegmentNumber != 0))
    {
        cerr << "WARNING: Pixel Padding Value designates non-zero pixel value " << bgSegmentNumber
             << " as background. Its segment will not be part of the JSON metadata, but pixels"
             << " with this value remain in the output image; consuming applications may not"
             << " recognize them as background." << endl;
    }

    // Determine the fill value for slice positions of the output image that are
    // not encoded as frames in the DICOM ("missing" interior slices; positions
    // missing at the volume borders merely shrink the computed image grid and
    // never need filling). Missing data is treated as background: Pixel Padding
    // Value if present, 0 otherwise. If no Pixel Padding Value is present, frames
    // are missing AND a real segment occupies pixel value 0, the filled-in
    // positions become indistinguishable from that segment in the output. We
    // still fill with 0 (the conventional background, and usually what a producer
    // that simply forgot to set Pixel Padding Value intended) but warn about the
    // ambiguity rather than refusing the conversion outright.
    m_fillValue   = 0;
    Uint16 ppv    = 0;
    OFBool havePpv = m_segDoc->getEquipment().getPixelPaddingValue(ppv).good();
    if (havePpv && (m_bytesPerPixel == 1) && (ppv > 255))
    {
        cerr << "WARNING: Pixel Padding Value " << ppv << " does not fit the 8 bit pixel data"
             << " of this labelmap; ignoring it." << endl;
        havePpv = OFFalse;
    }
    const OFBool hasMissingFrames = m_segDoc->getNumberOfFrames() < m_imageSize[2];
    if (havePpv)
    {
        m_fillValue = ppv;
    }
    else if (hasMissingFrames && (m_segDoc->getSegment(0) != NULL))
    {
        cerr << "WARNING: Labelmap encodes only " << m_segDoc->getNumberOfFrames() << " of "
             << m_imageSize[2] << " slice positions, has no Pixel Padding Value, and uses"
             << " pixel value 0 for a segment: the missing positions are filled with 0 and"
             << " are therefore indistinguishable from that segment in the output image." << endl;
    }

    // Prepare the meta information, which will be returned immediately from the dcmSegmentation2itkimage() call,
    // while the ITK images are made accessible through the begin()/next() iterators only.
    // createMetaInfo() requires groups of non-overlapping segments, but for labelmaps we know that
    // all segments are non-overlapping, so we can just create one group with all segments in it.
    OFVector<Uint32> segs;
    OFMap<Uint16, DcmSegment*>::const_iterator segIt = m_segDoc->getSegments().begin();
    while (segIt != m_segDoc->getSegments().end())
    {
        if (hasBgSegment && (segIt->first == bgSegmentNumber))
        {
            // skip the designated background segment in the JSON metadata
            ++segIt;
            continue;
        }
        if (DcmSegmentation::isBackgroundSegment(segIt->second))
        {
            cerr << "WARNING: Segment " << segIt->first << " is typed as Background (DCM,125040)"
                 << " but not designated via Pixel Padding Value; it is treated as a regular segment."
                 << endl;
        }
        segs.push_back(segIt->first);
        ++segIt;
    }
    m_segmentGroups.clear();
    m_segmentGroups.push_back(segs);

    // Now we are ready to create the meta information.
    // It is available directly after this call in m_metaInfo.
    // The ITK images will be accessible through the begin()/next() iterators only.
    result = createMetaInfo();
    if (result.bad())
    {
        return result;
    }

    return result;
}

// -------------------------------------------------------------------------------------

itk::SmartPointer<ShortImageType> Dicom2ItkConverterLabel::begin16Bit()
{
    m_frameIterator = 0;
    return next16Bit();
}

// -------------------------------------------------------------------------------------

itk::SmartPointer<CharImageType> Dicom2ItkConverterLabel::begin8Bit()
{
    m_frameIterator = 0;
    return next8Bit();
}

// -------------------------------------------------------------------------------------

itk::SmartPointer<ShortImageType> Dicom2ItkConverterLabel::next16Bit()
{
    return nextImpl<ShortImageType, Uint16>();
}

// -------------------------------------------------------------------------------------

itk::SmartPointer<CharImageType> Dicom2ItkConverterLabel::next8Bit()
{
    return nextImpl<CharImageType, Uint8>();
}

// -------------------------------------------------------------------------------------

template<typename TImageType, typename TPixelType>
itk::SmartPointer<TImageType> Dicom2ItkConverterLabel::nextImpl()
{
    // For Labelmaps, every frame is already a complete labelmap image, so we can just convert each frame into an ITK
    // image and return it as is (without having to worry about merging multiple segments into one image as in the
    // binary case).
    size_t numFrames = m_segDoc->getNumberOfFrames();
    if (m_frameIterator != 0)
    {
        m_frameIterator = numFrames;
        return nullptr;
    }

    typename TImageType::Pointer itkImage = allocateITKImage<TImageType>();
    // Slice positions without a frame in the DICOM read back as background: the
    // buffer is pre-filled with the fill value (Pixel Padding Value, or 0), and
    // encoded frames overwrite their slice completely (pixels that already hold
    // the fill value are skipped as a shortcut).
    const TPixelType fillValue = OFstatic_cast(TPixelType, m_fillValue);
    if (fillValue != 0)
        itkImage->FillBuffer(fillValue);
    for (size_t slice = 0; slice < numFrames; slice++)
    {
        ShortImageType::PointType frameOriginPoint;
        ShortImageType::IndexType frameOriginIndex;
        OFCondition result = getITKImageOrigin(m_frameIterator, frameOriginPoint);
        if (result.bad())
        {
            cerr << "ERROR: Failed to get origin for frame " << m_frameIterator << " of segment " << endl;
            return nullptr;
        }
        if (!itkImage->TransformPhysicalPointToIndex(frameOriginPoint, frameOriginIndex))
        {
            cerr << "ERROR: Frame " << m_frameIterator << " origin " << frameOriginPoint
                 << " is outside image geometry!" << frameOriginIndex << endl;
            cerr << "Image size: " << itkImage->GetBufferedRegion().GetSize() << endl;
            return nullptr;
        }
        const DcmIODTypes::FrameBase* frame = m_segDoc->getFrame(m_frameIterator);
        if (!frame)
        {
            cerr << "ERROR: Failed to get frame " << m_frameIterator << " of segment " << endl;
            return nullptr;
        }
        if (frame->bytesPerPixel() != sizeof(TPixelType))
        {
            cerr << "ERROR: Expected bit depth of " << sizeof(TPixelType) * 8
                 << " for labelmap segmentation, but got "
                 << OFstatic_cast(Uint32, frame->bytesPerPixel() * 8) << " for frame " << m_frameIterator << endl;
            return nullptr;
        }
        const TPixelType* pixelData = OFstatic_cast(const TPixelType*, frame->getPixelData());
        for (unsigned row = 0; row < m_imageSize[1]; row++)
        {
            for (unsigned col = 0; col < m_imageSize[0]; col++)
            {
                unsigned bitCnt = row * m_imageSize[0] + col;
                const TPixelType pixel = pixelData[bitCnt];
                if (pixel != fillValue)
                {
                    typename TImageType::IndexType index;
                    index[0] = col;
                    index[1] = row;
                    index[2] = frameOriginIndex[2];
                    itkImage->SetPixel(index, pixel);
                }
            }
        }
        m_frameIterator++;
    }
    return itkImage;
}


} // namespace dcmqi
