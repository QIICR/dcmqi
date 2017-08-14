#include "dcmqi/TID1500Reader.h"


Json::Value DSRCodedEntryValue2CodeSequence(const DSRCodedEntryValue &value) {
  Json::Value codeSequence;
  codeSequence["CodeValue"] = value.getCodeValue().c_str();
  codeSequence["CodeMeaning"] = value.getCodeMeaning().c_str();
  codeSequence["CodingSchemeDesignator"] = value.getCodingSchemeDesignator().c_str();
  return codeSequence;
}

TID1500Reader::TID1500Reader(const DSRDocumentTree &tree)
  : DSRDocumentTree(tree) {
    /* check for expected template identification */
    if (!compareTemplateIdentification("1500", "DCMR"))
      CERR << "warning: template identification \"TID 1500 (DCMR)\" not found" << OFendl;

}

Json::Value TID1500Reader::getMeasurements() {
  Json::Value measurements(Json::arrayValue);

  std::map<std::string, DSRCodedEntryValue> string2code;
  string2code["activitySession"] = CODE_NCIt_ActivitySession;
  string2code["timePoint"] = CODE_UMLS_TimePoint;
  string2code["measurementMethod"] = CODE_SRT_MeasurementMethod;
  string2code["SourceSeriesForImageSegmentation"] = CODE_DCM_SourceSeriesForSegmentation;
  string2code["TrackingIdentifier"] = CODE_DCM_TrackingIdentifier;
  string2code["Finding"] = CODE_DCM_Finding;
  string2code["FindingSite"] = CODE_SRT_FindingSite;

  /* iterate over the document tree and print the measurements */
  if (gotoNamedChildNode(CODE_DCM_ImagingMeasurements)) {
    if (gotoNamedChildNode(CODE_DCM_MeasurementGroup)) {
      do {
        // Caveat: consider only the first measurement group!
        Json::Value measurementGroup;
        /* remember cursor to current content item (as a starting point) */
        const DSRDocumentTreeNodeCursor groupCursor(getCursor());
        for(std::map<std::string, DSRCodedEntryValue>::const_iterator mIt=string2code.begin();
            mIt!=string2code.end();++mIt){
          Json::Value value = getContentItem(mIt->second, groupCursor);
          if(value!=Json::nullValue)
            measurementGroup[mIt->first] = value;
        }

        initSegmentationContentItems(groupCursor, measurementGroup);

        /* details on measurement value(s) */
        Json::Value measurementItems(Json::arrayValue);
        DSRDocumentTreeNodeCursor cursor(groupCursor);
        if (cursor.gotoChild()) {
          size_t counter = 0;
          //COUT << "- Measurements:" << OFendl;
          /* iterate over all direct child nodes */
          do {
            const DSRDocumentTreeNode *node = cursor.getNode();
            /* and check for numeric measurement value content items */
            if ((node != NULL) && (node->getValueType() == VT_Num)) {
              //COUT << "  #" << (++counter) << " ";
              //printMeasurement(*OFstatic_cast(const DSRNumTreeNode *, node), cursor);
              Json::Value singleMeasurement = getSingleMeasurement(*OFstatic_cast(const DSRNumTreeNode *, node), cursor);
              measurementItems.append(singleMeasurement);
            }
          } while (cursor.gotoNext());

          measurementGroup["measurementItems"] = measurementItems;
          measurements.append(measurementGroup);
        }
      } while (gotoNextNamedNode(CODE_DCM_MeasurementGroup));
    }
  }
  return measurements;
}

Json::Value TID1500Reader::getContentItem(const DSRCodedEntryValue &conceptName,
                                          DSRDocumentTreeNodeCursor cursor)
{
  Json::Value contentValue;
  /* try to go to the given content item */
  if (gotoNamedChildNode(conceptName, cursor)) {
    const DSRDocumentTreeNode *node = cursor.getNode();
    if (node != NULL) {
      //COUT << "- " << conceptName.getCodeMeaning() << ": ";
      /* use appropriate value for output */
      switch (node->getValueType()) {
        case VT_Text:
          contentValue = OFstatic_cast(
          const DSRTextTreeNode *, node)->getValue().c_str();
          break;
        case VT_UIDRef:
          contentValue = OFstatic_cast(
          const DSRUIDRefTreeNode *, node)->getValue().c_str();
          break;
        case VT_Code:
          contentValue = DSRCodedEntryValue2CodeSequence(OFstatic_cast(
          const DSRCodeTreeNode *, node)->getValue());
          break;
        case VT_Image:
          contentValue = OFstatic_cast(
          const DSRImageTreeNode *, node)->getValue().getSOPInstanceUID().c_str();
          break;
        default:
          COUT << "Error: failed to find content item" << OFendl;
      }
    }
  }
  return contentValue;
}

