# Installation and Integration {#installation}

[TOC]

## Overview

This page describes how to install Philote-Cpp and integrate it into your own projects.
After installation, you can use CMake's `find_package()` to easily link against the library.

## Prerequisites

Before installing Philote-Cpp, ensure you have:

- **C++20 compliant compiler** (gcc-12+, clang-16+, or equivalent)
- **CMake 3.23 or higher**
- **gRPC** and **Protocol Buffers** installed on your system

### Installing Dependencies

#### Ubuntu/Debian

```bash
# Install build essentials
sudo apt-get update
sudo apt-get install build-essential cmake

# Install gRPC and protobuf
sudo apt-get install libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc
```

#### macOS

```bash
# Using Homebrew
brew install cmake grpc protobuf
```

#### From Source

If you need to build gRPC from source, follow the [official gRPC installation guide](https://grpc.io/docs/languages/cpp/quickstart/).

## Building Philote-Cpp

### Clone the Repository

```bash
git clone https://github.com/MDO-Standards/Philote-Cpp.git
cd Philote-Cpp
```

### Configure and Build

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF

# Build the library
make -j 8
```

### Build Options

Philote-Cpp supports several CMake build options:

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | `ON` | Build unit tests |
| `BUILD_EXAMPLES` | `OFF` | Build example programs |
| `ENABLE_COVERAGE` | `OFF` | Enable code coverage analysis |
| `CMAKE_BUILD_TYPE` | `Release` | Build configuration (Debug, Release, etc.) |

Example with custom options:

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DBUILD_EXAMPLES=ON
```

## Installing Philote-Cpp

### System-Wide Installation

Install the library to system directories (requires root/admin privileges):

```bash
cd build
sudo make install
```

By default, this installs:
- **Library**: `/usr/local/lib/libPhiloteCpp.a`
- **Headers**: `/usr/local/include/philote/`
- **CMake config**: `/usr/local/lib/cmake/PhiloteCpp/`

### Custom Installation Prefix

Install to a custom location (no root required):

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install/location
make
make install
```

This installs to:
- **Library**: `<prefix>/lib/libPhiloteCpp.a`
- **Headers**: `<prefix>/include/`
- **CMake config**: `<prefix>/lib/cmake/PhiloteCpp/`

### User-Local Installation

Install to your home directory:

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
make install
```

## Integrating Philote-Cpp in Your Project

### Using CMake (Recommended)

Philote-Cpp provides a CMake configuration file for easy integration.

#### Project CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.23)
project(MyProject LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Philote-Cpp
find_package(PhiloteCpp REQUIRED)

# Create your executable or library
add_executable(my_discipline
    src/my_discipline.cpp
    src/main.cpp
)

# Link against Philote-Cpp
target_link_libraries(my_discipline
    PRIVATE
        PhiloteCpp::PhiloteCpp
)
```

#### Building Your Project

```bash
mkdir build
cd build
cmake ..
make
```

If Philote-Cpp is installed in a non-standard location:

```bash
cmake .. -DCMAKE_PREFIX_PATH=/path/to/philote/install
```

### Using pkg-config

If you prefer pkg-config over CMake, you can manually specify the paths:

```bash
# Compile
g++ -std=c++20 \
    -I/usr/local/include \
    -c my_discipline.cpp -o my_discipline.o

# Link
g++ my_discipline.o \
    -L/usr/local/lib \
    -lPhiloteCpp \
    $(pkg-config --libs protobuf grpc++) \
    -o my_discipline
```

### Including Headers

In your C++ source files:

```cpp
// For explicit disciplines
#include <explicit.h>

// For implicit disciplines
#include <implicit.h>

// For variables and data structures
#include <variable.h>

// For discipline base classes
#include <discipline.h>

// For clients
#include <discipline_client.h>  // Base client
// explicit.h and implicit.h also include their respective clients
```

## Example Project Structure

Here's a complete example of a project using Philote-Cpp:

### Directory Structure

```
my_discipline_project/
├── CMakeLists.txt
├── src/
│   ├── my_discipline.h
│   ├── my_discipline.cpp
│   └── main.cpp
└── build/
```

### my_discipline.h

```cpp
#pragma once
#include <explicit.h>

class MyDiscipline : public philote::ExplicitDiscipline {
private:
    void Setup() override;
    void Compute(const philote::Variables &inputs,
                 philote::Variables &outputs) override;
};
```

### my_discipline.cpp

```cpp
#include "my_discipline.h"

void MyDiscipline::Setup() {
    AddInput("x", {1}, "m");
    AddOutput("y", {1}, "m");
}

void MyDiscipline::Compute(const philote::Variables &inputs,
                          philote::Variables &outputs) {
    outputs.at("y")(0) = 2.0 * inputs.at("x")(0);
}
```

### main.cpp

```cpp
#include <grpcpp/grpcpp.h>
#include "my_discipline.h"

int main() {
    MyDiscipline discipline;

    grpc::ServerBuilder builder;
    builder.AddListeningPort("localhost:50051",
                            grpc::InsecureServerCredentials());
    discipline.RegisterServices(builder);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    std::cout << "Server listening on localhost:50051" << std::endl;
    server->Wait();

    return 0;
}
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.23)
project(MyDisciplineProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(PhiloteCpp REQUIRED)

# Create executable
add_executable(my_discipline_server
    src/my_discipline.cpp
    src/main.cpp
)

# Link libraries
target_link_libraries(my_discipline_server
    PRIVATE
        PhiloteCpp::PhiloteCpp
)

# Include directories
target_include_directories(my_discipline_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
```

### Building

```bash
mkdir build
cd build
cmake ..
make
./my_discipline_server
```

## Troubleshooting

### CMake Cannot Find PhiloteCpp

**Error**: `Could not find a package configuration file provided by "PhiloteCpp"`

**Solutions**:

1. Specify the installation prefix:
   ```bash
   cmake .. -DCMAKE_PREFIX_PATH=/path/to/philote/install
   ```

2. Set environment variable:
   ```bash
   export CMAKE_PREFIX_PATH=/path/to/philote/install:$CMAKE_PREFIX_PATH
   ```

3. Verify installation:
   ```bash
   ls /path/to/install/lib/cmake/PhiloteCpp/
   # Should see PhiloteCppConfig.cmake
   ```

### Missing gRPC or Protobuf

**Error**: `Could not find gRPC` or `Could not find Protobuf`

**Solution**: Ensure gRPC and Protobuf are installed and findable by CMake:

```bash
# Check if installed
pkg-config --modversion grpc++
pkg-config --modversion protobuf

# If not found, install dependencies (see Prerequisites)
```

### Linker Errors

**Error**: `undefined reference to philote::...`

**Solutions**:

1. Ensure you're linking against PhiloteCpp:
   ```cmake
   target_link_libraries(your_target PRIVATE PhiloteCpp::PhiloteCpp)
   ```

2. Verify library exists:
   ```bash
   ls /path/to/install/lib/libPhiloteCpp.a
   ```

3. Check that C++ standard is set correctly:
   ```cmake
   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)
   ```

### Header Not Found

**Error**: `fatal error: explicit.h: No such file or directory`

**Solutions**:

1. Use angle brackets for system headers:
   ```cpp
   #include <explicit.h>  // Correct
   // Not: #include "explicit.h"
   ```

2. Verify headers are installed:
   ```bash
   ls /path/to/install/include/
   # Should see: discipline.h, explicit.h, implicit.h, variable.h, etc.
   ```

3. Ensure CMake finds the package:
   ```cmake
   find_package(PhiloteCpp REQUIRED)
   ```

### Version Mismatch

If you encounter issues due to version incompatibility:

```cmake
# Require specific version
find_package(PhiloteCpp 0.4.0 REQUIRED)

# Or minimum version
find_package(PhiloteCpp 0.4 REQUIRED)
```

Check installed version:

```bash
grep "VERSION" /path/to/install/lib/cmake/PhiloteCpp/PhiloteCppConfig.cmake
```

## Using Philote-Cpp Without Installation

For development or testing, you can use Philote-Cpp without installation:

### Option 1: Build Tree Export

Philote-Cpp exports targets from the build tree:

```cmake
# In your project's CMakeLists.txt
# Point to Philote-Cpp build directory
set(PhiloteCpp_DIR /path/to/Philote-Cpp/build)
find_package(PhiloteCpp REQUIRED)

target_link_libraries(your_target PRIVATE PhiloteCpp::PhiloteCpp)
```

### Option 2: Add as Subdirectory

Add Philote-Cpp as a subdirectory in your project:

```cmake
# Clone or copy Philote-Cpp into your project
add_subdirectory(external/Philote-Cpp)

# Link against it
target_link_libraries(your_target PRIVATE PhiloteCpp)
```

### Option 3: Manual Paths

Manually specify include and library paths:

```cmake
target_include_directories(your_target
    PRIVATE
        /path/to/Philote-Cpp/include
        /path/to/Philote-Cpp/build/src/generated
)

target_link_libraries(your_target
    PRIVATE
        /path/to/Philote-Cpp/build/src/libPhiloteCpp.a
        protobuf::libprotobuf
        gRPC::grpc++
)
```

## Verifying Installation

Create a simple test program to verify installation:

```cpp
// test_install.cpp
#include <explicit.h>
#include <iostream>

int main() {
    std::cout << "Philote-Cpp successfully linked!" << std::endl;
    return 0;
}
```

Compile and run:

```bash
# Using CMake (recommended)
echo "cmake_minimum_required(VERSION 3.23)
project(Test)
set(CMAKE_CXX_STANDARD 20)
find_package(PhiloteCpp REQUIRED)
add_executable(test_install test_install.cpp)
target_link_libraries(test_install PRIVATE PhiloteCpp::PhiloteCpp)" > CMakeLists.txt

mkdir build && cd build
cmake ..
make
./test_install
```

Expected output:
```
Philote-Cpp successfully linked!
```

## Environment Variables

Useful environment variables for Philote-Cpp:

| Variable | Purpose | Example |
|----------|---------|---------|
| `CMAKE_PREFIX_PATH` | Help CMake find Philote-Cpp | `/opt/philote` |
| `LD_LIBRARY_PATH` | Runtime library search path (if shared libs) | `/opt/philote/lib` |
| `GRPC_VERBOSITY` | gRPC logging level | `DEBUG`, `INFO`, `ERROR` |
| `GRPC_TRACE` | gRPC trace options | `all`, `client_channel` |

Example:

```bash
export CMAKE_PREFIX_PATH=/opt/philote:$CMAKE_PREFIX_PATH
export GRPC_VERBOSITY=INFO
```

## Next Steps

After installation:

- @ref getting_started - Learn the basics
- @ref explicit_disciplines - Create your first discipline
- @ref client_usage - Connect to remote disciplines

## See Also

- [CMake Documentation](https://cmake.org/documentation/)
- [gRPC C++ Quick Start](https://grpc.io/docs/languages/cpp/quickstart/)
- @ref best_practices - Best practices for using Philote-Cpp
