
class MeasurementGroup(object):
  """
  Data structure plus convenience methods to create measurment groups following
  the required format to be processed by the DCMQI tid1500writer tool. Use this
  to populate the Measurements list in :class:`MeasurementReport`.
  """

  def __init__(self, 
               trackingIdentifier, trackingUniqueIdentifier, referencedSegment, 
               sourceSeriesInstanceUID, segmentationSOPInstanceUID,
               finding, findingSite):
    self.TrackingIdentifier = trackingIdentifier
    self.TrackingUniqueIdentifier = trackingUniqueIdentifier
    self.ReferencedSegment = referencedSegment
    self.SourceSeriesForImageSegmentation = sourceSeriesInstanceUID
    self.segmentationSOPInstanceUID = segmentationSOPInstanceUID
    self.Finding = finding
    self.FindingSite = findingSite
    self.measurementItems = []

  def addMeasurementItem(self, measurementItem):
    self.measurementItems.append(measurementItem)

    


