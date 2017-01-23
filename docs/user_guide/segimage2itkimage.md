# segimage2itkimage

`segimage2itkimage` can be used to convert the DICOM Segmentation Object into volumetric segmentation(s) stored as labeled pixels using any of the formats supported by ITK, such as NRRD or NIFTI + meta information stored in the JSON file format.

* `--inputSEGFileName`: file name of the DICOM Segmentation Object
* `--prefix`: prefix for output files
* `--outputDirName`: directory to store individual segments as NRRD files. File names will correspond to the segment numbers

