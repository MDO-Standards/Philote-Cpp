# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Philote-Cpp is a C++ implementation of the Philote MDO (Multidisciplinary Design Optimization) standard. It provides bindings for creating and connecting to analysis disciplines via gRPC, enabling distributed computation for optimization workflows.

**Key Concepts:**
- **Explicit Disciplines**: Direct function evaluations f(x) = y, where outputs can be computed directly from inputs
- **Implicit Disciplines**: Define residuals R(x,y) that must be solved to find outputs (e.g., systems of equations)
- **Client-Server Architecture**: Disciplines run as gRPC servers; MDO frameworks connect as clients
- **Remote Procedure Calls**: Function and gradient evaluations are performed via network calls (can use localhost)

## Build System

### Configuration and Building

```bash
# Standard build
mkdir build && cd build
cmake ..
cmake --build .

# With tests (recommended for development)
cmake .. -DBUILD_TESTS=ON
cmake --build .

# With coverage
cmake .. -DENABLE_COVERAGE=ON -DBUILD_TESTS=ON
cmake --build .

# Build types
cmake .. -DCMAKE_BUILD_TYPE=Debug    # or Release, RelWithDebInfo, MinSizeRel
```

### CMake Options
- `BUILD_TESTS` (default: TRUE) - Build unit tests
- `BUILD_EXAMPLES` (default: FALSE) - Build example disciplines
- `ENABLE_COVERAGE` (default: FALSE) - Enable code coverage analysis

### Running Tests

```bash
# All tests
ctest --output-on-failure

# Specific test pattern
ctest -R Explicit

# Run single test executable directly
./test/ExplicitDisciplineServerTests

# Run with GoogleTest filters
./test/ExplicitIntegrationTests --gtest_filter=*ComputeFunction*

# Integration tests with timeout (some spawn servers)
timeout 10 ./test/ExplicitIntegrationTests
```

**Important test executables:**
- `ExplicitDisciplineTests` - Core explicit discipline functionality
- `ExplicitDisciplineServerTests` - Server-side RPC handling
- `ExplicitClientTests` - Client-side communication
- `ExplicitIntegrationTests` - Full client-server integration
- `ImplicitDisciplineTests` - Core implicit discipline functionality
- `ImplicitDisciplineServerTests` - Server-side RPC handling
- `ImplicitClientTests` - Client-side communication

Note: Some implicit integration tests are temporarily disabled (see test/CMakeLists.txt:176-203).

### Coverage Reports

```bash
# Generate basic gcov reports
make coverage

# Generate HTML coverage reports (requires lcov)
make coverage-html
# View at: build/coverage_html/index.html

# Generate XML for Codecov (CI usage)
make coverage-xml
```

## Architecture

### Core Class Hierarchy

```
Discipline (base class)
├── ExplicitDiscipline
│   ├── User-defined explicit disciplines (override Compute, ComputePartials)
│   ├── ExplicitServer (gRPC service implementation)
│   └── DisciplineServer (base gRPC service)
└── ImplicitDiscipline
    ├── User-defined implicit disciplines (override ComputeResiduals, SolveResiduals, ComputeResidualGradients)
    ├── ImplicitServer (gRPC service implementation)
    └── DisciplineServer (base gRPC service)

Client Classes:
├── DisciplineClient (base)
├── ExplicitClient
└── ImplicitClient
```

### Directory Structure

```
include/                     # Public API headers
├── discipline.h            # Base discipline class
├── explicit.h              # Explicit discipline + client + server
├── implicit.h              # Implicit discipline + client + server
├── variable.h              # Variable and array handling
├── discipline_client.h     # Base client class
└── discipline_server.h     # Base server class

src/
├── discipline/             # Base discipline implementation
├── explicit/               # Explicit discipline implementation
├── implicit/               # Implicit discipline implementation
├── utils/                  # Variable utilities
└── generated/              # Generated protobuf/gRPC code (build-time)

test/                       # GoogleTest unit and integration tests
examples/                   # Example disciplines (paraboloid, quadratic, rosenbrock)
proto/                      # Protocol buffer definitions
```

### Key Components

