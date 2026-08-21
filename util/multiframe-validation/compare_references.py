#!/usr/bin/env python3
"""Compare a segmentation converted from a classic series with the one converted
from its Legacy Converted Enhanced equivalent.

Both segmentations are created by itkimage2segimage from the same ITK input, so
they must describe the same segmentation and reference the same source images -
the classic one instance by instance, the multiframe one frame by frame via
Referenced Frame Number.

The frame numbers are resolved back to the SOP Instance UIDs of the original
classic instances through the Conversion Source Attributes Sequence that the
Legacy Converted Enhanced image carries per frame. The check therefore compares
what the references actually denote and needs no assumption about slice
geometry - which matters because a segmentation may sit on a different slice
grid than the series it references, in which case several source frames legitimately
map to the same segmentation frame.

Usage:
    compare_references.py <classic-seg.dcm> <enhanced-seg.dcm> <legacy-converted-source.dcm>

Exit code 0 if all checks pass, 1 otherwise.

Requires: pydicom.
"""
import argparse
import sys

try:
    import pydicom
except ImportError:  # pragma: no cover - dependency hint
    sys.exit("ERROR: this script needs pydicom (pip install pydicom)")


class Checker:
    """Collects check results and prints them as they come in."""

    def __init__(self):
        self.ok = True

    def __call__(self, condition, message):
        print(("  OK   " if condition else "  FAIL ") + message)
        self.ok = self.ok and bool(condition)
        return condition


def frame_references(seg):
    """Per frame of the segmentation, map referenced SOP Instance UID to the
    tuple of referenced frame numbers (None if the attribute is absent, which
    denotes the whole instance)."""
    result = []
    for fg in seg.PerFrameFunctionalGroupsSequence:
        refs = {}
        derivation = getattr(fg, "DerivationImageSequence", None)
        if derivation:
            for source in derivation[0].SourceImageSequence:
                numbers = getattr(source, "ReferencedFrameNumber", None)
                if numbers is not None:
                    if not isinstance(numbers, pydicom.multival.MultiValue):
                        numbers = [numbers]
                    numbers = tuple(sorted(int(n) for n in numbers))
                refs[source.ReferencedSOPInstanceUID] = numbers
        result.append(refs)
    return result


def frame_to_source_instance(multiframe):
    """Frame number (1-based) -> SOP Instance UID of the classic instance it came from."""
    mapping = {}
    for i, fg in enumerate(multiframe.PerFrameFunctionalGroupsSequence):
        conversion = getattr(fg, "ConversionSourceAttributesSequence", None)
        if conversion:
            mapping[i + 1] = conversion[0].ReferencedSOPInstanceUID
    return mapping


def plane_positions(seg):
    return [tuple(fg.PlanePositionSequence[0].ImagePositionPatient)
            for fg in seg.PerFrameFunctionalGroupsSequence]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("classic_seg", help="segmentation converted from the classic series")
    ap.add_argument("enhanced_seg", help="segmentation converted from the multiframe source")
    ap.add_argument("multiframe_source", help="the Legacy Converted Enhanced source image")
    args = ap.parse_args()

    classic = pydicom.dcmread(args.classic_seg)
    enhanced = pydicom.dcmread(args.enhanced_seg)
    multiframe = pydicom.dcmread(args.multiframe_source, stop_before_pixels=True)
    check = Checker()

    # 1. the segmentation content must not depend on how the source is encoded
    check(int(classic.NumberOfFrames) == int(enhanced.NumberOfFrames),
          f"same number of frames ({classic.NumberOfFrames})")
    check(classic.PixelData == enhanced.PixelData,
          f"identical pixel data ({len(classic.PixelData)} bytes)")
    check([s.SegmentLabel for s in classic.SegmentSequence]
          == [s.SegmentLabel for s in enhanced.SegmentSequence],
          f"same segments {[s.SegmentLabel for s in classic.SegmentSequence]}")
    check(plane_positions(classic) == plane_positions(enhanced), "same frame positions")

    classic_refs = frame_references(classic)
    enhanced_refs = frame_references(enhanced)

    # 2. the classic case references whole instances, one per source image
    classic_instances = classic.ReferencedSeriesSequence[0].ReferencedInstanceSequence
    check(len(classic_instances) > 1,
          f"classic: {len(classic_instances)} referenced instances")
    check(all(frames is None for refs in classic_refs for frames in refs.values()),
          "classic: no Referenced Frame Numbers")

    # 3. the multiframe case references one instance, addressed per frame
    enhanced_instances = enhanced.ReferencedSeriesSequence[0].ReferencedInstanceSequence
    check(len(enhanced_instances) == 1
          and enhanced_instances[0].ReferencedSOPInstanceUID == multiframe.SOPInstanceUID,
          "enhanced: exactly the multiframe instance is referenced")
    check(all(list(refs) == [multiframe.SOPInstanceUID] for refs in enhanced_refs if refs),
          "enhanced: every frame references only the multiframe instance")

    # 4. both must denote the same source images
    frame2source = frame_to_source_instance(multiframe)
    check(len(frame2source) == int(multiframe.NumberOfFrames),
          f"source records its origin for all {multiframe.NumberOfFrames} frames")

    mismatches = 0
    referenced = 0
    for expected, actual in zip(classic_refs, enhanced_refs):
        wanted = set(expected)
        resolved = set()
        for frames in actual.values():
            if frames is None:      # whole instance: cannot be narrowed down
                resolved = wanted
                break
            resolved |= {frame2source.get(n) for n in frames}
        referenced += len(wanted)
        if resolved != wanted:
            mismatches += 1
    check(mismatches == 0,
          f"frame references denote the same {referenced} source images as the classic ones")

    print("\n" + ("ALL CHECKS PASSED" if check.ok else "CHECKS FAILED"))
    return 0 if check.ok else 1


if __name__ == "__main__":
    sys.exit(main())
