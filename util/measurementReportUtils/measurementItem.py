
class MeasurementItem(object):

  def __init__(self, value):
    self.value = self.convertNumericToDcmtkFittingString(value)

  def convertNumericToDcmtkFittingString(self, value):
    if isinstance(value, float) or isinstance(value, int):
      s = str(value)
      if (len(s) <= 16):
        return s
      elif (s.find(".") >= 0) and (s.find(".") < 15):
        return s[:16]
      else:
        raise ValueError("Value cannot be converted to 16 digits without loosing too much precision!")
    else:
      raise TypeError("Value to convert is not of type float or int")


class VolumeMeasurementItem(MeasurementItem):

  def __init__(self, value):
    super().__init__(value)
    self.quantity = {
      "CodeValue": "118565006", 
      "CodingSchemeDesignator": "SCT", 
      "CodeMeaning": "Volume"
    }
    self.units = {
      "CodeValue": "cm3",
      "CodingSchemeDesignator": "UCUM",
      "CodeMeaning": "cubic centimeter"
    }


class MeanADCMeasurementItem(MeasurementItem):
  def __init__(self, value):
    super().__init__(value)
    self.quantity = {    
      "CodeValue": "113041",
      "CodingSchemeDesignator": "DCM",
      "CodeMeaning": "Apparent Diffusion Coefficient"
    }
    self.units = {
      "CodeValue": "um2/s",
      "CodingSchemeDesignator": "UCUM",
      "CodeMeaning": "um2/s"
    }
    self.derivationModifier = {
      "CodeValue": "373098007",
      "CodingSchemeDesignator": "SCT",
      "CodeMeaning": "Mean"
    }
