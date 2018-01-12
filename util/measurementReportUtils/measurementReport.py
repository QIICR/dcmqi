import json

from .measurementGroup import MeasurementGroup
from .measurementItem import MeasurementItem
from .codeSequences import CodeSequence

class MeasurementReport(object):
  """
  Data structure plus convenience methods to create measurment reports following
  the required format to be processed by the DCMQI tid1500writer tool (using the
  JSON export of this).
  """

  def __init__(self, seriesNumber, compositeContext, dicomSourceFileList, timePoint, 
               seriesDescription = "Measurements", procedureReported = None):
    self.SeriesDescription = str(seriesDescription)
    self.SeriesNumber = str(seriesNumber)
    self.InstanceNumber = "1"

    self.compositeContext = [compositeContext]

    self.imageLibrary = dicomSourceFileList

    self.observerContext = {
      "ObserverType": "PERSON",
      "PersonObserverName": "Reader01"
    }

    if procedureReported:
      self.procedureReported = procedureReported

    self.VerificationFlag = "VERIFIED"
    self.CompletionFlag = "COMPLETE"

    self.activitySession = "1"
    self.timePoint = str(timePoint)

    self.Measurements = []


  def addMeasurementGroup(self, measurementGroup):
    self.Measurements.append(measurementGroup)

  
  def exportToJson(self, fileName):
    with open(fileName, 'w') as fp:
      json.dump(self._getAsDict(), fp, indent = 2)


  def getJsonStr(self):
    return json.dumps(self._getAsDict(), indent = 2)


  def _getAsDict(self):
    # This is a bit of a hack to get the "@schema" in there, didn't figure out how to 
    # do this otherwise with json.dumps. If this wasn't needed I could just dump
    # the json directly with my custom encoder.
    jsonStr = json.dumps(self, indent = 2, cls = self._MyJSONEncoder)
    tempDict = json.loads(jsonStr)
    outDict = {}
    outDict["@schema"] = "https://raw.githubusercontent.com/qiicr/dcmqi/master/doc/schemas/sr-tid1500-schema.json#"
    outDict.update(tempDict)
    return outDict

  # Inner private class to define a custom JSON encoder for serializing MeasurmentReport
  class _MyJSONEncoder(json.JSONEncoder):
    def default(self, obj):
      if (isinstance(obj, MeasurementReport) or 
          isinstance(obj, MeasurementGroup) or
          isinstance(obj, MeasurementItem) or
          isinstance(obj, CodeSequence)):
        return obj.__dict__
      else:
        return super(MyEncoder, self).default(obj)