# Summary

`dcmqi` provides command line tools can be used to convert rasterized segmentations stored in commonly used research formats, such as NRRD or NIfTI, into DICOM Segmentation image storage (DICOM Segmentation) object.

DICOM Segmentations are organized as a list of _segments_, where each segment corresponds to a separate object/label being segmented. Segments can overlap (i.e., a single voxel of the source image can have multiple labels). Each segment contains information about what it describes, and what method was used to generate it. 

To perform the conversion to DICOM, the segmentation (image volume representing the labeling of the individual image voxels) needs to be accompanied by a JSON file that describes segmentation metadata, and by the DICOM dataset corresponding to the source image data being segmented. The source DICOM dataset will be used to populate metadata attributes that are inherited by the segmentation, such as information about patient and imaging study.

Conversion of DICOM Segmentation to research formats produces one file per segment saving the labeled image raster in the research format, such as NRRD or NIfTI, and a metadata JSON file.

Usage details for each of the converter follow.