void TID1500Reader::initSegmentationContentItems(DSRDocumentTreeNodeCursor cursor, Json::Value &json){
  if (gotoNamedChildNode(CODE_DCM_ReferencedSegment, cursor)) {
    const DSRDocumentTreeNode *node = cursor.getNode();
    std::string segmentationUID = OFstatic_cast(
    const DSRImageTreeNode *, node)->getValue().getSOPInstanceUID().c_str();
    OFVector <Uint16> items;
    json["segmentationSOPInstanceUID"] = segmentationUID;
    OFstatic_cast(
    const DSRImageTreeNode *, node)->getValue().getSegmentList().getItems(items);
    if(items.size())
      json["ReferencedSegment"] = items[0];
  }
}

Json::Value TID1500Reader::getSingleMeasurement(const DSRNumTreeNode &numNode,
                                     DSRDocumentTreeNodeCursor cursor) {
  Json::Value singleMeasurement;
  //COUT << numNode.getConceptName().getCodeMeaning() << ": " << numNode.getValue().getNumericValue() << " " << numNode.getValue().getMeasurementUnit().getCodeMeaning() << OFendl;
  singleMeasurement["value"] = numNode.getValue().getNumericValue().c_str();
  singleMeasurement["units"] = DSRCodedEntryValue2CodeSequence(numNode.getMeasurementUnit());
  singleMeasurement["quantity"] = DSRCodedEntryValue2CodeSequence(numNode.getConceptName());

  /* check for any modifiers */
  if (cursor.gotoChild()) {
    Json::Value measurementModifiers(Json::arrayValue);
    Json::Value derivationParameters(Json::arrayValue);

    /* iterate over all direct child nodes */
    do {
      const DSRDocumentTreeNode *node = cursor.getNode();
      if (node != NULL) {
        if ((node->getRelationshipType() == RT_hasConceptMod) && (node->getValueType() == VT_Code)) {
          //COUT << "     - " << node->getConceptName().getCodeMeaning() << ": " << OFstatic_cast(
          //const DSRCodeTreeNode *, node)->getCodeMeaning() << OFendl;

          // There is only one "Derivation" concept modifier (row 8 in TID1419)
          //  http://dicom.nema.org/medical/dicom/current/output/chtml/part16/chapter_A.html#sect_TID_1419
          // so it has the special treatment
          if (node->getConceptName() == CODE_DCM_Derivation){
            singleMeasurement["derivationModifier"] = DSRCodedEntryValue2CodeSequence(OFstatic_cast(
            const DSRCodeTreeNode *, node)->getValue());
          } else {
            // Otherwise, assume that modifier corresponds to row 6.
            // NB: as a consequence, this means other types of concept modifiers must be factored out
            //   and defined at the measurement group level (rows 1-4)
            Json::Value measurementModifier;
            measurementModifier["modifier"] = DSRCodedEntryValue2CodeSequence(node->getConceptName());
            measurementModifier["modifierValue"] = DSRCodedEntryValue2CodeSequence(OFstatic_cast(
            const DSRCodeTreeNode *, node)->getValue());
            measurementModifiers.append(measurementModifier);
          }
        }
        // TID1419 is extensible, and thus it is possible incoming document will have other "INFERRED FROM" items,
        //   in which case this heuristic will break.
        // R-INFERRED FROM are not handled
        else if ((node->getRelationshipType() == RT_inferredFrom) && (node->getValueType() == VT_Num)) {
          //COUT << "     - " << node->getConceptName().getCodeMeaning() << ": " << OFstatic_cast(const DSRNumTreeNode *, node)->getNumericValue()
          //    << " " << OFstatic_cast(const DSRNumTreeNode *, node)->getValue().getMeasurementUnit().getCodeMeaning() << OFendl;
          Json::Value derivationParameter;
          derivationParameter["derivationParameter"] = DSRCodedEntryValue2CodeSequence(node->getConceptName());
          derivationParameter["derivationParameterUnits"] = DSRCodedEntryValue2CodeSequence(OFstatic_cast(const DSRNumTreeNode *, node)->getValue().getMeasurementUnit());
          derivationParameter["derivationParameterValue"] = OFstatic_cast(const DSRNumTreeNode *, node)->getNumericValue().c_str();
          derivationParameters.append(derivationParameter);
        }
      }
    } while (cursor.gotoNext());

    if(measurementModifiers.size())
      singleMeasurement["measurementModifiers"] = measurementModifiers;
    if(derivationParameters.size())
      singleMeasurement["measurementDerivationParameters"] = derivationParameters;
  }

  return singleMeasurement;

}

size_t TID1500Reader::gotoNamedChildNode(const DSRCodedEntryValue &conceptName,
                          DSRDocumentTreeNodeCursor &cursor)
{
  size_t nodeID = 0;
  /* goto the first child node */
  if (conceptName.isValid() && cursor.gotoChild()) {
    const DSRDocumentTreeNode *node;
    /* iterate over all nodes on this level */
    do {
      node = cursor.getNode();
      /* and check for the desired concept name */
      if ((node != NULL) && (node->getConceptName() == conceptName))
        nodeID = node->getNodeID();
    } while ((nodeID == 0) && cursor.gotoNext());
  }
  return nodeID;
}
