# `paramap2itkimage`

This tool can be used to convert a DICOM Parametric Map Image object into ITK image format, and generate a JSON file holding meta information.

```
   --xml
     Produce xml description of command line arguments (default: 0)

   --echo
     Echo the command line arguments (default: 0)

   -t <nrrd|mhd|mha|nii|nifti|hdr|img>,  --outputType <nrrd|mhd|mha|nii
      |nifti|hdr|img>
     Output ITK format for the output image. (default: nrrd)

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.

   <std::string>
     (required)  File name of the DICOM Parametric map image.

   <std::string>
     (required)  Directory to store parametric map in an ITK format.
```

# Examples of DICOM Parametric map objects

You can experiment with the converter using the following objects:
* [ADC map image of the prostate](https://github.com/QIICR/dcmqi/raw/master/data/paramaps/pm-example-dcm.zip) (zip archive)