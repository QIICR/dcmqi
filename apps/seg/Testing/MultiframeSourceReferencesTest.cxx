// Reference test for segmentations converted from a multiframe source series
// (https://github.com/QIICR/dcmqi/issues/150).
//
// Converts the same label image twice: once with the classic single-frame
// source series and once with its multiframe (e.g. Legacy Converted Enhanced)
// equivalent, and asserts that
//  - the segmentation content (frame count, pixel data) is identical,
//  - the multiframe variant references the multiframe instance per frame via
//    Referenced Frame Number, with each referenced frame located on the plane
//    of the segmentation frame that references it,
//  - the classic variant carries no Referenced Frame Number,
//  - the Common Instance Reference module of the multiframe variant references
//    exactly the multiframe instance,
//  - with the geometry check disabled, the multiframe instance is referenced
//    as a whole (shared derivation image FG without frame numbers).

#include "dcmqi/Helper.h"
#include "dcmqi/Itk2DicomConverter.h"

#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcsequen.h>

#include <itkImageFileReader.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace
{
using ImageType = itk::Image<short, 3U>;
using ReaderType = itk::ImageFileReader<ImageType>;

const double positionTolerance = 1e-3;

std::string readFile(const std::string& path)
{
  std::ifstream in(path.c_str(), std::ios::binary);
  if (!in)
  {
    std::cerr << "ERROR: cannot open " << path << std::endl;
    return std::string();
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

#define REQUIRE(expr)                                                                  \
  do {                                                                                 \
    if (!(expr)) {                                                                     \
      std::cerr << "FAIL: " << #expr << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
      return EXIT_FAILURE;                                                             \
    }                                                                                  \
  } while (0)

// Read the 3 Image Position (Patient) components of the item's
// PlanePositionSequence into position; false if absent.
bool getPlanePosition(DcmItem* fgItem, double position[3])
{
  DcmItem* planePosItem = nullptr;
  if (fgItem->findAndGetSequenceItem(DCM_PlanePositionSequence, planePosItem, 0).bad() || !planePosItem)
  {
    return false;
  }
  for (int j = 0; j < 3; ++j)
  {
    OFString value;
    if (planePosItem->findAndGetOFString(DCM_ImagePositionPatient, value, j).bad())
    {
      return false;
    }
    position[j] = atof(value.c_str());
  }
  return true;
}

bool samePosition(const double a[3], const double b[3])
{
  for (int j = 0; j < 3; ++j)
  {
    if (std::fabs(a[j] - b[j]) > positionTolerance)
    {
      return false;
    }
  }
  return true;
}
}

int main(int argc, char* argv[])
{
  if (argc < 5)
  {
    std::cerr << "Usage: " << argv[0]
              << " <metadata.json> <segmentation.nrrd> <multiframe.dcm> <classic1.dcm> [<classic2.dcm> ...]"
              << std::endl;
    return EXIT_FAILURE;
  }

  const std::string metadataPath = argv[1];
  const std::string segPath = argv[2];
  const std::vector<std::string> multiframeFile(1, std::string(argv[3]));

  std::vector<std::string> classicFiles;
  for (int i = 4; i < argc; ++i)
  {
    classicFiles.emplace_back(argv[i]);
  }

  const std::string metadata = readFile(metadataPath);
  REQUIRE(!metadata.empty());

  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(segPath);
  try
  {
    reader->Update();
  }
  catch (const itk::ExceptionObject& e)
  {
    std::cerr << "ERROR: failed to load segmentation: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<ImageType::ConstPointer> segmentations;
  segmentations.emplace_back(reader->GetOutput());

  // Identity and per-frame plane positions of the multiframe source instance
  OFString mfSOPInstanceUID, mfSOPClassUID;
  std::vector<std::vector<double> > mfFramePositions;
  {
    DcmFileFormat ff;
    REQUIRE(ff.loadFile(multiframeFile[0].c_str()).good());
    DcmDataset* mf = ff.getDataset();
    REQUIRE(mf->findAndGetOFString(DCM_SOPInstanceUID, mfSOPInstanceUID).good());
    REQUIRE(mf->findAndGetOFString(DCM_SOPClassUID, mfSOPClassUID).good());

    DcmSequenceOfItems* perFrameSeq = nullptr;
    REQUIRE(mf->findAndGetSequence(DCM_PerFrameFunctionalGroupsSequence, perFrameSeq).good() && perFrameSeq);
    for (unsigned long f = 0; f < perFrameSeq->card(); ++f)
    {
      std::vector<double> position(3, 0.0);
      REQUIRE(getPlanePosition(perFrameSeq->getItem(f), position.data()));
      mfFramePositions.push_back(position);
    }
    REQUIRE(mfFramePositions.size() > 1);
  }

  // Convert with the multiframe source, the classic source, and the multiframe
  // source with the geometry check disabled
  std::vector<DcmItem*> mfDatasets = dcmqi::Helper::loadDatasets(multiframeFile);
  std::vector<DcmItem*> classicDatasets = dcmqi::Helper::loadDatasets(classicFiles);
  std::vector<DcmItem*> mfDatasetsNoCheck = dcmqi::Helper::loadDatasets(multiframeFile);
  REQUIRE(mfDatasets.size() == 1);
  REQUIRE(!classicDatasets.empty());

  std::unique_ptr<DcmDataset> mfResult(
      dcmqi::Itk2DicomConverter::itkimage2dcmSegmentation(mfDatasets, segmentations, metadata,
                                                          false /* skipEmptySlices */));
  REQUIRE(mfResult != nullptr);

  std::unique_ptr<DcmDataset> classicResult(
      dcmqi::Itk2DicomConverter::itkimage2dcmSegmentation(classicDatasets, segmentations, metadata,
                                                          false /* skipEmptySlices */));
  REQUIRE(classicResult != nullptr);

  std::unique_ptr<DcmDataset> mfNoCheckResult(
      dcmqi::Itk2DicomConverter::itkimage2dcmSegmentation(mfDatasetsNoCheck, segmentations, metadata,
                                                          false /* skipEmptySlices */,
                                                          false /* useLabelIDAsSegmentNumber */,
                                                          false /* referencesGeometryCheck */));
  REQUIRE(mfNoCheckResult != nullptr);

  // The segmentation content must not depend on how the source series is encoded
  {
    Sint32 mfFrames = 0, classicFrames = 0;
    REQUIRE(mfResult->findAndGetSint32(DCM_NumberOfFrames, mfFrames).good());
    REQUIRE(classicResult->findAndGetSint32(DCM_NumberOfFrames, classicFrames).good());
    REQUIRE(mfFrames == classicFrames && mfFrames > 0);

    const Uint8* mfPixelData = nullptr;
    const Uint8* classicPixelData = nullptr;
    unsigned long mfCount = 0, classicCount = 0;
    REQUIRE(mfResult->findAndGetUint8Array(DCM_PixelData, mfPixelData, &mfCount).good());
    REQUIRE(classicResult->findAndGetUint8Array(DCM_PixelData, classicPixelData, &classicCount).good());
    REQUIRE(mfCount == classicCount);
    REQUIRE(memcmp(mfPixelData, classicPixelData, mfCount) == 0);
  }

  // The Common Instance Reference module must reference exactly the multiframe instance
  {
    DcmSequenceOfItems* refSeriesSeq = nullptr;
    REQUIRE(mfResult->findAndGetSequence(DCM_ReferencedSeriesSequence, refSeriesSeq).good() && refSeriesSeq);
    REQUIRE(refSeriesSeq->card() == 1);
    DcmSequenceOfItems* refInstanceSeq = nullptr;
    REQUIRE(refSeriesSeq->getItem(0)->findAndGetSequence(DCM_ReferencedInstanceSequence, refInstanceSeq).good() && refInstanceSeq);
    REQUIRE(refInstanceSeq->card() == 1);
    OFString uid;
    REQUIRE(refInstanceSeq->getItem(0)->findAndGetOFString(DCM_ReferencedSOPInstanceUID, uid).good());
    REQUIRE(uid == mfSOPInstanceUID);
  }

  // Every frame of the multiframe variant must reference the source frame
  // located on its plane via Referenced Frame Number; the classic variant must
  // not carry frame numbers
  {
    DcmSequenceOfItems* mfPerFrameSeq = nullptr;
    DcmSequenceOfItems* classicPerFrameSeq = nullptr;
    REQUIRE(mfResult->findAndGetSequence(DCM_PerFrameFunctionalGroupsSequence, mfPerFrameSeq).good() && mfPerFrameSeq);
    REQUIRE(classicResult->findAndGetSequence(DCM_PerFrameFunctionalGroupsSequence, classicPerFrameSeq).good() && classicPerFrameSeq);

    for (unsigned long f = 0; f < mfPerFrameSeq->card(); ++f)
    {
      DcmItem* fgItem = mfPerFrameSeq->getItem(f);

      double segFramePosition[3];
      REQUIRE(getPlanePosition(fgItem, segFramePosition));

      DcmItem* derivationItem = nullptr;
      REQUIRE(fgItem->findAndGetSequenceItem(DCM_DerivationImageSequence, derivationItem, 0).good() && derivationItem);
      DcmSequenceOfItems* sourceImageSeq = nullptr;
      REQUIRE(derivationItem->findAndGetSequence(DCM_SourceImageSequence, sourceImageSeq).good() && sourceImageSeq);
      REQUIRE(sourceImageSeq->card() == 1);
      DcmItem* sourceImageItem = sourceImageSeq->getItem(0);

      OFString uid, classUID;
      REQUIRE(sourceImageItem->findAndGetOFString(DCM_ReferencedSOPInstanceUID, uid).good());
      REQUIRE(sourceImageItem->findAndGetOFString(DCM_ReferencedSOPClassUID, classUID).good());
      REQUIRE(uid == mfSOPInstanceUID);
      REQUIRE(classUID == mfSOPClassUID);

      Sint32 referencedFrameNumber = 0;
      REQUIRE(sourceImageItem->findAndGetSint32(DCM_ReferencedFrameNumber, referencedFrameNumber).good());
      REQUIRE(referencedFrameNumber >= 1
              && static_cast<size_t>(referencedFrameNumber) <= mfFramePositions.size());
      REQUIRE(samePosition(segFramePosition, mfFramePositions[referencedFrameNumber - 1].data()));
    }

    for (unsigned long f = 0; f < classicPerFrameSeq->card(); ++f)
    {
      DcmItem* derivationItem = nullptr;
      REQUIRE(classicPerFrameSeq->getItem(f)->findAndGetSequenceItem(DCM_DerivationImageSequence, derivationItem, 0).good() && derivationItem);
      DcmItem* sourceImageItem = nullptr;
      REQUIRE(derivationItem->findAndGetSequenceItem(DCM_SourceImageSequence, sourceImageItem, 0).good() && sourceImageItem);
      REQUIRE(!sourceImageItem->tagExists(DCM_ReferencedFrameNumber));
    }
  }

  // With the geometry check disabled, the multiframe instance must be
  // referenced as a whole by the shared derivation image FG (no frame numbers)
  {
    DcmItem* sharedFGItem = nullptr;
    REQUIRE(mfNoCheckResult->findAndGetSequenceItem(DCM_SharedFunctionalGroupsSequence, sharedFGItem, 0).good() && sharedFGItem);
    DcmItem* derivationItem = nullptr;
    REQUIRE(sharedFGItem->findAndGetSequenceItem(DCM_DerivationImageSequence, derivationItem, 0).good() && derivationItem);
    DcmItem* sourceImageItem = nullptr;
    REQUIRE(derivationItem->findAndGetSequenceItem(DCM_SourceImageSequence, sourceImageItem, 0).good() && sourceImageItem);
    OFString uid;
    REQUIRE(sourceImageItem->findAndGetOFString(DCM_ReferencedSOPInstanceUID, uid).good());
    REQUIRE(uid == mfSOPInstanceUID);
    REQUIRE(!sourceImageItem->tagExists(DCM_ReferencedFrameNumber));
  }

  for (DcmItem* item : mfDatasets) { delete item; }
  for (DcmItem* item : classicDatasets) { delete item; }
  for (DcmItem* item : mfDatasetsNoCheck) { delete item; }

  std::cout << "PASS: multiframe-source segmentation matches the classic-source segmentation "
            << "and references the multiframe instance per frame (Referenced Frame Number), "
            << "as a single instance in the Common Instance Reference module, and as a whole "
            << "instance when the geometry check is disabled."
            << std::endl;
  return EXIT_SUCCESS;
}
