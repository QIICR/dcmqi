// Equivalence test for the two itkimage2dcmSegmentation overloads.
//
// The handler-based overload is the load-bearing implementation; the
// string-based overload is a thin wrapper that parses its JSON argument into
// a JSONSegmentationMetaInformationHandler and delegates. This test guards
// the wrapper by exercising both paths against the same input and asserting
// the resulting DICOM segmentation datasets agree on the attributes a
// consumer can reasonably rely on. We do not assert byte-level equality
// because DCMTK generates fresh UIDs and timestamps on every write.

#include "dcmqi/Helper.h"
#include "dcmqi/Itk2DicomConverter.h"
#include "dcmqi/JSONSegmentationMetaInformationHandler.h"

#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcsequen.h>

#include <itkImageFileReader.h>

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

// Return the count of items in a top-level sequence, or static_cast<size_t>(-1) on error.
size_t sequenceItemCount(DcmDataset* ds, const DcmTagKey& tag)
{
  DcmSequenceOfItems* seq = nullptr;
  if (ds->findAndGetSequence(tag, seq).bad() || seq == nullptr)
  {
    return static_cast<size_t>(-1);
  }
  return static_cast<size_t>(seq->card());
}

// Collect the SegmentLabel of every item in SegmentSequence, preserving order.
std::vector<std::string> collectSegmentLabels(DcmDataset* ds)
{
  std::vector<std::string> result;
  DcmSequenceOfItems* seq = nullptr;
  if (ds->findAndGetSequence(DCM_SegmentSequence, seq).bad() || seq == nullptr)
  {
    return result;
  }
  for (unsigned long i = 0; i < seq->card(); ++i)
  {
    DcmItem* item = seq->getItem(i);
    OFString label;
    if (item && item->findAndGetOFString(DCM_SegmentLabel, label).good())
    {
      result.emplace_back(label.c_str());
    }
    else
    {
      result.emplace_back();
    }
  }
  return result;
}

std::string readString(DcmDataset* ds, const DcmTagKey& tag)
{
  OFString value;
  if (ds->findAndGetOFString(tag, value).bad())
  {
    return std::string();
  }
  return std::string(value.c_str());
}

#define REQUIRE(expr)                                                                  \
  do {                                                                                 \
    if (!(expr)) {                                                                     \
      std::cerr << "FAIL: " << #expr << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
      return EXIT_FAILURE;                                                             \
    }                                                                                  \
  } while (0)
}

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    std::cerr << "Usage: " << argv[0]
              << " <metadata.json> <segmentation.nrrd> <dicomFile1> [<dicomFile2> ...]"
              << std::endl;
    return EXIT_FAILURE;
  }

  const std::string metadataPath = argv[1];
  const std::string segPath = argv[2];

  std::vector<std::string> dicomFiles;
  for (int i = 3; i < argc; ++i)
  {
    dicomFiles.emplace_back(argv[i]);
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

  // Both call sites need an independent set of source-image datasets because
  // the converter takes ownership semantics that may leave the items in an
  // unspecified state for a second pass.
  std::vector<DcmItem*> dcmDatasetsA = dcmqi::Helper::loadDatasets(dicomFiles);
  std::vector<DcmItem*> dcmDatasetsB = dcmqi::Helper::loadDatasets(dicomFiles);
  REQUIRE(!dcmDatasetsA.empty());
  REQUIRE(dcmDatasetsA.size() == dcmDatasetsB.size());

  // Path A: string overload (legacy).
  std::unique_ptr<DcmDataset> resultA(
      dcmqi::Itk2DicomConverter::itkimage2dcmSegmentation(dcmDatasetsA, segmentations, metadata));
  REQUIRE(resultA != nullptr);

  // Path B: handler overload (new).
  dcmqi::JSONSegmentationMetaInformationHandler handler(metadata);
  handler.read();
  std::unique_ptr<DcmDataset> resultB(
      dcmqi::Itk2DicomConverter::itkimage2dcmSegmentation(dcmDatasetsB, segmentations, handler));
  REQUIRE(resultB != nullptr);

  // Tag-level equivalence checks. We deliberately skip attributes that depend
  // on per-write state (instance/series UIDs, content dates/times) and assert
  // only those that a consumer would treat as semantic content.
  REQUIRE(sequenceItemCount(resultA.get(), DCM_SegmentSequence)
          == sequenceItemCount(resultB.get(), DCM_SegmentSequence));
  REQUIRE(sequenceItemCount(resultA.get(), DCM_PerFrameFunctionalGroupsSequence)
          == sequenceItemCount(resultB.get(), DCM_PerFrameFunctionalGroupsSequence));
  REQUIRE(collectSegmentLabels(resultA.get()) == collectSegmentLabels(resultB.get()));
  REQUIRE(readString(resultA.get(), DCM_SegmentsOverlap)
          == readString(resultB.get(), DCM_SegmentsOverlap));
  REQUIRE(readString(resultA.get(), DCM_SeriesDescription)
          == readString(resultB.get(), DCM_SeriesDescription));
  REQUIRE(readString(resultA.get(), DCM_ContentCreatorName)
          == readString(resultB.get(), DCM_ContentCreatorName));
  REQUIRE(readString(resultA.get(), DCM_ClinicalTrialSeriesID)
          == readString(resultB.get(), DCM_ClinicalTrialSeriesID));
  REQUIRE(readString(resultA.get(), DCM_ClinicalTrialTimePointID)
          == readString(resultB.get(), DCM_ClinicalTrialTimePointID));
  REQUIRE(readString(resultA.get(), DCM_Modality)
          == readString(resultB.get(), DCM_Modality));

  for (DcmItem* item : dcmDatasetsA) { delete item; }
  for (DcmItem* item : dcmDatasetsB) { delete item; }

  std::cout << "PASS: itkimage2dcmSegmentation string and handler overloads agree on "
            << "segment sequence, per-frame functional groups, segment labels, "
            << "SegmentsOverlap, SeriesDescription, ContentCreatorName, "
            << "ClinicalTrialSeriesID, ClinicalTrialTimePointID, Modality."
            << std::endl;
  return EXIT_SUCCESS;
}
