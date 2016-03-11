#include "dcmtk/config/osconfig.h"   // make sure OS specific configuration is included first
#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/oflog/oflog.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmseg/segdoc.h"
#include "dcmtk/dcmseg/segment.h"
#include "dcmtk/dcmseg/segutils.h"
#include "dcmtk/dcmfg/fginterface.h"
#include "dcmtk/dcmiod/iodutil.h"
#include "dcmtk/dcmiod/modmultiframedimension.h"
#include "dcmtk/dcmdata/dcsequen.h"

#include "dcmtk/dcmfg/fgderimg.h"
#include "dcmtk/dcmfg/fgplanor.h"
#include "dcmtk/dcmfg/fgpixmsr.h"
#include "dcmtk/dcmfg/fgfracon.h"
#include "dcmtk/dcmfg/fgplanpo.h"
#include "dcmtk/dcmfg/fgseg.h"

#include "dcmtk/oflog/loglevel.h"

#include "dcmtk/dcmdata/dcrledrg.h"

#include "vnl/vnl_cross.h"

#define INCLUDE_CSTDLIB
#define INCLUDE_CSTRING
#include "dcmtk/ofstd/ofstdinc.h"

#include <sstream>
#include <map>
#include <vector>

#ifdef WITH_ZLIB
#include <zlib.h>                     /* for zlibVersion() */
#endif

// ITK includes
#include <itkImageFileWriter.h>
#include <itkLabelImageToLabelMapFilter.h>
#include <itkImageRegionConstIterator.h>
#include <itkChangeInformationImageFilter.h>
#include <itkImageDuplicator.h>


//#include "framesorter.h"
#include "SegmentAttributes.h"

// CLP inclides
#include "segimage2itkimageCLP.h"

static OFLogger dcemfinfLogger = OFLog::getLogger("qiicr.apps");

#define CHECK_COND(condition) \
    do { \
        if (condition.bad()) { \
            OFLOG_FATAL(dcemfinfLogger, condition.text() << " in " __FILE__ << ":" << __LINE__ ); \
            throw -1; \
        } \
    } while (0)

typedef unsigned short PixelType;
typedef itk::Image<PixelType,3> ImageType;

double distanceBwPoints(vnl_vector<double> from, vnl_vector<double> to){
  return sqrt((from[0]-to[0])*(from[0]-to[0])+(from[1]-to[1])*(from[1]-to[1])+(from[2]-to[2])*(from[2]-to[2]));
}

int getImageDirections(FGInterface &fgInterface, ImageType::DirectionType &dir);
int getDeclaredImageSpacing(FGInterface &fgInterface, ImageType::SpacingType &spacing);
int computeVolumeExtent(FGInterface &fgInterface, vnl_vector<double> &sliceDirection, ImageType::PointType &imageOrigin, double &sliceSpacing, double &sliceExtent);

