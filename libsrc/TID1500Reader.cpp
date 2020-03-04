#include "dcmqi/TID1500Reader.h"

DSRCodedEntryValue json2cev(Json::Value& j){
  return DSRCodedEntryValue(j["CodeValue"].asCString(),
    j["CodingSchemeDesignator"].asCString(),
    j["CodeMeaning"].asCString());
}

Json::Value DSRCodedEntryValue2CodeSequence(const DSRCodedEntryValue &value) {
  Json::Value codeSequence;
  codeSequence["CodeValue"] = value.getCodeValue().c_str();
  codeSequence["CodeMeaning"] = value.getCodeMeaning().c_str();
  codeSequence["CodingSchemeDesignator"] = value.getCodingSchemeDesignator().c_str();
  return codeSequence;
}

TID1500Reader::TID1500Reader(const DSRDocumentTree &tree)
  : DSRDocumentTree(tree) {
    // check for expected template identification
    if (!compareTemplateIdentification("1500", "DCMR"))
      std::cerr << "warning: template identification \"TID 1500 (DCMR)\" not found" << OFendl;

}

Json::Value TID1500Reader::getProcedureReported(){
  Json::Value codeSequence;
  if (gotoNamedNode(CODE_DCM_ProcedureReported)){
    DSRDocumentTreeNodeCursor cursor(getCursor());
    const DSRDocumentTreeNode *node = cursor.getNode();
    codeSequence = DSRCodedEntryValue2CodeSequence(OFstatic_cast(
    const DSRCodeTreeNode *, node)->getValue());
  }
  return codeSequence;
}

Json::Value TID1500Reader::getObserverContext(){
  Json::Value observerContext;
  DSRDocumentTreeNodeCursor rootCursor;
  getCursorToRootNode(rootCursor);
  Json::Value observerType = getContentItem(CODE_DCM_ObserverType, rootCursor);
  if(observerType == Json::nullValue){
    std::cout << "Observer context not initialized!" << std::endl;
    return Json::nullValue;
  }
  if(json2cev(observerType) == CODE_DCM_Person){
    observerContext["ObserverType"] = "PERSON";
    Json::Value value = getContentItem(CODE_DCM_PersonObserverName, rootCursor);
    if(value!=Json::nullValue)
      observerContext["PersonObserverName"] = value;
  } else if(json2cev(observerType) == CODE_DCM_Device){
    observerContext["ObserverType"] = "DEVICE";
    Json::Value value = getContentItem(CODE_DCM_DeviceObserverUID, rootCursor);
    if(value!=Json::nullValue)
      observerContext["DeviceObserverUID"] = value;
    value = getContentItem(CODE_DCM_DeviceObserverName, rootCursor);
    if(value!=Json::nullValue)
      observerContext["DeviceObserverName"] = value;
    value = getContentItem(CODE_DCM_DeviceObserverManufacturer, rootCursor);
    if(value!=Json::nullValue)
      observerContext["DeviceObserverManufacturer"] = value;
    value = getContentItem(CODE_DCM_DeviceObserverSerialNumber, rootCursor);
      if(value!=Json::nullValue)
        observerContext["DeviceObserverSerialNumber"] = value;
  }
  return observerContext;
}

