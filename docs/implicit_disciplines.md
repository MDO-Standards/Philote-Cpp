# Implicit Disciplines {#implicit_disciplines}

[TOC]

## Introduction

Implicit disciplines define residuals R(x,y) that must be solved to find outputs. They are suitable
for analyses involving systems of equations, iterative solvers, or problems where outputs cannot
be computed directly.

## Understanding Implicit Formulations

An implicit discipline defines a residual function:
```
R(inputs, outputs) = 0
```

The discipline must:
1. **Compute residuals**: Evaluate R for given inputs and outputs
2. **Solve residuals**: Find outputs such that R = 0
3. **Compute gradients** (optional): Provide ∂R/∂inputs and ∂R/∂outputs

## Simple Implicit Discipline

Here's a basic example solving x² - y = 0:

```cpp
#include <implicit.h>
#include <cmath>

class SimpleImplicit : public philote::ImplicitDiscipline {
private:
    void Setup() override {
        AddInput("x", {1}, "m");
        AddOutput("y", {1}, "m**2");
    }

    void SetupPartials() override {
        DeclarePartials("y", "x");
        DeclarePartials("y", "y");  // Jacobian w.r.t. outputs
    }

    void ComputeResiduals(const philote::Variables &inputs,
                         const philote::Variables &outputs,
                         philote::Variables &residuals) override {
        double x = inputs.at("x")(0);
        double y = outputs.at("y")(0);
        residuals.at("y")(0) = std::pow(x, 2) - y;
    }

    void SolveResiduals(const philote::Variables &inputs,
                       philote::Variables &outputs) override {
        double x = inputs.at("x")(0);
        outputs.at("y")(0) = std::pow(x, 2);
    }

    void ComputeResidualGradients(const philote::Variables &inputs,
                                 const philote::Variables &outputs,
                                 philote::Partials &partials) override {
        double x = inputs.at("x")(0);
        partials[{"y", "x"}](0) = 2.0 * x;   // ∂R/∂x
        partials[{"y", "y"}](0) = -1.0;       // ∂R/∂y
    }
};
```

## Quadratic Equation Solver

A more complex example solving ax² + bx + c = 0:

```cpp
#include <implicit.h>
#include <cmath>

class Quadratic : public philote::ImplicitDiscipline {
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
        DeclarePartials("x", "x");  // ∂R/∂x
    }

    void ComputeResiduals(const philote::Variables &inputs,
                         const philote::Variables &outputs,
                         philote::Variables &residuals) override {
        double a = inputs.at("a")(0);
        double b = inputs.at("b")(0);
        double c = inputs.at("c")(0);
        double x = outputs.at("x")(0);

        // Residual: R = ax² + bx + c
        residuals.at("x")(0) = a * std::pow(x, 2) + b * x + c;
    }

    void SolveResiduals(const philote::Variables &inputs,
                       philote::Variables &outputs) override {
        double a = inputs.at("a")(0);
        double b = inputs.at("b")(0);
        double c = inputs.at("c")(0);

        // Quadratic formula: x = (-b + √(b² - 4ac)) / 2a
        outputs.at("x")(0) = (-b + std::sqrt(std::pow(b, 2) - 4*a*c)) / (2*a);
    }

    void ComputeResidualGradients(const philote::Variables &inputs,
                                 const philote::Variables &outputs,
                                 philote::Partials &partials) override {
        double a = inputs.at("a")(0);
        double b = inputs.at("b")(0);
        double x = outputs.at("x")(0);

        // Gradients of residual
        partials[{"x", "a"}](0) = std::pow(x, 2);  // ∂R/∂a
        partials[{"x", "b"}](0) = x;                // ∂R/∂b
        partials[{"x", "c"}](0) = 1.0;              // ∂R/∂c
        partials[{"x", "x"}](0) = 2*a*x + b;        // ∂R/∂x
    }
};
```

## Starting an Implicit Server

Server setup is identical to explicit disciplines:

