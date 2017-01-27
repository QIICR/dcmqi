# `itkimage2segimage`

`itkimage2segimage` tool can be used to save the volumetric segmentation(s) stored as labeled pixels using any of the formats supported by ITK, such as NRRD or NIFTI, as a DICOM Segmentation Object (further referred to as SEG).

## Usage

```

   --skip
     Skip empty slices while encoding segmentation image. By default, empty
     slices will not be encoded, resulting in a smaller output file size.
     (default: 0)

   --inputImageList <std::vector<std::string>>
     Comma-separated list of file names of the segmentation images in a
     format readable by ITK (NRRD, NIfTI, MHD, etc.). Each of the
     individual files can contain one or more labels (segments). Segments
     from different files are allowed to overlap. See documentation for
     details.

   --inputDICOMList <std::vector<std::string>>
     Comma-separated list of DICOM images that correspond to the original
     image that was segmented. This means you must have access to the
     original data in DICOM in order to use the converter (at least for
     now).

   --outputDICOM <std::string>
     File name of the DICOM SEG object that will store the result of
     conversion.

   --inputMetadata <std::string>
     JSON file containing the meta-information that describes the
     measurements to be encoded. See documentation for details.
```

## Detailed usage

Most of the effort will be required to populate the content of the meta-information JSON file. You can use the [helper web application](http://qiicr.org/dcmqi/#/seg) that provides a user interface to help with populating the content of the metadata JSON file. The details are discussed below.

The structure of the metadata JSON is defined by [this](https://github.com/QIICR/dcmqi/blob/master/doc/schemas/seg-schema.json) JSON-Schema file. Interpretation of JSON-Schema may require some effort, especially considering that this particular file uses externally defined items. It may be easier to start with an example JSON file that "instantiates" this schema, such as [this one](https://github.com/QIICR/dcmqi/blob/master/doc/examples/seg-example.json).

In the following, we will guide you through the contents of this file - line by line.

```JSON
{
  "@schema": "https://raw.githubusercontent.com/qiicr/dcmqi/master/doc/schemas/seg-schema.json#",
```

This opening line references the schema this parameter file should conform to. Make sure you include this line without changes!

```JSON
  "ContentCreatorName": "Doe^John",
  "ClinicalTrialSeriesID": "Session1",
  "ClinicalTrialTimePointID": "1",
  "ClinicalTrialCoordinatingCenterName": "BWH",
  "SeriesDescription": "Segmentation",
  "SeriesNumber": "300",
  "InstanceNumber": "1",
```

These lines correspond to the metadata attributes that will be populated in the resulting DICOM SEG object. It is your choice how you want to populate those. There are certain constraints on the values of these attributes. If those constraints are not met, converter will fail. In the future, we will provide instructions for validating your meta-information file.

```JSON
  "segmentAttributes": [
    [
      {
```

The remainder of the file is a nested list (top-level list corresponds to the input segmentation files, and the inner list corresponds to the individual segments within each file) that specifies metadata attributes for each of the segments that are present in the input segmentation files.

For each of the segments, you will need to specify the following attributes that are mandatory:

```JSON
        "LabelID": 1,
```

`LabelID` defines the value of the segment in the segmentation file that will be assigned attributes listed.

```JSON
        "SegmentDescription": "Liver Segmentation",
```

`SegmentDescription` is a short free-text description of the segment.

```JSON
        "SegmentAlgorithmType": "SEMIAUTOMATIC",
```

`SegmentAlgorithmType` can be assigned to `MANUAL`, `SEMIAUTOMATIC` or `AUTOMATIC`. If the value of this attribute is not `MANUA`, `SegmentAlgorithmName` attribute is required to be initialized!

```JSON
        "SegmentAlgorithmName": "SlicerEditor",
```

This attribute should be used to assign short name of the algorithm used to perform the segmentation.

```JSON
        "recommendedDisplayRGBValue": [
          221,
          130,
          101
        ]
```

This attribute can be used to specify the RGB color with the recommended. Alternatively, `RecommendedDisplayCIELabValue` attribute can be used to specify the color in CIELab color space.

```JSON        
        "SegmentedPropertyCategoryCodeSequence": {
          "CodeValue": "T-D0050",
          "CodingSchemeDesignator": "SRT",
          "CodeMeaning": "Tissue"
        },
        "SegmentedPropertyTypeCodeSequence": {
          "CodeValue": "T-62000",
          "CodingSchemeDesignator": "SRT",
          "CodeMeaning": "Liver"
        },
```

`SegmentedPropertyCategoryCodeSequence` and `SegmentedPropertyCategoryCodeSequence` are attributes that should be assigned code tuples to describe the meaning of what is being segmented. 

Each code tuple consists of the three components:  `CodeValue`, `CodingSchemeDesignator` and `CodeMeaning`. `CodingSchemeDesignator` defines the "authority", or source of the code. Each `CodeValue` should be unique for a given `CodingSchemeDesignator`. `CodeMeaning` is a human-readable meaning of the code. DICOM defines several coding schemes recognized by the standard listed [in PS3.16 Section 8](http://dicom.nema.org/medical/dicom/current/output/chtml/part16/chapter_8.html). 

The task of selecting a code to describe a given segment may not be trivial, since there are implicit constraints/expectations on the values of these codes. As an example, the possible values of `SegmentedPropertyTypeCodeSequence` are predicated on the value of the `SegmentedPropertyCategoryCodeSequence`. It is also possible to define `SegmentedPropertyTypeModifierCodeSequence` that can be used , for example, to define the laterality. In some situations, it is appropriate or required to also specify anatomical location of the segmentation (e.g., organ a tumor was segmented). The latter can be achieved using `AnatomicRegionSequence` and `AnatomicRegionModifierSequence` coded attributes.

To simplify selection of codes for defining semantics of the segment, we provide a [helper web application](http://qiicr.org/dcmqi/#/seg) that can be used to browse supported codes and automatically generate the corresponding section of the JSON file. When no suitable codes can be found, it is also permissible to define so called _private_, or local, coding schemes (see [PS3.16 Section 8.2](http://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_8.2.html)). 