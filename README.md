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

```bash
mkdir build && cd build
cmake .. [options]
```

#### CMake Build Options

- `BUILD_TESTS` (default: ON) - Build unit tests
- `BUILD_EXAMPLES` (default: OFF) - Build example disciplines
- `ENABLE_COVERAGE` (default: OFF) - Enable code coverage analysis (development/CI only)
- `CMAKE_BUILD_TYPE` - Build type: Debug, Release, RelWithDebInfo, or MinSizeRel
- `CMAKE_INSTALL_PREFIX` - Installation directory (default: /usr/local)

Example configurations:

```bash
# Standard release build without tests
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF

# Development build with tests and examples
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON

# Release build with custom install location
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/philote
```

After configuration, build the library:

```bash
cmake --build . -j 8
```

Or using make directly:

```bash
make -j 8
```

### Installing Philote-Cpp

After building, install the library to make it available system-wide or in a custom location:

```bash
# Install to the configured prefix (default: /usr/local)
cmake --build . --target install

# Or using make
make install

# Install to a specific directory (overrides CMAKE_INSTALL_PREFIX)
cmake --install . --prefix /custom/path
```

The installation will place:
- Headers in `${CMAKE_INSTALL_PREFIX}/include/`
- Library in `${CMAKE_INSTALL_PREFIX}/lib/` or `lib64/`
- CMake configuration files in `${CMAKE_INSTALL_PREFIX}/lib/cmake/PhiloteCpp/`

### Running Tests

If built with `BUILD_TESTS=ON`:

```bash
# Run all tests
ctest --output-on-failure

# Run specific test suite
ctest -R Explicit

# Run individual test executable
./test/ExplicitDisciplineServerTests
```


## Using Philote-Cpp in Your CMake Project

After installing Philote-Cpp, you can use it in your own CMake projects.

### Method 1: Using Installed Package (Recommended)

If Philote-Cpp is installed system-wide or in a standard location:

```cmake
cmake_minimum_required(VERSION 3.23)
project(MyDiscipline)

# Find the installed PhiloteCpp package
find_package(PhiloteCpp REQUIRED)

# Create your executable
add_executable(my_discipline my_discipline.cpp)

# Link against PhiloteCpp
target_link_libraries(my_discipline PRIVATE PhiloteCpp::PhiloteCpp)
```

If Philote-Cpp is installed in a custom location, specify the path when configuring:

```bash
cmake .. -DPhiloteCpp_DIR=/custom/path/lib/cmake/PhiloteCpp
```

### Method 2: Using Build Tree Directly

You can use Philote-Cpp directly from its build directory without installation:

```cmake
cmake_minimum_required(VERSION 3.23)
project(MyDiscipline)

# Point to the PhiloteCpp build directory
find_package(PhiloteCpp REQUIRED
    PATHS /path/to/Philote-Cpp/build
    NO_DEFAULT_PATH
)

add_executable(my_discipline my_discipline.cpp)
target_link_libraries(my_discipline PRIVATE PhiloteCpp::PhiloteCpp)
```

### Method 3: Using add_subdirectory

If you have Philote-Cpp source code as a subdirectory of your project:

```cmake
cmake_minimum_required(VERSION 3.23)
project(MyDiscipline)

# Add PhiloteCpp as a subdirectory
# This will build PhiloteCpp as part of your project
set(BUILD_TESTS OFF CACHE BOOL "Disable PhiloteCpp tests" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable PhiloteCpp examples" FORCE)
add_subdirectory(external/Philote-Cpp)

add_executable(my_discipline my_discipline.cpp)
target_link_libraries(my_discipline PRIVATE PhiloteCpp)
```

See the `examples/` directory for complete discipline implementations.


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


