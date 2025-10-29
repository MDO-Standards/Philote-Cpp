# Philote-Cpp Developer Documentation {#mainpage}

## Welcome

Philote-Cpp provides C++ bindings for the Philote MDO (Multidisciplinary Design Optimization) standard.
The library enables distributed computing for engineering disciplines using gRPC for remote procedure calls.

Philote disciplines rely on transmitting data and calling functions (remote procedure calls) via the network.
Disciplines can be defined as **explicit** (direct function evaluations like f(x) = y) or as **implicit**
(defining a residual that must be solved, e.g., R(x) = 0).

## Documentation Sections

### Getting Started
@ref getting_started

Learn the basics of Philote-Cpp including key concepts, building instructions, and your first discipline.

**Topics covered:**
- Overview of disciplines, variables, and gradients
- Building and running Philote-Cpp
- Quick start guide

### Installation and Integration
@ref installation

Complete guide to installing Philote-Cpp and integrating it into your projects.

**Topics covered:**
- Installing dependencies
- Building and installing the library
- Using CMake to link against Philote-Cpp
- Including headers in your code
- Example project structure
- Troubleshooting common issues

### Working with Variables
@ref variables

Comprehensive guide to the Variable class and data structures.

**Topics covered:**
- Creating and accessing variables
- Multi-dimensional arrays
- Segments and partial array access
- Partials and PairDict for gradients
- Variables collections

### Explicit Disciplines
@ref explicit_disciplines

Learn how to create explicit disciplines that compute direct function evaluations.

**Topics covered:**
- Creating explicit disciplines
- Simple and multi-input/output examples
- Discipline with configurable options
- Computing gradients
- Starting servers
- Complete examples

### Implicit Disciplines
@ref implicit_disciplines

Learn how to create implicit disciplines that solve residual equations.

**Topics covered:**
- Understanding implicit formulations
- Computing and solving residuals
- Computing residual gradients
- Differences from explicit disciplines
- Iterative solvers
- Complete examples

### Client Usage
@ref client_usage

Learn how to connect clients to discipline servers and perform computations.

**Topics covered:**
- Connecting to explicit and implicit servers
- Computing functions and gradients
- Computing and solving residuals
- Connection management
- Error handling
- Complete client workflows

### Best Practices and Limitations
@ref best_practices

Important patterns, guidelines, and known limitations.

**Topics covered:**
- Discipline development best practices
- Variable handling guidelines
- Error handling patterns
- Performance considerations
- Common pitfalls
- Current limitations
- Debugging tips

## Quick Reference

### Simple Explicit Discipline

```cpp
#include <explicit.h>

class MyDiscipline : public philote::ExplicitDiscipline {
    void Setup() override {
        AddInput("x", {1}, "m");
        AddOutput("y", {1}, "m");
    }

    void Compute(const philote::Variables &inputs,
                 philote::Variables &outputs) override {
        outputs.at("y")(0) = 2.0 * inputs.at("x")(0);
    }
};
```

### Starting a Server

```cpp
#include <grpcpp/grpcpp.h>

int main() {
    MyDiscipline discipline;

    grpc::ServerBuilder builder;
    builder.AddListeningPort("localhost:50051",
                            grpc::InsecureServerCredentials());
    discipline.RegisterServices(builder);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    server->Wait();
    return 0;
}
```

### Connecting a Client

```cpp
#include <explicit.h>

int main() {
    philote::ExplicitClient client;

    auto channel = grpc::CreateChannel("localhost:50051",
                                      grpc::InsecureChannelCredentials());
    client.ConnectChannel(channel);
    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    philote::Variables inputs;
    inputs["x"] = philote::Variable(philote::kInput, {1});
    inputs.at("x")(0) = 5.0;

    philote::Variables outputs = client.ComputeFunction(inputs);
    return 0;
}
```

## Core Classes

### Disciplines
- philote::Discipline - Base class for all disciplines
- philote::ExplicitDiscipline - Base for explicit disciplines
- philote::ImplicitDiscipline - Base for implicit disciplines

### Clients
- philote::ExplicitClient - Client for explicit disciplines
- philote::ImplicitClient - Client for implicit disciplines
- philote::DisciplineClient - Base client functionality

### Data Structures
- philote::Variable - Multi-dimensional array container
- philote::Variables - Map of variable names to Variable objects
- philote::Partials - Map of (output, input) pairs to gradients
- philote::PairDict - Template for cleaner two-key access

### Servers
- philote::DisciplineServer - Server for discipline metadata
- philote::ExplicitServer - Server for explicit computations
- philote::ImplicitServer - Server for implicit computations

## Complete Examples

The library includes several complete working examples in the `examples/` directory:

- **Paraboloid** (`examples/paraboloid/`) - Explicit discipline with options
- **Rosenbrock** (`examples/rosenbrock/`) - Classic optimization test function
- **Quadratic** (`examples/quadratic/`) - Implicit discipline solving equations

Each example includes both server and client implementations.

## Platform Support

Philote-Cpp is tested on:
- **Ubuntu 24.04** with gcc-12/13/14 and clang-16/17/18
- **Build types**: Release and Debug

The library should work on any system that supports gRPC and C++20.

## License

Copyright 2022-2025 Christopher A. Lupp

Licensed under the Apache License, Version 2.0. See LICENSE file for details.

This work has been cleared for public release, distribution unlimited, case number: AFRL-2023-5716.

## Further Reading

- [Philote MDO Standard](https://github.com/chrislupp/Philote-MDO) - The underlying standard
- [gRPC C++ Documentation](https://grpc.io/docs/languages/cpp/) - gRPC framework documentation

## Citation

If you use this code for academic work, please cite:

Lupp, C.A., Xu, A. "Creating a Universal Communication Standard to Enable Heterogeneous
Multidisciplinary Design Optimization." AIAA SCITECH 2024 Forum. American Institute of
Aeronautics and Astronautics. Orlando, FL.
