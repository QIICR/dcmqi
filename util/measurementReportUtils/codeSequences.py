
class CodeSequence(object):

  def __init__(self, codeMeaning, codingSchemeDesignator, codeValue):
    self.CodeValue = codeValue
    self.CodingSchemeDesignator = codingSchemeDesignator
    self.CodeMeaning = codeMeaning


class Finding(CodeSequence):

  def __init__(self, segmentedStructure):
    if segmentedStructure == "NormalROI_PZ_1":
      super().__init__("Normal", "SCT", "17621005")
    elif segmentedStructure == "PeripheralZone":
      super().__init__("Entire", "SCT", "255503000")
    elif segmentedStructure == "TumorROI_PZ_1":
      super().__init__("Abnormal", "SCT", "263654008")
    elif segmentedStructure == "WholeGland":
      super().__init__("Entire Gland", "SCT", "714645007")
    else:
      raise ValueError("Segmented Structure Type {} is not supported yet. Build your own Finding code sequence using the class CodeSequence".format(segmentedStructure))
  

class FindingSite(CodeSequence):
  
  def __init__(self, segmentedStructure):
    if segmentedStructure == "NormalROI_PZ_1":
      super().__init__("Peripheral zone of the prostate", "SCT", "279706003")
    elif segmentedStructure == "PeripheralZone":
      super().__init__("Peripheral zone of the prostate", "SCT", "279706003")
    elif segmentedStructure == "TumorROI_PZ_1":
      super().__init__("Peripheral zone of the prostate", "SCT", "279706003")
    elif segmentedStructure == "WholeGland":
      super().__init__("Prostate", "SCT", "41216001")
    else:
      raise ValueError("Segmented Structure Type {} is not supported yet. Build your own FindingSite code sequence using the class CodeSequence".format(segmentedStructure))


class ProcedureReported(CodeSequence):

  def __init__(self, codeMeaning):
    if codeMeaning == "Multiparametric MRI of prostate":
      super().__init__("Multiparametric MRI of prostate", "DCM", "126021")
    else:
      raise ValueError("Procedure Type {} is not supported yet. Build your own ProcedureReported code sequence using the class CodeSequence".format(codeMeaning))

