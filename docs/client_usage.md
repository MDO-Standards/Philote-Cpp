# Client Usage {#client_usage}

[TOC]

## Introduction

Philote provides client classes for connecting to and interacting with remote discipline servers.
Clients handle all gRPC communication and provide a simple interface for function and gradient evaluations.

## Explicit Discipline Clients

### Basic Connection

```cpp
#include <explicit.h>
#include <grpcpp/grpcpp.h>

int main() {
    philote::ExplicitClient client;

    // Connect to server
    auto channel = grpc::CreateChannel("localhost:50051",
                                      grpc::InsecureChannelCredentials());
    client.ConnectChannel(channel);

    // Initialize client (required sequence)
    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    return 0;
}
```

### Computing Functions

```cpp
// Prepare inputs
philote::Variables inputs;
inputs["x"] = philote::Variable(philote::kInput, {1});
inputs["y"] = philote::Variable(philote::kInput, {1});
inputs.at("x")(0) = 5.0;
inputs.at("y")(0) = -2.0;

// Compute function
philote::Variables outputs = client.ComputeFunction(inputs);
std::cout << "f(5, -2) = " << outputs.at("f_xy")(0) << std::endl;
```

### Computing Gradients

```cpp
// Get partial definitions first
client.GetPartialDefinitions();

// Compute gradients
philote::Partials gradients = client.ComputeGradient(inputs);

// Access gradient values
double df_dx = gradients[{"f_xy", "x"}](0);
double df_dy = gradients[{"f_xy", "y"}](0);

std::cout << "∂f/∂x = " << df_dx << std::endl;
std::cout << "∂f/∂y = " << df_dy << std::endl;
```

### Complete Workflow

Here's a complete explicit client workflow:

```cpp
philote::ExplicitClient client;
auto channel = grpc::CreateChannel("localhost:50051",
                                  grpc::InsecureChannelCredentials());
client.ConnectChannel(channel);

// Step 1: Get discipline information
client.GetInfo();

// Step 2: Setup the discipline
client.Setup();

// Step 3: Get variable definitions
client.GetVariableDefinitions();
std::vector<std::string> var_names = client.GetVariableNames();

// Step 4: Get partial definitions (if computing gradients)
client.GetPartialDefinitions();

// Step 5: Prepare inputs dynamically
philote::Variables inputs;
for (const auto& name : var_names) {
    auto meta = client.GetVariableMeta(name);
    if (meta.type() == philote::kInput) {
        inputs[name] = philote::Variable(meta);
        inputs[name](0) = 1.0;  // Set your values
    }
}

// Step 6: Compute
philote::Variables outputs = client.ComputeFunction(inputs);
philote::Partials partials = client.ComputeGradient(inputs);
```

## Implicit Discipline Clients

### Basic Connection

```cpp
#include <implicit.h>
#include <grpcpp/grpcpp.h>

int main() {
    philote::ImplicitClient client;

    // Connect to server
    auto channel = grpc::CreateChannel("localhost:50051",
                                      grpc::InsecureChannelCredentials());
    client.ConnectChannel(channel);

    // Initialize client
    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    return 0;
}
```

### Solving for Outputs

```cpp
// Prepare inputs
philote::Variables inputs;
inputs["a"] = philote::Variable(philote::kInput, {1});
inputs["b"] = philote::Variable(philote::kInput, {1});
inputs["c"] = philote::Variable(philote::kInput, {1});

inputs.at("a")(0) = 1.0;
inputs.at("b")(0) = -5.0;
inputs.at("c")(0) = 6.0;

// Solve for outputs
philote::Variables outputs = client.SolveResiduals(inputs);
std::cout << "Solution: x = " << outputs.at("x")(0) << std::endl;
```

### Computing Residuals

For verification or optimization, you can compute residuals directly:

```cpp
// Prepare both inputs and outputs
philote::Variables vars;
vars["a"] = philote::Variable(philote::kInput, {1});
vars["b"] = philote::Variable(philote::kInput, {1});
vars["c"] = philote::Variable(philote::kInput, {1});
vars["x"] = philote::Variable(philote::kOutput, {1});

vars.at("a")(0) = 1.0;
vars.at("b")(0) = -5.0;
vars.at("c")(0) = 6.0;
vars.at("x")(0) = 2.0;  // Guess or provided value

// Compute residual
philote::Variables residuals = client.ComputeResiduals(vars);
std::cout << "R(x=2) = " << residuals.at("x")(0) << std::endl;
```

### Computing Residual Gradients

```cpp
// Get partial definitions
client.GetPartialDefinitions();

// Compute residual gradients
philote::Partials gradients = client.ComputeResidualGradients(vars);

// Access gradient values (includes both input and output derivatives)
double dR_da = gradients[{"x", "a"}](0);
double dR_db = gradients[{"x", "b"}](0);
double dR_dc = gradients[{"x", "c"}](0);
double dR_dx = gradients[{"x", "x"}](0);  // Jacobian w.r.t. outputs
```

### Complete Implicit Workflow

