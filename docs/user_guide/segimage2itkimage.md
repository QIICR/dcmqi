# `segimage2itkimage`

This tool can be used to convert DICOM Segmentation into volumetric segmentations stored as labeled pixels using research format, such as NRRD or NIfTI, and meta information stored in the JSON file format.

## Usage

```

   -t <nrrd|mhd|mha|nii|nifti|hdr|img>,  --outputType <nrrd|mhd|mha|nii
      |nifti|hdr|img>
     Output file format for the resulting image data. (default: nrrd)

   -p <std::string>,  --prefix <std::string>
     Prefix for output file.

   --outputDirectory <std::string>
     Directory to store individual segments saved using the output format
     specified files. When specified, file names will contain prefix,
     followed by the segment number.

   --inputDICOM <std::string>
     File name of the input DICOM Segmentation image object.
```

# Examples of DICOM Segmentation objects

You can experiment with the converter on the following sample DICOM Segmentation datasets:
* [liver tissue segmentation](https://github.com/QIICR/dcmqi/raw/master/data/ct-3slice/liver.dcm); [corresponding image data (files named 01.dcm, 02.dcm and 03.dcm)](https://github.com/QIICR/dcmqi/tree/master/data/ct-3slice)
* [lung lesion segmentation](http://slicer.kitware.com/midas3/download/item/245783/legacy.dcm); [corresponding image series](http://slicer.kitware.com/midas3/download/item/245513/LIDC-IDRI-0314-CT.zip)