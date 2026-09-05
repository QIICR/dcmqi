
// DCMQI includes
#include "dcmqi/SourceImageIndex.h"

// DCMTK includes
#include <dcmtk/dcmdata/dcdeftag.h>

// ITK includes
#include <itkPoint.h>

// STD includes
#include <cstdlib>
#include <iostream>
#include <map>

namespace dcmqi {

  SourceImageIndex::SourceImageIndex(const vector<DcmItem*>& datasets)
    : m_datasets(datasets)
  {
    m_instances.resize(m_datasets.size());
    for(size_t i=0;i<m_datasets.size();i++){
      DcmItem* dataset = m_datasets[i];
      InstanceInfo& info = m_instances[i];

      dataset->findAndGetOFString(DCM_StudyInstanceUID, info.studyInstanceUID);
      dataset->findAndGetOFString(DCM_SeriesInstanceUID, info.seriesInstanceUID);
      dataset->findAndGetOFString(DCM_SOPClassUID, info.sopClassUID);
      dataset->findAndGetOFString(DCM_SOPInstanceUID, info.sopInstanceUID);
      Sint32 numberOfFrames = 0;
      if(dataset->findAndGetSint32(DCM_NumberOfFrames, numberOfFrames).bad() || numberOfFrames < 0)
        numberOfFrames = 0;
      info.numberOfFrames = OFstatic_cast(Uint32, numberOfFrames);

      // classic single-frame instance: one source frame, position from the
      // top-level Image Position (Patient)
      SourceFrame frame;
      frame.datasetIndex = i;
      frame.frameNumber = 0;
      frame.hasPosition = true;
      for(int j=0;j<3;j++){
        OFString ippStr;
        if(dataset->findAndGetOFString(DCM_ImagePositionPatient, ippStr, j).good()){
          frame.position[j] = atof(ippStr.c_str());
        } else {
          frame.hasPosition = false;
          break;
        }
      }
      if(!frame.hasPosition)
        cerr << "WARNING: Source image " << info.sopInstanceUID << " has no Image Position (Patient), "
             << "it cannot be mapped to slices of the converted image" << endl;
      m_frames.push_back(frame);
    }
  }

  // -------------------------------------------------------------------------------------

  vector<vector<size_t> > SourceImageIndex::mapSlicesToFrames(const itk::ImageBase<3>& geometry) const {
    // Find mapping from the slice number of the converted image to the source frames.
    // Assume that orientation of the converted image is the same as the source series.
    const unsigned numSlices = geometry.GetLargestPossibleRegion().GetSize()[2];
    vector<vector<size_t> > slice2frames(numSlices);

    unsigned slicesMapped = 0;
    for(size_t frameId=0;frameId<m_frames.size();frameId++){
      const SourceFrame& frame = m_frames[frameId];
      if(!frame.hasPosition)
        continue;
      itk::Point<double,3> position;
      for(int j=0;j<3;j++)
        position[j] = frame.position[j];
      itk::ImageBase<3>::IndexType index;
      if(!geometry.TransformPhysicalPointToIndex(position, index)){
        // if a source frame does not map to a slice, just skip it
        continue;
      }
      if(slice2frames[index[2]].empty())
        slicesMapped++;
      slice2frames[index[2]].push_back(frameId);
    }
    cout << slicesMapped << " of " << numSlices << " slices mapped to source DICOM images" << endl;
    return slice2frames;
  }

  // -------------------------------------------------------------------------------------

  OFCondition SourceImageIndex::addDerivationImageItem(FGDerivationImage& fgder,
                                                       const vector<size_t>& frameIds,
                                                       const CodeSequenceMacro& derivationCode,
                                                       const string& derivationDescription,
                                                       const CodeSequenceMacro& purposeOfReference)
  {
    if(frameIds.empty())
      return EC_Normal;

    DerivationImageItem* derimgItem = NULL;
    OFCondition result = fgder.addDerivationImageItem(derivationCode, derivationDescription.c_str(), derimgItem);
    if(result.bad())
      return result;

    // group the frames by the instance they belong to, in order of first appearance
    vector<size_t> datasetOrder;
    map<size_t, vector<Uint32> > dataset2frameNumbers;
    for(size_t i=0;i<frameIds.size();i++){
      const SourceFrame& frame = m_frames[frameIds[i]];
      if(dataset2frameNumbers.find(frame.datasetIndex) == dataset2frameNumbers.end())
        datasetOrder.push_back(frame.datasetIndex);
      dataset2frameNumbers[frame.datasetIndex].push_back(frame.frameNumber);
    }

    for(size_t i=0;i<datasetOrder.size();i++){
      const size_t datasetIndex = datasetOrder[i];
      SourceImageItem* srcimgItem = NULL;
      result = derimgItem->addSourceImageItem(m_datasets[datasetIndex], purposeOfReference, srcimgItem);
      if(result.bad())
        return result;
      recordReferencedInstance(datasetIndex);
    }
    return EC_Normal;
  }

  // -------------------------------------------------------------------------------------

