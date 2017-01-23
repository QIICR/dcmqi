# itkimage2paramap

`itkimage2paramap` can be used to convert a parametric map provided in any of the formats supported by ITK, such as NRRD or NIFTI, as a DICOM Parametric Map image object.

* `--inputFileName`: file name of the parametric map image in a format readable by ITK (NRRD, NIfTI, MHD, etc.).
* `--dicomImageFileName`: file name of the DICOM image file which has been used as a reference image while creating this parametric map
* `--metaDataFileName`: file names of the text files containing metadata attributes.
* `--outputParaMapFileName`: file name of the parametric map object that will keep the result.

Most of the effort will be required to populate the content of the meta-information JSON file. Its structure is defined by [this](https://github.com/QIICR/dcmqi/blob/master/doc/schemas/pm-schema.json) JSON-Schema file. Interpretation of JSON-Schema may require some effort, especially considering that this particular file uses externally defined items. It may be easier to start with an example JSON file that "instantiates" this schema, such as [this one](https://github.com/QIICR/dcmqi/blob/master/doc/examples/pm-example.json).

In the following, we will guide you through the contents of this file - line by line.

```JSON
{
  "@schema": "https://raw.githubusercontent.com/qiicr/dcmqi/master/doc/schemas/pm-schema.json#",
```


