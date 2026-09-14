#!/bin/bash
#
# End-to-end validation of dcmqi's multiframe source support against real data
# from the Imaging Data Commons (https://portal.imaging.datacommons.cancer.gov).
#
# For one IDC segmentation the script
#   1. downloads the segmentation and the image series it references,
#   2. converts the segmentation back to ITK (segimage2itkimage), which yields
#      both the label image(s) and the matching metadata JSON,
#   3. converts the classic series into a Legacy Converted Enhanced image using
#      highdicom, i.e. with a tool independent of dcmqi,
#   4. runs itkimage2segimage twice on the same ITK input - once against the
#      classic series, once against the multiframe image,
#   5. compares the two results (compare_references.py) and validates them.
#
# See README.md for what this adds over the built-in test suite.
#
# Usage:
#   validate_idc_case.sh <work-dir> <SEG-SeriesInstanceUID>
#
# The work directory is wiped at the start, so it must either not exist yet or
# have been created by an earlier run of this script.
#
# Environment:
#   DCMQI_BIN  directory holding itkimage2segimage/segimage2itkimage
#              (default: assumes they are on PATH)
#   PYTHON     interpreter that has highdicom and pydicom (default: python3)
#   IDC        idc-index command line tool (default: idc)
#
set -u -o pipefail

if [ $# -ne 2 ]; then
    sed -n '2,/^set -u/p' "$0" | sed 's/^# \?//'
    exit 1
fi

work=$1
seg_uid=$2
here=$(cd "$(dirname "$0")" && pwd)
DCMQI_BIN=${DCMQI_BIN:-}
PYTHON=${PYTHON:-python3}
IDC=${IDC:-idc}
itk2dcm=${DCMQI_BIN:+$DCMQI_BIN/}itkimage2segimage
dcm2itk=${DCMQI_BIN:+$DCMQI_BIN/}segimage2itkimage

for tool in "$itk2dcm" "$dcm2itk"; do
    command -v "$tool" >/dev/null 2>&1 || { echo "ERROR: $tool not found (set DCMQI_BIN)"; exit 1; }
done
command -v "$IDC" >/dev/null 2>&1 || { echo "ERROR: $IDC not found (pip install idc-index)"; exit 1; }

# The work directory is wiped below, so only accept one this script may own:
# either it does not exist yet, or it carries the marker of an earlier run.
marker=.dcmqi-multiframe-validation
case "$work" in
    ""|/|/*/..|*/..) echo "ERROR: refusing to use '$work' as work directory"; exit 1 ;;
esac
if [ -e "$work" ]; then
    if [ ! -d "$work" ]; then
        echo "ERROR: '$work' exists and is not a directory"; exit 1
    fi
    if [ ! -f "$work/$marker" ] && [ -n "$(ls -A "$work" 2>/dev/null)" ]; then
        echo "ERROR: '$work' is not empty and was not created by this script;"
        echo "       refusing to delete it. Pick a new directory."
        exit 1
    fi
    rm -rf "${work:?}"
fi
mkdir -p "$work/seg" "$work/series" "$work/itk"
touch "$work/$marker"

echo "--- downloading segmentation $seg_uid"
$IDC download --download-dir "$work/seg" --dir-template "" "$seg_uid" >/dev/null 2>&1 \
    || { echo "ERROR: could not download the segmentation"; exit 1; }

reference=$($PYTHON -c "
import glob, pydicom
seg = pydicom.dcmread(glob.glob('$work/seg/*.dcm')[0], stop_before_pixels=True)
print(seg.ReferencedSeriesSequence[0].SeriesInstanceUID)" 2>/dev/null)
[ -n "$reference" ] || { echo "ERROR: the segmentation names no referenced series"; exit 1; }

echo "--- downloading referenced series $reference"
$IDC download --download-dir "$work/series" --dir-template "" "$reference" >/dev/null 2>&1 \
    || { echo "ERROR: could not download the referenced series"; exit 1; }
echo "    $(ls "$work"/series/*.dcm 2>/dev/null | wc -l) instances"

echo "--- segmentation -> ITK (segimage2itkimage)"
$dcm2itk --inputDICOM "$work"/seg/*.dcm --outputDirectory "$work/itk" \
         --outputType nrrd --prefix s >/dev/null 2>&1 \
    || { echo "ERROR: segimage2itkimage failed"; exit 1; }
images=$(ls "$work"/itk/s-*.nrrd | paste -sd,)
echo "    $(ls "$work"/itk/s-*.nrrd | wc -l) label image(s)"

echo "--- classic series -> Legacy Converted Enhanced (highdicom)"
$PYTHON "$here/legacy_convert.py" "$work/series" "$work/source_enhanced.dcm" || exit 1

echo "--- itkimage2segimage against both sources"
# The conversions are piped into grep, so their own exit status has to be
# checked explicitly - a failure here must not fall through to the comparison,
# which would then report mismatches against missing or stale files.
convert() {
    label=$1; shift
    if ! "$@" > "$work/convert.log" 2>&1; then
        echo "ERROR: $label conversion failed:"
        tail -5 "$work/convert.log" | sed 's/^/    /'
        exit 1
    fi
    grep -E "slices mapped" "$work/convert.log" | tail -1 | sed "s|^|    $label: |"
}
convert "classic " "$itk2dcm" --inputMetadata "$work/itk/s-meta.json" --inputImageList "$images" \
        --inputDICOMDirectory "$work/series" --outputDICOM "$work/seg_classic.dcm" --skip 0
convert "enhanced" "$itk2dcm" --inputMetadata "$work/itk/s-meta.json" --inputImageList "$images" \
        --inputDICOMList "$work/source_enhanced.dcm" --outputDICOM "$work/seg_enhanced.dcm" --skip 0

echo "--- comparing"
$PYTHON "$here/compare_references.py" "$work/seg_classic.dcm" "$work/seg_enhanced.dcm" \
        "$work/source_enhanced.dcm"
status=$?

if command -v dciodvfy >/dev/null 2>&1; then
    echo "--- dciodvfy: classic $(dciodvfy "$work/seg_classic.dcm" 2>&1 | grep -c '^Error') /" \
         "enhanced $(dciodvfy "$work/seg_enhanced.dcm" 2>&1 | grep -c '^Error') errors"
fi

exit $status
