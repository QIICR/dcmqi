# `tid1500writer`

This tool can be used to save measurements calculated from the image over a volume defined by image segmentation into a DICOM Structured Report that follows [template TID1500](http://dicom.nema.org/medical/dicom/current/output/chtml/part16/chapter_A.html#sect_TID_1500).

## Usage

```
   --inputImageLibraryDirectory <std::string>
     Location of input DICOM Data to be used for populating image library.
     See documentation.

   --inputCompositeContextDirectory <std::string>
     Location of input DICOM Data to be used for populating composite
     context. See documentation.

   --outputDICOM <std::string>
     File name of the DICOM SR object that will store the result of the
     conversion.

   --inputMetadata <std::string>
     JSON file that contains the list of mesurements and other meta data
     items that can be specified by the user. See documentation for
     details.
```

## Usage details

Most of the effort will be required to populate the content of the meta-information JSON file. Its structure is defined by [this](https://github.com/QIICR/dcmqi/blob/master/doc/schemas/sr-tid1500-schema.json) JSON-Schema file. Interpretation of JSON-Schema may require some effort, especially considering that this particular file uses externally defined items. It may be easier to start with an example JSON file that "instantiates" this schema, such as [this one](https://github.com/QIICR/dcmqi/blob/master/doc/examples/sr-tid1500-ct-liver-example.json).

In the following, we will guide you through the contents of this file - line by line.

```JSON
{
  "@schema": "https://raw.githubusercontent.com/qiicr/dcmqi/master/doc/schemas/sr-tid1500-schema.json#",
```

This opening line references the schema this parameter file should conform to. Make sure you include this line without changes!

```JSON
  "SeriesDescription": "Measurements",
  "SeriesNumber": "1001",
  "InstanceNumber": "1",
```

These lines define top-level attributes of the resulting DICOM object. You can change these to adjust to your needs, subject to some constraints that are not covered here for now.

```

  "compositeContext": [
    "liver.dcm"
  ],

  "imageLibrary": [
    "01.dcm",
    "02.dcm",
    "03.dcm"
  ],
```

These two items contain lists of file names that should exist in the directories specified by the `--compositeContextDataDir` and `--imageLibraryDataDir`, correspondingly. You should include the file with the DICOM Segmentation object defining the segmented region in the `compositeContext` attribute!

```JSON
  "observerContext": {
    "ObserverType": "PERSON",
    "PersonObserverName": "Reader1"
  },
```

These are the attributes of either the person that performed the measurements. If you want to list the device instead of a person, it is also possible, but should be done differently. Please ask about details.

```JSON
  "VerificationFlag": "VERIFIED",
  "CompletionFlag": "COMPLETE",
```

Values for `VerificationFlag` can be one of `VERIFIED` or `UNVERIFIED`. `CompletionFlag` values are either `PARTIAL` or `COMPLETE`.

```JSON
  "activitySession": "1",
  "timePoint": "1",
```

`activitySession` attribute can be used to encode session number, when, for example, the same structure was segmented multiple times. `timePoint` can be used in the situation of longitudinal tracking of the measurements.

```JSON
  "Measurements": [
    {
      "MeasurementGroup": {
        "TrackingIdentifier": "Measurements group 1",
        "ReferencedSegment": 1,
        "SourceSeriesForImageSegmentation": "1.2.392.200103.20080913.113635.2.2009.6.22.21.43.10.23431.1",
        "segmentationSOPInstanceUID": "1.2.276.0.7230010.3.1.4.0.42154.1458337731.665796",
        "Finding": {
          "codeValue": "T-D0060",
          "codingSchemeDesignator": "SRT",
          "codeMeaning": "Organ"
        },
```

This is the beginning of the structure where the actual measurements are stored. The measurements are stored hierarchically, and can include 1 or more measurement groups, where each measurement group encodes one or more measurement items.

For each measurement group, you will need to define certain common attributes shared by all measurement items within that group:
* `TrackingIdentifier` is a human-readable string naming the group
* `ReferencedSegment` is the ID of the segment within the DICOM segmentation object that defines the region used to calculate the measurement.
* `SourceSeriesForImageSegmentation` is the SeriesInstanceUID of the DICOM Segmentation object
* `segmentationSOPInstanceUID` is the SOPInstanceUID of the DICOM Segmentation object
* `Finding` is a triplet of (code, codingSchemeDesignator, codeMeaning) defining the finding over which the measurement is being performed. You can read more about how these triples are defined [here](https://peerj.com/articles/2057/#p-37).

```JSON
        "measurementItems": [
          {
            "value": "37.3289",
            "quantity": {
              "codeValue": "122713",
              "codingSchemeDesignator": "DCM",
              "codeMeaning": "Attenuation Coefficient"
            },
            "units": {
              "codeValue": "[hnsf'U]",
              "codingSchemeDesignator": "UCUM",
              "codeMeaning": "Hounsfield unit"
            },
            "derivationModifier": {
              "codeValue": "R-00317",
              "codingSchemeDesignator": "SRT",
              "codeMeaning": "Mean"
            }
          },
```

Finally, `measurementItems` contains the list of individual measurement. Each measurement is encoded by specifying the following properties:
* `value`: the number, the actual measurement
* `quantity`: code triplet encoding the quantity. 
* `units`: code triplet defining the units corresponding of the value. DICOM is using the Unified Code for Units of Measure (UCUM) system for encoding units.
* `derivationModifier`: code triplet encoding the quantity modifier.

The most challenging part of encoding measurements is arguably the process of identifying the codes corresponding to the quantity and derivation modifier (if necessary). You may want to read the discussion on this topic on p.19 of [this paper](https://peerj.com/preprints/1541v3/). For practical purposes, you can study the measurements encoded in this example and follow the pattern: https://github.com/QIICR/dcmqi/blob/master/doc/sr-tid1500-ct-liver-example.json. In the future, we will add more details, more examples, and more user-level tools to simplify the process of selecting such codes.

Once you generated the output DICOM object using `tid1500writer` tool, it is ***always*** a ***very good idea*** to validate the resulting object. For this purpose we recommend `DicomSRValidator` tool from the [Pixelmed toolkit](http://www.dclunie.com/pixelmed/software/):

```shell
bash DicomSRValidator.sh <sr_object>
```

You can also examine the content of the resulting document with various tools such as [dcsrdump](http://manpages.ubuntu.com/manpages/precise/man1/dcsrdump.1.html) from the [dicom3tools](http://www.dclunie.com/dicom3tools.html) suite

![image](https://cloud.githubusercontent.com/assets/313942/18153133/6ea80720-6fc9-11e6-839b-7956b0aa3be0.png)

or (more colorful!) [dsrdump](http://support.dcmtk.org/docs/dsrdump.html) from [DCMTK](http://dcmtk.org)

![image](https://cloud.githubusercontent.com/assets/313942/18153147/9f6ee6a8-6fc9-11e6-99bd-0bbd72be556b.png)

You can also use [dicom-dump plugin](https://atom.io/packages/dicom-dump) in the [Atom editor](http://atom.io) to conveniently view the content without having to use the terminal.

## Examples

### Encoding measurements over segmentation of liver in CT

* 