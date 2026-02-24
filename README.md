# Highdicom #393

Version to check issue reported in highghdicom #393
You can use the itkimage2segimage in the following way, to process
the NIFTI file without having access to the base images.

itkimage2segimage --inputImageList organ_autoseg.nii \
 --inputDICOMList <any-dicom-file> \
 --inputMetadata meta.json  \
 --outputDICOM output.dcm

organ_autoseg.nii is the file provided in the highdicom issue tracker.
<any-dicom-file> can be any DICOM image file. It is not really used
but for now must be provided as a dummy.

meta.json is the source code main directory. I just re-used an
existing meta file and modified it to match the number of segments.

output.dcm will have the binary DICOM segmentation.

Slicer will not be able to open the segmentation output, probably
because the base images are missing.

However, you can render all frames of the output using one DCMTK
tool:

dcm2pnm +Fa +ob output.dcm render.bmp

This will quickly create > 500 BMP images each representing a frame
from the output segmentation.

From what I can see, the renderings look good.

The roundtrip also seems to work:

segimage2itkimage --outputDirectory output --inputDICOM output.dcm --mergeSegments

The resulting NRRD file can be loaded into Slicer. A quick scroll over the segments
looks valid. See [video](segmentation_after_roundtrip_dcmqi.mp4).