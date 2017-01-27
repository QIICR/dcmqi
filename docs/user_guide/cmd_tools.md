# General principles

dcmqi provides a set of command line tools that perform conversion between research formats and [DICOM](http://dicom.nema.org/medical/dicom/current/output/chtml/part01/chapter_1.html#sect_1.1).

We use [JSON](http://www.json.org/) to represent metadata that is either passed to the converter into DICOM, or that is extracted from DICOM representation.

As such, you will need to have some understanding of both DICOM and JSON if you want to use dcmqi.

We provide the following tools that can help you use dcmqi more effectively.

* [dicom-dump](https://atom.io/packages/dicom-dump) is an extension for [Atom](http://atom.io) text editor that can be used to explore the content of DICOM data
* [dcmqi validators](http://qiicr.org/dcmqi/#/validators) web app can be used to see examples of metadata JSON for specific object types, to explore the schema, and to validate your own metadata JSON against the schema

You can check out our [Learning resources](resources.md) section for some reference material on the subject that we found helpful.