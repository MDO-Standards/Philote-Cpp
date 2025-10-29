# Philote-Cpp Developer Documentation

## Overview

Philote-Cpp provides C++ bindings for the Philote MDO (Multidisciplinary Design Optimization) standard.
The library enables distributed computing for engineering disciplines using gRPC for remote procedure calls.

## Key Concepts

### Disciplines

A **discipline** represents a computational analysis or simulation component. Philote supports two types:

- **Explicit Disciplines**: Direct function evaluations (e.g., f(x) = y)
- **Implicit Disciplines**: Residual-based evaluations that must be solved (e.g., R(x,y) = 0)

### Variables

Variables in Philote are multi-dimensional arrays that can represent:
- **Inputs**: Data provided to the discipline
- **Outputs**: Results computed by the discipline

Each variable has:
- A unique name
- A shape (dimensions)
- Physical units

### Partials (Gradients)

Disciplines can optionally provide gradient information (Jacobians) for optimization algorithms.
Partials represent derivatives of outputs with respect to inputs: ∂f/∂x

## Quick Start

### Creating an Explicit Discipline

Here's a simple example of an explicit discipline that computes a paraboloid function:

```cpp
#include <explicit.h>
#include <cmath>

class Paraboloid : public philote::ExplicitDiscipline {
public:
    Paraboloid() = default;

private:
    void Setup() override {
        // Define inputs
        AddInput("x", {1}, "m");
        AddInput("y", {1}, "m");

        // Define outputs
        AddOutput("f_xy", {1}, "m**2");
    }

    void SetupPartials() override {
        // Declare which gradients are available
        DeclarePartials("f_xy", "x");
        DeclarePartials("f_xy", "y");
    }

    void Compute(const philote::Variables &inputs,
                 philote::Variables &outputs) override {
        double x = inputs.at("x")(0);
        double y = inputs.at("y")(0);

        outputs.at("f_xy")(0) = pow(x - 3.0, 2.0) + x * y +
                                pow(y + 4.0, 2.0) - 3.0;
    }

    void ComputePartials(const philote::Variables &inputs,
                        philote::Partials &partials) override {
        double x = inputs.at("x")(0);
        double y = inputs.at("y")(0);

        partials[{"f_xy", "x"}](0) = 2.0 * x - 6.0 + y;
        partials[{"f_xy", "y"}](0) = 2.0 * y + 8.0 + x;
    }
};
```

### Starting a Server

Once you've defined your discipline, create a server to host it:

```cpp
#include <grpcpp/grpcpp.h>

int main() {
    std::string address("localhost:50051");
    Paraboloid discipline;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    discipline.RegisterServices(builder);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    std::cout << "Server listening on " << address << std::endl;

    server->Wait();
    return 0;
}
```

### Connecting a Client

Connect to a remote discipline server:

```cpp
#include <explicit.h>
#include <grpcpp/grpcpp.h>

int main() {
    philote::ExplicitClient client;

    // Connect to server
    auto channel = grpc::CreateChannel("localhost:50051",
                                      grpc::InsecureChannelCredentials());
    client.ConnectChannel(channel);

    // Initialize client
    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Prepare inputs
    philote::Variables inputs;
    inputs["x"] = philote::Variable(philote::kInput, {1});
    inputs["y"] = philote::Variable(philote::kInput, {1});
    inputs.at("x")(0) = 5.0;
    inputs.at("y")(0) = -2.0;

    // Compute function
    philote::Variables outputs = client.ComputeFunction(inputs);
    std::cout << "f(5, -2) = " << outputs.at("f_xy")(0) << std::endl;

    // Compute gradients
    philote::Partials gradients = client.ComputeGradient(inputs);
    std::cout << "∂f/∂x = " << gradients[{"f_xy", "x"}](0) << std::endl;
    std::cout << "∂f/∂y = " << gradients[{"f_xy", "y"}](0) << std::endl;

    return 0;
}
```

## Creating an Implicit Discipline

Implicit disciplines define residuals that must be solved. Here's an example solving the quadratic equation:

```cpp
#include <implicit.h>
#include <cmath>

class Quadratic : public philote::ImplicitDiscipline {
public:
    Quadratic() = default;

private:
    void Setup() override {
        // Coefficients as inputs
        AddInput("a", {1}, "m");
        AddInput("b", {1}, "m");
        AddInput("c", {1}, "m");

        // Solution as output
        AddOutput("x", {1}, "m**2");
    }

    void SetupPartials() override {
        DeclarePartials("x", "a");
        DeclarePartials("x", "b");
        DeclarePartials("x", "c");
        DeclarePartials("x", "x");  // Jacobian w.r.t. outputs
    }

    void ComputeResiduals(const philote::Variables &inputs,
                         const philote::Variables &outputs,
                         philote::Variables &residuals) override {
        double a = inputs.at("a")(0);
        double b = inputs.at("b")(0);
        double c = inputs.at("c")(0);
        double x = outputs.at("x")(0);

        // Residual: R = ax² + bx + c
        residuals.at("x")(0) = a * pow(x, 2) + b * x + c;
    }

    void SolveResiduals(const philote::Variables &inputs,
                       philote::Variables &outputs) override {
        double a = inputs.at("a")(0);
        double b = inputs.at("b")(0);
        double c = inputs.at("c")(0);

        // Quadratic formula: x = (-b + √(b² - 4ac)) / 2a
        outputs.at("x")(0) = (-b + sqrt(pow(b, 2) - 4 * a * c)) / (2 * a);
    }

    void ComputeResidualGradients(const philote::Variables &inputs,
                                 const philote::Variables &outputs,
                                 philote::Partials &partials) override {
        double a = inputs.at("a")(0);
        double b = inputs.at("b")(0);
        double x = outputs.at("x")(0);

        partials[{"x", "a"}](0) = pow(x, 2);
        partials[{"x", "b"}](0) = x;
        partials[{"x", "c"}](0) = 1.0;
        partials[{"x", "x"}](0) = 2 * a * x + b;
    }
};
```

