# Validating multiframe source support against real data

Tooling to check dcmqi's support for multiframe source images against real
image data, as a complement to the test suite that runs in CI.

## What it does

For one segmentation from the
[Imaging Data Commons](https://portal.imaging.datacommons.cancer.gov):

1. download that segmentation and the image series it references;
2. convert the segmentation back to ITK with `segimage2itkimage`, which yields
   both the label image(s) and the matching metadata JSON;
3. convert the classic series into a Legacy Converted Enhanced image with
   [highdicom](https://highdicom.readthedocs.io);
4. run `itkimage2segimage` twice on that same ITK input - once against the
   classic series, once against the multiframe image;
5. compare the two resulting segmentations.

Both runs receive identical input and differ only in how the source images are
encoded, so their results have to agree. The comparison asserts

- the same number of frames, byte-identical pixel data, the same segments and
  the same frame positions,
- classic output: many referenced instances, no Referenced Frame Number,
- multiframe output: exactly one referenced instance, addressed per frame,
- and, as the actual point, that the frame references **denote the same source
  images** as the instance references of the classic run.

The last check resolves each Referenced Frame Number back to the SOP Instance
UID of the classic instance it came from, using the Conversion Source
Attributes Sequence that a Legacy Converted Enhanced image carries per frame.
It therefore compares what the references mean, without assuming anything about
slice geometry - which matters, because a segmentation may sit on a different
slice grid than the series it references, and then several source frames
legitimately map onto one segmentation frame.

## Why this is useful

The CTest suite covers the same ground with small generated fixtures
(`data/segmentations/24x38x3/multiframe/`, see
`util/make_multiframe_test_data.py`). Those are fast, deterministic and run
everywhere, but they are built by us from one PixelMed-converted phantom. This
tooling adds what they cannot:

- **An independent producer.** The multiframe images come from highdicom, not
  from dcmqi and not from the tool that produced the checked-in fixtures. It
  verifies that dcmqi reads Enhanced objects as third-party tools write them.
- **Real geometry.** Series with gaps, hundreds of slices, segmentations whose
  slice grid does not match the referenced series, several overlapping
  segments. A validation run over seven IDC collections turned up a real case
  where two source slices fall into one segmentation slice, producing the
  multi-valued Referenced Frame Number that the fixture
  `mf-2frames-per-plane.dcm` reproduces synthetically.
- **Modalities beyond CT.** The converter picks the Legacy Converted Enhanced
  IOD from the input modality, so CT, MR and PET series can all be used as
  sources. dcmqi's handling is SOP Class agnostic and this confirms it.

It is deliberately *not* wired into CTest: it needs network access, the IDC
index and highdicom, and it downloads tens to hundreds of megabytes per case.
Run it by hand when the source image handling changes, or to check a dataset a
user reported a problem with.

## Requirements

- `highdicom` and `pydicom` (`pip install highdicom`)
- `idc-index` for the downloads (`pip install idc-index`), providing the `idc` command
- a dcmqi build providing `itkimage2segimage` and `segimage2itkimage`
- optional: `dciodvfy` (from dicom3tools), used for validation if present

## Usage

```bash
export DCMQI_BIN=/path/to/dcmqi-build/bin
export PYTHON=/path/to/python-with-highdicom      # if not the default python3

./validate_idc_case.sh /tmp/case1 <SEG-SeriesInstanceUID>
```

The argument is the Series Instance UID of an IDC **segmentation**; the script
finds the referenced image series itself. Pick one for example by browsing the
IDC portal, or from the index:

```python
from idc_index import IDCClient
idx = IDCClient().index
ct  = idx[(idx.Modality == "CT") & idx.instanceCount.between(60, 200)]
seg = idx[idx.Modality == "SEG"]
both = set(ct.StudyInstanceUID) & set(seg.StudyInstanceUID)
print(seg[seg.StudyInstanceUID.isin(both)].SeriesInstanceUID.head())
```

Note that a study often holds several series: the segmentation may reference a
different one than the CT you started from, and it may reference a PET rather
than a CT series. The script always follows the reference recorded in the
segmentation.

The individual steps can also be used on their own:

```bash
./legacy_convert.py <series-directory> <output.dcm>
./compare_references.py <classic-seg.dcm> <enhanced-seg.dcm> <legacy-converted-source.dcm>
```

`validate_idc_case.sh` exits non-zero if a check fails.

## Interpreting the results

Two observations from running this that are worth knowing before chasing a
failure:

- **Many IDC segmentations were themselves written by dcmqi** (their
  Manufacturer is QIICR); the AIMI analysis results are examples. That is fine
  for this comparison, because both runs get the same ITK input and the
  segmentation only supplies realistic masks - but it does mean these cases do
  not independently validate dcmqi's segmentation *writing*. The independence
  that matters here is on the source image side, and that is provided by
  highdicom.
- **`dciodvfy` may report `Missing attribute Type 2 Required
  Element=<ClinicalTrialCoordinatingCenterName>`** on the outputs. This is
  unrelated to multiframe sources and appears for classic sources just the
  same: dcmqi always writes Clinical Trial Series ID and Time Point ID, which
  makes the Clinical Trial Series module present, but writes the type 2
  Coordinating Center Name only when the metadata supplies a value.
