
// DCMQI includes
#include "dcmqi/ConverterBase.h"

// DCMTK includes
#include <dcmtk/dcmseg/segdoc.h>
#include <dcmtk/dcmseg/segment.h>


namespace dcmqi {

  OFCondition ConverterBase::addBackgroundSegmentIfNeeded(DcmSegmentation* segdoc,
                                                           bool setCIELabValue,
                                                           bool* bgAdded)
  {
    if (bgAdded)
      *bgAdded = false;
    if (!segdoc)
      return EC_IllegalParameter;

    // Check whether any frame contains pixel value 0 (early-exit on first hit).
    // Note: this makes the pixel data being traversed twice, since DCMTK scans all
    // frames again when the object is written (coverage enforcement in
    // harmonizeLabelmapBackground(): every pixel value must be described by a
    // Segment, and that scan cannot early-exit). A DCMTK-side switch to skip the
    // write-time scan for callers that guarantee coverage - as dcmqi does by
    // construction, since it builds the frames from the segment numbers it just
    // assigned - could remove that redundancy.
    OFBool hasZeroPixel = OFFalse;
    size_t numFrames = segdoc->getNumberOfFrames();
    for (size_t f = 0; f < numFrames && !hasZeroPixel; f++)
    {
      const DcmIODTypes::FrameBase* frame = segdoc->getFrame(f);
      if (frame == NULL)
        continue;
      const size_t numPixels = frame->getLengthInBytes() / frame->bytesPerPixel();
      for (size_t p = 0; p < numPixels; p++)
      {
        if (frame->bytesPerPixel() == 1)
        {
          Uint8 val = 0;
          frame->getUint8AtIndex(val, p);
          if (val == 0) { hasZeroPixel = OFTrue; break; }
        }
        else
        {
          Uint16 val = 0;
          frame->getUint16AtIndex(val, p);
          if (val == 0) { hasZeroPixel = OFTrue; break; }
        }
      }
    }

    if (!hasZeroPixel)
      return EC_Normal;

    // Designate pixel value 0 as the labelmap background. DcmSegmentation
    // immediately inserts a Background segment at Segment Number 0 (Segmented
    // Property Category Code (SCT,309825002,"Spatial and Relational Concept"),
    // Type Code (DCM,125040,"Background")) and derives Pixel Padding Value
    // (0028,0120) from the designation when the object is written, so the
    // element and the segment can never disagree.
    OFCondition result = segdoc->setBackgroundPixelValue(0);
    if (result.bad())
    {
      cerr << "ERROR: Failed to designate background pixel value 0: " << result.text() << endl;
      return result;
    }

    // Set Recommended Display CIELab Value to black (DICOM-encoded 0, 32768, 32768
    // for L*=0, a*=0, b*=0). Skipped in PALETTE mode where the per-segment CIELab
    // macro must not be written; the LUT entry must instead be provided by the caller.
    if (setCIELabValue)
    {
      DcmSegment* bgSeg = segdoc->getSegment(0);
      if (!bgSeg)
      {
        cerr << "ERROR: Background segment missing after designating background pixel value 0" << endl;
        return EC_IllegalCall;
      }
      result = bgSeg->setRecommendedDisplayCIELabValue(0, 32768, 32768);
      if (result.bad())
      {
        cerr << "ERROR: Failed to set CIELab color for background segment: " << result.text() << endl;
        return result;
      }
    }

    if (bgAdded)
      *bgAdded = true;
    return EC_Normal;
  }


  IODGeneralEquipmentModule::EquipmentInfo ConverterBase::getEquipmentInfo() {
    // TODO: change to following for most recent dcmtk
    // return IODGeneralEquipmentModule::EquipmentInfo(QIICR_MANUFACTURER, QIICR_DEVICE_SERIAL_NUMBER,
    //                                                 QIICR_MANUFACTURER_MODEL_NAME, QIICR_SOFTWARE_VERSIONS);
    IODGeneralEquipmentModule::EquipmentInfo eq;
    eq.m_Manufacturer = QIICR_MANUFACTURER;
    eq.m_DeviceSerialNumber = QIICR_DEVICE_SERIAL_NUMBER;
    eq.m_ManufacturerModelName = QIICR_MANUFACTURER_MODEL_NAME;
    eq.m_SoftwareVersions = QIICR_SOFTWARE_VERSIONS;
    return eq;
  }

  IODEnhGeneralEquipmentModule::EquipmentInfo ConverterBase::getEnhEquipmentInfo() {
    return IODEnhGeneralEquipmentModule::EquipmentInfo(QIICR_MANUFACTURER, QIICR_DEVICE_SERIAL_NUMBER,
                                                       QIICR_MANUFACTURER_MODEL_NAME, QIICR_SOFTWARE_VERSIONS);
  }

  // TODO: defaults for sub classes needs to be defined
  ContentIdentificationMacro ConverterBase::createContentIdentificationInformation(JSONMetaInformationHandlerBase &metaInfo) {
    ContentIdentificationMacro ident;
    CHECK_COND(ident.setContentCreatorName("dcmqi"));
    if(metaInfo.metaInfoRoot["seriesAttributes"].isMember("ContentDescription")){
      CHECK_COND(ident.setContentDescription(metaInfo.metaInfoRoot["seriesAttributes"]["ContentDescription"].asCString()));
    } else {
      CHECK_COND(ident.setContentDescription("DCMQI"));
    }
    if(metaInfo.metaInfoRoot["seriesAttributes"].isMember("ContentLabel")){
      CHECK_COND(ident.setContentLabel(metaInfo.metaInfoRoot["seriesAttributes"]["ContentLabel"].asCString()));
    } else {
      CHECK_COND(ident.setContentLabel("DCMQI"));
    }
    return ident;
  }
}
