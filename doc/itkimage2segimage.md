# itkimage2segimage

`itkimage2segimage` converts one or more research-format segmentations (ITK
images such as NRRD, NIfTI or MetaImage) together with a JSON metadata file
into a DICOM Segmentation object. It can produce either a DICOM **binary**
Segmentation (the default) or a DICOM **Labelmap** Segmentation
(`--segmentationType labelmap`, per DICOM Supplement 243). The JSON file
describes the per-segment metadata (codes, labels, colors, …); its structure is
documented in [doc/examples](examples/README.md).

This page collects behavioral and implementation notes that are not obvious
from the `--help` output.

## Implementation notes

### Segment numbering and `--useLabelIDAsSegmentNumber`

Every segment in a DICOM Segmentation has a **Segment Number** — a positive
integer that identifies it within the object. When building a segmentation,
`itkimage2segimage` must decide which Segment Number each input label receives.
The `--useLabelIDAsSegmentNumber` option controls that decision.

The input label value `0` is treated specially and is **never** turned into a
numbered foreground segment, regardless of this option (the flag never even
sees it). What happens to it depends on the output type:

- In a **labelmap** segmentation, value `0` becomes the background — it is
  given Segment Number 0, coded as Background (DCM, 125040), and Pixel Padding
  Value (0028,0120) is set to `0` — whenever value `0` occurs in the pixel
  data.
- In a **binary** segmentation there is no background segment at all; label-`0`
  pixels simply do not belong to any segment.

**Default (option off).** Segments are numbered **sequentially from 1** in the
order they are processed: input files are taken in the order given on the
command line, and the labels within each file in ascending order. The first
segment encountered becomes Segment Number 1, the second becomes 2, and so on.
The original label IDs from your ITK image are *not* carried over — they only
determine processing order, not the resulting numbers.

**Option on.** The input label ID is used **directly** as the Segment Number,
so the numbering you chose in your ITK image is preserved in the DICOM object.
The behaviour differs between the two output types because the two DICOM
representations are different.

#### Binary segmentations (default output)

In a binary segmentation each segment is stored as its own bit-plane, and the
Segment Number is just an identifier that the frames refer to. With the option
on, the segments are renumbered to match the input label IDs and the Segment
Sequence is re-sorted by those numbers (the per-frame references are updated to
stay consistent).

Because binary Segment Numbers must, by the DICOM standard, start at 1 and
increase by 1 without gaps, this option works **only if your label IDs are
exactly 1, 2, 3, … N**. If they contain gaps or do not start at 1, the
conversion fails. Within that constraint the option effectively *reorders* the
segments so that Segment Number = label ID.

This has a visible effect only when the input order differs from the label-ID
order — for example when segmentation files are passed in a different order
than their labels. If you provide a single file, or files already in label-ID
order, the output is identical whether the option is on or off.

*Example:* two files passed as `spine` (label 2), then `liver` (label 1).

- Off → Segment 1 = spine, Segment 2 = liver (processing order).
- On → Segment 1 = liver, Segment 2 = spine (label-ID order).

#### Labelmap segmentations (`--segmentationType labelmap`)

In a labelmap each pixel directly stores the Segment Number of the segment at
that location, so **Segment Number = pixel value**. With the option on, each
input label ID becomes both the Segment Number and the pixel value used for
that segment, so the pixel values in the DICOM labelmap are exactly the label
values from your ITK image.

Labelmaps do **not** require consecutive numbering, so **gaps are allowed**:
label IDs `{1, 5, 100}` are accepted and produce pixel values `1`, `5`, `100`.
The only requirements are that each label ID is positive and **unique across
the whole input** — a label ID used by more than one input segment is rejected,
because two segments cannot share one pixel value. (The bit depth, 8- or
16-bit, is chosen from the largest label ID, not the number of segments.)

With the option off, a labelmap is still produced correctly, but its pixel
values are renumbered `1 … N` and no longer match your original label IDs.

#### When to use it

Turn the option on when you need the DICOM Segment Numbers — and, for
labelmaps, the pixel values — to match the label IDs of your input image, most
importantly to keep label IDs stable across an `ITK → DICOM → ITK` round-trip
(reading a segmentation back assigns each ITK label from the Segment Number).
Leave it off if you only care that the segments are present and any consistent
numbering is acceptable. For labelmaps it is the only way to preserve
non-consecutive label IDs; for binary segmentations it is a no-op unless your
input ordering differs from the label-ID order.

### Labelmap background and Pixel Padding Value

A labelmap must describe every pixel value it uses with a Segment Sequence
item. The value left over where no foreground segment is present — pixel value
`0`, coming either from input label `0` or from areas (and whole slices) that no
segment covers — is treated as the **background**.

When pixel value `0` occurs in a labelmap, `itkimage2segimage` designates it as
the background and writes:

- a segment with **Segment Number 0**, label `Background`, Segmented Property
  Category Code (SCT, 309825002, "Spatial and Relational Concept"), Type Code
  (DCM, 125040, "Background"), algorithm type `MANUAL`, and a black Recommended
  Display CIELab value; and
- **Pixel Padding Value (0028,0120) = 0** in the General Equipment Module.

Pixel Padding Value is the indicator, per the DICOM standard, that the
corresponding Segment Number is to be treated as background — a segment merely
*typed* as Background but without a matching Pixel Padding Value is just a
regular segment. dcmqi therefore always writes the two together, so they cannot
disagree. (This follows the discussion in
[Slicer/Slicer#9163](https://github.com/Slicer/Slicer/pull/9163).)

If every pixel is covered by a foreground segment — a "total segmentation",
where value `0` never occurs — no background segment and no Pixel Padding Value
are written.

Binary segmentations never receive a background segment or a Pixel Padding
Value: there is no background concept (label-`0` pixels simply belong to no
segment), and Pixel Padding Value is not permitted for a non-labelmap
Segmentation.

When converting back with [`segimage2itkimage`](segimage2itkimage.md), the
segment designated as background by Pixel Padding Value is *not* listed in the
output JSON metadata (its pixels are still written to the output image), so the
background does not appear as a spurious segment.
