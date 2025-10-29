![Philote](https://github.com/chrislupp/Philote-MDO/blob/main/doc/graphics/logos/philote.svg?raw=true)

[![Build and Regression Tests](https://github.com/chrislupp/Philote-Cpp/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/chrislupp/Philote-Cpp/actions/workflows/build.yml)

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

    - Ubuntu 22.04
        - gRPC 1.51.1 using:
            - gcc-9/g++-9
            - gcc-10/g++-10
            - gcc-11/g++-11
            - gcc-12/g++-12
            - clang-13/clang++-13
            - clang-14/clang++-14

While early tests were written to run on all three major operating system
(Linux, Windows, and macOS), the GitHub billing for the non-Linux OS's is so
high (macOS is billed at a factor of ten higher than Linux) that it is not
feasible to run anything other than Linux. However, as the main dependency for
Philote-Cpp is gRPC, it is likely that any system that can compile and run gRPC
will be able to run Philote-Cpp. This is especially true, as Philote-Cpp adheres
to the same C++ standards as gRPC.

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
If you use this code for academic work, please cite the following paper:

Lupp, C.A., Xu, A., Carrere, A.L. "Creating a Universal Communication 
Standard to Enable Heterogeneous Multidisciplinary Design Optimization." 
AIAA SCITECH 2024 Forum. American Institute of Aeronautics and Astronautics. 
Orlando, FL, (forthcoming).


