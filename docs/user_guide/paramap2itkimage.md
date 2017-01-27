# `paramap2itkimage`

This tool can be used to convert a DICOM Parametric Map Image object into ITK image format, and generate a JSON file holding meta information.

## Usage

```

   -t <nrrd|mhd|mha|nii|nifti|hdr|img>,  --outputType <nrrd|mhd|mha|nii
      |nifti|hdr|img>
     Output ITK format for the output image. (default: nrrd)

   --outputDirectory <std::string>
     Directory to store parametric map in an ITK format, and the JSON
     metadata file.

   --inputDICOM <std::string>
     File name of the DICOM Parametric map image.
```

## Examples of DICOM Parametric map objects

You can experiment with the converter using the following objects:
* [ADC map image of the prostate](https://github.com/QIICR/dcmqi/raw/master/data/paramaps/pm-example-dcm.zip) (zip archive)