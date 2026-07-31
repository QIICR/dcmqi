#!/usr/bin/env python3
"""Generate the multiframe source image variants used by the segmentation tests.

The 24x38x3 phantom already ships a multiframe (Legacy Converted Enhanced CT)
encoding of its source series in multiframe/mf.dcm: three frames, one per slice
of nrrd/label.nrrd, each with its own Plane Position (Patient). Two structures
that real Enhanced images commonly use are not covered by it, and both change
how source image references have to be written
(https://github.com/QIICR/dcmqi/issues/150):

  mf-2frames-per-plane.dcm
      Six frames on the same three planes, interleaved as two "acquisitions"
      (frames 1-3 and 4-6), i.e. two frames per slice of the segmentation.
      Each segmentation frame must reference both source frames of its plane,
      so Referenced Frame Number becomes multi-valued and non-contiguous
      ([1,4], [2,5], [3,6]). Uses its own Series Instance UID so that it can be
      combined with mf.dcm to exercise references spanning two series.

  mf-shared-plane-position.dcm
      Four frames that all lie on the plane of the first slice, with the Plane
      Position (Patient) functional group shared instead of per-frame (as e.g.
      cine acquisitions do). Only the first slice of the segmentation maps to
      it, and since all frames of the instance are referenced, Referenced Frame
      Number has to be omitted (a reference without frame numbers applies to
      the whole instance).

Both are derived from mf.dcm so that patient, study and frame of reference stay
consistent with the phantom, and use deterministic UIDs so that regenerating
them produces byte-identical output.

This script is NOT part of the build or the test suite: the generated files are
committed to the repository and read from there by the tests, so neither
building nor running dcmqi requires pydicom. Run it by hand only to recreate
the two files (e.g. after mf.dcm changed) or as the starting point for adding
further variants -- the same role the PixelMed command line documented in
commit 58d8fe7 plays for mf.dcm itself, just reproducible. Regenerating without
changing anything leaves the files byte-identical and therefore produces no
diff.

Usage: make_multiframe_test_data.py [<data/segmentations/24x38x3/multiframe>]

Requires pydicom (not a dcmqi dependency otherwise).
"""
import copy
import os
import sys

import pydicom


def derive_uids(template):
    """Deterministic UIDs derived from the template, keeping its UID roots."""
    sop_root = str(template.SOPInstanceUID).rsplit(".", 1)[0]
    series_root = str(template.SeriesInstanceUID).rsplit(".", 1)[0]
    series_last = str(template.SeriesInstanceUID).rsplit(".", 1)[1]
    return {
        "2frames": (f"{sop_root}.2", f"{series_root}.{int(series_last) + 1}"),
        "shared": (f"{sop_root}.3", f"{series_root}.{int(series_last) + 2}"),
    }


def frame_bytes(ds, index):
    """Raw pixel data of one frame (uncompressed)."""
    frame_size = ds.Rows * ds.Columns * (ds.BitsAllocated // 8)
    return bytes(ds.PixelData[index * frame_size:(index + 1) * frame_size])


def make_two_frames_per_plane(template, sop_uid, series_uid):
    ds = copy.deepcopy(template)
    n = int(template.NumberOfFrames)

    per_frame = []
    pixel_data = b""
    for acquisition in range(2):
        for i in range(n):
            fg = copy.deepcopy(template.PerFrameFunctionalGroupsSequence[i])
            # keep the plane position, distinguish the acquisitions by the
            # frame content so the frames are not identical copies
            if "FrameContentSequence" in fg:
                fc = fg.FrameContentSequence[0]
                fc.InStackPositionNumber = i + 1
                fc.StackID = "1"
                fc.TemporalPositionIndex = acquisition + 1
            per_frame.append(fg)
            pixel_data += frame_bytes(template, i)

    ds.PerFrameFunctionalGroupsSequence = pydicom.Sequence(per_frame)
    ds.NumberOfFrames = 2 * n
    ds.PixelData = pixel_data
    ds.SOPInstanceUID = sop_uid
    ds.SeriesInstanceUID = series_uid
    ds.SeriesNumber = int(template.SeriesNumber) + 1
    ds.SeriesDescription = "Multiframe source, two frames per plane"
    ds.file_meta.MediaStorageSOPInstanceUID = sop_uid
    return ds


def make_shared_plane_position(template, sop_uid, series_uid, num_frames=4):
    ds = copy.deepcopy(template)

    # move the plane position of the first frame into the shared FGs
    shared = ds.SharedFunctionalGroupsSequence[0]
    shared.PlanePositionSequence = copy.deepcopy(
        template.PerFrameFunctionalGroupsSequence[0].PlanePositionSequence)

    per_frame = []
    pixel_data = b""
    for i in range(num_frames):
        fg = copy.deepcopy(template.PerFrameFunctionalGroupsSequence[0])
        del fg.PlanePositionSequence  # now shared by all frames
        if "FrameContentSequence" in fg:
            fc = fg.FrameContentSequence[0]
            fc.InStackPositionNumber = 1
            fc.StackID = "1"
            fc.TemporalPositionIndex = i + 1
        per_frame.append(fg)
        pixel_data += frame_bytes(template, 0)

    ds.PerFrameFunctionalGroupsSequence = pydicom.Sequence(per_frame)
    ds.NumberOfFrames = num_frames
    ds.PixelData = pixel_data
    ds.SOPInstanceUID = sop_uid
    ds.SeriesInstanceUID = series_uid
    ds.SeriesNumber = int(template.SeriesNumber) + 2
    ds.SeriesDescription = "Multiframe source, shared plane position"
    ds.file_meta.MediaStorageSOPInstanceUID = sop_uid
    return ds


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    default_dir = os.path.join(here, os.pardir, "data", "segmentations", "24x38x3", "multiframe")
    out_dir = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else default_dir)

    template = pydicom.dcmread(os.path.join(out_dir, "mf.dcm"))
    uids = derive_uids(template)

    variants = [
        ("mf-2frames-per-plane.dcm",
         make_two_frames_per_plane(template, *uids["2frames"])),
        ("mf-shared-plane-position.dcm",
         make_shared_plane_position(template, *uids["shared"])),
    ]

    for name, ds in variants:
        path = os.path.join(out_dir, name)
        ds.save_as(path, write_like_original=False)
        print(f"wrote {path}: {ds.NumberOfFrames} frames, "
              f"{os.path.getsize(path)} bytes, SOP ...{ds.SOPInstanceUID[-8:]}")


if __name__ == "__main__":
    main()