```cpp
#include <grpcpp/grpcpp.h>

int main() {
    std::string address("localhost:50051");
    Quadratic discipline;

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

### ComputeResiduals()

**Must be implemented** - evaluates the residual:

```cpp
void ComputeResiduals(const philote::Variables &inputs,
                     const philote::Variables &outputs,
                     philote::Variables &residuals) override {
    // Extract inputs and outputs
    double x = inputs.at("x")(0);
    double y = outputs.at("y")(0);

    // Compute residual: R(x, y)
    residuals.at("y")(0) = f(x, y);
}
```

### SolveResiduals()

**Must be implemented** - solves for outputs:

```cpp
void SolveResiduals(const philote::Variables &inputs,
                   philote::Variables &outputs) override {
    // Extract inputs
    double x = inputs.at("x")(0);

    // Solve for outputs such that R(x, y) = 0
    outputs.at("y")(0) = solve(x);
}
```

### ComputeResidualGradients()

Optional - computes Jacobian of residuals:

```cpp
void ComputeResidualGradients(const philote::Variables &inputs,
                             const philote::Variables &outputs,
                             philote::Partials &partials) override {
    // Compute ∂R/∂inputs and ∂R/∂outputs
    partials[{"output", "input"}](0) = dR_dx;
    partials[{"output", "output"}](0) = dR_dy;
}
```

## Key Differences from Explicit Disciplines

| Aspect | Explicit | Implicit |
|--------|----------|----------|
| Function signature | `Compute(inputs, outputs)` | `ComputeResiduals(inputs, outputs, residuals)` |
| What it computes | Outputs directly | Residuals R(inputs, outputs) |
| Additional method | - | `SolveResiduals(inputs, outputs)` |
| Gradients | ∂outputs/∂inputs | ∂R/∂inputs and ∂R/∂outputs |
| Use case | Direct calculations | Systems of equations |

## Lifecycle

The discipline lifecycle:

1. **Initialize()** - Define available options
2. **SetOptions()** - Client sets option values
3. **Configure()** - Process option values
4. **Setup()** - Define variables
5. **SetupPartials()** - Declare available gradients
6. **ComputeResiduals()** - Called to evaluate residuals
7. **SolveResiduals()** - Called to solve for outputs
8. **ComputeResidualGradients()** - Called for gradient evaluation

## Best Practices

1. **Implement both required methods**: ComputeResiduals() and SolveResiduals()
2. **Include output gradients**: Declare partials w.r.t. both inputs and outputs
3. **Test residual accuracy**: After solving, R should be near zero
4. **Handle solver failures**: Check for convergence in SolveResiduals()
5. **Use const appropriately**: Inputs and outputs are const in ComputeResiduals()
6. **Document assumptions**: Specify which solution branch (e.g., positive root)

## Common Patterns

### Iterative Solvers

```cpp
void SolveResiduals(const philote::Variables &inputs,
                   philote::Variables &outputs) override {
    double x = inputs.at("x")(0);
    double y_guess = 1.0;  // Initial guess

    // Newton-Raphson or similar
    for (int iter = 0; iter < max_iterations; ++iter) {
        double residual = ComputeR(x, y_guess);
        double jacobian = ComputedR_dy(x, y_guess);

        y_guess = y_guess - residual / jacobian;

        if (std::abs(residual) < tolerance) {
            break;
        }
    }

    outputs.at("y")(0) = y_guess;
}
```

### Systems of Equations

```cpp
void Setup() override {
    AddInput("a", {1}, "m");
    AddInput("b", {1}, "m");

    AddOutput("x", {1}, "m");
    AddOutput("y", {1}, "m");
}

void ComputeResiduals(const philote::Variables &inputs,
                     const philote::Variables &outputs,
                     philote::Variables &residuals) override {
    double a = inputs.at("a")(0);
    double b = inputs.at("b")(0);
    double x = outputs.at("x")(0);
    double y = outputs.at("y")(0);

    // System: R1(a,b,x,y) = 0
    //         R2(a,b,x,y) = 0
    residuals.at("x")(0) = f1(a, b, x, y);
    residuals.at("y")(0) = f2(a, b, x, y);
}
```

## Verifying Solutions

Always verify that your solution satisfies the residual:

```cpp
// In your solver implementation
philote::Variables residuals;
for (const auto& [name, var] : outputs) {
    residuals[name] = philote::Variable(philote::kOutput, var.Shape());
}

ComputeResiduals(inputs, outputs, residuals);

// Check that residuals are near zero
for (const auto& [name, var] : residuals) {
    assert(std::abs(var(0)) < 1e-10);
}
```

## Handling Cancellation

For long-running implicit solvers, disciplines can detect client cancellations by checking `IsCancelled()`:

```cpp
class IterativeSolver : public philote::ImplicitDiscipline {
    void SolveResiduals(const philote::Variables &inputs,
                       philote::Variables &outputs) override {
        int max_iter = 10000;
        double tolerance = 1e-10;

        for (int iter = 0; iter < max_iter; iter++) {
            // Check cancellation every 100 iterations
            if (iter % 100 == 0 && IsCancelled()) {
                throw std::runtime_error("Solver cancelled by client");
            }

            // ... Newton iteration or other solver method ...

            if (residual_norm < tolerance) break;
        }
    }
};
```

**Key Points**:
- Server automatically detects cancellations - `IsCancelled()` is optional
- Particularly useful for iterative solvers that may not converge
- Check periodically to minimize overhead
- Works across all client languages
- See @ref explicit_disciplines for more cancellation examples

## Complete Examples

See the examples directory:
- `examples/quadratic/` - Quadratic equation solver

## See Also

- @ref client_usage - Connecting clients to implicit disciplines
- @ref variables - Working with variables
- @ref explicit_disciplines - Alternative discipline type
- philote::ImplicitDiscipline - API documentation
- philote::ImplicitServer - Server API documentation
