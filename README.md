# Introduction

dcmqi (DICOM (**dcm**) for Quantitative Imaging (**qi**)) is a collection of libraries and command line tools with minimum dependencies to support standardized communication of quantitative image analysis research data using [DICOM standard](http://dicom.nema.org/standard.html).

Specifically, the current focus of development for dcmqi is to support conversion of the following data types to and from DICOM:
* voxel-based segmentations using [DICOM Segmentation IOD](http://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_A.51.html)
* parametric maps using [DICOM Parametric map IOD](http://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_A.75.html)
* image-based measurements using [DICOM Structured Reporting (SR) template TID1500](http://dicom.nema.org/medical/dicom/current/output/chtml/part16/chapter_A.html#sect_TID_1500)

As an introduction to the motivation, capabilities and advantages of using the DICOM standard, and the objects mentioned above, you might want to read this open access paper:

Fedorov A, Clunie D, Ulrich E, Bauer C, Wahle A, Brown B, Onken M, Riesmeier J, Pieper S, Kikinis R, Buatti J, Beichel RR. (2016) DICOM for quantitative imaging biomarker development: a standards based approach to sharing clinical data and structured PET/CT analysis results in head and neck cancer research. *PeerJ* 4:e2057 https://doi.org/10.7717/peerj.2057

dcmqi is under active development. The functionality available is being refined, may (and most likely, will) change in the future, and may not work as expected. The organization of the documentation will also change in the future.

dcmqi is developed and maintained by the [QIICR](http://qiicr.org) project.

# Getting started

At the moment, we do not provide pre-built packages for dcmqi. If you want to give it a try, you will need to check out the source code and build it yourself. 

dcmqi is not platform specific. Our goal is to support its use on Windows, Mac and Linux.

In the future (pending resolution of issues on the gitbooks platform), instructions on how to build dcmqi and how to use it will be provided in the [dcmqi gitbook manual](https://fedorov.gitbooks.io/dcmqi/content/v/gitbook/).

For now, please check out [dcmqi faq](https://github.com/QIICR/dcmqi-faq/issues) to find instructions on how to build dcmqi, and how to use certain components.

# License

dcmqi is distributed under non-restrictive BSD-style license (see full text [here](https://github.com/QIICR/dcmqi/blob/master/LICENSE.txt)) that does not have any constraints on either commercial or academic usage.

Our goal is to support and encourage adoption of the DICOM standard in both academic and research tools. We will be happy to hear about your usage of dcmqi, but you don't have to report back to us.

# Support

We are most interested in the feedback of early testers and adopters. You can communicate you feedback, feature requests, comments or problem reports using any of the methods below:

* [submit issue](https://github.com/QIICR/dcmqi/issues/new) on dcmqi bug tracker
* send email to [Andrey Fedorov](http://fedorov.github.io)
* leave comments in the [dcmqi gitbook manual](https://fedorov.gitbooks.io/dcmqi/content/v/gitbook/)


Note that dcmqi is under active development; command line arguments, schemas, examples can (and most likely, will) change as we refined the software.

# dcmqi health monitors

Indicators below can be used to check whether current version of dcmqi source code has any build or testing issues on our continuous integration platforms (green == good).

* Linux: [![Circle CI](https://circleci.com/gh/QIICR/dcmqi.svg?style=svg)](https://circleci.com/gh/QIICR/dcmqi)
* Windows: [![Build status](https://ci.appveyor.com/api/projects/status/04l87y2j6prboap7?svg=true)](https://ci.appveyor.com/project/fedorov/dcmqi)
* Mac OS X: [![TravisCI](https://travis-ci.org/QIICR/dcmqi.svg?branch=master)](https://travis-ci.org/QIICR/dcmqi)

# Acknowledgments

This work is supported in part the National Institutes of Health, National
Cancer Institute, Informatics Technology for Cancer Research (ITCR) program,
grant Quantitative Image Informatics for Cancer Research (QIICR) (U24
CA180918, PIs Kikinis and Fedorov).

# References

1. Fedorov A, Clunie D, Ulrich E, Bauer C, Wahle A, Brown B, Onken M, Riesmeier J, Pieper S, Kikinis R, Buatti J, Beichel RR. (2016) DICOM for quantitative imaging biomarker development: a standards based approach to sharing clinical data and structured PET/CT analysis results in head and neck cancer research. *PeerJ* 4:e2057 https://doi.org/10.7717/peerj.2057