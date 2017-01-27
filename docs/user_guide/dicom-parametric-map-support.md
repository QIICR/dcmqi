`dcmqi` provides command line tools to convert results of post-processing of the image data, such as by applying certain model to the data, into DICOM format. As an example, Apparent Diffusion Coefficient (ADC) maps derived by fitting various models to the Diffusion-Weighted Magnetic Resonance Imaging (DW-MRI) data have been shown promising in characterizing aggressiveness of prostate cancer. The result of conversion is [DICOM Parametric map object](http://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_A.75.html).

Mandatory metadata that needs to be specified to enable conversion include:
* Quantity being measured
* Units of the quantity being measured
* Measurement method

Each of these items, in addition to some other attributes, must be specified using coded values. An example of the metadata file is available [here](https://github.com/QIICR/dcmqi/blob/master/doc/examples/pm-example.json).