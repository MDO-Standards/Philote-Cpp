# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Comprehensive implicit discipline test suite** (~2,600 lines of new tests)
  - Complete unit tests for ImplicitDiscipline, ImplicitClient, and ImplicitServer
  - Integration tests for end-to-end implicit discipline workflows
  - Error scenario tests covering boundary conditions and failure modes
  - Test coverage mirroring explicit discipline test patterns
  - Reuse of test helpers library (TestServerManager, test disciplines, utilities)

### Fixed
- **Fixed type validation in implicit server methods**
  - Added validation to ensure Array message type matches variable metadata type
  - ComputeResidualsImpl and ComputeResidualGradientsImpl now reject mismatched types
  - Returns INVALID_ARGUMENT error when type mismatch detected
- **Fixed Variable::CreateChunk to include type field**
  - Array messages now properly include variable type during gRPC transmission
  - Ensures type information flows correctly through client-server communication
- **Fixed ImplicitClient inheritance structure**
  - ImplicitClient now properly inherits from DisciplineClient
  - Enables reuse of base class methods (GetInfo, SetOptions, etc.)
- **Fixed duplicate library linker warnings**
  - Removed redundant PhiloteCpp links from test targets that already link PhiloteTestHelpers
  - PhiloteTestHelpers provides PhiloteCpp as a PUBLIC dependency

## [0.4.0] - 2025-10-30

### Added
- Comprehensive unit test suite using Google Test and Google Mock
  - Unit tests for discipline server, explicit discipline server, and implicit discipline server
  - Unit tests for discipline client and explicit client
  - Unit tests for variable handling, stream options, and set options
- **Extensive test coverage for explicit disciplines, clients, and servers** (~2,600 lines)
  - Reusable test helpers library with test discipline implementations (Paraboloid, MultiOutput, Vectorized, Error disciplines)
  - Comprehensive unit tests with mocks for ExplicitClient (36→524 lines) and ExplicitServer (74→587 lines)
  - End-to-end integration tests with real gRPC communication over localhost (450+ lines)
  - Error scenario and stress tests covering boundary conditions and failure modes (400+ lines)
  - TestServerManager utility for automatic server lifecycle management in integration tests
  - Variable creation helpers, assertion utilities, and data generators for test reusability
  - Test for RegisterServices() method verifying both discipline and explicit servers are registered
- Code coverage support in CMake build system
- CMake package configuration to support using Philote-Cpp as a subproject or installed library
- Installation support for headers and proto files
- Nullptr checks in discipline server to prevent crashes
- Documentation scaffolding using hdoc
- Philote logo added to README
- **Usage examples and limitations documentation in README**
  - Code examples for creating explicit disciplines, servers, and clients
  - Clear documentation that Compute() and ComputePartials() must be overridden
  - Explicit documentation of concurrency limitations

### Changed
- Reorganized project structure: moved headers to dedicated `include/` directory
- Split discipline header into separate components for better modularity
- Updated library API for parity with Python package
- Improved gRPC stub generation and dependency management
- **Refactored gRPC code generation to use build directory instead of source tree**
  - Generated files now created in `build/src/generated/` following CMake best practices
  - Created `cmake/GenerateGrpc.cmake` module for proto generation logic
  - Removed `src/generated/` directory from source tree
  - Updated all include paths to reference build directory
  - Added directory creation to support parallel builds
- **Simplified ExplicitDiscipline base class**
  - Removed default validation logic from Compute() and ComputePartials()
  - These methods now have no implementation and must be overridden by derived classes
  - Cleaner design aligning with intended usage pattern
- Updated copyright year to 2022-2025 in source files
- Cleaned up headers and removed unused code
- CI now builds gRPC targets first before building the rest of the project
- Removed unsupported concurrent RPC call tests to reflect actual library limitations

