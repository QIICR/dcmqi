# Examples of JSON files parameterizing dcmqi conversion

[This folder](https://github.com/QIICR/dcmqi/tree/master/doc/examples) contains the following example JSON files illustrating parameterization of conversion of the DICOM objects supported by `dcmqi`.

## DICOM Segmentation object

The most common confusion about the format of the JSON file for segmentation conversion parameterization is about the organization of the `segmentAttributes` field. That field is an array of arrays, where the outer ordered array entries correspond to the input segmentation files passed to the converter, and the inner arrays for each of the segmentation files assign metadata indexed by the labels within the individual segmentation files.

Remember that you can always use the [`dcmqi` webapp](http://qiicr.org/dcmqi/#/seg) to prepare JSON files for your conversion task, or to build intuition about how things work.

* `seg-example.json`: the most basic example where the input segmentation being converted is stored in a single file, and the file contains a single label
* `seg-example_multiple_segments_single_input_file.json`: single segmentation file as input, with multiple labels in the input file. This is reflected in the `segmentAttributes` field, which contains outer array with a single entry, and the inner array containing 3 entries corresponding to the 3 labels in the input segmentation.
* `seg-example_multiple_segments.json`: this is an example of parameterization where each of the segmentation labels is saved in a separate file: outer array contains 3 entries, and each of the innner arrays has just one entry.

## DICOM Parametric maps

TBD

## DICOM Structured Report TID1500

TDB