**Variable System** (include/variable.h):
- `Variable`: Multi-dimensional array with metadata (type, shape, units)
- `Variables`: Map of variable name → Variable
- `Partials`: Map of (output, input) pairs → Variable (gradients)
- `VariableMetaData`, `PartialsMetaData`: Protobuf-defined metadata

**Discipline Lifecycle**:
1. `Initialize()` - Define available options (called in constructor)
2. `SetOptions()` - Client sets option values
3. `Configure()` - Process options before setup
4. `Setup()` - Define inputs/outputs using `AddInput()`, `AddOutput()`
5. `SetupPartials()` - Declare available gradients using `DeclarePartials()`
6. `Compute()` or `ComputeResiduals()` - Perform computations (user-defined)
7. `ComputePartials()` or `ComputeResidualGradients()` - Compute gradients (user-defined)

**gRPC Communication**:
- Uses bidirectional streaming for variable transmission
- Variables are chunked based on `stream_opts().num_double()` (default: send all at once)
- Protobuf definitions in `proto/disciplines.proto` and `proto/data.proto`
- Generated code goes to `build/src/generated/`

### Creating Custom Disciplines

**Explicit Discipline Pattern**:
```cpp
class MyDiscipline : public philote::ExplicitDiscipline {
    void Setup() override {
        AddInput("x", {1}, "m");
        AddOutput("f", {1}, "N");
    }

    void SetupPartials() override {
        DeclarePartials("f", "x");
    }

    void Compute(const philote::Variables &inputs,
                 philote::Variables &outputs) override {
        // MUST implement - no default implementation
        outputs.at("f")(0) = 2.0 * inputs.at("x")(0);
    }

    void ComputePartials(const philote::Variables &inputs,
                        philote::Partials &partials) override {
        // Optional, but must be implemented if DeclarePartials() was called
        partials[{"f", "x"}](0) = 2.0;
    }
};
```

**Implicit Discipline Pattern**:
```cpp
class MyImplicitDiscipline : public philote::ImplicitDiscipline {
    void Setup() override {
        AddInput("a", {1}, "m");
        AddOutput("x", {1}, "m");
    }

    void SetupPartials() override {
        DeclarePartials("x", "a");
        DeclarePartials("x", "x");  // Jacobian w.r.t. outputs
    }

    void ComputeResiduals(const philote::Variables &inputs,
                         const philote::Variables &outputs,
                         philote::Variables &residuals) override {
        // MUST implement - compute R(inputs, outputs)
        double a = inputs.at("a")(0);
        double x = outputs.at("x")(0);
        residuals.at("x")(0) = a * x * x - 4.0;  // Example: ax² = 4
    }

    void SolveResiduals(const philote::Variables &inputs,
                       philote::Variables &outputs) override {
        // MUST implement - solve for outputs such that R = 0
        double a = inputs.at("a")(0);
        outputs.at("x")(0) = std::sqrt(4.0 / a);
    }

    void ComputeResidualGradients(const philote::Variables &inputs,
                                 const philote::Variables &outputs,
                                 philote::Partials &partials) override {
        // Optional, but must be implemented if DeclarePartials() was called
        double a = inputs.at("a")(0);
        double x = outputs.at("x")(0);
        partials[{"x", "a"}](0) = x * x;
        partials[{"x", "x"}](0) = 2.0 * a * x;
    }
};
```

**CRITICAL**: `Compute()`, `ComputePartials()`, `ComputeResiduals()`, `SolveResiduals()`, and `ComputeResidualGradients()` have NO default implementations and MUST be overridden by discipline developers.

### Server and Client Patterns

**Starting a Server**:
```cpp
MyDiscipline discipline;
grpc::ServerBuilder builder;
builder.AddListeningPort("localhost:50051", grpc::InsecureServerCredentials());
discipline.RegisterServices(builder);
std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
server->Wait();
```

**Client Workflow**:
```cpp
philote::ExplicitClient client;
auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
client.ConnectChannel(channel);

// Initialize sequence
client.GetInfo();
client.Setup();
client.GetVariableDefinitions();
client.GetPartialDefinitions();  // If computing gradients

// Prepare inputs and compute
philote::Variables inputs;
// ... populate inputs
philote::Variables outputs = client.ComputeFunction(inputs);
philote::Partials gradients = client.ComputeGradient(inputs);
```

### Configuring Disciplines with Options

