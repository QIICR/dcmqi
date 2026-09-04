#ifndef DCMQI_SOURCEIMAGEINDEX_H
#define DCMQI_SOURCEIMAGEINDEX_H

// STD includes
#include <map>
#include <set>
#include <string>
#include <vector>

// DCMTK includes
#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dcitem.h>
#include <dcmtk/dcmfg/fgderimg.h>
#include <dcmtk/dcmiod/iodmacro.h>
#include <dcmtk/dcmiod/modcommoninstanceref.h>

// ITK includes
#include <itkImageBase.h>

using namespace std;

namespace dcmqi {

  /**
   * @brief Inventory of the source DICOM images a derived object (SEG, PM) was created from.
   *
   * Normalizes the source datasets into a flat list of referenceable "source
   * frames" (a classic single-frame instance contributes one frame), maps the
   * slices of the ITK image being converted to those frames by geometry, and
   * creates the DICOM references derived from that mapping:
   * - Derivation Image functional group items with their Source Image Sequence,
   * - the Common Instance Reference module, fed by the instances referenced
   *   through the derivation items: instances from the study of the created
   *   object go into its Referenced Series Sequence, instances from other
   *   studies into its Studies Containing Other Referenced Instances Sequence.
   *
   * Factored out of the SEG and PM converters
   * (https://github.com/QIICR/dcmqi/issues/192).
   */
  class SourceImageIndex {

  public:

    /**
     * @brief One referenceable entity of the source data.
     *
     * Either a classic single-frame instance, or a single frame of a
     * multiframe instance.
     */
    struct SourceFrame {
      size_t datasetIndex; ///< index into the dataset vector passed to the constructor
      Uint32 frameNumber;  ///< 1-based DICOM frame number; 0 for single-frame instances
      double position[3];  ///< ImagePositionPatient of the slice/frame
      bool hasPosition;    ///< whether the position could be determined
    };

    /**
     * @brief Build the frame inventory for the given source datasets.
     *
     * Datasets without usable position information are kept (they can still be
     * referenced as whole instances) but their frames are excluded from the
     * geometric slice mapping, with a warning.
     *
     * @param datasets Source image datasets. Not owned; must outlive this object.
     */
    explicit SourceImageIndex(const vector<DcmItem*>& datasets);

    /**
     * @brief Map each slice of the given image geometry to the source frames located on it.
     *
     * A source frame is assigned to the slice its ImagePositionPatient falls
     * into; frames mapping outside the image are skipped. Assumes the source
     * images have the same orientation as the image being converted.
     *
     * @param geometry Geometry of the ITK image being converted.
     * @return Per slice, the indices (into getFrames()) of the source frames on that slice.
     */
    vector<vector<size_t> > mapSlicesToFrames(const itk::ImageBase<3>& geometry) const;

    /**
     * @brief Add a Derivation Image item referencing the given source frames.
     *
     * Creates one Source Image Sequence item per referenced instance and
     * records the instances for populateCommonInstanceReference().
     *
     * @param fgder Derivation Image functional group to add the item to.
     * @param frameIds Indices (into getFrames()) of the source frames to reference,
     *        e.g. one slice entry of mapSlicesToFrames(). No-op if empty.
     * @param derivationCode Code describing the derivation (CID 7203).
     * @param derivationDescription Free-text description of the derivation, may be empty.
     * @param purposeOfReference Purpose of reference code (CID 7202).
     * @return EC_Normal on success, error otherwise.
     */
    OFCondition addDerivationImageItem(FGDerivationImage& fgder,
                                       const vector<size_t>& frameIds,
                                       const CodeSequenceMacro& derivationCode,
                                       const string& derivationDescription,
                                       const CodeSequenceMacro& purposeOfReference);

    /**
     * @brief Add a Derivation Image item referencing all source instances as a whole.
     *
     * No frame-level bookkeeping is performed (a reference without frame
     * numbers applies to all frames of an instance). Used when the geometric
     * mapping is bypassed. Instances are recorded for
     * populateCommonInstanceReference(); datasets that cannot be referenced
     * (e.g. missing SOP Class/Instance UID) are skipped.
     *
     * @param fgder Derivation Image functional group to add the item to.
     * @param derivationCode Code describing the derivation (CID 7203).
     * @param derivationDescription Free-text description of the derivation, may be empty.
     * @param purposeOfReference Purpose of reference code (CID 7202).
     * @return EC_Normal on success, error otherwise.
     */
    OFCondition addWholeInstanceDerivationImageItem(FGDerivationImage& fgder,
                                                    const CodeSequenceMacro& derivationCode,
                                                    const string& derivationDescription,
                                                    const CodeSequenceMacro& purposeOfReference);

    /**
     * @brief Populate the given Common Instance Reference module with the
     * instances referenced so far.
     *
     * Instances that belong to the study of the created object go into the
     * Referenced Series Sequence; instances from other studies are filed under
     * their Study Instance UID in the Studies Containing Other Referenced
     * Instances Sequence, as required by PS3.3 C.12.2. Within either, instances
     * are grouped by their Series Instance UID, in order of first reference.
     * No-op if no instance has been referenced.
     *
     * Source instances without a Study Instance UID, and all instances if
     * objectStudyInstanceUID is empty, are treated as belonging to the study
     * of the created object (with a warning for the former).
     *
     * @param commref Common Instance Reference module of the target object.
     * @param objectStudyInstanceUID Study Instance UID of the object being created.
     * @return EC_Normal on success, error otherwise.
     */
    OFCondition populateCommonInstanceReference(IODCommonInstanceReferenceModule& commref,
                                                const OFString& objectStudyInstanceUID) const;

    /**
     * @brief Access the frame inventory (indexed by the ids used in the mapping methods).
     */
    const vector<SourceFrame>& getFrames() const;

  private:

    /// Cached per-dataset identification, parallel to m_datasets
    struct InstanceInfo {
      OFString studyInstanceUID;
      OFString seriesInstanceUID;
      OFString sopClassUID;
      OFString sopInstanceUID;
      Uint32 numberOfFrames; ///< NumberOfFrames value; 0 for single-frame instances
    };

    /// Remember that an instance is referenced (deduplicated by dataset index)
    void recordReferencedInstance(size_t datasetIndex);

    /// Append one instance reference to the series list, creating the series
    /// item (tracked in series2item, owned by the list) on first use
    static OFCondition addToSeriesLevelReferences(
        OFVector<IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*>& refseries,
        map<OFString, IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*>& series2item,
        const InstanceInfo& info);

    vector<DcmItem*> m_datasets;
    vector<InstanceInfo> m_instances;
    vector<SourceFrame> m_frames;
    vector<size_t> m_referencedDatasets; ///< in order of first reference
    set<size_t> m_referencedDatasetSet;
  };

}

#endif // DCMQI_SOURCEIMAGEINDEX_H
