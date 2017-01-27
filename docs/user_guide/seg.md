# Summary

`dcmqi` provides command line tools to convert rasterized segmentations stored in commonly used research formats, such as NRRD or NIfTI, into DICOM Segmentation image storage (DICOM Segmentation) object.

DICOM Segmentations are organized as a lists of _segments_, where each segment corresponds to a separate object/label being segmented. Segments can overlap (i.e., a single voxel of the source image can have multiple labels). Each segment contains information about what it describes, and what method was used to generate it. 

To perform the conversion to DICOM, the segmentation (image volume representing the labeling of the individual image voxels) needs to be accompanied by a JSON file that describes segmentation metadata (such as the one in [this example](https://github.com/QIICR/dcmqi/blob/master/doc/examples/seg-example.json)), and by the DICOM dataset corresponding to the source image data being segmented. The source DICOM dataset is used to populate metadata attributes that are inherited by the segmentation (i.e., _composite context_), such as information about patient and imaging study.

Conversion from DICOM Segmentation to research formats produces one file per segment saving the labeled image raster in the research format, such as NRRD or NIfTI, and a metadata JSON file.