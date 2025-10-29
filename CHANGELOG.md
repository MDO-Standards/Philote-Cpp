# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed
- Fixed typo in public API: `DisiplinePointerNull()` renamed to `DisciplinePointerNull()` (Breaking change)
- Fixed typo in documentation comment for `ExplicitDiscipline` destructor
- Added proper error handling in `implicit_server.cpp` for invalid variable types
- Removed debug `std::cout` statements from `discipline_server.cpp`
- Fixed missing return statements in exception handlers for `Setup()` and `SetupPartials()`
- Updated copyright year to 2022-2025 in source files

## [0.5.2] - Prior Release

Readme changes only to make documentation discoverable.

### Fixed
- Added link to documentation to project readme
- Renamed project title in readme (was old placeholder)

## [0.5.1] - Prior Release

This version only fixes missing public release information in preparation for
making the project public.

### Fixed
- Added public release information

## [0.5.0] - Prior Release

This version revises the implicit discipline API, cleans up unused data fields
and expands/revises the documentation.

### Added
- Revised documentation
- Revised implicit discipline API

### Fixed
- Cleaned up protobuf message fields to remove unused definitions

## [0.4.0] - Prior Release

This version restructures the Philote MDO standard. A hierarchy was developed to avoid code duplication:

- Base discipline service. This service is responsible for all common API calls such as the Setup call.
- Explicit discipline service. This service implements API calls that are unique to explicit disciplines.
- Implicit discipline service. This service implements API calls that are unique to implicit disciplines.

It should be noted that the base service is required to implement both explicit and implicit disciplines, as the common API calls are needed setup the discipline.

This release also contains several changes to the messages used by the service.

## [0.3.0] - Prior Release

This version changes function names and will therefore certainly break backwards
compatibility.

### Fixed
- Renamed the Compute RPC function to Functions to deconflict the C++ bindings
- Renamed the ComputePartials RPC function to Gradient to deconflict the C++ bindings

## [0.2.0] - Prior Release

This release introduces a number of bug fixes and features needed to implement
clients and servers. While 0.1 was released, this is actually the first
functional version of Philote.

### Added
- Reorganized data representations
- Added functions for defining partials on the remote server

### Fixed
- Fixed protobuf files that weren't compiling

## [0.1.0] - Prior Release

Initial draft version of Philote.

[Unreleased]: https://github.com/chrislupp/Philote-Cpp/compare/v0.5.2...HEAD
[0.5.2]: https://github.com/chrislupp/Philote-Cpp/compare/v0.5.1...v0.5.2
[0.5.1]: https://github.com/chrislupp/Philote-Cpp/compare/v0.5.0...v0.5.1
[0.5.0]: https://github.com/chrislupp/Philote-Cpp/compare/v0.4.0...v0.5.0
[0.4.0]: https://github.com/chrislupp/Philote-Cpp/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/chrislupp/Philote-Cpp/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/chrislupp/Philote-Cpp/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/chrislupp/Philote-Cpp/releases/tag/v0.1.0