Json::Value TID1500Reader::getMeasurements() {
  Json::Value measurements(Json::arrayValue);

  // These modifiers are expected to be defined at the level of measurement group
  std::map<std::string, DSRCodedEntryValue> string2code;
  string2code["activitySession"] = CODE_NCIt_ActivitySession;
  string2code["timePoint"] = CODE_UMLS_TimePoint;
  string2code["measurementMethod"] = CODE_SCT_MeasurementMethod;
  string2code["SourceSeriesForImageSegmentation"] = CODE_DCM_SourceSeriesForSegmentation;
  string2code["TrackingIdentifier"] = CODE_DCM_TrackingIdentifier;
  string2code["TrackingUniqueIdentifier"] = CODE_DCM_TrackingUniqueIdentifier;
  string2code["Finding"] = CODE_DCM_Finding;
  string2code["FindingSite"] = CODE_SCT_FindingSite;

  std::vector<DSRCodedEntryValue> knownConcepts;
  for(std::map<std::string, DSRCodedEntryValue>::const_iterator mIt=string2code.begin();
        mIt!=string2code.end();++mIt){
    knownConcepts.push_back(mIt->second);
  }
  knownConcepts.push_back(CODE_SRT_MeasurementMethod);
  knownConcepts.push_back(CODE_SRT_FindingSite);

  std::vector<DSRCodedEntryValue> algorithmIdentificationConcepts;
  algorithmIdentificationConcepts.push_back(CODE_DCM_AlgorithmName);
  algorithmIdentificationConcepts.push_back(CODE_DCM_AlgorithmVersion);
  algorithmIdentificationConcepts.push_back(CODE_DCM_AlgorithmParameters);

  const DSRDocumentTreeNodeCursor cursor(getCursor());

  // iterate over the document tree and read the measurements
  if (gotoNamedNode(CODE_DCM_ImagingMeasurements)) {
    if (gotoNamedChildNode(CODE_DCM_MeasurementGroup)) {
      do {
        // Caveat: we consider only the first measurement group!
        Json::Value measurementGroup;
        // remember cursor to current content item (as a starting point)
        const DSRDocumentTreeNodeCursor groupCursor(getCursor());
        for(std::map<std::string, DSRCodedEntryValue>::const_iterator mIt=string2code.begin();
            mIt!=string2code.end();++mIt){
          Json::Value value = getContentItem(mIt->second, groupCursor);
          if(value!=Json::nullValue)
            measurementGroup[mIt->first] = value;
        }

        // NB: only the first AlgorithmParameters item is considered
        {
          Json::Value algorithmName = getContentItem(CODE_DCM_AlgorithmName, groupCursor);
          Json::Value algorithmVersion = getContentItem(CODE_DCM_AlgorithmVersion, groupCursor);
          Json::Value algorithmParameters = getContentItem(CODE_DCM_AlgorithmParameters, groupCursor);
          if(algorithmName!=Json::nullValue){
            if(algorithmVersion == Json::nullValue){
              std::cerr << "ERROR: AlgorithmName is present, but AlgorithmVersion is not!" << std::endl;
            }
            measurementGroup["measurementAlgorithmIdentification"]["AlgorithmName"] = algorithmName;
            measurementGroup["measurementAlgorithmIdentification"]["AlgorithmVersion"] = algorithmVersion;
          }
          if(algorithmParameters!=Json::nullValue){
            measurementGroup["measurementAlgorithmIdentification"]["AlgorithmParameters"] = Json::arrayValue;
            measurementGroup["measurementAlgorithmIdentification"]["AlgorithmParameters"].append(algorithmParameters);
          }
        }

        initSegmentationContentItems(groupCursor, measurementGroup);

        // details on measurement value(s)
        Json::Value measurementItems(Json::arrayValue);
        Json::Value qualitativeEvaluations(Json::arrayValue);
        DSRDocumentTreeNodeCursor cursor(groupCursor);

        if (cursor.gotoChild()) {
          //size_t counter = 0;
          //COUT << "- Measurements:" << OFendl;
          // iterate over all direct child nodes
          do {
            const DSRDocumentTreeNode *node = cursor.getNode();

            {
              Json::Value laterality = Json::nullValue;
              if(node->getConceptName() == CODE_SCT_FindingSite)
                laterality = getContentItem(CODE_SCT_Laterality, cursor);
              else if(node->getConceptName() == CODE_SRT_FindingSite)
                laterality = getContentItem(CODE_SRT_Laterality, cursor);
              if(laterality!=Json::nullValue)
                measurementGroup["Laterality"] = laterality;
            }

            /* and check for numeric measurement value content items */
            if ((node != NULL) && (node->getValueType() == VT_Num)) {
              //COUT << "  #" << (++counter) << " ";
              //printMeasurement(*OFstatic_cast(const DSRNumTreeNode *, node), cursor);
              Json::Value singleMeasurement = getSingleMeasurement(*OFstatic_cast(const DSRNumTreeNode *, node), cursor);
              measurementItems.append(singleMeasurement);
            } else if (( node != NULL) && (node->getValueType() == VT_Text)) {
              // check if the concept assigned to this item is not in the list of concepts
              // that can be encountered otherwise (same for the below)
              // If not, then conclude this is a qualitative evaluation item

              if(find(knownConcepts.begin(), knownConcepts.end(), node->getConceptName()) == knownConcepts.end() &&
              find(algorithmIdentificationConcepts.begin(), algorithmIdentificationConcepts.end(), node->getConceptName()) == algorithmIdentificationConcepts.end()){
                Json::Value singleQualitativeEvaluation;
                std::cout << "Found concept that is not known, and as such is qualitative: " << node->getConceptName() << std::endl;
                singleQualitativeEvaluation["conceptCode"] = DSRCodedEntryValue2CodeSequence(node->getConceptName());
                singleQualitativeEvaluation["conceptValue"] = OFstatic_cast(
                const DSRTextTreeNode *, node)->getValue().c_str();
                qualitativeEvaluations.append(singleQualitativeEvaluation);
              }
            } else if (( node != NULL) && (node->getValueType() == VT_Code)) {
              if(find(knownConcepts.begin(), knownConcepts.end(), node->getConceptName()) == knownConcepts.end() &&
              find(algorithmIdentificationConcepts.begin(), algorithmIdentificationConcepts.end(), node->getConceptName()) == algorithmIdentificationConcepts.end() ){
                Json::Value singleQualitativeEvaluation;
                singleQualitativeEvaluation["conceptCode"] = DSRCodedEntryValue2CodeSequence(node->getConceptName());
                singleQualitativeEvaluation["conceptValue"] = DSRCodedEntryValue2CodeSequence(OFstatic_cast(
                const DSRCodeTreeNode *, node)->getValue());
                qualitativeEvaluations.append(singleQualitativeEvaluation);
              }
            }
          } while (cursor.gotoNext());

          measurementGroup["measurementItems"] = measurementItems;
          if(qualitativeEvaluations.size())
            measurementGroup["qualitativeEvaluations"] = qualitativeEvaluations;
          measurements.append(measurementGroup);
        }
      } while (gotoNextNamedNode(CODE_DCM_MeasurementGroup, OFFalse /*searchIntoSub*/));
    }
  }
  return measurements;
}

