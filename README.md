![Philote](https://github.com/chrislupp/Philote-MDO/blob/main/doc/graphics/logos/philote.svg?raw=true)

[![Build and Regression Tests](https://github.com/chrislupp/Philote-Cpp/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/chrislupp/Philote-Cpp/actions/workflows/build.yml)
[![codecov](https://codecov.io/gh/MDO-Standards/Philote-Cpp/graph/badge.svg?token=8149CQP5EH)](https://codecov.io/gh/MDO-Standards/Philote-Cpp)

# Philote-Cpp

***Author**: Christopher A. Lupp*

Philote MDO is a standard currently being defined by a working group sponsored
by the AIAA Multidisciplinary Design Optimization Technical Committee. While the
Philote standard defines how disciplines are defined and how data is
transmitted, the standard does not encompass language interfaces. This library
is an implementation of C++ bindings for the Philote standard.

## Premise

Philote disciplines rely on transmitting data and calling functions (remote
procedure calls) via the network. This does not necessarily mean that the
disciplines must run on a separate computer (calling via localhost is a
common use case). Disciplines can be defined as explicit (think, f(x) = x)
or as implicit (defining a residual, e.g., R(x) = f(x) - x, which must be
solved). The disciplines must provide a function evaluation and optionally
may provide gradients (for gradient-based optimization).

## Usage

### Creating an Explicit Discipline

To create an explicit discipline, inherit from `philote::ExplicitDiscipline` and implement the required methods:

```cpp
#include "explicit.h"

class MyDiscipline : public philote::ExplicitDiscipline {
public:
    void Setup() override {
        // Define inputs and outputs
        AddInput("x", {1}, "m");
        AddOutput("f", {1}, "N");
    }

    void SetupPartials() override {
        // Declare which gradients are available
        DeclarePartials("f", "x");
    }

    void Compute(const philote::Variables &inputs,
                 philote::Variables &outputs) override {
        // Implement your computation
        outputs.at("f")(0) = inputs.at("x")(0) * 2.0;
    }

    void ComputePartials(const philote::Variables &inputs,
                        philote::Partials &partials) override {
        // Implement gradient computation
        partials[{"f", "x"}](0) = 2.0;
    }
};
```

**Note**: You must override `Compute()` and (optionally) `ComputePartials()`.
These methods have no default implementation and are intended to contain your
discipline-specific logic.

### Starting a Server

```cpp
#include <grpcpp/grpcpp.h>
#include "explicit.h"

int main() {
    MyDiscipline discipline;

    grpc::ServerBuilder builder;
    builder.AddListeningPort("localhost:50051", grpc::InsecureServerCredentials());
    discipline.RegisterServices(builder);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    server->Wait();

    return 0;
}
```

### Connecting a Client

```cpp
#include "explicit.h"

int main() {
    philote::ExplicitClient client;

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
    inputs.at("x")(0) = 3.0;

    // Compute
    philote::Variables outputs = client.ComputeFunction(inputs);

    return 0;
}
```

## Limitations

- **No Concurrent RPC Support**: The library currently supports only one RPC call at a time.
  Concurrent calls from multiple threads or clients to the same server are not supported and
  may result in undefined behavior.

- **Single Server Instance**: Each discipline should be hosted on a single server instance.

- **Sequential Operations**: All client operations (GetInfo, Setup, ComputeFunction, etc.)
  should be called sequentially, not concurrently.


## Building Philote-Cpp

### Dependencies

To build Philote-Cpp you must have
[gRPC](https://grpc.io/docs/languages/cpp/quickstart/) 
and [protobuf](https://protobuf.dev) 
installed (building gRPC should also install protobuf). This library uses 
the C++-20 standard, so you must use a compliant compiler. Finally, CMake 
(version 3.23 or higher) is used to configure and build the code.

### Build Process

To configure the build process, create a build directory and run CMake:

    cmake <path to the source directory> <additional options>

You do not need to provide additional options, however in some cases they 
can be used to customize the build (for example set the build type to 
Release or Debug with -DCMAKE_BUILD_TYPE=Release). Next, the build is 
started (here, using make, as it is the default build method for Unix[-like] 
systems):

    make -j 8

In this instance the build will compile with 8 workers, due to the -j flag.


## Platform Support

Philote-Cpp is tested against the following platforms and compilers:

- **Ubuntu 24.04** with:
  - gcc-12/g++-12
  - gcc-13/g++-13
  - gcc-14/g++-14
  - clang-16/clang++-16
  - clang-17/clang++-17
  - clang-18/clang++-18
- **Build types**: Release and Debug

While the library is only tested on Ubuntu 24.04 in CI, it is likely that any
system that can compile and run gRPC will be able to run Philote-Cpp, as the
main dependency is gRPC and Philote-Cpp adheres to the same C++ standards as
gRPC. However, only the platforms and compilers listed above are officially
supported and tested.

## License

This library is open source and licensed under the Apache 2 license:

    Copyright 2022-2025 Christopher A. Lupp
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

This work has been cleared for public release, distribution unlimited, case
number: AFRL-2023-5716.

The views expressed are those of the authors and do not reflect the official
guidance or position of the United States Government, the Department of Defense
or of the United States Air Force.

Statement from DoD: The Appearance of external hyperlinks does not constitute
endorsement by the United States Department of Defense (DoD) of the linked
websites, of the information, products, or services contained therein. The DoD
does not exercise any editorial, security, or other control over the
information you may find at these locations.


## Citation
If you use this code for academic work (e.g., for a paper), please cite the following paper:

Lupp, C.A., Xu, A. "Creating a Universal Communication 
Standard to Enable Heterogeneous Multidisciplinary Design Optimization." 
AIAA SCITECH 2024 Forum. American Institute of Aeronautics and Astronautics. 
Orlando, FL.