Disciplines can define configurable options that clients can set at runtime. This involves overriding three lifecycle methods:

1. **`Initialize()`** - Declare available options using `AddOption(name, type)`
2. **`SetOptions()`** - Extract values from protobuf Struct and store in member variables
3. **`Configure()`** - Optional post-processing after options are set

**Pattern for Configurable Disciplines**:
```cpp
class ConfigurableDiscipline : public philote::ExplicitDiscipline {
private:
    // Strongly-typed member variables with default values
    double scale_factor_ = 1.0;
    int dimension_ = 2;
    bool enable_feature_ = false;
    std::string mode_ = "default";

    // Declare available options
    void Initialize() override {
        ExplicitDiscipline::Initialize();
        AddOption("scale_factor", "float");
        AddOption("dimension", "int");
        AddOption("enable_feature", "bool");
        AddOption("mode", "string");
    }

    // Extract option values from protobuf Struct
    void SetOptions(const google::protobuf::Struct &options_struct) override {
        // Extract float/double values
        auto it = options_struct.fields().find("scale_factor");
        if (it != options_struct.fields().end()) {
            scale_factor_ = it->second.number_value();
        }

        // Extract int values (cast from double)
        it = options_struct.fields().find("dimension");
        if (it != options_struct.fields().end()) {
            dimension_ = static_cast<int>(it->second.number_value());
        }

        // Extract bool values
        it = options_struct.fields().find("enable_feature");
        if (it != options_struct.fields().end()) {
            enable_feature_ = it->second.bool_value();
        }

        // Extract string values
        it = options_struct.fields().find("mode");
        if (it != options_struct.fields().end()) {
            mode_ = it->second.string_value();
        }

        // IMPORTANT: Call parent to invoke Configure()
        ExplicitDiscipline::SetOptions(options_struct);
    }

    // Optional: post-process options after they're set
    void Configure() override {
        // Validation, derived calculations, etc.
        if (dimension_ < 1) {
            throw std::runtime_error("dimension must be positive");
        }
    }

    void Compute(const philote::Variables &inputs,
                 philote::Variables &outputs) override {
        // Use the configured option values
        outputs.at("y")(0) = scale_factor_ * inputs.at("x")(0);
    }
};
```

**Client-side: Sending Options**:
```cpp
// Create protobuf Struct with option values
google::protobuf::Struct options;
(*options.mutable_fields())["scale_factor"].set_number_value(2.5);
(*options.mutable_fields())["dimension"].set_number_value(3);
(*options.mutable_fields())["enable_feature"].set_bool_value(true);
(*options.mutable_fields())["mode"].set_string_value("advanced");

// Package and send options
philote::DisciplineOptions options_message;
options_message.mutable_options()->CopyFrom(options);
client.SendOptions(options_message);
```

**Protobuf Struct Value Types**:
- `number_value()` / `set_number_value(double)` - For float, double, and int (cast required for int)
- `bool_value()` / `set_bool_value(bool)` - For boolean values
- `string_value()` / `set_string_value(string)` - For string values

**Key Points**:
- Options are optional - if not provided by client, default member values are used
- Always check if option exists in struct before extracting (`find() != end()`)
- Must call parent `SetOptions()` to invoke `Configure()`
- See `examples/rosenbrock/rosenbrock_server.cpp` for a complete working example

### Handling Cancellation in Long-Running Computations

The server automatically detects client cancellations (timeouts, disconnects, explicit cancellation) and can stop wasted computation. User disciplines can optionally check for cancellation during long-running operations.

**Automatic Server-Side Detection**:
The server checks for cancellation at three points:
1. Before starting the RPC operation
2. Before calling user compute methods
3. Before sending results back to client
4. Between chunks during `Variable::Send()`

When cancellation is detected, the server returns `grpc::StatusCode::CANCELLED` and stops processing.

**Opt-In: Checking Cancellation in User Code**:
Disciplines can check `IsCancelled()` during long computations for early termination:

```cpp
class LongRunningDiscipline : public philote::ExplicitDiscipline {
    void Compute(const philote::Variables &inputs,
                 philote::Variables &outputs) override {
        // Perform iterative computation
        for (int iter = 0; iter < 1000000; iter++) {
            // Check cancellation every 1000 iterations
            if (iter % 1000 == 0 && IsCancelled()) {
                throw std::runtime_error("Computation cancelled by client");
            }

            // ... expensive computation ...
        }

        outputs.at("result")(0) = final_value;
    }
};
```

