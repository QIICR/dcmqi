// DCMQI includes
#include "dcmqi/Dicom2ItkConverterBin.h"
#include "dcmqi/ColorUtilities.h"

// DCMTK includes
#include <cstddef>
#include <dcmtk/dcmseg/overlaputil.h>
#include <dcmtk/dcmiod/cielabutil.h>
#include <dcmtk/dcmsr/codes/dcm.h>
#include <dcmtk/ofstd/ofmem.h>
#include <itkSmartPointer.h>
#include <memory>

namespace dcmqi
{

Dicom2ItkConverterBin::Dicom2ItkConverterBin()
    : Dicom2ItkConverterBase()
    , m_groupIterator()
    , m_overlapUtil() {};

// -------------------------------------------------------------------------------------

OFCondition Dicom2ItkConverterBin::dcmSegmentation2itkimage(const bool mergeSegments)
{
    if (m_isLabelmap)
    {
        cerr << "ERROR: Dicom2ItkConverterBin can only be used for binary segmentations!" << endl;
        return EC_IllegalParameter;
    }

    // Find groups of segments that can go into the same ITK image (i.e. that are non-overlapping)
    m_overlapUtil.setSegmentationObject(m_segDoc.get());
    OFCondition result = getNonOverlappingSegmentGroups(mergeSegments, m_segmentGroups);
    if (result.bad())
    {
        return result;
    }

    // Create JSON meta info for all segments first since this is returned
    // immediately from this call, while the ITK result images are
    // made accessible through result iterators only.
    result = createMetaInfo();
    if (result.bad())
    {
        return result;
    }

    return result;
}


// -------------------------------------------------------------------------------------

itk::SmartPointer<ShortImageType> Dicom2ItkConverterBin::begin16Bit()
{
    // Set result iterator to first group, i.e. make sure that the first call to nextResult()
    // will return the ITK image for the first group.
    m_groupIterator = m_segmentGroups.begin();
    return next16Bit();
}

// -------------------------------------------------------------------------------------
itk::SmartPointer<ShortImageType> Dicom2ItkConverterBin::next16Bit()
{
    OFCondition result;
    ShortImageType::Pointer itkImage = nullptr;
    if (m_groupIterator != m_segmentGroups.end())
    {
        // Create target ITK image for this group
        itkImage = allocateITKImage<ShortImageType>();

        // Loop over segments belonging to this segment group
        auto segNum = m_groupIterator->begin();
        while (segNum != m_groupIterator->end())
        {
            // Iterate over frames for this segment, and copy the data into the ITK image.
            // Afterwards, the ITK image will have the complete data belonging to that segment
            OverlapUtil::FramesForSegment::value_type framesForSegment;
            m_overlapUtil.getFramesForSegment(*segNum, framesForSegment);
            for (size_t frameIndex = 0; frameIndex < framesForSegment.size(); frameIndex++)
            {
                // Copy the data from the frame into the ITK image
                ShortImageType::PointType frameOriginPoint;
                ShortImageType::IndexType frameOriginIndex;
                result = getITKImageOrigin(framesForSegment[frameIndex], frameOriginPoint);
                if (result.bad())
                {
                    cerr << "ERROR: Failed to get origin for frame " << framesForSegment[frameIndex] << " of segment "
                         << *segNum << endl;
                    m_groupIterator = m_segmentGroups.end();
                    return nullptr;
                }
                if (!itkImage->TransformPhysicalPointToIndex(frameOriginPoint, frameOriginIndex))
                {
                    cerr << "ERROR: Frame " << framesForSegment[frameIndex] << " origin " << frameOriginPoint
                         << " is outside image geometry!" << frameOriginIndex << endl;
                    cerr << "Image size: " << itkImage->GetBufferedRegion().GetSize() << endl;
                    m_groupIterator = m_segmentGroups.end();
                    return nullptr;
                }
                // Handling differs depending on whether the segmentation is binary or fractional
                // (we have to unpack binary frames before copying them into the ITK image)
                const DcmIODTypes::FrameBase* rawFrame      = m_segDoc->getFrame(framesForSegment[frameIndex]);
                const DcmIODTypes::FrameBase* unpackedFrame = NULL;
                if (m_segDoc->getSegmentationType() == DcmSegTypes::ST_BINARY)
                {
                    unpackedFrame = DcmSegUtils::unpackBinaryFrame(OFstatic_cast(const DcmIODTypes::Frame<Uint8>*, rawFrame),
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
                    //   //
                    //   https://itk.org/Doxygen/html/SphinxExamples_2src_2Core_2Common_2ConvertArrayToImage_2Code_8cxx-example.html#_a1
                    //   const unsigned int numberOfPixels = imageSize[0]*imageSize[1];
                    //   using ImportFilterType = itk::ImportImageFilter<CharPixelType, 3>;
                    //   ImportFilterType::Pointer importFilter = ImportFilterType::New();
                    //   importFilter->SetImportPointer((unsigned char*)unpackedFrame->pixData, numberOfPixels,
                    //   importImageFilterWillOwnTheBuffer); importFilter->SetRegion(frameRegion);
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

                for (unsigned row = 0; row < m_imageSize[1]; row++)
                {
                    for (unsigned col = 0; col < m_imageSize[0]; col++)
                    {
                        ShortImageType::PixelType pixel;
                        unsigned bitCnt = row * m_imageSize[0] + col;
                        // This is always 8 bit data after unpacking
                        pixel           = OFstatic_cast(Uint8*, unpackedFrame->getPixelData())[bitCnt];
                        ShortImageType::IndexType index;
                        if (pixel != 0)
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
            segNum++;
        }
        m_groupIterator++;
    }
    return itk::SmartPointer<ShortImageType>(itkImage);
}


// -------------------------------------------------------------------------------------

OFCondition Dicom2ItkConverterBin::getNonOverlappingSegmentGroups(const bool mergeSegments,
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
    return result;
    ;
}


} // namespace dcmqi