```cpp
philote::ImplicitClient client;
auto channel = grpc::CreateChannel("localhost:50051",
                                  grpc::InsecureChannelCredentials());
client.ConnectChannel(channel);

// Step 1-3: Initialize
client.GetInfo();
client.Setup();
client.GetVariableDefinitions();

// Step 4: Prepare inputs
philote::Variables inputs;
inputs["a"] = philote::Variable(philote::kInput, {1});
inputs["a"](0) = 1.0;
// ... set other inputs

// Step 5: Solve for outputs
philote::Variables outputs = client.SolveResiduals(inputs);

// Step 6 (optional): Verify solution
philote::Variables combined = inputs;
for (const auto& [name, var] : outputs) {
    combined[name] = var;
}
philote::Variables residuals = client.ComputeResiduals(combined);
// residuals should be near zero

// Step 7 (optional): Compute gradients
client.GetPartialDefinitions();
philote::Partials partials = client.ComputeResidualGradients(combined);
```

## Connection Management

### Channel Creation

```cpp
// Insecure channel (for testing)
auto channel = grpc::CreateChannel("localhost:50051",
                                  grpc::InsecureChannelCredentials());

// Secure channel with SSL/TLS
auto creds = grpc::SslCredentials(grpc::SslCredentialsOptions());
auto channel = grpc::CreateChannel("myserver.com:50051", creds);
```

### Channel Options

```cpp
grpc::ChannelArguments args;
args.SetMaxReceiveMessageSize(100 * 1024 * 1024);  // 100 MB
args.SetMaxSendMessageSize(100 * 1024 * 1024);

auto channel = grpc::CreateCustomChannel("localhost:50051",
                                        grpc::InsecureChannelCredentials(),
                                        args);
```

### Multiple Servers

```cpp
// Connect to multiple disciplines
philote::ExplicitClient aero_client;
philote::ExplicitClient structures_client;

auto aero_channel = grpc::CreateChannel("localhost:50051",
                                       grpc::InsecureChannelCredentials());
auto struct_channel = grpc::CreateChannel("localhost:50052",
                                         grpc::InsecureChannelCredentials());

aero_client.ConnectChannel(aero_channel);
structures_client.ConnectChannel(struct_channel);

// Initialize both
aero_client.GetInfo();
aero_client.Setup();
// ... etc
```

## Error Handling

Always handle potential errors:

```cpp
try {
    client.ConnectChannel(channel);
    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    philote::Variables outputs = client.ComputeFunction(inputs);

} catch (const std::exception& e) {
    std::cerr << "Client error: " << e.what() << std::endl;
    return 1;
}
```

## Initialization Sequence

The client initialization must follow this order:

1. **ConnectChannel()** - Establish gRPC connection
2. **GetInfo()** - Retrieve discipline properties
3. **Setup()** - Initialize the remote discipline
4. **GetVariableDefinitions()** - Get variable metadata
5. **GetPartialDefinitions()** - (Optional) Get gradient metadata

**Important**: Calling methods out of order will result in errors.

## Querying Variable Information

### Get All Variable Names

```cpp
std::vector<std::string> names = client.GetVariableNames();
for (const auto& name : names) {
    std::cout << "Variable: " << name << std::endl;
}
```

### Get Variable Metadata

```cpp
philote::VariableMetaData meta = client.GetVariableMeta("x");

std::cout << "Name: " << meta.name() << std::endl;
std::cout << "Type: " << (meta.type() == philote::kInput ? "Input" : "Output")
          << std::endl;
std::cout << "Units: " << meta.units() << std::endl;

std::cout << "Shape: ";
for (int64_t dim : meta.shape()) {
    std::cout << dim << " ";
}
std::cout << std::endl;
```

### Get Partials Metadata

```cpp
std::vector<philote::PartialsMetaData> partials = client.GetPartialsMeta();

for (const auto& partial : partials) {
    std::cout << "∂" << partial.name() << "/∂" << partial.subname() << std::endl;
}
```

## Best Practices

1. **Initialize in order**: Follow the required initialization sequence
2. **Reuse clients**: Create client once and reuse for multiple evaluations
3. **Check connectivity**: Verify channel state before operations
4. **Handle timeouts**: Set appropriate timeout values for long-running computations
5. **Sequential operations**: Don't call client methods concurrently
6. **Error handling**: Wrap client calls in try-catch blocks
7. **Variable shapes**: Always create inputs with correct shapes from metadata

## Common Patterns

### Repeated Evaluations

```cpp
// Initialize once
client.ConnectChannel(channel);
client.GetInfo();
client.Setup();
client.GetVariableDefinitions();

// Reuse for multiple evaluations
for (double x = 0.0; x < 10.0; x += 0.1) {
    inputs.at("x")(0) = x;
    philote::Variables outputs = client.ComputeFunction(inputs);
    // Process outputs...
}
```

### Parameter Sweeps

```cpp
std::vector<double> x_values = {1.0, 2.0, 3.0, 4.0, 5.0};
std::vector<double> y_values;

for (double x : x_values) {
    inputs.at("x")(0) = x;
    philote::Variables outputs = client.ComputeFunction(inputs);
    y_values.push_back(outputs.at("y")(0));
}
```

## Limitations

- **No concurrent RPCs**: One RPC at a time per client
- **Sequential operations**: All client methods must be called sequentially
- **Single connection**: Each client connects to one server
- **No auto-reconnect**: If connection drops, create new client

## See Also

- @ref explicit_disciplines - Creating explicit disciplines
- @ref implicit_disciplines - Creating implicit disciplines
- @ref variables - Working with variables
- philote::ExplicitClient - API documentation
- philote::ImplicitClient - API documentation