  OFCondition SourceImageIndex::addWholeInstanceDerivationImageItem(FGDerivationImage& fgder,
                                                                    const CodeSequenceMacro& derivationCode,
                                                                    const string& derivationDescription,
                                                                    const CodeSequenceMacro& purposeOfReference)
  {
    DerivationImageItem* derimgItem = NULL;
    OFCondition result = fgder.addDerivationImageItem(derivationCode, derivationDescription.c_str(), derimgItem);
    if(result.bad())
      return result;

    for(size_t i=0;i<m_datasets.size();i++){
      SourceImageItem* srcimgItem = NULL;
      if(derimgItem->addSourceImageItem(m_datasets[i], purposeOfReference, srcimgItem).bad()){
        cerr << "WARNING: Failed to reference source image " << m_instances[i].sopInstanceUID
             << ", skipping it" << endl;
        continue;
      }
      recordReferencedInstance(i);
    }
    return EC_Normal;
  }

  // -------------------------------------------------------------------------------------

  OFCondition SourceImageIndex::populateCommonInstanceReference(IODCommonInstanceReferenceModule& commref,
                                                                const OFString& objectStudyInstanceUID) const {
    if(m_referencedDatasets.empty())
      return EC_Normal;

    // instances from the study of the created object, grouped by series
    map<OFString, IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*> series2item;
    // instances from other studies, grouped by study and, within it, by series
    map<OFString, IODCommonInstanceReferenceModule::StudiesOtherInstancesItem*> study2item;
    map<OFString, map<OFString, IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*> > foreignSeries2item;

    // all in order of first reference; the created items are owned by the module
    for(size_t i=0;i<m_referencedDatasets.size();i++){
      const InstanceInfo& info = m_instances[m_referencedDatasets[i]];

      if(info.studyInstanceUID.empty() && !objectStudyInstanceUID.empty())
        cerr << "WARNING: Source image " << info.sopInstanceUID << " has no Study Instance UID, "
             << "referencing it as part of this object's study" << endl;

      const bool foreignStudy = !info.studyInstanceUID.empty()
          && !objectStudyInstanceUID.empty()
          && info.studyInstanceUID != objectStudyInstanceUID;

      OFCondition result;
      if(!foreignStudy){
        result = addToSeriesLevelReferences(commref.getReferencedSeriesItems(), series2item, info);
      } else {
        // Studies Containing Other Referenced Instances Sequence (PS3.3 C.12.2):
        // one item per foreign study, holding its own series/instance references
        IODCommonInstanceReferenceModule::StudiesOtherInstancesItem* studyItem = NULL;
        map<OFString, IODCommonInstanceReferenceModule::StudiesOtherInstancesItem*>::iterator studyIt =
            study2item.find(info.studyInstanceUID);
        if(studyIt == study2item.end()){
          studyItem = new IODCommonInstanceReferenceModule::StudiesOtherInstancesItem();
          result = studyItem->setStudyInstanceUID(info.studyInstanceUID);
          if(result.bad()){
            delete studyItem;
            return result;
          }
          study2item[info.studyInstanceUID] = studyItem;
          commref.getStudiesContainingOtherReferences().push_back(studyItem);
        } else {
          studyItem = studyIt->second;
        }
        result = addToSeriesLevelReferences(
            studyItem->getReferencedSeriesAndInstanceReferences().getReferencedSeriesItems(),
            foreignSeries2item[info.studyInstanceUID], info);
      }
      if(result.bad())
        return result;
    }
    return EC_Normal;
  }

  // -------------------------------------------------------------------------------------

  OFCondition SourceImageIndex::addToSeriesLevelReferences(
      OFVector<IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*>& refseries,
      map<OFString, IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*>& series2item,
      const InstanceInfo& info) {
    IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem* refseriesItem = NULL;
    map<OFString, IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*>::iterator seriesIt =
        series2item.find(info.seriesInstanceUID);
    if(seriesIt == series2item.end()){
      refseriesItem = new IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem();
      OFCondition result = refseriesItem->setSeriesInstanceUID(info.seriesInstanceUID);
      if(result.bad()){
        delete refseriesItem;
        return result;
      }
      series2item[info.seriesInstanceUID] = refseriesItem;
      refseries.push_back(refseriesItem);
    } else {
      refseriesItem = seriesIt->second;
    }

    SOPInstanceReferenceMacro* refinstancesItem = new SOPInstanceReferenceMacro();
    OFCondition result = refinstancesItem->setReferencedSOPClassUID(info.sopClassUID);
    if(result.good())
      result = refinstancesItem->setReferencedSOPInstanceUID(info.sopInstanceUID);
    if(result.bad()){
      delete refinstancesItem;
      return result;
    }
    refseriesItem->getReferencedInstanceItems().push_back(refinstancesItem);
    return EC_Normal;
  }

  // -------------------------------------------------------------------------------------

  const vector<SourceImageIndex::SourceFrame>& SourceImageIndex::getFrames() const {
    return m_frames;
  }

  // -------------------------------------------------------------------------------------

  void SourceImageIndex::recordReferencedInstance(size_t datasetIndex) {
    if(m_referencedDatasetSet.insert(datasetIndex).second)
      m_referencedDatasets.push_back(datasetIndex);
  }

}
