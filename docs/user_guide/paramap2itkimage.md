# `paramap2itkimage`

This tool can be used to convert a DICOM Parametric Map Image object into ITK image format, and generate a JSON file holding meta information.

## Usage

```
   ./bin/paramap2itkimage  [--returnparameterfile <std::string>]
                           [--processinformationaddress <std::string>]
                           [--xml] [--echo] [-t <nrrd|mhd|mha|nii|nifti|hdr
                           |img>] [--] [--version] [-h] <std::string>
                           <std::string>


Where:

   --returnparameterfile <std::string>
     Filename in which to write simple return parameters (int, float,
     int-vector, etc.) as opposed to bulk return parameters (image,
     geometry, transform, measurement, table).

   --processinformationaddress <std::string>
     Address of a structure to store process information (progress, abort,
     etc.). (default: 0)

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
     (required)  Directory to store parametric map in an ITK format, and
     the JSON metadata file.
```

## Examples of DICOM Parametric map objects

You can experiment with the converter using the following objects:
* [ADC map image of the prostate](https://github.com/QIICR/dcmqi/raw/master/data/paramaps/pm-example-dcm.zip) (zip archive)