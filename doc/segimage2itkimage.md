# segimage2itkimage

`segimage2itkimage` converts a DICOM Segmentation object back into one or more
research-format images (ITK images such as NRRD or NIfTI) together with a JSON
metadata file describing the segments. It reads both **binary** and **labelmap**
(`SOP Class` Label Map Segmentation, per DICOM Supplement 243) Segmentations.

This page collects behavioral and implementation notes that are not obvious
from the `--help` output. They concern labelmap input; see also the
write-direction notes in [itkimage2segimage.md](itkimage2segimage.md).

## Implementation notes

### Labelmap background and Pixel Padding Value

In a labelmap, the segment designated as **background** is identified by Pixel
Padding Value (0028,0120): the value it holds is the Segment Number treated as
background (see [itkimage2segimage.md](itkimage2segimage.md) for how this is
written). On reading, that segment is handled specially:

- It is **omitted from the JSON metadata** — the background does not appear as a
  segment in the segment list. Its pixels are still written to the output image
  unchanged (for a typical file the background value is `0`, which is also the
  image's zero/empty value).
- If Pixel Padding Value designates a **non-zero** value, a warning is logged:
  the segment is still omitted from the JSON, but its pixels keep that non-zero
  value in the output image, and consuming applications may not recognize them
  as background.

Pixel Padding Value is the *only* indicator used. A segment merely **typed** as
Background (DCM, 125040) but **not** designated by Pixel Padding Value is treated
as an ordinary segment: it is kept in the JSON metadata, and a warning is logged
to point out the discrepancy. This mirrors the write side, where the two are
always emitted together.

### Missing frames in sparse labelmaps

A labelmap need not encode a frame for every slice position of the volume it
spans (`itkimage2segimage`, for instance, omits slices that contain no segment
unless `--skip 0` is given). The output ITK image is a dense grid, so every
position must hold a value; `segimage2itkimage` reconstructs the grid from the
frame positions that *are* present and fills the rest:

- Positions missing at the **borders** of the volume are not reconstructed at
  all — the grid simply shrinks to span the encoded frames (the outermost
  encoded frame becomes the first/last slice). World coordinates of the encoded
  data are preserved, but the grid no longer matches the original input volume.
- Positions missing in the **interior** are filled. The fill value is the Pixel
  Padding Value if present, otherwise `0`. Filling with the background value
  means these positions read back as background, which is normally what was
  intended.

If there is **no** Pixel Padding Value, interior frames are missing, **and** a
real segment occupies pixel value `0`, the missing positions are still filled
with `0`, but a warning is logged: the filled-in positions then cannot be
distinguished from that segment in the output image.

A Pixel Padding Value that does not fit the pixel data (a value greater than 255
in an 8-bit labelmap) is ignored, with a warning, and the fill value falls back
to `0`.
