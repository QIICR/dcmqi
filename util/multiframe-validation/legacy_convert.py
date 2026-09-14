#!/usr/bin/env python3
"""Convert a classic single-frame DICOM series into a Legacy Converted Enhanced image.

Uses highdicom to produce the multiframe equivalent of a classic series, which
is what dcmqi's multiframe source support has to consume. The modality decides
the IOD: CT, MR and PET are supported, matching the Legacy Converted Enhanced
SOP Classes defined for them.

The frames are ordered along the patient's z axis, and highdicom records the
SOP Instance UID each frame came from in the Conversion Source Attributes
Sequence, which compare_references.py uses to trace frame references back to
the original instances.

Usage:
    legacy_convert.py <input-directory> <output.dcm> [--series-number N]

Requires: highdicom (which pulls in pydicom).
"""
import argparse
import glob
import os
import sys

try:
    import highdicom as hd
    from pydicom import dcmread
except ImportError:  # pragma: no cover - dependency hint
    sys.exit("ERROR: this script needs highdicom (pip install highdicom)")


# Legacy Converted Enhanced IOD per modality of the input series
CONVERTERS = {
    "CT": "LegacyConvertedEnhancedCTImage",
    "MR": "LegacyConvertedEnhancedMRImage",
    "PT": "LegacyConvertedEnhancedPETImage",
}


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input_dir", help="directory holding the classic series")
    ap.add_argument("output", help="Legacy Converted Enhanced file to write")
    ap.add_argument("--series-number", type=int, default=100,
                    help="Series Number of the created series (default: 100)")
    args = ap.parse_args()

    files = sorted(glob.glob(os.path.join(args.input_dir, "*.dcm")))
    if not files:
        sys.exit(f"ERROR: no DICOM files in {args.input_dir}")

    series = [dcmread(f) for f in files]
    modalities = {ds.Modality for ds in series}
    if len(modalities) != 1:
        sys.exit(f"ERROR: mixed modalities in the input series: {sorted(modalities)}")
    modality = modalities.pop()
    if modality not in CONVERTERS:
        sys.exit(f"ERROR: modality {modality} has no Legacy Converted Enhanced IOD "
                 f"(supported: {', '.join(sorted(CONVERTERS))})")

    # a defined frame order keeps the result reproducible and comparable
    series.sort(key=lambda ds: float(ds.ImagePositionPatient[2]))

    converter = getattr(hd.legacy, CONVERTERS[modality])
    print(f"  {len(series)} instances, {series[0].Rows}x{series[0].Columns}, "
          f"modality {modality} -> {CONVERTERS[modality]}")

    multiframe = converter(
        legacy_datasets=series,
        series_instance_uid=hd.UID(),
        series_number=args.series_number,
        sop_instance_uid=hd.UID(),
        instance_number=1,
    )
    multiframe.save_as(args.output, enforce_file_format=True)
    print(f"  wrote {args.output}: {multiframe.NumberOfFrames} frames, "
          f"SOP Class {multiframe.SOPClassUID}")


if __name__ == "__main__":
    main()
