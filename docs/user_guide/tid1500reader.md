# `tid1500reader`

This tool can be used to convert a DICOM Structured Report object that follows [template TID1500](http://dicom.nema.org/medical/dicom/current/output/chtml/part16/chapter_A.html#sect_TID_1500) into a JSON representation of the measurements. The converter was developed and tested specifically to recognize SR TID1500 objects that store measurements derived from volumetric rasterized segmentations. It will not work for other use cases of TID1500.

## Usage

```
   --xml
     Produce xml description of command line arguments (default: 0)

   --echo
     Echo the command line arguments (default: 0)

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.

   <std::string>
     (required)  File name of the DICOM SR TID1500 object.

   <std::string>
     (required)  File name of the JSON file that will keep the metadata and
     measurements information.
```

## Example objects

* [measurements over liver segmentation](https://fedorov.gitbooks.io/rsna2016-qirr-dicom4qi/content/instructions/sr-tid1500.html#test-dataset-1)
* [measurements for multiple regions segmented from PET](https://fedorov.gitbooks.io/rsna2016-qirr-dicom4qi/content/instructions/sr-tid1500.html#test-dataset-2)