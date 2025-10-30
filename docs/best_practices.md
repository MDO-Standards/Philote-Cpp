# Best Practices and Limitations {#best_practices}

[TOC]

## Best Practices

### Discipline Development

#### Always Override Compute Methods

The `Compute()` and `ComputePartials()` (explicit) or `ComputeResiduals()` and `SolveResiduals()` (implicit)
methods have no default implementations:

```cpp
// Required for explicit disciplines
void Compute(const philote::Variables &inputs,
             philote::Variables &outputs) override {
    // Must implement - no default
}

// Required for implicit disciplines
void ComputeResiduals(const philote::Variables &inputs,
                     const philote::Variables &outputs,
                     philote::Variables &residuals) override {
    // Must implement - no default
}

void SolveResiduals(const philote::Variables &inputs,
                   philote::Variables &outputs) override {
    // Must implement - no default
}
```

#### Call Parent Initialize

When overriding `Initialize()`, always call the parent version first:

```cpp
void Initialize() override {
    ExplicitDiscipline::Initialize();  // Call parent first
    AddOption("my_option", "float");
}
```

#### Declare All Partials

Use `DeclarePartials()` for every gradient you compute:

```cpp
void SetupPartials() override {
    // Declare all output/input pairs
    DeclarePartials("f", "x");
    DeclarePartials("f", "y");

    // For implicit: also declare output/output pairs
    DeclarePartials("residual", "output");
}
```

### Variable Handling

#### Check Variable Dimensions

Always verify shapes match your expectations:

```cpp
void Compute(const philote::Variables &inputs,
             philote::Variables &outputs) override {
    const auto& vec = inputs.at("vec");
    assert(vec.Size() == expected_size);  // Verify size

    // Now safe to access
    for (size_t i = 0; i < vec.Size(); ++i) {
        outputs.at("result")(i) = process(vec(i));
    }
}
```

#### Use Meaningful Units

Provide physical units for all variables:

```cpp
// Good
AddInput("pressure", {1}, "Pa");
AddInput("temperature", {1}, "K");
AddInput("length", {1}, "m");

// Avoid
AddInput("p", {1}, "");  // Unclear name and no units
```

#### Use Type-Safe Access

Prefer `.at()` over `[]` for bounds checking:

```cpp
// Recommended - throws exception if key doesn't exist
double x = inputs.at("x")(0);

// Avoid - creates entry if key doesn't exist
double x = inputs["x"](0);  // May hide errors
```

### Error Handling

#### Wrap Discipline Logic

Wrap computations in try-catch blocks:

```cpp
void Compute(const philote::Variables &inputs,
             philote::Variables &outputs) override {
    try {
        double x = inputs.at("x")(0);

        if (x < 0) {
            throw std::runtime_error("x must be non-negative");
        }

        outputs.at("y")(0) = std::sqrt(x);

    } catch (const std::exception& e) {
        std::cerr << "Compute error: " << e.what() << std::endl;
        throw;  // Re-throw to propagate to client
    }
}
```

#### Validate Inputs

Check input validity before computation:

```cpp
void Compute(const philote::Variables &inputs,
             philote::Variables &outputs) override {
    double divisor = inputs.at("divisor")(0);

    if (std::abs(divisor) < 1e-14) {
        throw std::runtime_error("Division by zero");
    }

    outputs.at("result")(0) = inputs.at("numerator")(0) / divisor;
}
```

### Client Usage

#### Follow Initialization Sequence

Always initialize clients in the correct order:

```cpp
// Correct order
client.ConnectChannel(channel);
client.GetInfo();
client.Setup();
client.GetVariableDefinitions();
client.GetPartialDefinitions();  // Optional

// Incorrect - will fail
client.GetVariableDefinitions();  // Error: not initialized
```

#### Sequential Operations

Don't call client methods concurrently:

```cpp
// Good - sequential
philote::Variables out1 = client.ComputeFunction(inputs1);
philote::Variables out2 = client.ComputeFunction(inputs2);

// Bad - concurrent (not supported)
std::thread t1([&]() { client.ComputeFunction(inputs1); });
std::thread t2([&]() { client.ComputeFunction(inputs2); });
```

#### Reuse Clients

Create clients once and reuse:

```cpp
// Good - reuse client
philote::ExplicitClient client;
client.ConnectChannel(channel);
client.GetInfo();
client.Setup();
client.GetVariableDefinitions();

for (int i = 0; i < 1000; ++i) {
    // Reuse same client
    philote::Variables outputs = client.ComputeFunction(inputs);
}

// Avoid - creates new client each time (expensive)
for (int i = 0; i < 1000; ++i) {
    philote::ExplicitClient client;  // Don't do this
    // ...
}
```

### Documentation

#### Document Assumptions

Clearly document assumptions in your code:

```cpp
/**
 * Solves quadratic equation ax² + bx + c = 0
 *
 * @note Returns the positive root only
 * @note Assumes b² - 4ac ≥ 0 (real roots exist)
 * @note Assumes a ≠ 0
 */
void SolveResiduals(const philote::Variables &inputs,
                   philote::Variables &outputs) override {
    // Implementation...
}
```

#### Comment Complex Logic

Add comments for non-obvious computations:

```cpp
void ComputePartials(const philote::Variables &inputs,
                    philote::Partials &partials) override {
    double x = inputs.at("x")(0);

    // Chain rule: d/dx[f(g(x))] = f'(g(x)) * g'(x)
    double inner = g(x);
    double df_dg = f_prime(inner);
    double dg_dx = g_prime(x);

    partials[{"f", "x"}](0) = df_dg * dg_dx;
}
```

## Limitations

### No Concurrent RPC Support

The library currently supports only one RPC call at a time:

