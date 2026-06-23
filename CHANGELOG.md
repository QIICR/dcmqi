# Changelog

All notable changes to **dcmqi** (DICOM for Quantitative Imaging) are documented here.

Releases predating this file were **back-filled from GitHub forensics**: GitHub's
pull-request release notes merged with direct-to-`master` commit messages, then
de-duplicated, noise-filtered and categorized. Entries therefore describe what was
worked on within each release range; a few items may have been superseded or reverted
within the same release. New releases are appended with the same tool -- see
`tools/changelog/`. The format follows [Keep a Changelog](https://keepachangelog.com/).


## [1.5.5](https://github.com/QIICR/dcmqi/releases/tag/v1.5.5) - 2026-06-23

### Added
- Add an in-memory handler overload of `itkimage2dcmSegmentation` (convert without going through disk) ([`5074f43`](https://github.com/QIICR/dcmqi/commit/5074f43), @rfloca)
- Add back-filled CHANGELOG.md and release-notes tooling ([`24c5f45`](https://github.com/QIICR/dcmqi/commit/24c5f45), @fedorov)

### Changed
- Designate a labelmap's background via Pixel Padding Value (0028,0120) instead of an inserted Background segment; this emits Pixel Padding Value on write and drops the `labelID 0` read entry that shifted segment metadata in 3D Slicer ([#541](https://github.com/QIICR/dcmqi/pull/541), @michaelonken)
- Allow gapped labelmap segment numbers with `--useLabelIDAsSegmentNumber`, reject colliding label IDs across inputs, and size bit depth from the highest segment number (fixes [#537](https://github.com/QIICR/dcmqi/issues/537)) ([#541](https://github.com/QIICR/dcmqi/pull/541), @michaelonken)
- Address SonarCloud findings: drop redundant `c_str()`, use `is_same_v` ([`145b7b4`](https://github.com/QIICR/dcmqi/commit/145b7b4), @rfloca)
- Remove noisy console output ([`3e32642`](https://github.com/QIICR/dcmqi/commit/3e32642), @fedorov)

### Fixed
- Fill missing labelmap slice positions with the Pixel Padding Value (or 0, with a warning) instead of refusing the conversion ([#541](https://github.com/QIICR/dcmqi/pull/541), @michaelonken)
- Fix ReDoS-prone regexes in changelog generator (SonarCloud S5852) ([`a29753b`](https://github.com/QIICR/dcmqi/commit/a29753b), @fedorov)
- Validate and tighten macOS dependency cache usage in CI ([`2dea14e`](https://github.com/QIICR/dcmqi/commit/2dea14e), @Copilot)

### Build & dependencies
- Bump DCMTK (5708ba6) so the designated labelmap background drives both the Background segment and Pixel Padding Value on write, as required by [#541](https://github.com/QIICR/dcmqi/pull/541) ([`bedc18d`](https://github.com/QIICR/dcmqi/commit/bedc18d), @michaelonken)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.5.4...v1.5.5)_

## [1.5.4](https://github.com/QIICR/dcmqi/releases/tag/v1.5.4) - 2026-05-18

### Added
- Add segment for background if required ([`1a7a074`](https://github.com/QIICR/dcmqi/commit/1a7a074), @michaelonken)

### Changed
- Make noDicomValueChecks an input parameter ([`830bdd8`](https://github.com/QIICR/dcmqi/commit/830bdd8), @michaelonken)
- Don't remove CIELab colors for monochr. labelmaps ([`fb4eea1`](https://github.com/QIICR/dcmqi/commit/fb4eea1), @michaelonken)
- No background Rec. CIE Lab Color in PALETTE mode ([`88bbb76`](https://github.com/QIICR/dcmqi/commit/88bbb76), @michaelonken)
- Make sure to update Palette / CIELab recomm ([`8bf5e5b`](https://github.com/QIICR/dcmqi/commit/8bf5e5b), @michaelonken)

### Build & dependencies
- Add direct ITK to DICOM Labelmap conversion ([#536](https://github.com/QIICR/dcmqi/pull/536), @michaelonken)
- Upgrade DCMTK to include recent enhancements ([`08ece47`](https://github.com/QIICR/dcmqi/commit/08ece47), @michaelonken)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.5.3...v1.5.4)_

## [1.5.3](https://github.com/QIICR/dcmqi/releases/tag/v1.5.3) - 2026-04-01

### Added
- Add concurrency groups to prevent stale prerelease artifacts ([#529](https://github.com/QIICR/dcmqi/pull/529), @fedorov)

### Changed
- Ignore invalid DICOM values when writing DICOM ([#530](https://github.com/QIICR/dcmqi/pull/530), @michaelonken)
- GitHub workflows improvements ([#532](https://github.com/QIICR/dcmqi/pull/532), @fedorov)
- Remove redundant virtual keyword from override methods ([`835b5d5`](https://github.com/QIICR/dcmqi/commit/835b5d5), @fedorov)
- Unify compression flags into single --compress enum option ([`47423dc`](https://github.com/QIICR/dcmqi/commit/47423dc), @fedorov)
- Re-enable --compress flag for itkimage2segimage with zlib support ([`250a7a1`](https://github.com/QIICR/dcmqi/commit/250a7a1), @fedorov)

### Fixed
- Use tight globs for tagged release uploads to avoid stale files ([#528](https://github.com/QIICR/dcmqi/pull/528), @fedorov)

### Build & dependencies
- Add labelmap-to-ITK conv. and round-trip tests ([#531](https://github.com/QIICR/dcmqi/pull/531), @michaelonken)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.5.2...v1.5.3)_

## [1.5.2](https://github.com/QIICR/dcmqi/releases/tag/v1.5.2) - 2026-02-25

### Changed
- Deprecate scikit, simplify release process ([#527](https://github.com/QIICR/dcmqi/pull/527), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.5.1...v1.5.2)_

## [1.5.1](https://github.com/QIICR/dcmqi/releases/tag/v1.5.1) - 2026-02-25

### Changed
- Allow RLE and ZLIB compressed output ([`45ae414`](https://github.com/QIICR/dcmqi/commit/45ae414), @michaelonken)

### Fixed
- Include architecture in macOS package name to fix release collision ([`324391b`](https://github.com/QIICR/dcmqi/commit/324391b), @fedorov)

### Build & dependencies
- Trying to fix slicer cdash linker error ([#524](https://github.com/QIICR/dcmqi/pull/524), @fedorov)
- Fix Windows build error ([#525](https://github.com/QIICR/dcmqi/pull/525), @lassoan)
- Add dcmqi library for tests ([`37e7ec7`](https://github.com/QIICR/dcmqi/commit/37e7ec7), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.5.0...v1.5.1)_

## [1.5.0](https://github.com/QIICR/dcmqi/releases/tag/v1.5.0) - 2026-01-16

### Added
- Add tests to confirm labelmap converter runs ([`3f0d7a7`](https://github.com/QIICR/dcmqi/commit/3f0d7a7), @fedorov)

### Changed
- Github workflows improvements ([#522](https://github.com/QIICR/dcmqi/pull/522), @fedorov)
- Apply suggestions from code review ([`a4de327`](https://github.com/QIICR/dcmqi/commit/a4de327), @fedorov)
- Safe random numbers, per-frame derivation images ([`80c9e02`](https://github.com/QIICR/dcmqi/commit/80c9e02), @michaelonken)
- Updated ZLIB repo to match Slicer's ([`4c8980a`](https://github.com/QIICR/dcmqi/commit/4c8980a), @michaelonken)
- DICOM binary segmentation to labelmap converter ([`54fbe9f`](https://github.com/QIICR/dcmqi/commit/54fbe9f), @michaelonken)
- Switch to windows-2022 runner for publish step ([`bd2015c`](https://github.com/QIICR/dcmqi/commit/bd2015c), @michaelonken)

### Fixed
- Fix incompatible macos arch ([#521](https://github.com/QIICR/dcmqi/pull/521), @fedorov)
- Indicate that overlapping binary seg test is expected to fail ([`b147a7a`](https://github.com/QIICR/dcmqi/commit/b147a7a), @fedorov)
- Fix DCMTK version comment (3.6.9++ not 3.6.8++) ([`afa4912`](https://github.com/QIICR/dcmqi/commit/afa4912), @michaelonken)

### Build & dependencies
- Update DCMTK to 3.7.0 patched + add converter from binary to labelmap segmentation ([#523](https://github.com/QIICR/dcmqi/pull/523), @fedorov)
- Enable position-independent code for zlib ([`5f99625`](https://github.com/QIICR/dcmqi/commit/5f99625), @fedorov)
- Switch to official DCMTK 3.6.8++ (June 6th 2025) ([`09e1833`](https://github.com/QIICR/dcmqi/commit/09e1833), @michaelonken)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.4.0...v1.5.0)_

## [1.4.0](https://github.com/QIICR/dcmqi/releases/tag/v1.4.0) - 2025-04-07

### Build & dependencies
- Switch away from external dependencies ([#516](https://github.com/QIICR/dcmqi/pull/516), @fedorov)
- Update from DCMTK 3.6.6 to 3.6.8 ([#500](https://github.com/QIICR/dcmqi/pull/500), @fedorov)
- Update ITK to 5.4.3 ([`ca02d00`](https://github.com/QIICR/dcmqi/commit/ca02d00), @jcfr)
- Update SlicerExecutionModel to include runtime libraries ([`85b460b`](https://github.com/QIICR/dcmqi/commit/85b460b), @jcfr)
- Update SlicerExecutionModel adding comments and renaming variables ([`d36c437`](https://github.com/QIICR/dcmqi/commit/d36c437), @jcfr)
- Update ExternalProjectDependency based on commontk/Artichoke@613e373 ([`c51f4c0`](https://github.com/QIICR/dcmqi/commit/c51f4c0), @jcfr)
- Ensure expected DCMTK revision is downloaded ([`bbe14f6`](https://github.com/QIICR/dcmqi/commit/bbe14f6), @jcfr)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.3.5...v1.4.0)_

## [1.3.5](https://github.com/QIICR/dcmqi/releases/tag/v1.3.5) - 2025-02-25

### Added
- Add template for input ImageType for itk2Dicom ([#507](https://github.com/QIICR/dcmqi/pull/507), @jadh4v)
- Add PixelType = short restriction to Itk2DicomConverter::itkimage2dcmSegmentation ([`9675603`](https://github.com/QIICR/dcmqi/commit/9675603), @jadh4v)

### Changed
- Use const pointer to input segmentations ([#502](https://github.com/QIICR/dcmqi/pull/502), @jadh4v)
- Upgrade actions/download-artifact ([#508](https://github.com/QIICR/dcmqi/pull/508), @fedorov)
- Improve error messages when input is empty ([#514](https://github.com/QIICR/dcmqi/pull/514), @fedorov)

### Fixed
- Explicitly convert to OFVector ([#504](https://github.com/QIICR/dcmqi/pull/504), @fedorov)
- Fix parameter incompatible with certain configuration flags ([#505](https://github.com/QIICR/dcmqi/pull/505), @fedorov)

### Build & dependencies
- Fix compile error ([#506](https://github.com/QIICR/dcmqi/pull/506), @fedorov)
- Update python executable location and usage ([#511](https://github.com/QIICR/dcmqi/pull/511), @thewtex)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.3.4...v1.3.5)_

## [1.3.4](https://github.com/QIICR/dcmqi/releases/tag/v1.3.4) - 2024-06-26

### Added
- Support source image references in the shared FG ([#501](https://github.com/QIICR/dcmqi/pull/501), @fedorov)

### Fixed
- Fixes for upgraded DCMTK and web-assembly build ([#496](https://github.com/QIICR/dcmqi/pull/496), @jadh4v)
- Includes for higher version of DCMTK (> v366) ([`d7b56e0`](https://github.com/QIICR/dcmqi/commit/d7b56e0), @jadh4v)

### Build & dependencies
- Do not reuse prebuilt DCMTK/ITK/ZLIB ([`d7b4dec`](https://github.com/QIICR/dcmqi/commit/d7b4dec), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.3.3...v1.3.4)_

## [1.3.3](https://github.com/QIICR/dcmqi/releases/tag/v1.3.3) - 2024-06-19

### Changed
- Update test to account for changed behavior of the parameter ([`bc19b0d`](https://github.com/QIICR/dcmqi/commit/bc19b0d), @fedorov)

### Fixed
- Switch to integer for the skip flag ([#499](https://github.com/QIICR/dcmqi/pull/499), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.3.2...v1.3.3)_

## [1.3.2](https://github.com/QIICR/dcmqi/releases/tag/v1.3.2) - 2024-04-23

### Changed
- Use SegmentDescription to populate SegmentLabel ([#488](https://github.com/QIICR/dcmqi/pull/488), @fedorov)
- Allow saving merged segments to all formats ([#495](https://github.com/QIICR/dcmqi/pull/495), @fedorov)
- Remove incorrect statement re 4D NRRD output ([`c0bef7a`](https://github.com/QIICR/dcmqi/commit/c0bef7a), @fedorov)
- Update python to 3.10.11 ([`c782574`](https://github.com/QIICR/dcmqi/commit/c782574), @fedorov)
- Update test JSON to make SegmentLabel defined ([`6c718af`](https://github.com/QIICR/dcmqi/commit/6c718af), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.3.1...v1.3.2)_

## [1.3.1](https://github.com/QIICR/dcmqi/releases/tag/v1.3.1) - 2024-01-04

### Added
- Add comment to clarify label filter application ([`4f98007`](https://github.com/QIICR/dcmqi/commit/4f98007), @fedorov)
- Added documentation ([`4d2bbb3`](https://github.com/QIICR/dcmqi/commit/4d2bbb3), @michaelonken)

### Changed
- Switch to framesorter class ([#484](https://github.com/QIICR/dcmqi/pull/484), @michaelonken)
- Use label id as segment numbers ([#485](https://github.com/QIICR/dcmqi/pull/485), @michaelonken)
- Removed unused debug printouts ([`2990dbd`](https://github.com/QIICR/dcmqi/commit/2990dbd), @michaelonken)
- Fixing/enhancing this PR ([`a4b433a`](https://github.com/QIICR/dcmqi/commit/a4b433a), @michaelonken)
- Use Label IDs as Segment Numbers ([`08a5ebd`](https://github.com/QIICR/dcmqi/commit/08a5ebd), @michaelonken)
- Various small enhancements ([`cee108c`](https://github.com/QIICR/dcmqi/commit/cee108c), @michaelonken)
- Replace deprecated writing routine ([`107c359`](https://github.com/QIICR/dcmqi/commit/107c359), @michaelonken)
- Use framesorter class instead homegrown sorting ([`3614ed3`](https://github.com/QIICR/dcmqi/commit/3614ed3), @michaelonken)
- Remember frame positions in Result, small changes ([`05b36fb`](https://github.com/QIICR/dcmqi/commit/05b36fb), @michaelonken)
- Ensure to also handle more than 65535 frames ([`5a9cc2f`](https://github.com/QIICR/dcmqi/commit/5a9cc2f), @michaelonken)

### Fixed
- Fix order of pixel spacing converting ITK->DICOM ([#486](https://github.com/QIICR/dcmqi/pull/486), @michaelonken)
- Fixed typos ([`3576a86`](https://github.com/QIICR/dcmqi/commit/3576a86), @michaelonken)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.3.0...v1.3.1)_

## [1.3.0](https://github.com/QIICR/dcmqi/releases/tag/v1.3.0) - 2023-11-19

### Highlights
- Upgrade to ITK v5
- Switch to GitHub Actions from platform-specific CI solutions
- Major performance improvements for DICOM SEG conversion
- Added ability to save non-overlapping DICOM SEG segments into a single file

### Added
- Enable testing on win and mac ([#481](https://github.com/QIICR/dcmqi/pull/481), @fedorov)
- Add support for saving segments in a single file ([#464](https://github.com/QIICR/dcmqi/pull/464), @fedorov)
- Add fetch tags to exisitng workflows ([#483](https://github.com/QIICR/dcmqi/pull/483), @vkt1414)
- Added test for merging segments into NRRD files ([`25d8165`](https://github.com/QIICR/dcmqi/commit/25d8165), @michaelonken)
- New method for finding frame sorting coordinate ([`4fbcfe3`](https://github.com/QIICR/dcmqi/commit/4fbcfe3), @michaelonken)
- Introduce OverlapUtil ([`3f85903`](https://github.com/QIICR/dcmqi/commit/3f85903), @michaelonken)

### Changed
- Only publish on master and on qiicr/dcmqi repo ([#480](https://github.com/QIICR/dcmqi/pull/480), @michaelonken)
- Don't check functional groups (speedup) ([#479](https://github.com/QIICR/dcmqi/pull/479), @michaelonken)
- Replace use of deprecated jsoncpp class ([`35e761f`](https://github.com/QIICR/dcmqi/commit/35e761f), @michaelonken)
- Return result image by image to save memory ([`549d24e`](https://github.com/QIICR/dcmqi/commit/549d24e), @michaelonken)
- Refactored converter classes ([`5761fc5`](https://github.com/QIICR/dcmqi/commit/5761fc5), @michaelonken)
- Cleanups, robustness, documentation ([`8c32931`](https://github.com/QIICR/dcmqi/commit/8c32931), @michaelonken)
- Error handling ([`921b947`](https://github.com/QIICR/dcmqi/commit/921b947), @michaelonken)
- Better error handling ([`57cd7ef`](https://github.com/QIICR/dcmqi/commit/57cd7ef), @michaelonken)
- Fully integrated code producing first results ([`d9dc34f`](https://github.com/QIICR/dcmqi/commit/d9dc34f), @michaelonken)
- Use ImportImageFilter + iterators to copy frame ([`0a4f0a1`](https://github.com/QIICR/dcmqi/commit/0a4f0a1), @fedorov)
- Remove blanks per codefactor ([`980ccd3`](https://github.com/QIICR/dcmqi/commit/980ccd3), @fedorov)
- Adding parameterization for merged segments ([`65b9418`](https://github.com/QIICR/dcmqi/commit/65b9418), @fedorov)
- Write all segments to the same file ([`52de374`](https://github.com/QIICR/dcmqi/commit/52de374), @fedorov)

### Fixed
- Fix release package publishing ([`923e6e5`](https://github.com/QIICR/dcmqi/commit/923e6e5), @vkt1414)
- Fix bug when creating JSON output for merges ([`c1f09ef`](https://github.com/QIICR/dcmqi/commit/c1f09ef), @michaelonken)
- Fixed tests and bug, minor changes ([`d677992`](https://github.com/QIICR/dcmqi/commit/d677992), @michaelonken)
- Fix assignment of frames to NRRD, speedup ([`ce07b70`](https://github.com/QIICR/dcmqi/commit/ce07b70), @michaelonken)
- Fix Image Origin bug and speed up processing ([`ca703ba`](https://github.com/QIICR/dcmqi/commit/ca703ba), @michaelonken)
- Fix metadata init with mergedSegments on ([`2daf29c`](https://github.com/QIICR/dcmqi/commit/2daf29c), @fedorov)
- Fix overwrite of segment metadata ([`cbad6aa`](https://github.com/QIICR/dcmqi/commit/cbad6aa), @fedorov)
- Fix tests ([`0b6e726`](https://github.com/QIICR/dcmqi/commit/0b6e726), @fedorov)
- Add segments to existing segmentation ([`ae1cab1`](https://github.com/QIICR/dcmqi/commit/ae1cab1), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.2.6...v1.3.0)_

## [1.2.6](https://github.com/QIICR/dcmqi/releases/tag/v1.2.6) - 2023-10-27

### Changed
- Explore upgrading to use ITKv5 ([#444](https://github.com/QIICR/dcmqi/pull/444), @fedorov)
- Migrate to Github Action ([#475](https://github.com/QIICR/dcmqi/pull/475), @fedorov)
- Don't release if user is not QIICR ([`b22ac4c`](https://github.com/QIICR/dcmqi/commit/b22ac4c), @michaelonken)
- Update zlib package ([`bc2049c`](https://github.com/QIICR/dcmqi/commit/bc2049c), @fedorov)
- Switch to use ITKv5 ([`df8be18`](https://github.com/QIICR/dcmqi/commit/df8be18), @fedorov)

### Fixed
- Ensure binaries bundled into dcmqi docker image can run ([#466](https://github.com/QIICR/dcmqi/pull/466), @jcfr)
- Fix Windows Release publishing ([#476](https://github.com/QIICR/dcmqi/pull/476), @michaelonken)

### Build & dependencies
- Support for new dcmtk targets ([#459](https://github.com/QIICR/dcmqi/pull/459), @lorteddie)
- Find dcmtk and itk in exported config, removed hard coded dcmtk path ([#461](https://github.com/QIICR/dcmqi/pull/461), @lorteddie)
- Separate build and publish phase in CI ([#477](https://github.com/QIICR/dcmqi/pull/477), @michaelonken)
- Cache ITK, DCMTK and ZLIB for dcmqi build ([`36aa988`](https://github.com/QIICR/dcmqi/commit/36aa988), @michaelonken)
- Add regression test "docker_image_test" ([`d061233`](https://github.com/QIICR/dcmqi/commit/d061233), @jcfr)
- Update to use ITK with XML IO ([`cc4c0f7`](https://github.com/QIICR/dcmqi/commit/cc4c0f7), @fedorov)
- Remove references to multi threader ([`30976f2`](https://github.com/QIICR/dcmqi/commit/30976f2), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.2.5...v1.2.6)_

## [1.2.5](https://github.com/QIICR/dcmqi/releases/tag/v1.2.5) - 2022-07-18

### Changed
- Dcmqi install step improvements ([#432](https://github.com/QIICR/dcmqi/pull/432), @lorteddie)
- Switch from ceil() to round() for slice number calculation ([#442](https://github.com/QIICR/dcmqi/pull/442), @fedorov)
- Fixing TravisCI integration ([#443](https://github.com/QIICR/dcmqi/pull/443), @fedorov)
- Migrate to Ubuntu 20.04 ([#456](https://github.com/QIICR/dcmqi/pull/456), @fedorov)
- Attempt at fixing load of RLE encoded segmentations ([#457](https://github.com/QIICR/dcmqi/pull/457), @fedorov)
- Update TravisCI pointer ([`a6dbda4`](https://github.com/QIICR/dcmqi/commit/a6dbda4), @fedorov)
- Removed unnecessary include ([`d6085c9`](https://github.com/QIICR/dcmqi/commit/d6085c9), Tobias Weihs)
- Install generated internal include directory ([`205dd94`](https://github.com/QIICR/dcmqi/commit/205dd94), Tobias Weihs)

### Fixed
- Fix initialization of referenced instances ([#450](https://github.com/QIICR/dcmqi/pull/450), @fedorov)
- Check for in-plane consistency of seg size ([#455](https://github.com/QIICR/dcmqi/pull/455), @fedorov)
- Register RLE codecs before loading data ([`099a079`](https://github.com/QIICR/dcmqi/commit/099a079), @fedorov)
- Install config with correct paths ([`9e1529d`](https://github.com/QIICR/dcmqi/commit/9e1529d), Tobias Weihs)

### Build & dependencies
- Will it work with C++ 14? ([#448](https://github.com/QIICR/dcmqi/pull/448), @fedorov)
- Use https instead of git for clones ([#454](https://github.com/QIICR/dcmqi/pull/454), @fedorov)
- Fix Slicer extension build by not forcing CMAKE_CXX_STANDARD value ([#458](https://github.com/QIICR/dcmqi/pull/458), @jcfr)
- Simplify setting of CMAKE_CXX_STANDARD defaulting to C++14 ([`d499867`](https://github.com/QIICR/dcmqi/commit/d499867), @jcfr)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.2.4...v1.2.5)_

## [1.2.4](https://github.com/QIICR/dcmqi/releases/tag/v1.2.4) - 2021-03-10

### Added
- Added guard to ensure segmentation object is deleted ([#418](https://github.com/QIICR/dcmqi/pull/418), @lorteddie)
- Added target export installing if superbuild is disabled ([`191d9d8`](https://github.com/QIICR/dcmqi/commit/191d9d8), Tobias Weihs)
- Added missing headers ([`6a4ea56`](https://github.com/QIICR/dcmqi/commit/6a4ea56), Tobias Weihs)

### Changed
- #419 do not delete a shallow copied frame, only delete if necessary ([#420](https://github.com/QIICR/dcmqi/pull/420), @lorteddie)
- #421 added overload taking a seg object ([#422](https://github.com/QIICR/dcmqi/pull/422), @lorteddie)
- Dcmqi target install step ([#423](https://github.com/QIICR/dcmqi/pull/423), @lorteddie)
- No cpp11 ([`4f6040a`](https://github.com/QIICR/dcmqi/commit/4f6040a), Tobias Weihs)
- Cannot use auto ([`c5a176f`](https://github.com/QIICR/dcmqi/commit/c5a176f), Tobias Weihs)
- Replaced nullptr by NULL macro ([`96c0df0`](https://github.com/QIICR/dcmqi/commit/96c0df0), Tobias Weihs)
- #421 added overload taking a seg object, for this overload the json meta generation is optional ([`54b5f69`](https://github.com/QIICR/dcmqi/commit/54b5f69), Tobias Weihs)

### Fixed
- Serialize floats with maximum precision ([#416](https://github.com/QIICR/dcmqi/pull/416), @kislinsk)
- PixelSpacing order incorrect on read ([#426](https://github.com/QIICR/dcmqi/pull/426), @fedorov)

### Build & dependencies
- Update DCMTK to 3.6.6 patched for seg ([#431](https://github.com/QIICR/dcmqi/pull/431), @fedorov)
- Made include directories pointing into build directory build dependent ([`8ffaa9a`](https://github.com/QIICR/dcmqi/commit/8ffaa9a), Tobias Weihs)
- Force DCMTK build to link dynamic runtime library to avoid mixing static vs dynamic ([`233e0bd`](https://github.com/QIICR/dcmqi/commit/233e0bd), Tobias Weihs)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.2.3...v1.2.4)_

## [1.2.3](https://github.com/QIICR/dcmqi/releases/tag/v1.2.3) - 2020-12-28

### Added
- Add cardinality constraint to the schemas ([#401](https://github.com/QIICR/dcmqi/pull/401), @fedorov)
- Added detailed logging option to itkimage2segimage and segimage2itkimage ([#409](https://github.com/QIICR/dcmqi/pull/409), @lassoan)
- Improve seg conversion support ([`9fd114b`](https://github.com/QIICR/dcmqi/commit/9fd114b), @fedorov)
- Added the PYTHON script which replaces srt wiht sct ([`0e4d22c`](https://github.com/QIICR/dcmqi/commit/0e4d22c), afshinmessiah)

### Changed
- Replaced the SRT codes with SCT codes ([#393](https://github.com/QIICR/dcmqi/pull/393), @afshinmessiah)
- Compare to epsilon instead of zero ([#403](https://github.com/QIICR/dcmqi/pull/403), @fedorov)
- Initialize SegmentsOverlap for simple case ([#411](https://github.com/QIICR/dcmqi/pull/411), @fedorov)
- Switch to CIELab conversion from @dclunie ([`ab6a207`](https://github.com/QIICR/dcmqi/commit/ab6a207), @fedorov)
- Update readme ([`6526213`](https://github.com/QIICR/dcmqi/commit/6526213), @fedorov)
- Update support options ([`1ed403e`](https://github.com/QIICR/dcmqi/commit/1ed403e), @fedorov)
- Revise multi-segment example ([`93b544c`](https://github.com/QIICR/dcmqi/commit/93b544c), @fedorov)
- Final step SRT to SCT conversion (1) 'G-C306'-->'370129005' (2) 'A-10044'-->'111102' (3) 'T-00317'-->'55940004' (4) 'R-404A4'-->'255503000' ([`802f7f6`](https://github.com/QIICR/dcmqi/commit/802f7f6), afshinmessiah)
- Replaced all SRT wiht their SCT counterparts except for 4: (1)G-C306		"Measurement Method" (2)A-10044		"Non-Lesion Object Type" (3)T-00317		"FindingSite" (4)R-404A4		"Entire" ([`0cf2829`](https://github.com/QIICR/dcmqi/commit/0cf2829), afshinmessiah)
- Got the reader to read both srt/sct codes ([`3301704`](https://github.com/QIICR/dcmqi/commit/3301704), afshinmessiah)
- Used dcm.h macros instead of direct code strings for dcm codes. The list was incomplete for sct&ucum ([`626b52f`](https://github.com/QIICR/dcmqi/commit/626b52f), afshinmessiah)
- Replaced the srt code with sct codes (the ones are available at snomed dictionary of pydicom) ([`9720beb`](https://github.com/QIICR/dcmqi/commit/9720beb), afshinmessiah)
- Check both SCT and SRT codes ([`0203ebf`](https://github.com/QIICR/dcmqi/commit/0203ebf), @fedorov)

### Fixed
- Fix loading of single-frame segmentation ([#407](https://github.com/QIICR/dcmqi/pull/407), @lassoan)
- Fix RGB-CIELab conversion ([#413](https://github.com/QIICR/dcmqi/pull/413), @fedorov)
- Downgrade ajv to 3.3.0 ([`235da0e`](https://github.com/QIICR/dcmqi/commit/235da0e), @fedorov)
- Switch to dcmtk for CIELab conversion ([`af51c62`](https://github.com/QIICR/dcmqi/commit/af51c62), @fedorov)
- Fix typos ([`65fb9f3`](https://github.com/QIICR/dcmqi/commit/65fb9f3), @fedorov)
- Create a new example ([`86ca966`](https://github.com/QIICR/dcmqi/commit/86ca966), @fedorov)
- Remove midas pointers ([`101df86`](https://github.com/QIICR/dcmqi/commit/101df86), @fedorov)
- Fix SR roundtrip test by checking for SCT codes ([`17b46c9`](https://github.com/QIICR/dcmqi/commit/17b46c9), @fedorov)
- Update SRT to SCT codes ([`8f98b6f`](https://github.com/QIICR/dcmqi/commit/8f98b6f), @fedorov)

### Build & dependencies
- Update to DCMTK 3.6.5 ([#391](https://github.com/QIICR/dcmqi/pull/391), @fedorov)
- Turn off SNDFILE to fix include path ([#397](https://github.com/QIICR/dcmqi/pull/397), @pieper)
- Update the documentation link ([`3efde87`](https://github.com/QIICR/dcmqi/commit/3efde87), @fedorov)
- Turn of SNDFILE to fix include path ([`40f8eee`](https://github.com/QIICR/dcmqi/commit/40f8eee), @pieper)
- Update DCMTK/ITK to fix static link issue ([`d07f2f4`](https://github.com/QIICR/dcmqi/commit/d07f2f4), @fedorov)
- Remove use of CERR and COUT ([`f5e4b79`](https://github.com/QIICR/dcmqi/commit/f5e4b79), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.2.2...v1.2.3)_

## [1.2.2](https://github.com/QIICR/dcmqi/releases/tag/v1.2.2) - 2020-01-22

### Added
- Add SEG SegmentLabel support ([#376](https://github.com/QIICR/dcmqi/pull/376), @fedorov)
- Add warning if composite context is not initialized ([`55dc95e`](https://github.com/QIICR/dcmqi/commit/55dc95e), @fedorov)

### Changed
- Improve error reporting for seg converter ([#380](https://github.com/QIICR/dcmqi/pull/380), @fedorov)
- Improve error handling ([#381](https://github.com/QIICR/dcmqi/pull/381), @fedorov)
- Populate 3dSlicerIntegerLabel in JSON seg context ([`ab4e594`](https://github.com/QIICR/dcmqi/commit/ab4e594), @fedorov)

### Fixed
- Fix and add missing codes to the Slicer terminology file ([#389](https://github.com/QIICR/dcmqi/pull/389), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.2.1...v1.2.2)_

## [1.2.1](https://github.com/QIICR/dcmqi/releases/tag/v1.2.1) - 2019-02-01

### Added
- Add missing checkout comment to CircleCI deploy-* jobs ([#373](https://github.com/QIICR/dcmqi/pull/373), @jcfr)

### Changed
- Update to jsoncpp 1.8.4 ([#368](https://github.com/QIICR/dcmqi/pull/368), @fedorov)
- Circleci: Use workflow to separately upload latest and tagged releases ([#370](https://github.com/QIICR/dcmqi/pull/370), @jcfr)
- Update deploy-* jobs activating the associated virtual env ([#372](https://github.com/QIICR/dcmqi/pull/372), @jcfr)
- Circleci: Ensure workflow is not triggered when latest(-tmp) branch are pushed ([#375](https://github.com/QIICR/dcmqi/pull/375), @jcfr)

### Fixed
- Fix installation of python packages in deploy-* jobs ([#371](https://github.com/QIICR/dcmqi/pull/371), @jcfr)
- Change handling of RWV slope to fix test regression ([`fe3d681`](https://github.com/QIICR/dcmqi/commit/fe3d681), @fedorov)

### Build & dependencies
- Ensure dcmqi can be build with DCMQI_BUILD_APPS set to OFF ([#369](https://github.com/QIICR/dcmqi/pull/369), @jcfr)
- Fix building of project against DCMQI when DCMQI_BUILD_APPS is OFF ([#374](https://github.com/QIICR/dcmqi/pull/374), @jcfr)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.2.0...v1.2.1)_

## [1.2.0](https://github.com/QIICR/dcmqi/releases/tag/v1.2.0) - 2018-11-16

### Added
- Add new small non-square seg test datasets ([#334](https://github.com/QIICR/dcmqi/pull/334), @fedorov)
- Add qualitative evaluations to TID 1500 converters ([#353](https://github.com/QIICR/dcmqi/pull/353), @fedorov)
- Add support of segment tracking ID/UID ([#357](https://github.com/QIICR/dcmqi/pull/357), @fedorov)
- Add support for finding site laterality ([#359](https://github.com/QIICR/dcmqi/pull/359), @fedorov)
- Add support for describing device observer context ([#360](https://github.com/QIICR/dcmqi/pull/360), @fedorov)
- Add support for AlgorithmIdentification at the group level ([`3b87e92`](https://github.com/QIICR/dcmqi/commit/3b87e92), @fedorov)
- Add support for algorithm parameters ([`07e26d4`](https://github.com/QIICR/dcmqi/commit/07e26d4), @fedorov)
- Add support of tracking ID/UID ([`3eb503a`](https://github.com/QIICR/dcmqi/commit/3eb503a), @fedorov)
- Add support of qualitative assessment to tid1500reader ([`c346fe3`](https://github.com/QIICR/dcmqi/commit/c346fe3), @fedorov)
- Added support for writing SR qualitative evaluation items ([`163574f`](https://github.com/QIICR/dcmqi/commit/163574f), @fedorov)
- Update schema and example to support qualitative evaluations ([`7a77a88`](https://github.com/QIICR/dcmqi/commit/7a77a88), @fedorov)
- Add a test with non-numeric measurement values ([`d08f92e`](https://github.com/QIICR/dcmqi/commit/d08f92e), @fedorov)
- Add a tool to print the content of SEG frames ([`9cd494f`](https://github.com/QIICR/dcmqi/commit/9cd494f), @fedorov)
- Screenshots of the new objects from BrainLab ([`c172992`](https://github.com/QIICR/dcmqi/commit/c172992), @fedorov)
- Added screenshots for test images ([`d74bfca`](https://github.com/QIICR/dcmqi/commit/d74bfca), @fedorov)
- Added multiframe images for the sample SEG datasets ([`58d8fe7`](https://github.com/QIICR/dcmqi/commit/58d8fe7), @fedorov)
- Add new small non-square seg test dataset ([`8118cb3`](https://github.com/QIICR/dcmqi/commit/8118cb3), @fedorov)

### Changed
- Skip invalid measurements ([#290](https://github.com/QIICR/dcmqi/pull/290), @fedorov)
- Initial iteration of CircleCI 2.0 migration ([#339](https://github.com/QIICR/dcmqi/pull/339), @fedorov)
- Tweak AppVeyor, TravisCI and CircleCI ([#351](https://github.com/QIICR/dcmqi/pull/351), @jcfr)
- Clean up console output ([#350](https://github.com/QIICR/dcmqi/pull/350), @fedorov)
- Circleci/publish_github_release: do not fail if there no token available ([#355](https://github.com/QIICR/dcmqi/pull/355), @jcfr)
- Improve algorithm identification support ([#363](https://github.com/QIICR/dcmqi/pull/363), @fedorov)
- Install jsondiff to prepare switch from jsoncompare ([`5bcc93d`](https://github.com/QIICR/dcmqi/commit/5bcc93d), @fedorov)
- Remove unnecessary temporary cursor ([`6c9aa2a`](https://github.com/QIICR/dcmqi/commit/6c9aa2a), @fedorov)
- Travis_ci: remove travis_wait (command not found?!) ([`91fd5ca`](https://github.com/QIICR/dcmqi/commit/91fd5ca), @fedorov)
- Circle: Simplify using install_cmake and ctest_junit_formatter add-on ([`1cbf225`](https://github.com/QIICR/dcmqi/commit/1cbf225), @jcfr)
- Replace 99QIICR codes for parametric map converter ([`6fd6e94`](https://github.com/QIICR/dcmqi/commit/6fd6e94), @fedorov)
- Improve test dataset ([`5f53138`](https://github.com/QIICR/dcmqi/commit/5f53138), @fedorov)
- Adding another test ([`482d357`](https://github.com/QIICR/dcmqi/commit/482d357), @fedorov)

### Fixed
- ClinicalTrialSeriesID was never read properly from metadata ([#344](https://github.com/QIICR/dcmqi/pull/344), @che85)
- Fix dockcross script creation container cleanup ([#347](https://github.com/QIICR/dcmqi/pull/347), @thewtex)
- Update sample dataset ([`6add0c5`](https://github.com/QIICR/dcmqi/commit/6add0c5), @fedorov)
- Fix issues missed earlier due to jsoncompare ([`3eba636`](https://github.com/QIICR/dcmqi/commit/3eba636), @fedorov)
- Make algorithm parameter an array on read ([`96ecbc5`](https://github.com/QIICR/dcmqi/commit/96ecbc5), @fedorov)
- ObserverName was not read from SR ([`97115aa`](https://github.com/QIICR/dcmqi/commit/97115aa), @fedorov)
- Set the verificatin flag to UNVERIFIED ([`2b422ab`](https://github.com/QIICR/dcmqi/commit/2b422ab), @fedorov)
- Remove invalid source of describing person observer ([`3b2dc9c`](https://github.com/QIICR/dcmqi/commit/3b2dc9c), @fedorov)
- Fix unintentional commit ([`21fb851`](https://github.com/QIICR/dcmqi/commit/21fb851), @fedorov)
- Increase no_output_timeout from the default 10m ([`83c243b`](https://github.com/QIICR/dcmqi/commit/83c243b), @fedorov)
- Add non-nan measurement to the nan test ([`9c0ac72`](https://github.com/QIICR/dcmqi/commit/9c0ac72), @fedorov)
- Skip measurements that cause issues on write ([`4f70fdc`](https://github.com/QIICR/dcmqi/commit/4f70fdc), @fedorov)

### Build & dependencies
- Update DCMTK and ITK appveyor packages ([#345](https://github.com/QIICR/dcmqi/pull/345), @fedorov)
- Do not use C++11 specific feature to count mapped slices ([`05c4355`](https://github.com/QIICR/dcmqi/commit/05c4355), @fedorov)
- Update DCMTK ([`485f966`](https://github.com/QIICR/dcmqi/commit/485f966), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.1.0...v1.2.0)_

## [1.1.0](https://github.com/QIICR/dcmqi/releases/tag/v1.1.0) - 2018-03-07

### Added
- Add support of algorithm identification ([#329](https://github.com/QIICR/dcmqi/pull/329), @fedorov)
- Added automatic generation of extension config ([#330](https://github.com/QIICR/dcmqi/pull/330), @che85)

### Changed
- Update buildsystem ([#337](https://github.com/QIICR/dcmqi/pull/337), @jcfr)
- Reformat and document circleci 1.0 config to facilitate transition to 2.0 ([#340](https://github.com/QIICR/dcmqi/pull/340), @jcfr)
- Dcmsr related style improvements ([`4abdfd2`](https://github.com/QIICR/dcmqi/commit/4abdfd2), @fedorov)
- Initial work to add support of algorithm identification ([`289fa8e`](https://github.com/QIICR/dcmqi/commit/289fa8e), @fedorov)

### Fixed
- Move down mis-placed ctest include ([`ab9e047`](https://github.com/QIICR/dcmqi/commit/ab9e047), @fedorov)
- Fixing discovery of python ([`6d15aeb`](https://github.com/QIICR/dcmqi/commit/6d15aeb), @fedorov)
- Remove dependency on jsonlint ([`7ba722e`](https://github.com/QIICR/dcmqi/commit/7ba722e), @fedorov)
- Adapt to the DCMTK 3.6.3 API changes ([`7bd229e`](https://github.com/QIICR/dcmqi/commit/7bd229e), @fedorov)
- Bug fixes ([`a13a062`](https://github.com/QIICR/dcmqi/commit/a13a062), @fedorov)

### Build & dependencies
- Update to work with the API of DCMTK 3.6.3 ([#333](https://github.com/QIICR/dcmqi/pull/333), @fedorov)
- Cmake: Initialize CMAKE_CXX_STANDARD to 11 if building against Slicer ([#336](https://github.com/QIICR/dcmqi/pull/336), @jcfr)
- Cmake: Simplify build system using new feature from ExternalProjectDependency ([`6609c08`](https://github.com/QIICR/dcmqi/commit/6609c08), @jcfr)
- Cmake: Update ExternalProjectDependency based on commontk/Artichoke@dff914a ([`cebcf11`](https://github.com/QIICR/dcmqi/commit/cebcf11), @jcfr)
- Use cmake to find python ([`9d59bfc`](https://github.com/QIICR/dcmqi/commit/9d59bfc), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.0.9...v1.1.0)_

## [1.0.9](https://github.com/QIICR/dcmqi/releases/tag/v1.0.9) - 2018-01-12

### Changed
- Update version numbers for the release ([`8657ffe`](https://github.com/QIICR/dcmqi/commit/8657ffe), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.0.8...v1.0.9)_

## [1.0.8](https://github.com/QIICR/dcmqi/releases/tag/v1.0.8) - 2018-01-12

### Added
- Added ProcedureReported and introduced a generic code sequence base class ([`9475369`](https://github.com/QIICR/dcmqi/commit/9475369), Michael Schwier)
- Introduced trackingUniqueIdentifier and more flexibility for SeriesDescription ([`d22dcbb`](https://github.com/QIICR/dcmqi/commit/d22dcbb), Michael Schwier)
- Added Visual Studio Code settings directory to ignored files ([`4f72dea`](https://github.com/QIICR/dcmqi/commit/4f72dea), Michael Schwier)

### Changed
- Utils to create measurement reports from python ([`c5582fc`](https://github.com/QIICR/dcmqi/commit/c5582fc), Michael Schwier)

### Fixed
- Update CircleCi config to fix upload of releases ([#327](https://github.com/QIICR/dcmqi/pull/327), @jcfr)

### Build & dependencies
- Util Classes to build Measurement Report and generate JSON ([#317](https://github.com/QIICR/dcmqi/pull/317), @michaelschwier)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.0.7...v1.0.8)_

## [1.0.7](https://github.com/QIICR/dcmqi/releases/tag/v1.0.7) - 2018-01-11

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.0.6...v1.0.7)_

## [1.0.6](https://github.com/QIICR/dcmqi/releases/tag/v1.0.6) - 2018-01-11

### Added
- Add output of missing SR TID1500 attributes ([#257](https://github.com/QIICR/dcmqi/pull/257), @fedorov)
- Add a test of converting pm with no derivation items ([#270](https://github.com/QIICR/dcmqi/pull/270), @fedorov)
- Experiment to enable codecov coverage testing ([#272](https://github.com/QIICR/dcmqi/pull/272), @fedorov)
- Add modifiers and parameters ([#287](https://github.com/QIICR/dcmqi/pull/287), @fedorov)
- Add an option to specify CMAKE_CXX_STANDARD ([#293](https://github.com/QIICR/dcmqi/pull/293), @nolden)
- Add support for initializing measurement properties ([#309](https://github.com/QIICR/dcmqi/pull/309), @fedorov)
- Implement improved tid1500reader ([`59e4776`](https://github.com/QIICR/dcmqi/commit/59e4776), @fedorov)
- Add support of modifiers and derivation parameters ([`d2d819a`](https://github.com/QIICR/dcmqi/commit/d2d819a), @fedorov)
- Enable codecov coverage testing ([`b51e0dc`](https://github.com/QIICR/dcmqi/commit/b51e0dc), @fedorov)
- Add separate tests for a square and non-square pixel matrices ([`b0ce2e3`](https://github.com/QIICR/dcmqi/commit/b0ce2e3), @fedorov)
- Added an option to specify CMAKE_CXX_STANDARD ([`961cc82`](https://github.com/QIICR/dcmqi/commit/961cc82), @nolden)
- Add clarification comment ([`aacfcbe`](https://github.com/QIICR/dcmqi/commit/aacfcbe), @fedorov)

### Changed
- Update jsoncpp amalgamated header to 0.10.6 ([#238](https://github.com/QIICR/dcmqi/pull/238), @fedorov)
- Improve package naming and generate packages ([#236](https://github.com/QIICR/dcmqi/pull/236), @jcfr)
- Adding checks for existence of file/files in file list (issue #225) ([#254](https://github.com/QIICR/dcmqi/pull/254), @che85)
- Use concept code constants ([#258](https://github.com/QIICR/dcmqi/pull/258), @fedorov)
- Make composite context/image library directories parameters optional in tid1500writer ([#269](https://github.com/QIICR/dcmqi/pull/269), @fedorov)
- Update scikit-ci-addons to handle leftover temporary release tag ([#285](https://github.com/QIICR/dcmqi/pull/285), @jcfr)
- Simplify code by better utilizing dcmsr capabilities ([#311](https://github.com/QIICR/dcmqi/pull/311), @fedorov)
- Update DCM codes assigned in CP-1665 ([#313](https://github.com/QIICR/dcmqi/pull/313), @fedorov)
- Allow to specify Procedure Reported code ([#318](https://github.com/QIICR/dcmqi/pull/318), @fedorov)
- Check that source file contains PixelData ([#323](https://github.com/QIICR/dcmqi/pull/323), @fedorov)
- Update README.md ([`6ddae90`](https://github.com/QIICR/dcmqi/commit/6ddae90), @fedorov)
- Update publication citation ([`463fa31`](https://github.com/QIICR/dcmqi/commit/463fa31), @fedorov)
- Update publication ([`a7847ee`](https://github.com/QIICR/dcmqi/commit/a7847ee), @fedorov)
- Update TID1500 reader to read additional properties ([`2029f87`](https://github.com/QIICR/dcmqi/commit/2029f87), @fedorov)
- Remove verbose output about added source image items ([`87fce09`](https://github.com/QIICR/dcmqi/commit/87fce09), @fedorov)
- Make comments consistent ([`76f5ebd`](https://github.com/QIICR/dcmqi/commit/76f5ebd), @fedorov)
- Minor changes ([`2c6602e`](https://github.com/QIICR/dcmqi/commit/2c6602e), @fedorov)
- Finalize file order mapping ([`2464b26`](https://github.com/QIICR/dcmqi/commit/2464b26), @fedorov)
- Make mkdir happy if the directory exists ([`60d66e1`](https://github.com/QIICR/dcmqi/commit/60d66e1), @fedorov)
- Further simplify mentioning of the license ([`ebdd1b0`](https://github.com/QIICR/dcmqi/commit/ebdd1b0), @fedorov)
- Update license wording in README ([`8a55b89`](https://github.com/QIICR/dcmqi/commit/8a55b89), @fedorov)
- Switch to 3-clause BSD license ([`3ccf847`](https://github.com/QIICR/dcmqi/commit/3ccf847), @fedorov)
- Make paths optional in the main app checks ([`d8af879`](https://github.com/QIICR/dcmqi/commit/d8af879), @fedorov)
- Make directory paths for SR converter optional ([`88830a0`](https://github.com/QIICR/dcmqi/commit/88830a0), @fedorov)
- Update README ([`adedb36`](https://github.com/QIICR/dcmqi/commit/adedb36), @fedorov)
- Read measurement method at the measurement group level ([`4a9dc73`](https://github.com/QIICR/dcmqi/commit/4a9dc73), @fedorov)
- Adding checks for existence of path/paths in file list (issue #225) ([`5f95138`](https://github.com/QIICR/dcmqi/commit/5f95138), @che85)
- Remove empty lines from Makefile ([`1127a37`](https://github.com/QIICR/dcmqi/commit/1127a37), @fedorov)
- Circle: Remove prerelease tag "latest" to avoid confusing "git describe" ([`7a9da64`](https://github.com/QIICR/dcmqi/commit/7a9da64), @jcfr)
- Packages: Automatically upload prerelease and release packages ([`00d21fe`](https://github.com/QIICR/dcmqi/commit/00d21fe), @jcfr)
- Update the latest release version ([`398d973`](https://github.com/QIICR/dcmqi/commit/398d973), @fedorov)

### Fixed
- Fixed typo that has been corrected in most recent dcmtk ([#241](https://github.com/QIICR/dcmqi/pull/241), @che85)
- Fix linux packaging issues ([#245](https://github.com/QIICR/dcmqi/pull/245), @fedorov)
- Check for the number of inner lists in metadata ([#246](https://github.com/QIICR/dcmqi/pull/246), @fedorov)
- Fix referenced instances item initialization ([#249](https://github.com/QIICR/dcmqi/pull/249), @fedorov)
- Run npm with sudo to fix the issue on CircleCI ([#256](https://github.com/QIICR/dcmqi/pull/256), @fedorov)
- Fix initialization of the derivation ([#264](https://github.com/QIICR/dcmqi/pull/264), @fedorov)
- Fix segfaults due to missing null check and a typo ([#271](https://github.com/QIICR/dcmqi/pull/271), @fedorov)
- Fix the order of parameters to import() call ([#282](https://github.com/QIICR/dcmqi/pull/282), @fedorov)
- Update scikit-ci-addons to fix upload of latest releases. See #260 ([#284](https://github.com/QIICR/dcmqi/pull/284), @jcfr)
- Read TrackingUniqueIdentifier ([#288](https://github.com/QIICR/dcmqi/pull/288), @fedorov)
- Fix missing anatomic region modifier on read ([#296](https://github.com/QIICR/dcmqi/pull/296), @fedorov)
- Fix warnings ([#299](https://github.com/QIICR/dcmqi/pull/299), @jcfr)
- Cmake: Fix forcebuild step ([#300](https://github.com/QIICR/dcmqi/pull/300), @jcfr)
- Fix setting of cxx standard ([#298](https://github.com/QIICR/dcmqi/pull/298), @jcfr)
- Fix some CXX11 errors ([#304](https://github.com/QIICR/dcmqi/pull/304), @nolden)
- Read unavailable JSON::Value attribute initializes it to NULL ([#321](https://github.com/QIICR/dcmqi/pull/321), @che85)
- Read unavailable JSON::Value attribute initializes it to NULL (issue #320) ([`f4af788`](https://github.com/QIICR/dcmqi/commit/f4af788), @che85)
- Fix attribute name for procedureReported ([`e5c3934`](https://github.com/QIICR/dcmqi/commit/e5c3934), @fedorov)
- Fix tree traversal ([`942d29b`](https://github.com/QIICR/dcmqi/commit/942d29b), @fedorov)
- Change num property type to string ([`1308e08`](https://github.com/QIICR/dcmqi/commit/1308e08), @fedorov)
- Fixed DCMTK iterator usage ([`bafe3c8`](https://github.com/QIICR/dcmqi/commit/bafe3c8), @nolden)
- Remove link to SPL Publications database PDF ([`ef17bba`](https://github.com/QIICR/dcmqi/commit/ef17bba), @fedorov)
- Update DCMTK tag and packages ([`0f81557`](https://github.com/QIICR/dcmqi/commit/0f81557), @fedorov)
- Fix typos and errors ([`01b4dcd`](https://github.com/QIICR/dcmqi/commit/01b4dcd), @fedorov)
- Fix duplicate test name ([`4edabc8`](https://github.com/QIICR/dcmqi/commit/4edabc8), @fedorov)
- Add segmentAttributesFileMapping ([`d56edb6`](https://github.com/QIICR/dcmqi/commit/d56edb6), @fedorov)
- Reduce intensity tolerance test parameter ([`cb7a701`](https://github.com/QIICR/dcmqi/commit/cb7a701), @fedorov)
- Do not cache codecov build directory ([`4b2d11e`](https://github.com/QIICR/dcmqi/commit/4b2d11e), @fedorov)
- Remove extra directory from coverage report ([`c2cb641`](https://github.com/QIICR/dcmqi/commit/c2cb641), @fedorov)
- Fix gcov path ([`6221cd9`](https://github.com/QIICR/dcmqi/commit/6221cd9), @fedorov)
- Fix dependencies for the new tests ([`fb828ad`](https://github.com/QIICR/dcmqi/commit/fb828ad), @fedorov)
- Fix initialization of the image geometry ([`dbccefa`](https://github.com/QIICR/dcmqi/commit/dbccefa), @fedorov)
- Test if touching the source would fix the package generation ([`4323629`](https://github.com/QIICR/dcmqi/commit/4323629), @fedorov)
- Fix macOS ([`fd90122`](https://github.com/QIICR/dcmqi/commit/fd90122), @fedorov)
- Generalize and fix bash syntax related issues ([`c9b04bd`](https://github.com/QIICR/dcmqi/commit/c9b04bd), @fedorov)
- Fixed type that has been corrected in most recent dcmtk ([`9b6b315`](https://github.com/QIICR/dcmqi/commit/9b6b315), @che85)

### Build & dependencies
- Updated dcmtk revision, included header files in the cmake library sources ([#267](https://github.com/QIICR/dcmqi/pull/267), @che85)
- Updated dcmtk version ([#281](https://github.com/QIICR/dcmqi/pull/281), @che85)
- Adding attribute to link segmentAttributes sub-arrays to specific files ([#278](https://github.com/QIICR/dcmqi/pull/278), @fedorov)
- Consistently propagate CMAKE_MACOSX_RPATH to all projects ([#295](https://github.com/QIICR/dcmqi/pull/295), @jcfr)
- Add -fPIC to allow for dynamic linking ([#303](https://github.com/QIICR/dcmqi/pull/303), @nolden)
- Fix Slicer extension packaging ([#310](https://github.com/QIICR/dcmqi/pull/310), @jcfr)
- Update PDF link ([`eb4b1fb`](https://github.com/QIICR/dcmqi/commit/eb4b1fb), @fedorov)
- Enable position independent code to allow for dynamic linking ([`951475d`](https://github.com/QIICR/dcmqi/commit/951475d), @nolden)
- Pass CXX_STANDARD to ITK build in superbuild ([`b97504f`](https://github.com/QIICR/dcmqi/commit/b97504f), @nolden)
- Fix unused variable warnings ([`9484dc6`](https://github.com/QIICR/dcmqi/commit/9484dc6), @jcfr)
- Fix sign-compare warnings ([`cd0833d`](https://github.com/QIICR/dcmqi/commit/cd0833d), @jcfr)
- Cmake: Propagate Cxx standard CMake option to external project ([`326c07e`](https://github.com/QIICR/dcmqi/commit/326c07e), @jcfr)
- Cmake: Allow Cxx standard to be configured using official CMake option ([`61984ab`](https://github.com/QIICR/dcmqi/commit/61984ab), @jcfr)
- Add link to the dcmqi paper pdf ([`73ba5bb`](https://github.com/QIICR/dcmqi/commit/73ba5bb), @fedorov)
- Add link to the paper PDF ([`495a32c`](https://github.com/QIICR/dcmqi/commit/495a32c), @fedorov)
- Updated dcmtk revision ([`1e92314`](https://github.com/QIICR/dcmqi/commit/1e92314), @che85)
- Cmake: Display result of the "Checking if building a release" test ([`c624840`](https://github.com/QIICR/dcmqi/commit/c624840), @jcfr)
- Cmake: Generate dcmqi packages on all CI services ([`db1756a`](https://github.com/QIICR/dcmqi/commit/db1756a), @jcfr)
- Cmake: Refactor and remove unneeded dcmqiMacroExtractRepositoryInfo macro ([`2307bc6`](https://github.com/QIICR/dcmqi/commit/2307bc6), @jcfr)
- Set DCMQI CMake version variables as "DCMQI_VERSION_*" ([`739d09f`](https://github.com/QIICR/dcmqi/commit/739d09f), @jcfr)
- Ensure "ExtractRepositoryInfo" CMake macro set uppercase variables ([`9306264`](https://github.com/QIICR/dcmqi/commit/9306264), @jcfr)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.0.5...v1.0.6)_

## [1.0.5](https://github.com/QIICR/dcmqi/releases/tag/v1.0.5) - 2017-04-06

### Added
- Added pmap/sr dciodvfy and meta roundtrip test and added missing attributes to pmap meta output ([#154](https://github.com/QIICR/dcmqi/pull/154), @che85)
- Add a Gitter chat badge to README.md ([#228](https://github.com/QIICR/dcmqi/pull/228), @gitter-badger)
- Added pmap/tid1500 dciodvfy and meta roundtrip tests (issue #9) ([`967b33b`](https://github.com/QIICR/dcmqi/commit/967b33b), @che85)
- Add an option to specify DerivedPixelContrast in the PM context ([`2dd5dbd`](https://github.com/QIICR/dcmqi/commit/2dd5dbd), @fedorov)

### Changed
- Refactoring tests ([#213](https://github.com/QIICR/dcmqi/pull/213), @che85)
- Updating DICOM segmentation context ([#211](https://github.com/QIICR/dcmqi/pull/211), @fedorov)
- Update diffusion and perfusion parametric map contexts ([#215](https://github.com/QIICR/dcmqi/pull/215), @fedorov)
- Remove DerivedPixelContrast initialization ([#219](https://github.com/QIICR/dcmqi/pull/219), @fedorov)
- Schema correction, misspelling, typos ([#214](https://github.com/QIICR/dcmqi/pull/214), @che85)
- Log the name of the file that could not be read ([#220](https://github.com/QIICR/dcmqi/pull/220), @fedorov)
- Removed DerivationCode as required attribute ([#221](https://github.com/QIICR/dcmqi/pull/221), @che85)
- Adding DerivationCode as required ([#222](https://github.com/QIICR/dcmqi/pull/222), @che85)
- Update latest version ([`7803a83`](https://github.com/QIICR/dcmqi/commit/7803a83), @fedorov)
- Make converter names, as exposed in Slicer, more consistent ([`49f00ad`](https://github.com/QIICR/dcmqi/commit/49f00ad), @fedorov)
- Update label for CI builds ([`6014000`](https://github.com/QIICR/dcmqi/commit/6014000), @fedorov)
- Relax the requirements for DerivedPixelContrast ([`6226d35`](https://github.com/QIICR/dcmqi/commit/6226d35), @fedorov)
- Initializing DerivedPixelContrast based on the quantity ([`9e5dd87`](https://github.com/QIICR/dcmqi/commit/9e5dd87), @fedorov)
- Use local coding scheme for the codes that do not yet have official DCM codes assigned ([`df6b76f`](https://github.com/QIICR/dcmqi/commit/df6b76f), @dclunie)
- Include laterality only for the Anatomic Structure category ([`45dc33a`](https://github.com/QIICR/dcmqi/commit/45dc33a), @dclunie)
- Refactored creation of context validation tests ([`6a7e23d`](https://github.com/QIICR/dcmqi/commit/6a7e23d), @che85)
- Changed module name to paramap ([`7041cee`](https://github.com/QIICR/dcmqi/commit/7041cee), @che85)
- Updating SlicerGeneralAnatomy segmentation context ([`216a7f2`](https://github.com/QIICR/dcmqi/commit/216a7f2), @dclunie)

### Fixed
- MeasurementUnitsCode attribute should be a list ([#209](https://github.com/QIICR/dcmqi/pull/209), @fedorov)
- Fix default values for parametric maps ([#218](https://github.com/QIICR/dcmqi/pull/218), @che85)
- Fix several issues with CLI XML ([#224](https://github.com/QIICR/dcmqi/pull/224), @fedorov)
- Fix referenced instances item initialization ([#227](https://github.com/QIICR/dcmqi/pull/227), @fedorov)
- Fixes Bad XML for segimage2itkimage (issue #230) ([#231](https://github.com/QIICR/dcmqi/pull/231), @che85)
- Add missing showAnatomy attribute ([#234](https://github.com/QIICR/dcmqi/pull/234), @fedorov)
- Fix showAnatomy type ([`332a322`](https://github.com/QIICR/dcmqi/commit/332a322), @fedorov)
- Converted 3dSlicerIntegerLabel values from string to integer ([`1fe5e3a`](https://github.com/QIICR/dcmqi/commit/1fe5e3a), @che85)
- Remove DerivedPixelContrast from pm examples ([`e393ae1`](https://github.com/QIICR/dcmqi/commit/e393ae1), @fedorov)
- Recover Slicer context and rename incorrectly labeled DICOM master list ([`4cb3db2`](https://github.com/QIICR/dcmqi/commit/4cb3db2), @fedorov)
- Remove old DICOM context ([`868b4ab`](https://github.com/QIICR/dcmqi/commit/868b4ab), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.0.4...v1.0.5)_

## [1.0.4](https://github.com/QIICR/dcmqi/releases/tag/v1.0.4) - 2017-02-21

### Added
- Add version reporting to the converters output ([#207](https://github.com/QIICR/dcmqi/pull/207), @fedorov)

### Changed
- Circle: Change owner from "qiicr" to "QIICR" ([`b167845`](https://github.com/QIICR/dcmqi/commit/b167845), @jcfr)
- Tweak info message to include repo URL ([`24bca08`](https://github.com/QIICR/dcmqi/commit/24bca08), @fedorov)
- Remove top-level docker_entry.sh ([`5bd3525`](https://github.com/QIICR/dcmqi/commit/5bd3525), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.0.3...v1.0.4)_

## [1.0.3](https://github.com/QIICR/dcmqi/releases/tag/v1.0.3) - 2017-02-20

### Changed
- Circle: set owner for master deployment ([`7534694`](https://github.com/QIICR/dcmqi/commit/7534694), @fedorov)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.0.2...v1.0.3)_

## [1.0.2](https://github.com/QIICR/dcmqi/releases/tag/v1.0.2) - 2017-02-20

### Changed
- Update badge ([#204](https://github.com/QIICR/dcmqi/pull/204), @jcfr)
- Circle: Trigger deployment on release updates ([`55287c4`](https://github.com/QIICR/dcmqi/commit/55287c4), @jcfr)

### Fixed
- Circle: Fix discovery of git tag associated with docker image generation ([#205](https://github.com/QIICR/dcmqi/pull/205), @jcfr)

### Build & dependencies
- README: Display build status in a table ([`9d8c80d`](https://github.com/QIICR/dcmqi/commit/9d8c80d), @jcfr)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.0.1...v1.0.2)_

## [1.0.1](https://github.com/QIICR/dcmqi/releases/tag/v1.0.1) - 2017-02-20

### Added
- Add derivation images to parametric map ([#193](https://github.com/QIICR/dcmqi/pull/193), @fedorov)
- Introduce parametric map context schema and contexts for diffusion and perfusion analysis ([#201](https://github.com/QIICR/dcmqi/pull/201), @fedorov)
- Add checks for duplicates while loading DICOM instances (issue #182) ([`4b36b6d`](https://github.com/QIICR/dcmqi/commit/4b36b6d), @che85)
- Added error message in case that input DICOM files/directory couldn't be loaded ([`49ee3ff`](https://github.com/QIICR/dcmqi/commit/49ee3ff), @che85)
- Added population of the derivation items ([`a438211`](https://github.com/QIICR/dcmqi/commit/a438211), @fedorov)
- Add an option to pass DICOM directory to the PM converter ([`38f83cf`](https://github.com/QIICR/dcmqi/commit/38f83cf), @fedorov)

### Changed
- Update package name to: dcmqi-(linux|mac|win64).(tar.gz|zip) ([`a3c6987`](https://github.com/QIICR/dcmqi/commit/a3c6987), @jcfr)
- Package windows as ZIP, package MacOSX as TGZ ([`00efba8`](https://github.com/QIICR/dcmqi/commit/00efba8), @jcfr)
- Update the base image ([`e19f097`](https://github.com/QIICR/dcmqi/commit/e19f097), @fedorov)
- Clean up docker_entry script ([`cda0be5`](https://github.com/QIICR/dcmqi/commit/cda0be5), @fedorov)
- Factored out method for loading DICOMDatasets from file list and avoidance of loading the same dataset twice (issue #182) ([`7182007`](https://github.com/QIICR/dcmqi/commit/7182007), @che85)
- Factored out functionality into Helper::getFileListRecursively (issue #182) ([`49405b8`](https://github.com/QIICR/dcmqi/commit/49405b8), @che85)
- Using --inputDICOMList for itkimage2segimage ([`978a237`](https://github.com/QIICR/dcmqi/commit/978a237), @che85)
- Reorganized data directory for segmentation (issue #182) ([`b9eada4`](https://github.com/QIICR/dcmqi/commit/b9eada4), @che85)
- Allow specifying DerivationCode/Description ([`34dbfb7`](https://github.com/QIICR/dcmqi/commit/34dbfb7), @fedorov)
- Refactoring - move up function for mapping slices to ref instances ([`f10b936`](https://github.com/QIICR/dcmqi/commit/f10b936), @fedorov)
- Make handling of FGs more consistent between PM and SEG ([`5275f88`](https://github.com/QIICR/dcmqi/commit/5275f88), @fedorov)

### Fixed
- CPack: Ensure windows package name is of the form "dcmqi-win(32|64).zip" ([`1fc94d1`](https://github.com/QIICR/dcmqi/commit/1fc94d1), @jcfr)
- Fix test name, add reference to common schema for ajv test ([`34e03ec`](https://github.com/QIICR/dcmqi/commit/34e03ec), @fedorov)
- Fix attribute reference ([`68d0522`](https://github.com/QIICR/dcmqi/commit/68d0522), @fedorov)
- Removing const from string for being allowed to replace path separators on Windows (issue #182) ([`914d24d`](https://github.com/QIICR/dcmqi/commit/914d24d), @che85)
- Fix the crash ([`bc2e1ce`](https://github.com/QIICR/dcmqi/commit/bc2e1ce), @michaelonken)
- Fix issues related to duplicate type definitions between PM and SEG ([`28b5fb8`](https://github.com/QIICR/dcmqi/commit/28b5fb8), @fedorov)

### Build & dependencies
- Using dcmtk searchDirectoryRecursively (superseeds #191) ([#196](https://github.com/QIICR/dcmqi/pull/196), @fedorov)
- Standalone packaging rebased ([#203](https://github.com/QIICR/dcmqi/pull/203), @jcfr)
- Cmake: Improve support for ninja build fixing verbose mode ([`43b5a9a`](https://github.com/QIICR/dcmqi/commit/43b5a9a), @jcfr)
- Build standalone package using static ITK/DCMTK/SlicerExecutionModel ([`2cca669`](https://github.com/QIICR/dcmqi/commit/2cca669), @jcfr)
- Enable packaging of standalone DCMQI ([`1e77031`](https://github.com/QIICR/dcmqi/commit/1e77031), @jcfr)
- SlicerExecutionModel: remove unneeded option ([`49409f5`](https://github.com/QIICR/dcmqi/commit/49409f5), @jcfr)
- Using dcmtk to recursively search inputDICOMDirectory (issue #182) ([`e9b9855`](https://github.com/QIICR/dcmqi/commit/e9b9855), @che85)

_[Full set of changes](https://github.com/QIICR/dcmqi/compare/v1.0.0...v1.0.1)_

## [1.0.0](https://github.com/QIICR/dcmqi/releases/tag/v1.0.0) - 2017-02-01

### Highlights
- First official release of dcmqi, accompanying the article submitted to the ITCR special issue of *Cancer Research*
- Shipped with documentation and introductory tutorials

### Added
- Add jsoncpp ([#3](https://github.com/QIICR/dcmqi/pull/3), @fedorov)
- Add support for multiple source instances per seg frame ([#49](https://github.com/QIICR/dcmqi/pull/49), @fedorov)
- Add segmentation contexts in JSON based off "AIM on ClearCanvas" lists ([#57](https://github.com/QIICR/dcmqi/pull/57), @fedorov)
- Add handling of multiple labels per input image file ([#65](https://github.com/QIICR/dcmqi/pull/65), @fedorov)
- Add jsonlint testing ([#67](https://github.com/QIICR/dcmqi/pull/67), @fedorov)
- Add initial support for creating tid1500 structured reports ([#70](https://github.com/QIICR/dcmqi/pull/70), @fedorov)
- Add ajv validation of schemas tests ([#71](https://github.com/QIICR/dcmqi/pull/71), @fedorov)
- Add more common tid1500 example ([#75](https://github.com/QIICR/dcmqi/pull/75), @fedorov)
- Add SPL Generic Atlas context ([#80](https://github.com/QIICR/dcmqi/pull/80), @fedorov)
- Add a test for generating paramap with values less than 1 ([#92](https://github.com/QIICR/dcmqi/pull/92), @fedorov)
- Added dciodvfy test for segmentation (issue #9) ([#106](https://github.com/QIICR/dcmqi/pull/106), @che85)
- Support use of DCMQI as a C++ library, building as a standalone package and/or Slicer extension ([#121](https://github.com/QIICR/dcmqi/pull/121), @jcfr)
- Added test for itkimage2segimage with multiple segmentation files (issue #142) ([#144](https://github.com/QIICR/dcmqi/pull/144), @che85)
- Add debug message ([#151](https://github.com/QIICR/dcmqi/pull/151), @fedorov)
- Add seg context schema ([#164](https://github.com/QIICR/dcmqi/pull/164), @fedorov)
- Add contribution guidelines ([#169](https://github.com/QIICR/dcmqi/pull/169), @fedorov)
- Add integration with docker ([#174](https://github.com/QIICR/dcmqi/pull/174), @fedorov)

### Changed
- Updated JSON by adding missing attributes ([#5](https://github.com/QIICR/dcmqi/pull/5), @che85)
- Extracting a library from existing cli ([#6](https://github.com/QIICR/dcmqi/pull/6), @che85)
- Move the library-related components into lib ([#16](https://github.com/QIICR/dcmqi/pull/16), @fedorov)
- JSON-LD/schema refactoring ([#24](https://github.com/QIICR/dcmqi/pull/24), @fedorov)
- Reorganize library ([#63](https://github.com/QIICR/dcmqi/pull/63), @msmolens)
- In memory objects only for segmentation converters ([#59](https://github.com/QIICR/dcmqi/pull/59), @che85)
- Itkimage2paramap in memory only implemented ([#68](https://github.com/QIICR/dcmqi/pull/68), @che85)
- Removed default values for optional attributes and changed position for required code sequence defaults ([#74](https://github.com/QIICR/dcmqi/pull/74), @che85)
- Reorganize the tests into groups by object type ([#73](https://github.com/QIICR/dcmqi/pull/73), @fedorov)
- Cleanup tid1500writer ([#83](https://github.com/QIICR/dcmqi/pull/83), @fedorov)
- Improvements to PM converter ([#85](https://github.com/QIICR/dcmqi/pull/85), @fedorov)
- Removed key "seriesAttributes" and moved all underlying keys to root level ([#103](https://github.com/QIICR/dcmqi/pull/103), @che85)
- FindingSite should only be populated when anatomical context is available ([#107](https://github.com/QIICR/dcmqi/pull/107), @che85)
- Reading DICOM SR and creation of JSON with its attributes and measurements ([#110](https://github.com/QIICR/dcmqi/pull/110), @che85)
- Tweak slicer extension support ([#124](https://github.com/QIICR/dcmqi/pull/124), @jcfr)
- Trigger appveyor/travis builds only with PR or master branch updates ([#128](https://github.com/QIICR/dcmqi/pull/128), @jcfr)
- Display message if ajv or jsonlint is missing ([#136](https://github.com/QIICR/dcmqi/pull/136), @jcfr)
- Simplify app cmakelists ([#138](https://github.com/QIICR/dcmqi/pull/138), @jcfr)
- Reorganize the content of doc folder ([#171](https://github.com/QIICR/dcmqi/pull/171), @fedorov)
- Renamed attributes by means of DICOM Dictionary ([#170](https://github.com/QIICR/dcmqi/pull/170), @che85)
- Remove warning about inconsistent frame spacing ([#179](https://github.com/QIICR/dcmqi/pull/179), @fedorov)
- Clean up CLI XML ([#180](https://github.com/QIICR/dcmqi/pull/180), @fedorov)
- Allow passing of reference DICOMs as directory ([#183](https://github.com/QIICR/dcmqi/pull/183), @fedorov)
- Save software version and .manufacturer information into output DICOM ([#190](https://github.com/QIICR/dcmqi/pull/190), @che85)

### Fixed
- Added missing header files for testing ([#15](https://github.com/QIICR/dcmqi/pull/15), @che85)
- Fix some invocation crashes ([#18](https://github.com/QIICR/dcmqi/pull/18), @nolden)
- Allow user-defined value for DerivedPixelContrast ([#29](https://github.com/QIICR/dcmqi/pull/29), @fedorov)
- Fixed missing SeriesNumber ([#31](https://github.com/QIICR/dcmqi/pull/31), @che85)
- Resolve several CI issues ([#32](https://github.com/QIICR/dcmqi/pull/32), @fedorov)
- Remove explicit mentions of 'Iowa' and more ([#51](https://github.com/QIICR/dcmqi/pull/51), @fedorov)
- Fixes for issues identified while working on BMMR example ([#52](https://github.com/QIICR/dcmqi/pull/52), @fedorov)
- Propagate pixel contrast and anatomy from json meta ([#60](https://github.com/QIICR/dcmqi/pull/60), @fedorov)
- Fixed json schemata ([#66](https://github.com/QIICR/dcmqi/pull/66), @che85)
- Fixed compiling with referenced schemata ([#72](https://github.com/QIICR/dcmqi/pull/72), @che85)
- Fix appveyor npm ([#81](https://github.com/QIICR/dcmqi/pull/81), @fedorov)
- Fixes to improve handling of single frame SEG ([#89](https://github.com/QIICR/dcmqi/pull/89), @fedorov)
- Fixed issues with json validation (issue #98): ([#100](https://github.com/QIICR/dcmqi/pull/100), @che85)
- Roundtrip test and some fixes to inconsistencies between the schema, examples, and JSONMetadataHandler ([#101](https://github.com/QIICR/dcmqi/pull/101), @che85)
- Cmake/SuperBuild: Ensure svn and git paths are propagated ([#127](https://github.com/QIICR/dcmqi/pull/127), @jcfr)
- Fix extension packaging on the Slicer factories ([#129](https://github.com/QIICR/dcmqi/pull/129), @jcfr)
- Enable parametric maps component ([#130](https://github.com/QIICR/dcmqi/pull/130), @fedorov)
- Cmake: fix location of the binaries for the remaining tests ([#134](https://github.com/QIICR/dcmqi/pull/134), @fedorov)
- Fix warnings ([#135](https://github.com/QIICR/dcmqi/pull/135), @jcfr)
- Creating new reader for each input segmentation file (issue #140) ([#141](https://github.com/QIICR/dcmqi/pull/141), @che85)
- Slicer-extension: Run tests using SEM_LAUNCH_COMMAND ([#143](https://github.com/QIICR/dcmqi/pull/143), @jcfr)
- Reorganize files and fix concurrent execution of tests ([#147](https://github.com/QIICR/dcmqi/pull/147), @jcfr)
- Added SeriesTime and SeriesDate to Structured Report (issue #145) ([#148](https://github.com/QIICR/dcmqi/pull/148), @che85)
- Make capitalization consistent with the converters parameters ([#149](https://github.com/QIICR/dcmqi/pull/149), @fedorov)
- Fix parallel execution of tests ([#152](https://github.com/QIICR/dcmqi/pull/152), @jcfr)
- Improving the code to handle situations when derivation images are missing ([#158](https://github.com/QIICR/dcmqi/pull/158), @fedorov)
- Use URLs to stashed copies of the dicom3tools ([#160](https://github.com/QIICR/dcmqi/pull/160), @fedorov)
- Adding nrrd as element instead of having it as default only ([#178](https://github.com/QIICR/dcmqi/pull/178), @che85)

### Build & dependencies
- Implementation of parametric map itk to dcm converter ([#53](https://github.com/QIICR/dcmqi/pull/53), @che85)
- Implementation for DICOM parametric map to itk converter ([#77](https://github.com/QIICR/dcmqi/pull/77), @che85)
- Update build system ([#111](https://github.com/QIICR/dcmqi/pull/111), @jcfr)
- Add support for build apps option ([#112](https://github.com/QIICR/dcmqi/pull/112), @jcfr)
- Improve build system ([#118](https://github.com/QIICR/dcmqi/pull/118), @jcfr)
- Cmake: Enable testing including "CTest" module ([#133](https://github.com/QIICR/dcmqi/pull/133), @jcfr)
- Support of most popular itk file formats for conversion output from dcm to itk ([#176](https://github.com/QIICR/dcmqi/pull/176), @che85)
