# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Thread safety documentation for all public API classes** (closes #42)
  - Added thread safety notes to Variable, PairDict, Variables, and Partials
  - Added thread safety notes to Discipline, DisciplineClient, and DisciplineServer
  - Added thread safety notes to ExplicitDiscipline, ExplicitClient, and ExplicitServer
  - Added thread safety notes to ImplicitDiscipline, ImplicitClient, and ImplicitServer
  - Documents that classes are NOT thread-safe and require external synchronization
  - Clarifies that gRPC channels/stubs are thread-safe, enabling multiple clients per thread
  - Warns users that concurrent RPC calls to the same server instance may occur
- **Comprehensive implicit discipline test suite** (~2,600 lines of new tests)
  - Complete unit tests for ImplicitDiscipline, ImplicitClient, and ImplicitServer
  - Integration tests for end-to-end implicit discipline workflows
  - Error scenario tests covering boundary conditions and failure modes
  - Test coverage mirroring explicit discipline test patterns
  - Reuse of test helpers library (TestServerManager, test disciplines, utilities)

### Changed
- **Added noexcept specifications to appropriate methods** (closes #47)
  - Added noexcept to all class destructors (Variable, PairDict, Discipline, all server and client classes)
  - Added noexcept to Variable::Shape() and Variable::Size() getters
  - Added noexcept to PairDict query methods: contains(), size(), empty()
  - Added noexcept to PairDict move constructor and move assignment operator
  - Added noexcept to Discipline const getters: var_meta(), partials_meta(), stream_opts()
  - Added noexcept to DisciplineServer::DisciplinePointerNull()
  - Added noexcept to DisciplineClient const getters: GetStreamOptions(), GetProperties(), GetVariableMetaAll(), GetPartialsMetaConst(), GetRPCTimeout()
  - Enables compiler optimizations and improves STL container performance (especially std::vector<Variable>)
  - Makes exception guarantees explicit for const member access
  - Follows selective noexcept approach: added to destructors, move operations, simple getters, and query methods
- **Modernized pointer management to use shared_ptr** (closes #43)
  - Discipline base class now inherits from std::enable_shared_from_this<Discipline>
  - DisciplineServer, ExplicitServer, and ImplicitServer now use shared_ptr instead of raw pointers
  - LinkPointers() methods now accept shared_ptr<Discipline> instead of raw pointers
  - RegisterServices() calls shared_from_this() to safely link servers to disciplines
  - Examples updated to use std::make_shared for discipline instantiation
  - Test helpers (TestServerManager, ImplicitTestServerManager) updated to accept shared_ptr
  - All test files updated to use make_shared for discipline creation
  - **BREAKING CHANGE**: Disciplines must now be created with std::make_shared instead of stack allocation
  - Improves ownership semantics and eliminates dangling pointer risks

### Fixed

- **Fixed integer overflow vulnerability in Variable::AssignChunk()** (closes #36)
  - Added validation to check data.start() and data.end() are non-negative before casting to size_t
  - Prevents integer overflow attacks where negative values wrap to SIZE_MAX
  - Protects against memory corruption from malicious network input
  - Added comprehensive security tests for negative indices and edge cases
  - Mitigates denial of service and potential code execution vulnerabilities
- **Fixed ImplicitDiscipline constructor missing discipline_server_ link** (closes #33)
  - Added discipline_server_.LinkPointers(this) call in constructor
  - Added discipline_server_.UnlinkPointers() call in destructor
  - Now properly links both base discipline server and implicit server, matching ExplicitDiscipline pattern
  - Prevents segmentation faults when calling base discipline RPC methods on implicit disciplines
- **Added null pointer checks to DisciplineServer RPC methods** (closes #32)
  - All 7 RPC methods now check if discipline_ pointer is null before dereferencing
  - Methods return FAILED_PRECONDITION error instead of crashing when discipline not linked
  - Prevents segmentation faults if RPC called before LinkPointers() or after UnlinkPointers()
  - Added comprehensive test coverage with 7 new null pointer scenario tests
  - Improves server robustness and security by preventing remote crash vulnerability
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
- Fixed missing discipline_client.h and discipline_server.h in CMake target_sources configuration
- Removed unused BaseDisciplineClient class declaration from discipline.h (closes #31)
- - **Variable::Send() and DisciplineServer now check stream Write() failures** (closes #35)
- All three Variable::Send() method overloads now check Write() return values
- Throws runtime_error with descriptive messages including variable name and chunk number
- DisciplineServer::GetVariableDefinitions() and GetPartialDefinitions() now check Write() failures
- Returns grpc::Status with INTERNAL error code on metadata write failures
- Prevents silent data loss on network failures, broken connections, or buffer overflows
- Added 5 comprehensive unit tests using mock stream classes to verify error handling
- Tests cover first-chunk failure, mid-transmission failure, and success scenarios
- **Documented SetOptions() override pattern for configurable disciplines** (closes #34)
  - Added comprehensive comments in base Discipline::SetOptions() explaining override pattern
  - Fixed Rosenbrock example to properly override Initialize() and SetOptions() instead of using broken signature
  - Added 9 new tests demonstrating extraction of all protobuf value types (float, int, bool, string)
  - Added detailed documentation section in CLAUDE.md with complete code examples
  - Clarified lifecycle: Initialize() declares options, SetOptions() extracts values, Configure() post-processes
- **Client methods now throw exceptions on gRPC errors** (closes #48)
  - ExplicitClient::ComputeFunction() and ComputeGradient() now check gRPC status
  - ImplicitClient::ComputeResiduals(), SolveResiduals(), and ComputeResidualGradients() now check gRPC status
  - All computation methods throw std::runtime_error with error code and message on RPC failure
  - **BREAKING CHANGE**: Previously these methods returned silently with potentially invalid data
  - Error handling is now consistent with other client methods (GetInfo, Setup, etc.)
  - Prevents silent failures and propagation of corrupted data

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
