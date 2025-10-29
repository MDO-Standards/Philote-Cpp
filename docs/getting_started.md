# Getting Started {#getting_started}

[TOC]

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

## Prerequisites

- C++20 compliant compiler
- gRPC and Protocol Buffers
- CMake 3.23 or higher

## Building Philote-Cpp

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

## Next Steps

- @ref explicit_disciplines - Learn how to create explicit disciplines
- @ref implicit_disciplines - Learn how to create implicit disciplines
- @ref variables - Working with variables and arrays
- @ref client_usage - Connecting clients to servers
