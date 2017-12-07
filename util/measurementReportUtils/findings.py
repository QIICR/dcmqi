
class GenericFindingStruct(object):

  def __init__(self, codeMeaning, codingSchemeDesignator, codeValue):
    self.CodeValue = codeValue
    self.CodingSchemeDesignator = codingSchemeDesignator
    self.CodeMeaning = codeMeaning


class Finding(GenericFindingStruct):

  def __init__(self, segmentedStructure):
    if segmentedStructure == "NormalROI_PZ_1":
      super().__init__("Normal", "SRT", "G-A460")
    elif segmentedStructure == "PeripheralZone":
      super().__init__("Entire", "SRT", "R-404A4")
    elif segmentedStructure == "TumorROI_PZ_1":
      super().__init__("Abnormal", "SRT", "R-42037")
    elif segmentedStructure == "WholeGland":
      super().__init__("Entire Gland", "SRT", "T-F6078")
    else:
      raise ValueError("Segmented Structure Type {} is not supported yet. Build your own Finding object using GenericFindingStruct".format(segmentedStructure))
  

class FindingSite(GenericFindingStruct):
  
  def __init__(self, segmentedStructure):
    if segmentedStructure == "NormalROI_PZ_1":
      super().__init__("Peripheral zone of the prostate", "SRT", "T-D05E4")
    elif segmentedStructure == "PeripheralZone":
      super().__init__("Peripheral zone of the prostate", "SRT", "T-D05E4")
    elif segmentedStructure == "TumorROI_PZ_1":
      super().__init__("Peripheral zone of the prostate", "SRT", "T-D05E4")
    elif segmentedStructure == "WholeGland":
      super().__init__("Prostate", "SRT", "T-9200B")
    else:
      raise ValueError("Segmented Structure Type {} is not supported yet. Build your own FindingSite object using GenericFindingStruct".format(segmentedStructure))