```cpp
// Not supported - concurrent calls
std::thread t1([&]() {
    client1.ComputeFunction(inputs);
});
std::thread t2([&]() {
    server.ProcessRequest();  // May conflict with t1
});

// Workaround - use separate servers/clients
philote::ExplicitClient client1;
philote::ExplicitClient client2;
// Each connects to different server instances
```

### Single Server Instance

Each discipline should be hosted on a single server instance:

```cpp
// Good - one discipline, one server
MyDiscipline discipline;
grpc::ServerBuilder builder;
discipline.RegisterServices(builder);
auto server = builder.BuildAndStart();

// Avoid - multiple servers for same discipline (not tested)
grpc::ServerBuilder builder1, builder2;
discipline.RegisterServices(builder1);
discipline.RegisterServices(builder2);  // Undefined behavior
```

### Sequential Client Operations

All client operations must be called sequentially:

```cpp
// Correct
client.GetInfo();
client.Setup();
client.GetVariableDefinitions();

// Incorrect - parallel initialization
std::async(std::launch::async, [&](){ client.GetInfo(); });
std::async(std::launch::async, [&](){ client.Setup(); });  // Will fail
```

### No Auto-Reconnect

If the connection drops, create a new client:

```cpp
try {
    philote::Variables outputs = client.ComputeFunction(inputs);
} catch (const std::exception& e) {
    // Connection dropped - create new client
    client = philote::ExplicitClient();
    client.ConnectChannel(new_channel);
    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Retry
    outputs = client.ComputeFunction(inputs);
}
```

### Variable Shape Constraints

Variable shapes are fixed after Setup():

```cpp
void Setup() override {
    AddInput("x", {10}, "m");  // Fixed size of 10
}

void Compute(const philote::Variables &inputs,
             philote::Variables &outputs) override {
    // Input "x" will always have size 10
    // Cannot dynamically change shape
}
```

## Performance Considerations

### Minimize Data Transfers

Reduce the number of RPC calls:

```cpp
// Good - batch inputs
philote::Variables inputs;
inputs["x"] = philote::Variable(philote::kInput, {100});  // 100 values
philote::Variables outputs = client.ComputeFunction(inputs);

// Avoid - multiple calls for each value
for (int i = 0; i < 100; ++i) {
    inputs["x"](0) = values[i];
    client.ComputeFunction(inputs);  // 100 RPCs instead of 1
}
```

### Preallocate Variables

Preallocate variables instead of reallocating:

```cpp
// Good - allocate once
philote::Variables inputs;
inputs["x"] = philote::Variable(philote::kInput, {1});

for (double val : values) {
    inputs.at("x")(0) = val;  // Reuse same variable
    client.ComputeFunction(inputs);
}

// Avoid - reallocate every iteration
for (double val : values) {
    philote::Variables inputs;  // Reallocates every time
    inputs["x"] = philote::Variable(philote::kInput, {1});
    inputs.at("x")(0) = val;
    client.ComputeFunction(inputs);
}
```

### Use Efficient Data Structures

Choose appropriate shapes for your data:

```cpp
// Good for large sequential data
AddInput("time_series", {10000}, "s");

// Good for matrix operations
AddInput("matrix", {100, 100}, "m");

// Avoid deeply nested structures
AddInput("tensor", {10, 10, 10, 10, 10}, "m");  // May be inefficient
```

## Common Pitfalls

### Forgetting to Declare Partials

```cpp
// Wrong - computes partial but didn't declare it
void SetupPartials() override {
    // Forgot to declare!
}

void ComputePartials(const philote::Variables &inputs,
                    philote::Partials &partials) override {
    partials[{"f", "x"}](0) = 2.0;  // Error: not declared
}

// Correct
void SetupPartials() override {
    DeclarePartials("f", "x");  // Must declare first
}
```

### Modifying Const Inputs

```cpp
// Wrong - inputs are const
void Compute(const philote::Variables &inputs,
             philote::Variables &outputs) override {
    inputs.at("x")(0) = 5.0;  // Compile error: inputs is const
}
```

### Incorrect Variable Types

```cpp
// Wrong - using kOutput for inputs
philote::Variables inputs;
inputs["x"] = philote::Variable(philote::kOutput, {1});  // Should be kInput

// Correct
inputs["x"] = philote::Variable(philote::kInput, {1});
```

## Debugging Tips

### Enable Verbose Logging

```cpp
// Enable gRPC logging
setenv("GRPC_VERBOSITY", "DEBUG", 1);
setenv("GRPC_TRACE", "all", 1);
```

### Verify Variable Metadata

```cpp
void Setup() override {
    AddInput("x", {10}, "m");

    // Debug: print metadata
    for (const auto& var : var_meta()) {
        std::cout << "Variable: " << var.name()
                  << ", Size: " << var.shape(0)
                  << ", Units: " << var.units() << std::endl;
    }
}
```

### Check Gradient Accuracy

Use finite differences to verify analytic gradients:

```cpp
double eps = 1e-7;
double x = inputs.at("x")(0);

// Analytic gradient
philote::Partials partials;
ComputePartials(inputs, partials);
double analytic = partials[{"f", "x"}](0);

// Finite difference
inputs.at("x")(0) = x + eps;
philote::Variables out_plus;
Compute(inputs, out_plus);

inputs.at("x")(0) = x - eps;
philote::Variables out_minus;
Compute(inputs, out_minus);

double numeric = (out_plus.at("f")(0) - out_minus.at("f")(0)) / (2 * eps);

std::cout << "Analytic: " << analytic << ", Numeric: " << numeric << std::endl;
```

## See Also

- @ref getting_started - Getting started guide
- @ref explicit_disciplines - Explicit discipline development
- @ref implicit_disciplines - Implicit discipline development
- @ref client_usage - Client usage patterns
