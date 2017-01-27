# `itkimage2paramap`

`itkimage2paramap` can be used to convert a parametric map provided in any of the formats supported by ITK, such as NRRD or NIFTI, as a DICOM Parametric Map image object.

## Usage

```
   --inputDICOM <std::string>
     File name of the DICOM image file that should be used to populate the
     composite context (attributes related to the patient and imaging
     study).

   --outputDICOM <std::string>
     File name of the DICOM Parametric map object with the result of the
     conversion.

   --inputMetadata <std::string>
     File name of the JSON files containing metadata attributes.

   --inputImage <std::string>
     File name of the parametric map image in a format readable by ITK
     (NRRD, NIfTI, MHD, etc.).
```

## Detailed usage

Most of the effort will be required to populate the content of the meta-information JSON file. Its structure is defined by [this](https://github.com/QIICR/dcmqi/blob/master/doc/schemas/pm-schema.json) JSON-Schema file. Interpretation of JSON-Schema may require some effort, especially considering that this particular file uses externally defined items. It may be easier to start with an example JSON file that "instantiates" this schema, such as [this one](https://github.com/QIICR/dcmqi/blob/master/doc/examples/pm-example.json).

In the following, we will guide you through the contents of this file - line by line.

```JSON
{
  "@schema": "https://raw.githubusercontent.com/qiicr/dcmqi/master/doc/schemas/pm-schema.json#",
```

```JSON
  "SeriesDescription": "Apparent Diffusion Coefficient",
  "SeriesNumber": "701",
  "InstanceNumber": "1",
  "BodyPartExamined": "PROSTATE",
```

These lines correspond to the metadata attributes that will be populated in the resulting DICOM Parametric Map image object. It is your choice how you want to populate those. There are certain constraints on the values of these attributes. If those constraints are not met, converter will fail. In the future, we will provide instructions for validating your meta-information file.

```JSON
  "QuantityValueCode": {
    "CodeValue": "113041",
    "CodingSchemeDesignator": "DCM",
    "CodeMeaning": "Apparent Diffusion Coefficient"
  },
  "MeasurementUnitsCode": {
    "CodeValue": "um2/s",
    "CodingSchemeDesignator": "UCUM",
    "CodeMeaning": "um2/s"
  },
  "MeasurementMethodCode": {
    "CodeValue": "DWMPxxxx10",
    "CodingSchemeDesignator": "99QIICR",
    "CodeMeaning": "Mono-exponential diffusion model"
  },
  "SourceImageDiffusionBValues": [
    "0",
    "1400"
  ],
  "AnatomicRegionSequence": {
    "CodeValue": "T-9200B",
    "CodingSchemeDesignator": "SRT",
    "CodeMeaning": "Prostate"
  },
  "FrameLaterality": "U",
  "RealWorldValueSlope": 1,
  "DerivedPixelContrast": "ADC"
}
```

`QuantityValueCode`, `MeasurementUnitsCode`, `MeasurementMethodCode`, `AnatomicRegionSequence` are attributes (code tuples) to describe the meaning the pixels stored in this parametric map. `AnatomicRegionSequence`, `DerivedPixelContrast`, `FrameLaterality` are the only attributes that are required. All others are optional.

Each code tuple consists of the three components:  `CodeValue`, `CodingSchemeDesignator` and `CodeMeaning`. `CodingSchemeDesignator` defines the "authority", or source of the code. Each `CodeValue` should be unique for a given `CodingSchemeDesignator`. `CodeMeaning` is a human-readable meaning of the code. DICOM defines several coding schemes recognized by the standard listed [in PS3.16 Section 8](http://dicom.nema.org/medical/dicom/current/output/chtml/part16/chapter_8.html). 


