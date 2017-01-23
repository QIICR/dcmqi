# itkimage2paramap

`itkimage2paramap` can be used to convert a parametric map provided in any of the formats supported by ITK, such as NRRD or NIFTI, as a DICOM Parametric Map image object.

* `--inputFileName`: file name of the parametric map image in a format readable by ITK (NRRD, NIfTI, MHD, etc.).
* `--dicomImageFileName`: file name of the DICOM image file which has been used as a reference image while creating this parametric map
* `--metaDataFileName`: file names of the text files containing metadata attributes.
* `--outputParaMapFileName`: file name of the parametric map object that will keep the result.