### Fixed
- Fixed error handling paths in implicit server for cases without errors
- Fixed getinfo bug that affected discipline metadata retrieval
- Fixed compiler warnings across the codebase
- Fixed duplicate library linker warnings (multiple instances)
- Fixed CMake verbatim directive that caused issues for some users
- Fixed example includes to properly reference headers in new include/ directory
- Fixed gRPC plugin symlink issues in CI
- Minor fix to protoc generation
- **Fixed Variable API usage throughout test suite**
  - Corrected element access from `.data()[i]` to `(i)` operator
  - Corrected shape access from `.shape()` to `.Shape()`
  - Fixed client metadata methods naming (`SetVariableMeta`, `SetPartialsMetaData`)
  - Added missing mock interface methods for complete gRPC stream testing
- **Fixed critical bugs in Array protocol and variable streaming** (39 test failures resolved)
  - Fixed Array end index to be inclusive (changed `set_end(size)` to `set_end(size-1)`)
  - Added Array Clear() calls to prevent data accumulation across Read() invocations
  - Fixed stream options initialization (set default chunk size to 1000 to prevent division by zero)
  - Fixed GetInfo() RPC signature to include `const` and `override` keywords
  - Fixed constructor pointer linking in ExplicitDiscipline to enable base RPC calls
  - Implemented template-based approach for RPC methods to support both mocks and real gRPC
  - Added input existence checks in ExplicitClient before accessing map
  - Fixed chunking off-by-one error in Variable::Send() (changed `end = start + chunk_size` to `end = start + chunk_size - 1`)
  - Updated test expectations to match corrected behavior

## [0.3.0] - 2023-11-09

### Added
- Flag to optionally build examples (`BUILD_EXAMPLES` CMake option)
- Citation section added to README
- Public Affairs (PA) clearance number added to banners

### Changed
- README file renamed to markdown format
- Modified source file banners with PA number
- Generated headers modified to support using Philote-Cpp as a subproject

### Fixed
- Fixed metadata bug in client
- Commented out unfinished mock test to avoid build issues

## [0.2.0] - 2023-10-03

### Added
- **Implicit discipline support** - full implementation of implicit disciplines
  - Implicit discipline client with `SolveResiduals()` and partials support
  - Implicit discipline server implementation
  - Implicit quadratic example demonstrating residual-based disciplines
- Rosenbrock function example (explicit discipline)
  - Both function evaluation and gradients
  - Client-server communication example
- Set options RPC for configuring disciplines at runtime
- Unit testing framework activated on CI
- Variable segment and chunk handling with comprehensive tests

### Changed
- Cleaned up for loops in both explicit and implicit disciplines
- Added const references where appropriate for better performance
- Updated implicit discipline metadata handling
- Improved CI configuration with full test matrix
- Updated CMake version requirements for CI

### Fixed
- Fixed variable sending and chunking issues
- Fixed client array chunk/send functionality
- Fixed assign chunk operations
- Fixed residual gradients function
- Fixed solve residuals call
- Fixed implicit declare partials
- Fixed segment function
- Fixed call to deprecated protobuf field
- Fixed various warnings in the codebase
- Corrected variable tests and segment tests
- Changed empty check implementation

## [0.1.0] - 2023-08-22

### Added
- Initial implementation of Philote-Cpp C++ bindings
- Explicit discipline support
  - Discipline client for connecting to remote disciplines
  - Discipline server for hosting disciplines
  - Explicit discipline base classes
- Variable class for data transmission
- Partials (gradients) support
- Compute partials functionality
- Basic examples demonstrating usage
- gRPC-based communication infrastructure
- CMake build system
- CI/CD pipeline with GitHub Actions
- Apache 2.0 license

[Unreleased]: https://github.com/MDO-Standards/Philote-Cpp/compare/v0.4.0...develop
[0.4.0]: https://github.com/MDO-Standards/Philote-Cpp/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/MDO-Standards/Philote-Cpp/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/MDO-Standards/Philote-Cpp/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/MDO-Standards/Philote-Cpp/releases/tag/v0.1.0