**Client-Side Cancellation via Timeout**:
Clients can trigger cancellation by setting a timeout:

```cpp
philote::ExplicitClient client;
// ... connect and setup ...

// Set timeout - server will detect if exceeded
client.SetRPCTimeout(std::chrono::milliseconds(5000));  // 5 second timeout

try {
    philote::Variables outputs = client.ComputeFunction(inputs);
} catch (const std::runtime_error& e) {
    // Handle timeout/cancellation
    std::cerr << "Operation timed out: " << e.what() << std::endl;
}
```

**Cross-Language Support**:
Cancellation works across all client languages (Python, C++, etc.) since it's handled at the gRPC protocol level. Any client can cancel operations:

```python
# Python client example
import grpc
from philote import ExplicitServiceStub

channel = grpc.insecure_channel('localhost:50051')
stub = ExplicitServiceStub(channel)

# Timeout triggers server-side cancellation detection
try:
    response = stub.ComputeFunction(request, timeout=5.0)
except grpc.RpcError as e:
    if e.code() == grpc.StatusCode.DEADLINE_EXCEEDED:
        print("Operation cancelled/timed out")
```

**Key Points**:
- Server automatically detects all forms of cancellation (timeout, disconnect, explicit cancel)
- `IsCancelled()` is optional - only use for long-running computations that benefit from early termination
- Check `IsCancelled()` periodically, not on every iteration (performance overhead)
- Throw an exception or return early when `IsCancelled()` returns true
- Works transparently across all client languages
- No protocol changes needed - uses built-in gRPC cancellation

## Code Style

Follows Google C++ Style Guide with **4-space indentation** (not 2 spaces, not tabs).

Use provided `.clang-format`:
```bash
clang-format -i src/myfile.cpp
find src include -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

## Testing Strategy

- **Unit Tests**: Test individual classes with mocks (e.g., `ExplicitDisciplineTests`)
- **Server Tests**: Test RPC handlers with mock streams (e.g., `ExplicitDisciplineServerTests`)
- **Client Tests**: Test client communication with mock stubs (e.g., `ExplicitClientTests`)
- **Integration Tests**: Full server-client communication (e.g., `ExplicitIntegrationTests`)

Tests use GoogleTest and GoogleMock. Test helpers are in `test/test_helpers.cpp`.

## Known Limitations

- **No Concurrent RPC Support**: Only one RPC call at a time per server
- **Single Server Instance**: Each discipline should run on one server
- **Sequential Operations**: Client operations must be called sequentially

## Dependencies

- C++20 compiler (gcc-12+, clang-16+)
- CMake 3.23+
- gRPC and Protobuf (gRPC installation includes Protobuf)
- GoogleTest (GTest + GMock) - for testing

## Common Development Tasks

### Adding a New Test
1. Create test file in `test/` (e.g., `my_feature_test.cpp`)
2. Add to `test/CMakeLists.txt`:
   ```cmake
   add_executable(MyFeatureTests my_feature_test.cpp)
   target_link_libraries(MyFeatureTests PhiloteCpp GTest::gtest_main GTest::gmock)
   enable_coverage(MyFeatureTests)
   gtest_discover_tests(MyFeatureTests)
   ```
3. Build and run: `cmake --build . && ./test/MyFeatureTests`

### Modifying Protocol Buffers
1. Edit `proto/disciplines.proto` or `proto/data.proto`
2. Rebuild - CMake automatically regenerates code
3. Generated files appear in `build/src/generated/`

### Running Examples
```bash
cmake .. -DBUILD_EXAMPLES=ON
cmake --build .
./examples/paraboloid/paraboloid_server    # Terminal 1
./examples/paraboloid/paraboloid_client    # Terminal 2
```

## CI/CD

GitHub Actions workflows (`.github/workflows/`):
- `build.yml` - Build and test on Ubuntu 24.04 with multiple compilers
- `coverage.yml` - Generate and upload coverage to Codecov
- `documentation.yml` - Build Doxygen documentation
- `release.yml` - Automated releases with semantic versioning

All tests must pass on supported platforms before merging.
