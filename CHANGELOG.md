# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Version 3.1.0] - 2023-05-20

### Added

- Added default sheet.
- Added description and template definitions.

### Changed

- Reporting unpackable sprites as warnings.
- y-parameter of grid is no longer optional.
- Atlas completion does not add skipped.

## [Version 3.0.0] - 2023-05-07

### Changed

- Added _glob_, removed _source_ definition.
- Replaced several commandline parameters with single run mode parameter.
- Outputting sprite indices by input/source.
- Checking sequence bounds.

### Added

- Autocompletion can update.
- Allow to autocomplete only specific inputs.
- Generalized _skip_ to atlas and sequences.
- Allow to specify grid's offset from the bottom-right corner.

## [Version 2.3.0] - 2023-03-24

### Added

- Added describe commandline option.
- Added autocompletion for globbing and sequences.
- Added inputs to output description.
- Added errors as warnings option.

### Changed

- Replaced debug commandline option with definition.
- Outputting either autocompleted definition or output description.
- No longer supporting multiple input files.

### Fixed

- Filename sequence improved.

## [Version 2.2.0] - 2023-03-03

### Added

- Added gif output.
- Added min-bounds.
- Added common-bounds.
- Added align.
- Added align-pivot.
- Allow to separate arguments by comma.

### Changed

- Renamed align-width to divisible-width.
- Renamed common-divisor to divisible-bounds.

## [Version 2.1.0] - 2023-02-24

### Fixed

- Fixed output scope.
- Fixed expression evaluation.
- Allow unexpanded variables in filename sequences.

## [Version 2.0.0] - 2023-02-22

### Changed
- Split output in sheet and output.
- Removed implicit output.
- Renamed layers to maps.
- Added pack mode layers.
- Replaced scalings with scale.

### Added
- Parallelized trimming.
- Added texture to JSON output.

## [Version 1.9.0] - 2023-02-12

### Added

- Added sources to JSON output.
- Added set for setting variables.
- Added data for setting per sprite data.
- Added variable evaluation.

### Changed

- Improved tag output.
- Improved output order.
- Compacting number arrays in JSON output.
- Compacted vertices in JSON output.

### Fixed

- Fixed pack method keep for multiple sources.
- Fixed deduplication when sprites are reordered.

## [Version 1.8.0] - 2023-02-07

### Added

- Not updating outputs when sources did not change.
- Added _scalings_.
- Added extrude modes _mirror_ and _repeat_.
- Added pack methods _rows_ and _columns_.
- Added output of _inputFilename_.

### Changed

- Removed -- commandline argument.

### Fixed

- Fixed debug output of vertices when rotated.

## [Version 1.7.0] - 2023-01-26

### Added

- Allow to set grid and grid-cells at once.
- Improved debug overlay.
- Support writing BMP.

## [Version 1.6.0] - 2023-01-21

### Added

- Created github.com/houmain/spright-test-suite.
- Detecting definitions without effects.
- Updating output files only when they change.
- Displaying input filename in error message.
- Support writing TGA.
- Added crop-pivot.

### Changed

- Changed span's scope from input to sprite.
- Removed support for reading JPEG, PSD and HDR.
- Outputting layers in parallel.

### Fixed

- Ignore layers during file globbing.
- Fixed appending output path to output files.

## [Version 1.5.0] - 2022-11-14

### Added

- Added grid-cells.

### Changed

- Cleanup of JSON output.

### Fixed

- Fixed bug in pack mode compact.
- Fixed pack mode compact without trim convex.
- Fixed allow-rotate with pack mode compact.
- Fixed Visual Studio compile flags for catch2.

## [Version 1.4.0] - 2022-10-15

### Added

- Added layers (issue #4).
- Support mixed number and word pivot declarations. (e.g. pivot center 8)
- Allow to offset pivot points (e.g. pivot top + 3 right - 5)

### Changed

- Removed old definition names sheet/texture for input/output.
- Improved invalid argument count error messages.

### Fixed

- Fixed -- commandline arguments.
- Fixed sample cpp.template.

[version 3.1.0]: https://github.com/houmain/spright/compare/3.0.0...3.1.0
[version 3.0.0]: https://github.com/houmain/spright/compare/2.3.0...3.0.0
[version 2.3.0]: https://github.com/houmain/spright/compare/2.2.0...2.3.0
[version 2.2.0]: https://github.com/houmain/spright/compare/2.1.0...2.2.0
[version 2.1.0]: https://github.com/houmain/spright/compare/2.0.0...2.1.0
[version 2.0.0]: https://github.com/houmain/spright/compare/1.9.0...2.0.0
[version 1.9.0]: https://github.com/houmain/spright/compare/1.8.0...1.9.0
[version 1.8.0]: https://github.com/houmain/spright/compare/1.7.0...1.8.0
[version 1.7.0]: https://github.com/houmain/spright/compare/1.6.0...1.7.0
[version 1.6.0]: https://github.com/houmain/spright/compare/1.5.0...1.6.0
[version 1.5.0]: https://github.com/houmain/spright/compare/1.4.0...1.5.0
[version 1.4.0]: https://github.com/houmain/spright/compare/1.3.1...1.4.0