## Working with Variables

### Creating Variables

Variables can be created in several ways:

```cpp
// Create from metadata
philote::VariableMetaData meta;
meta.set_name("pressure");
meta.add_shape(10);
meta.add_shape(10);
meta.set_units("Pa");
philote::Variable var1(meta);

// Create directly with type and shape
philote::Variable var2(philote::kInput, {10, 10});

// Access and modify values
var2(0) = 101325.0;  // Set first element
double val = var2(0);  // Get first element
```

### Working with Multi-dimensional Arrays

```cpp
// Create a 3x3 matrix
philote::Variable matrix(philote::kOutput, {3, 3});

// Set values
for (size_t i = 0; i < 9; ++i) {
    matrix(i) = static_cast<double>(i);
}

// Get shape information
std::vector<size_t> shape = matrix.Shape();  // Returns {3, 3}
size_t total_size = matrix.Size();           // Returns 9
```

### Accessing Variable Segments

```cpp
philote::Variable large_array(philote::kOutput, {1000});

// Set a segment
std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};
large_array.Segment(10, 15, data);

// Get a segment
std::vector<double> retrieved = large_array.Segment(10, 15);
```

## Advanced Features

### Discipline Options

Disciplines can define configurable options:

```cpp
class ConfigurableDiscipline : public philote::ExplicitDiscipline {
private:
    void Initialize() override {
        ExplicitDiscipline::Initialize();

        // Define available options
        AddOption("tolerance", "float");
        AddOption("max_iterations", "int");
        AddOption("use_adaptive_mesh", "bool");
    }

    void Configure() override {
        // Configuration based on options happens here
        // Options are set by the client before Setup() is called
    }
};
```

### PairDict for Cleaner Partial Access

For more readable gradient access, use `PairDict`:

```cpp
void ComputePartials(const philote::Variables &inputs,
                    philote::Partials &partials) override {
    // Traditional approach
    partials[std::make_pair("output", "input")](0) = 2.0;

    // Using PairDict (if converted from Partials)
    // More readable alternative for accessing partials with two keys
}
```

## Complete Examples

The library includes complete working examples:

### Paraboloid (Explicit Discipline)
- **File**: `examples/paraboloid/paraboloid_server.cpp`
- Demonstrates basic explicit discipline with gradients
- Shows option initialization and configuration

### Rosenbrock (Explicit Discipline)
- **File**: `examples/rosenbrock/rosenbrock_server.cpp`
- Classic optimization test function
- Demonstrates 2D function with gradient computation

### Quadratic Solver (Implicit Discipline)
- **File**: `examples/quadratic/quadratic_server.cpp`
- Solves quadratic equations using implicit formulation
- Shows residual computation and solving

## API Reference

### Core Classes

- philote::Discipline - Base class for all disciplines
- philote::ExplicitDiscipline - Base for explicit disciplines
- philote::ImplicitDiscipline - Base for implicit disciplines
- philote::ExplicitClient - Client for explicit disciplines
- philote::ImplicitClient - Client for implicit disciplines
- philote::Variable - Multi-dimensional array container
- philote::Variables - Map of variable names to Variable objects
- philote::Partials - Map of (output, input) pairs to gradients

### Server Classes

- philote::DisciplineServer - Base server for discipline metadata
- philote::ExplicitServer - Server for explicit discipline computations
- philote::ImplicitServer - Server for implicit discipline computations

## Building and Running

### Prerequisites

- C++20 compliant compiler
- gRPC and Protocol Buffers
- CMake 3.23 or higher

### Build Instructions

```bash
mkdir build && cd build
cmake ..
make -j 8
```

### Running Examples

Start a server:
```bash
./paraboloid_server
```

In another terminal, run the client:
```bash
./paraboloid_client
```

## Best Practices

1. **Always override Compute()**: The base class provides no default implementation
2. **Call parent Initialize()**: When overriding `Initialize()`, call parent first
3. **Declare all partials**: Use `DeclarePartials()` in `SetupPartials()` for every gradient you compute
4. **Check variable dimensions**: Ensure input/output shapes match expectations
5. **Handle exceptions**: Wrap discipline logic in try-catch blocks for robustness
6. **Use meaningful units**: Provide physical units for all variables
7. **Sequential operations**: Call client methods sequentially (not concurrently)

## Limitations

- **No concurrent RPCs**: One RPC at a time per server
- **Single server instance**: Each discipline should have one server
- **Sequential client calls**: Don't call client methods concurrently

## Further Reading

- [Philote MDO Standard](https://github.com/chrislupp/Philote-MDO)
- [gRPC C++ Documentation](https://grpc.io/docs/languages/cpp/)
- API Reference (see class documentation in headers)

## License

Copyright 2022-2025 Christopher A. Lupp
Licensed under the Apache License, Version 2.0