Json::Value TID1500Reader::getContentItem(const DSRCodedEntryValue &conceptName,
                                          DSRDocumentTreeNodeCursor cursor)
{
  Json::Value contentValue;
  // try to go to the given content item
  if (gotoNamedChildNode(conceptName, cursor)) {
    const DSRDocumentTreeNode *node = cursor.getNode();
    if (node != NULL) {
      //COUT << "  - " << conceptName.getCodeMeaning() << ": ";
      // use appropriate value for output
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
        case VT_PName:
          // TODO: investigate why roundtrip JSON test didn't detect that
          //  observer name was not recovered!
          contentValue = OFstatic_cast(
          const DSRPNameTreeNode *, node)->getValue().c_str();
          break;
        default:
          std::cout << "Error: failed to find content item for " << conceptName.getCodeMeaning() << OFendl;
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

  // check for any modifiers
  if (cursor.gotoChild()) {

    Json::Value measurementModifiers(Json::arrayValue);
    Json::Value derivationParameters(Json::arrayValue);
    Json::Value populationDescription = Json::nullValue;
    Json::Value measurementNumProperties(Json::arrayValue);

    // iterate over all direct child nodes
    do {
      const DSRDocumentTreeNode *node = cursor.getNode();
      if (node != NULL) {
        if ((node->getRelationshipType() == RT_hasConceptMod) && (node->getValueType() == VT_Code)) {
          //COUT << "     - " << node->getConceptName().getCodeMeaning() << ": " << OFstatic_cast(
          //const DSRCodeTreeNode *, node)->getCodeMeaning() << OFendl;

          // There is only one "Derivation" concept modifier (row 8 in TID1419)
          //  http://dicom.nema.org/medical/dicom/current/output/chtml/part16/chapter_A.html#sect_TID_1419
          // so it has the special treatment
          if (node->getConceptName() == CODE_DCM_Derivation) {
            singleMeasurement["derivationModifier"] = DSRCodedEntryValue2CodeSequence(OFstatic_cast(
            const DSRCodeTreeNode *, node)->getValue());
          } else if (node->getConceptName() == CODE_SCT_FindingSite || node->getConceptName() == CODE_SRT_FindingSite) {
            std::cerr << "Warning: For now, FindingSite modifier is interpreted only at the MeasurementGroup level." << OFendl;
          } else if (node->getConceptName() == CODE_SCT_MeasurementMethod || node->getConceptName() == CODE_SRT_MeasurementMethod) {
            std::cerr << "Warning: For now, Measurement Method modifier is interpreted only at the MeasurementGroup level." << OFendl;
          } else if (node->getValueType() == VT_Code) {
            // Otherwise, assume that modifier corresponds to row 6.
            // NB: as a consequence, this means other types of concept modifiers must be factored out
            //   and defined at the measurement group level (rows 1-4)
            Json::Value measurementModifier;
            measurementModifier["modifier"] = DSRCodedEntryValue2CodeSequence(node->getConceptName());
            measurementModifier["modifierValue"] = DSRCodedEntryValue2CodeSequence(OFstatic_cast(
            const DSRCodeTreeNode *, node)->getValue());
            measurementModifiers.append(measurementModifier);
          }
        } else if ((node->getRelationshipType() == RT_hasConceptMod) && (node->getValueType() == VT_Text)) {
          if (node->getConceptName() == CODE_DCM_AlgorithmName) {
            singleMeasurement["measurementAlgorithmIdentification"] = Json::Value();
            singleMeasurement["measurementAlgorithmIdentification"]["AlgorithmName"] =
              OFstatic_cast(const DSRTextTreeNode *, node)->getValue().c_str();
          }
          // This node must show up after algorithm name! (and this is required, since
          //   the order is significant in the template)
          if (node->getConceptName() == CODE_DCM_AlgorithmVersion) {
            singleMeasurement["measurementAlgorithmIdentification"]["AlgorithmVersion"] =
              OFstatic_cast(const DSRTextTreeNode *, node)->getValue().c_str();
          }
          if (node->getConceptName() == CODE_DCM_AlgorithmParameters) {
            if(!singleMeasurement["measurementAlgorithmIdentification"].isMember("AlgorithmParameters"))
              singleMeasurement["measurementAlgorithmIdentification"]["AlgorithmParameters"] = Json::arrayValue;
            singleMeasurement["measurementAlgorithmIdentification"]["AlgorithmParameters"].append(OFstatic_cast(const DSRTextTreeNode *, node)->getValue().c_str());
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

        else if (node->getRelationshipType() == RT_hasProperties && (node->getValueType() == VT_Text)) {
          if (node->getConceptName() == CODE_DCM_PopulationDescription) {
            populationDescription = OFstatic_cast(const DSRTextTreeNode *, node)->getValue().c_str();
          }
        }

        else if (node->getRelationshipType() == RT_hasProperties && (node->getValueType() == VT_Num)) {
          Json::Value measurementNumProperty;
          measurementNumProperty["numProperty"] = DSRCodedEntryValue2CodeSequence(node->getConceptName());
          measurementNumProperty["numPropertyUnits"] = DSRCodedEntryValue2CodeSequence(OFstatic_cast(const DSRNumTreeNode *, node)->getValue().getMeasurementUnit());
          measurementNumProperty["numPropertyValue"] = OFstatic_cast(const DSRNumTreeNode *, node)->getNumericValue().c_str();
          measurementNumProperties.append(measurementNumProperty);
        }
      }
    } while (cursor.gotoNext());

    if(measurementModifiers.size())
      singleMeasurement["measurementModifiers"] = measurementModifiers;
    if(derivationParameters.size())
      singleMeasurement["measurementDerivationParameters"] = derivationParameters;
    if(populationDescription != Json::nullValue){
      singleMeasurement["measurementPopulationDescription"] = populationDescription;
    }
    if(measurementNumProperties.size())
      singleMeasurement["measurementNumProperties"] = measurementNumProperties;

  }

  return singleMeasurement;

}

size_t TID1500Reader::gotoNamedChildNode(const DSRCodedEntryValue &conceptName,
                          DSRDocumentTreeNodeCursor &cursor)
{
  size_t nodeID = 0;
  // goto the first child node
  if (conceptName.isValid() && cursor.gotoChild()) {
    const DSRDocumentTreeNode *node;
    // iterate over all nodes on this level
    do {
      node = cursor.getNode();
      // and check for the desired concept name
      if ((node != NULL) && (node->getConceptName() == conceptName))
        nodeID = node->getNodeID();
    } while ((nodeID == 0) && cursor.gotoNext());
  }
  return nodeID;
}
