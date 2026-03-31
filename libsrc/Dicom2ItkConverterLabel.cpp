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

    // Prepare the meta information, which will be returned immediately from the dcmSegmentation2itkimage() call,
    // while the ITK images are made accessible through the begin()/next() iterators only.
    // createMetaInfo() requires groups of non-overlapping segments, but for labelmaps we know that
    // all segments are non-overlapping, so we can just create one group with all segments in it.
    size_t numSegs = m_segDoc->getNumberOfSegments();
    OFVector<Uint32> segs;
    for (size_t i = 1; i <= numSegs; ++i)
    {
        segs.push_back(i);
    }
    m_segmentGroups.clear();
    m_segmentGroups.push_back(segs);
    std::cout << "All " << numSegs << " segments will be put into one group for the metadata creation" << std::endl;

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
                typename TImageType::PixelType pixel = pixelData[bitCnt];
                if (pixel != 0)
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
