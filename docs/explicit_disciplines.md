# Explicit Disciplines {#explicit_disciplines}

[TOC]

## Introduction

Explicit disciplines compute direct function evaluations (f(x) = y) and optionally their gradients.
They are suitable for analyses where outputs can be computed directly from inputs without
solving implicit equations.

## Creating a Simple Explicit Discipline

Here's a minimal explicit discipline:

```cpp
#include <explicit.h>

class SimpleFunction : public philote::ExplicitDiscipline {
private:
    void Setup() override {
        AddInput("x", {1}, "m");
        AddOutput("y", {1}, "m");
    }

    void SetupPartials() override {
        DeclarePartials("y", "x");
    }

    void Compute(const philote::Variables &inputs,
                 philote::Variables &outputs) override {
        double x = inputs.at("x")(0);
        outputs.at("y")(0) = 2.0 * x + 1.0;
    }

    void ComputePartials(const philote::Variables &inputs,
                        philote::Partials &partials) override {
        partials[{"y", "x"}](0) = 2.0;
    }
};
```

## Multi-Input/Output Discipline

A more complex example with multiple inputs and outputs:

```cpp
#include <explicit.h>
#include <cmath>

class Paraboloid : public philote::ExplicitDiscipline {
private:
    void Setup() override {
        AddInput("x", {1}, "m");
        AddInput("y", {1}, "m");
        AddOutput("f_xy", {1}, "m**2");
    }

    void SetupPartials() override {
        DeclarePartials("f_xy", "x");
        DeclarePartials("f_xy", "y");
    }

    void Compute(const philote::Variables &inputs,
                 philote::Variables &outputs) override {
        double x = inputs.at("x")(0);
        double y = inputs.at("y")(0);

        outputs.at("f_xy")(0) = std::pow(x - 3.0, 2.0) + x * y +
                                std::pow(y + 4.0, 2.0) - 3.0;
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

## Discipline with Options

Disciplines can define configurable options:

```cpp
class ConfigurableDiscipline : public philote::ExplicitDiscipline {
private:
    double scale_factor_ = 1.0;

    void Initialize() override {
        ExplicitDiscipline::Initialize();
        AddOption("scale_factor", "float");
        AddOption("enable_scaling", "bool");
    }

    void Configure() override {
        // Configuration based on options set by client
        // Options are available via discipline properties
    }

    void Setup() override {
        AddInput("x", {1}, "m");
        AddOutput("y", {1}, "m");
    }

    void Compute(const philote::Variables &inputs,
                 philote::Variables &outputs) override {
        outputs.at("y")(0) = scale_factor_ * inputs.at("x")(0);
    }
};
```

## Starting a Server

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

## Required Methods

### Setup()

Called once to define variables:

```cpp
void Setup() override {
    // Define all inputs
    AddInput("name", {shape}, "units");

    // Define all outputs
    AddOutput("name", {shape}, "units");
}
```

### Compute()

**Must be implemented** - performs the actual computation:

```cpp
void Compute(const philote::Variables &inputs,
             philote::Variables &outputs) override {
    // Extract input values
    double x = inputs.at("x")(0);

    // Compute outputs
    outputs.at("y")(0) = f(x);
}
```

### SetupPartials() and ComputePartials()

Optional, but recommended for optimization:

```cpp
void SetupPartials() override {
    // Declare which gradients are available
    DeclarePartials("output", "input");
}

void ComputePartials(const philote::Variables &inputs,
                    philote::Partials &partials) override {
    // Compute gradients
    partials[{"output", "input"}](0) = df_dx;
}
```

## Lifecycle

The discipline lifecycle when a client connects:

1. **Initialize()** - Define available options
2. **SetOptions()** - Client sets option values
3. **Configure()** - Process option values
4. **Setup()** - Define variables
5. **SetupPartials()** - Declare available gradients
6. **Compute()** - Called for each function evaluation
7. **ComputePartials()** - Called for each gradient evaluation

## Best Practices

1. **Always override Compute()**: No default implementation exists
2. **Call parent Initialize()**: When overriding, call `ExplicitDiscipline::Initialize()` first
3. **Declare all partials**: Use `DeclarePartials()` for every gradient you compute
4. **Use const for inputs**: Input variables should not be modified
5. **Preallocate outputs**: Framework preallocates based on `Setup()`
6. **Handle exceptions**: Wrap computation in try-catch for robustness

## Common Patterns

### Vector Operations

```cpp
void Setup() override {
    AddInput("vec", {10}, "m");
    AddOutput("scaled", {10}, "m");
}

void Compute(const philote::Variables &inputs,
             philote::Variables &outputs) override {
    for (size_t i = 0; i < 10; ++i) {
        outputs.at("scaled")(i) = 2.0 * inputs.at("vec")(i);
    }
}
```

### Matrix Operations

```cpp
void Setup() override {
    AddInput("matrix", {3, 3}, "m");
    AddOutput("determinant", {1}, "m**3");
}

void Compute(const philote::Variables &inputs,
             philote::Variables &outputs) override {
    // Access matrix elements with linear indexing
    // Element [i,j] is at index i*cols + j
    const auto& A = inputs.at("matrix");
    outputs.at("determinant")(0) = ComputeDeterminant(A);
}
```

## Handling Cancellation

For long-running computations, disciplines can detect and respond to client cancellations (timeouts, disconnects) by checking `IsCancelled()`:

```cpp
class LongComputation : public philote::ExplicitDiscipline {
    void Compute(const philote::Variables &inputs,
                 philote::Variables &outputs) override {
        for (int iter = 0; iter < 1000000; iter++) {
            // Check cancellation periodically (every 1000 iterations)
            if (iter % 1000 == 0 && IsCancelled()) {
                throw std::runtime_error("Computation cancelled by client");
            }

            // ... perform expensive work ...
        }

        outputs.at("result")(0) = computed_value;
    }
};
```

**Key Points**:
- Server automatically detects cancellations - `IsCancelled()` is optional
- Only use for computations that would benefit from early termination
- Check periodically (not every iteration) to minimize overhead
- Works across all client languages (Python, C++, etc.)
- Throw exception or return early when cancelled

**Client-Side**:
```cpp
client.SetRPCTimeout(std::chrono::milliseconds(5000));  // 5s timeout
try {
    auto outputs = client.ComputeFunction(inputs);
} catch (const std::runtime_error& e) {
    // Handle timeout/cancellation
}
```

## Complete Examples

See the examples directory for complete working examples:
- `examples/paraboloid/` - Basic explicit discipline
- `examples/rosenbrock/` - Classic optimization test function

## See Also

- @ref client_usage - Connecting clients to explicit disciplines
- @ref variables - Working with variables
- @ref implicit_disciplines - Alternative discipline type
- philote::ExplicitDiscipline - API documentation
- philote::ExplicitServer - Server API documentation