int main(int argc, char *argv[])
{
  PARSE_ARGS;

  DcmRLEDecoderRegistration::registerCodecs();

  dcemfinfLogger.setLogLevel(dcmtk::log4cplus::OFF_LOG_LEVEL);

  DcmFileFormat segFF;
  DcmDataset *segDataset = NULL;
  if(segFF.loadFile(inputSEGFileName.c_str()).good()){
    segDataset = segFF.getDataset();
  } else {
    std::cerr << "Failed to read input " << std::endl;
    return EXIT_FAILURE;
  }

  OFCondition cond;
  DcmSegmentation *segdoc = NULL;
  cond = DcmSegmentation::loadFile(inputSEGFileName.c_str(), segdoc);
  if(!segdoc){
    std::cerr << "Failed to load seg! " << cond.text() << std::endl;
    return EXIT_FAILURE;
  }

  DcmSegment *segment = segdoc->getSegment(1);
  FGInterface &fgInterface = segdoc->getFunctionalGroups();

  ImageType::PointType imageOrigin;
  ImageType::RegionType imageRegion;
  ImageType::SizeType imageSize;
  ImageType::SpacingType imageSpacing;
  ImageType::Pointer segImage = ImageType::New();

  // Directions
  ImageType::DirectionType dir;
  if(getImageDirections(fgInterface, dir)){
    std::cerr << "Failed to get image directions" << std::endl;
    return EXIT_FAILURE;
  }

  // Spacing and origin
  double computedSliceSpacing, computedVolumeExtent;
  vnl_vector<double> sliceDirection(3);
  sliceDirection[0] = dir[0][2];
  sliceDirection[1] = dir[1][2];
  sliceDirection[2] = dir[2][2];
  if(computeVolumeExtent(fgInterface, sliceDirection, imageOrigin, computedSliceSpacing, computedVolumeExtent)){
    std::cerr << "Failed to compute origin and/or slice spacing!" << std::endl;
    return EXIT_FAILURE;
  }

  imageSpacing.Fill(0);
  if(getDeclaredImageSpacing(fgInterface, imageSpacing)){
    std::cerr << "Failed to get image spacing from DICOM!" << std::endl;
    return EXIT_FAILURE;
  }

  const double tolerance = 1e-5;
  if(!imageSpacing[2]){
    imageSpacing[2] = computedSliceSpacing;
  }
  else if(fabs(imageSpacing[2]-computedSliceSpacing)>tolerance){
    std::cerr << "WARNING: Declared slice spacing is significantly different from the one declared in DICOM!" <<
                 " Declared = " << imageSpacing[2] << " Computed = " << computedSliceSpacing << std::endl;
  }

  // Region size
  {
    OFString str;
    if(segDataset->findAndGetOFString(DCM_Rows, str).good()){
      imageSize[1] = atoi(str.c_str());
    }
    if(segDataset->findAndGetOFString(DCM_Columns, str).good()){
      imageSize[0] = atoi(str.c_str());
    }
  }
  // number of slices should be computed, since segmentation may have empty frames
  // Small differences in image spacing could lead to computing number of slices incorrectly
  // (it is always floored), round it with a tolerance instead.
  imageSize[2] = int(computedVolumeExtent/imageSpacing[2]/tolerance+0.5)*tolerance+1;

  // Initialize the image
  imageRegion.SetSize(imageSize);
  segImage->SetRegions(imageRegion);
  segImage->SetOrigin(imageOrigin);
  segImage->SetSpacing(imageSpacing);
  segImage->SetDirection(dir);
  segImage->Allocate();
  segImage->FillBuffer(0);

  // ITK images corresponding to the individual segments
  std::map<unsigned,ImageType::Pointer> segment2image;
  // list of strings that
  std::map<unsigned,std::string> segment2meta;

  // Iterate over frames, find the matching slice for each of the frames based on
  // ImagePositionPatient, set non-zero pixels to the segment number. Notify
  // about pixels that are initialized more than once.

  DcmIODTypes::Frame *unpackedFrame = NULL;

  for(int frameId=0;frameId<fgInterface.getNumberOfFrames();frameId++){
    const DcmIODTypes::Frame *frame = segdoc->getFrame(frameId);
    bool isPerFrame;

    FGPlanePosPatient *planposfg =
        OFstatic_cast(FGPlanePosPatient*,fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
    assert(planposfg);

    FGFrameContent *fracon =
        OFstatic_cast(FGFrameContent*,fgInterface.get(frameId, DcmFGTypes::EFG_FRAMECONTENT, isPerFrame));
    assert(fracon);

    FGSegmentation *fgseg =
        OFstatic_cast(FGSegmentation*,fgInterface.get(frameId, DcmFGTypes::EFG_SEGMENTATION, isPerFrame));
    assert(fgseg);

    Uint16 segmentId = -1;
    if(fgseg->getReferencedSegmentNumber(segmentId).bad()){
      std::cerr << "Failed to get seg number!";
      return EXIT_FAILURE;
    }


    // WARNING: this is needed only for David's example, which numbers
    // (incorrectly!) segments starting from 0, should start from 1
    if(segmentId == 0){
      std::cerr << "Segment numbers should start from 1!" << std::endl;
      return EXIT_FAILURE;
    }

    if(segment2image.find(segmentId) == segment2image.end()){
      typedef itk::ImageDuplicator<ImageType> DuplicatorType;
      DuplicatorType::Pointer dup = DuplicatorType::New();
      dup->SetInputImage(segImage);
      dup->Update();
      ImageType::Pointer newSegmentImage = dup->GetOutput();
      newSegmentImage->FillBuffer(0);
      segment2image[segmentId] = newSegmentImage;
    }

    // populate meta information needed for Slicer ScalarVolumeNode initialization
    //  (for example)
    {
      // NOTE: according to the standard, segment numbering should start from 1,
      //  not clear if this is intentional behavior or a bug in DCMTK expecting
      //  it to start from 0
      std::ostringstream metastr;

      DcmSegment* segment = segdoc->getSegment(segmentId);
      if(segment == NULL){
        std::cerr << "Failed to get segment for segment ID " << segmentId << std::endl;
        return EXIT_FAILURE;
      }

      // get CIELab color for the segment
      Uint16 ciedcm[3];
      unsigned cielabScaled[3];
      float cielab[3], ciexyz[3];
      unsigned rgb[3];
      if(segment->getRecommendedDisplayCIELabValue(
        ciedcm[0], ciedcm[1], ciedcm[2]
      ).bad()) {
        // NOTE: if the call above fails, it overwrites the values anyway,
        //  not sure if this is a dcmtk bug or not
        ciedcm[0] = 43803;
        ciedcm[1] = 26565;
        ciedcm[2] = 37722;
        std::cerr << "Failed to get CIELab values - initializing to default " <<
          ciedcm[0] << "," << ciedcm[1] << "," << ciedcm[2] << std::endl;
      }
      cielabScaled[0] = unsigned(ciedcm[0]);
      cielabScaled[1] = unsigned(ciedcm[1]);
      cielabScaled[2] = unsigned(ciedcm[2]);

      getCIELabFromIntegerScaledCIELab(&cielabScaled[0],&cielab[0]);

      getCIEXYZFromCIELab(&cielab[0],&ciexyz[0]);

      getRGBFromCIEXYZ(&ciexyz[0],&rgb[0]);

      // line format:
      // labelNum;RGB:R,G,B;SegmentedPropertyCategory:code,scheme,meaning;SegmentedPropertyType:code,scheme,meaning;SegmentedPropertyTypeModifier:code,scheme,meaning;AnatomicRegion:code,scheme,meaning;AnatomicRegionModifier:code,scheme,meaning
      metastr << segmentId;
      metastr << ";RGB:" << rgb[0] << "," << rgb[1] << "," << rgb[2];

      OFString meaning, designator, code;
      segment->getSegmentedPropertyCategoryCode().getCodeMeaning(meaning);
      segment->getSegmentedPropertyCategoryCode().getCodeValue(code);
      segment->getSegmentedPropertyCategoryCode().getCodingSchemeDesignator(designator);
      metastr << ";SegmentedPropertyCategory:" << code << ","  << designator << "," << meaning;

      segment->getSegmentedPropertyTypeCode().getCodeMeaning(meaning);
      segment->getSegmentedPropertyTypeCode().getCodeValue(code);
      segment->getSegmentedPropertyTypeCode().getCodingSchemeDesignator(designator);
      metastr << ";SegmentedPropertyType:" << code << "," << designator << "," << meaning;
      if(segment->getSegmentedPropertyTypeModifierCode().size()>0){
        segment->getSegmentedPropertyTypeModifierCode()[0]->getCodeMeaning(meaning);
        segment->getSegmentedPropertyTypeModifierCode()[0]->getCodeValue(code);
        segment->getSegmentedPropertyTypeModifierCode()[0]->getCodingSchemeDesignator(designator);
        metastr << ";SegmentedPropertyTypeModifier:" << code << "," << designator << "," << meaning;
      }

      // get anatomy codes
      GeneralAnatomyMacro &anatomyMacro = segment->getGeneralAnatomyCode();
      anatomyMacro.getAnatomicRegion().getCodeMeaning(meaning);
      anatomyMacro.getAnatomicRegion().getCodeValue(code);
      anatomyMacro.getAnatomicRegion().getCodingSchemeDesignator(designator);
      metastr << ";AnatomicRegion:" << code << "," << designator << "," << meaning;
      if(anatomyMacro.getAnatomicRegionModifier().size()>0){
        anatomyMacro.getAnatomicRegionModifier()[0]->getCodeMeaning(meaning);
        anatomyMacro.getAnatomicRegionModifier()[0]->getCodeValue(code);
        anatomyMacro.getAnatomicRegionModifier()[0]->getCodingSchemeDesignator(designator);
        metastr << ";AnatomicRegionModifier:" << code << "," << designator << "," << meaning;
      }
      metastr << std::endl;
      segment2meta[segmentId] = metastr.str();
    }

    // get string representation of the frame origin
    ImageType::PointType frameOriginPoint;
    ImageType::IndexType frameOriginIndex;
    for(int j=0;j<3;j++){
      OFString planposStr;
      if(planposfg->getImagePositionPatient(planposStr, j).good()){
        frameOriginPoint[j] = atof(planposStr.c_str());
      }
    }

    if(!segment2image[segmentId]->TransformPhysicalPointToIndex(frameOriginPoint, frameOriginIndex)){
      std::cerr << "ERROR: Frame " << frameId << " origin " << frameOriginPoint <<
                   " is outside image geometry!" << frameOriginIndex << std::endl;
      std::cerr << "Image size: " << segment2image[segmentId]->GetBufferedRegion().GetSize() << std::endl;
      return EXIT_FAILURE;
    }

    unsigned slice = frameOriginIndex[2];

    if(segdoc->getSegmentationType() == DcmSegTypes::ST_BINARY)
      unpackedFrame = DcmSegUtils::unpackBinaryFrame(frame,
        imageSize[1], // Rows
        imageSize[0]); // Cols
    else
      unpackedFrame = new DcmIODTypes::Frame(*frame);

    // initialize slice with the frame content
    for(int row=0;row<imageSize[1];row++){
      for(int col=0;col<imageSize[0];col++){
        ImageType::PixelType pixel;
        unsigned bitCnt = row*imageSize[0]+col;
        pixel = unpackedFrame->pixData[bitCnt];

        if(pixel!=0){
          ImageType::IndexType index;
          index[0] = col;
          index[1] = row;
          index[2] = slice;
          segment2image[segmentId]->SetPixel(index, segmentId);
        }
      }
    }

    if(unpackedFrame != NULL)
      delete unpackedFrame;
  }

  for(std::map<unsigned,ImageType::Pointer>::const_iterator sI=segment2image.begin();sI!=segment2image.end();++sI){
    typedef itk::ImageFileWriter<ImageType> WriterType;
    std::stringstream imageFileNameSStream, infoFileNameSStream;
    // is this safe?
    imageFileNameSStream << outputDirName << "/" << sI->first << ".nrrd";
    infoFileNameSStream << outputDirName << "/" << sI->first << ".info";

    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName(imageFileNameSStream.str());
    writer->SetInput(sI->second);
    writer->SetUseCompression(1);
    writer->Update();

    std::ofstream metaf(infoFileNameSStream.str().c_str());
    metaf << segment2meta[sI->first] << std::endl;
    metaf.close();
  }

  return 0;
}

int getImageDirections(FGInterface &fgInterface, ImageType::DirectionType &dir){
  // For directions, we can only handle segments that have patient orientation
  //  identical for all frames, so either find it in shared FG, or fail
  // TODO: handle the situation when FoR is not initialized
  OFBool isPerFrame;
  vnl_vector<double> rowDirection(3), colDirection(3);

  FGPlaneOrientationPatient *planorfg = OFstatic_cast(FGPlaneOrientationPatient*,
                                                      fgInterface.get(0, DcmFGTypes::EFG_PLANEORIENTPATIENT, isPerFrame));
  if(!planorfg){
    std::cerr << "Plane Orientation (Patient) is missing, cannot parse input " << std::endl;
    return EXIT_FAILURE;
  }
  OFString orientStr;
  for(int i=0;i<3;i++){
    if(planorfg->getImageOrientationPatient(orientStr, i).good()){
      rowDirection[i] = atof(orientStr.c_str());
    } else {
      std::cerr << "Failed to get orientation " << i << std::endl;
      return EXIT_FAILURE;
    }
  }
  for(int i=3;i<6;i++){
    if(planorfg->getImageOrientationPatient(orientStr, i).good()){
      colDirection[i-3] = atof(orientStr.c_str());
    } else {
      std::cerr << "Failed to get orientation " << i << std::endl;
      return EXIT_FAILURE;
    }
  }
  vnl_vector<double> sliceDirection = vnl_cross_3d(rowDirection, colDirection);
  sliceDirection.normalize();

  for(int i=0;i<3;i++){
    dir[i][0] = rowDirection[i];
    dir[i][1] = colDirection[i];
    dir[i][2] = sliceDirection[i];
  }

  return 0;
}

int computeVolumeExtent(FGInterface &fgInterface, vnl_vector<double> &sliceDirection, ImageType::PointType &imageOrigin, double &sliceSpacing, double &sliceExtent){
  // Size
  // Rows/Columns can be read directly from the respective attributes
  // For number of slices, consider that all segments must have the same number of frames.
  //   If we have FoR UID initialized, this means every segment should also have Plane
  //   Position (Patient) initialized. So we can get the number of slices by looking
  //   how many per-frame functional groups a segment has.

  std::vector<double> originDistances;
  std::map<OFString, double> originStr2distance;
  std::map<OFString, unsigned> frame2overlap;
  double minDistance;

  unsigned numFrames = fgInterface.getNumberOfFrames();

  /* Framesorter is to be moved to DCMTK at some point
   * in the future. For now it is causing build issues on windows

  FrameSorterIPP fsIPP;
  FrameSorterIPP::Results sortResults;
  fsIPP.setSorterInput(&fgInterface);
  fsIPP.sort(sortResults);

  */

  // Determine ordering of the frames, keep mapping from ImagePositionPatient string
  //   to the distance, and keep track (just out of curiousity) how many frames overlap
  vnl_vector<double> refOrigin(3);
  {
    OFBool isPerFrame;
    FGPlanePosPatient *planposfg =
        OFstatic_cast(FGPlanePosPatient*,fgInterface.get(0, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
    for(int j=0;j<3;j++){
      OFString planposStr;
      if(planposfg->getImagePositionPatient(planposStr, j).good()){
          refOrigin[j] = atof(planposStr.c_str());
      } else {
        std::cerr << "Failed to read patient position" << std::endl;
      }
    }
  }

  for(int frameId=0;frameId<numFrames;frameId++){
    OFBool isPerFrame;
    FGPlanePosPatient *planposfg =
        OFstatic_cast(FGPlanePosPatient*,fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));

    if(!planposfg){
      std::cerr << "PlanePositionPatient is missing" << std::endl;
      return EXIT_FAILURE;
    }

    if(!isPerFrame){
      std::cerr << "PlanePositionPatient is required for each frame!" << std::endl;
      return EXIT_FAILURE;
    }

    vnl_vector<double> sOrigin;
    OFString sOriginStr = "";
    sOrigin.set_size(3);
    for(int j=0;j<3;j++){
      OFString planposStr;
      if(planposfg->getImagePositionPatient(planposStr, j).good()){
          sOrigin[j] = atof(planposStr.c_str());
          sOriginStr += planposStr;
          if(j<2)
            sOriginStr+='/';
      } else {
        std::cerr << "Failed to read patient position" << std::endl;
        return EXIT_FAILURE;
      }
    }

    // check if this frame has already been encountered
    if(originStr2distance.find(sOriginStr) == originStr2distance.end()){
      vnl_vector<double> difference;
      difference.set_size(3);
      difference[0] = sOrigin[0]-refOrigin[0];
      difference[1] = sOrigin[1]-refOrigin[1];
      difference[2] = sOrigin[2]-refOrigin[2];
      double dist = dot_product(difference,sliceDirection);
      frame2overlap[sOriginStr] = 1;
      originStr2distance[sOriginStr] = dist;
      assert(originStr2distance.find(sOriginStr) != originStr2distance.end());
      originDistances.push_back(dist);

      if(frameId==0){
        minDistance = dist;
        imageOrigin[0] = sOrigin[0];
        imageOrigin[1] = sOrigin[1];
        imageOrigin[2] = sOrigin[2];
      }
      else
        if(dist<minDistance){
          imageOrigin[0] = sOrigin[0];
          imageOrigin[1] = sOrigin[1];
          imageOrigin[2] = sOrigin[2];
          minDistance = dist;
        }
    } else {
      frame2overlap[sOriginStr]++;
    }
  }

  // sort all unique distances, this will be used to check consistency of
  //  slice spacing, and also to locate the slice position from ImagePositionPatient
  //  later when we read the segments
  sort(originDistances.begin(), originDistances.end());

  sliceSpacing = fabs(originDistances[0]-originDistances[1]);

  for(int i=1;i<originDistances.size();i++){
    float dist1 = fabs(originDistances[i-1]-originDistances[i]);
    float delta = sliceSpacing-dist1;
    if(delta > 0.001){
      std::cerr << "WARNING: Inter-slice distance " << originDistances[i] << " difference exceeded threshold: " << delta << std::endl;
    }
  }

  sliceExtent = fabs(originDistances[0]-originDistances[originDistances.size()-1]);
  unsigned overlappingFramesCnt = 0;
  for(std::map<OFString, unsigned>::const_iterator it=frame2overlap.begin();
      it!=frame2overlap.end();++it){
    if(it->second>1)
      overlappingFramesCnt++;
  }

  std::cout << "Total frames: " << numFrames << std::endl;
  std::cout << "Total frames with unique IPP: " << originDistances.size() << std::endl;
  std::cout << "Total overlapping frames: " << overlappingFramesCnt << std::endl;
  std::cout << "Origin: " << imageOrigin << std::endl;

  return 0;
}

int getDeclaredImageSpacing(FGInterface &fgInterface, ImageType::SpacingType &spacing){
  OFBool isPerFrame;
  FGPixelMeasures *pixm = OFstatic_cast(FGPixelMeasures*,
                                                      fgInterface.get(0, DcmFGTypes::EFG_PIXELMEASURES, isPerFrame));
  if(!pixm){
    std::cerr << "Pixel measures FG is missing!" << std::endl;
    return EXIT_FAILURE;
  }

  pixm->getPixelSpacing(spacing[0], 0);
  pixm->getPixelSpacing(spacing[1], 1);

  Float64 spacingFloat;
  if(pixm->getSpacingBetweenSlices(spacingFloat,0).good() && spacingFloat != 0){
    spacing[2] = spacingFloat;
  } else if(pixm->getSliceThickness(spacingFloat,0).good() && spacingFloat != 0){
    // SliceThickness can be carried forward from the source images, and may not be what we need
    // As an example, this ePAD example has 1.25 carried from CT, but true computed thickness is 1!
    std::cerr << "WARNING: SliceThickness is present and is " << spacingFloat << ". NOT using it!" << std::endl;
  }

  return 0;
}
