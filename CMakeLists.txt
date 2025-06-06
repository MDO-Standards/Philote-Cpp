#===============================================================================
#    Philote C++ Bindings
#
#    Copyright 2022-2025 Christopher A. Lupp
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
#    This work has been cleared for public release, distribution unlimited, case
#    number: AFRL-2023-5716.
#
#    The views expressed are those of the authors and do not reflect the
#    official guidance or position of the United States Government, the
#    Department of Defense or of the United States Air Force.
#
#    Statement from DoD: The Appearance of external hyperlinks does not
#    constitute endorsement by the United States Department of Defense (DoD) of
#    the linked websites, of the information, products, or services contained
#    therein. The DoD does not exercise any editorial, security, or other
#    control over the information you may find at these locations.
#===============================================================================

# cmake 3.23 is required due to the use of file sets
cmake_minimum_required (VERSION 3.23)


project(PhiloteCpp
    VERSION 0.4.0
)

# cache option variables
set(
    BUILD_TESTS
    TRUE
    CACHE BOOL "Build unit tests"
    )
set(
    BUILD_EXAMPLES
    FALSE
    CACHE BOOL "Build examples"
)

# add cmake scripts
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# set the C++ standard
set(CMAKE_CXX_STANDARD 17)



# Find gRPC and its dependencies
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# add include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src/generated)


# add subdirectories
add_subdirectory(src)
add_subdirectory(include)

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()



if (BUILD_TESTS)
    message("Building with tests.")

    enable_testing()
    find_package(GTest REQUIRED)
    include(GoogleTest)
    add_subdirectory(test)
endif (BUILD_TESTS)


# installation (and package definitions)
install(TARGETS
    PhiloteCpp
    EXPORT PhiloteCppConfig
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET public_headers
)
export(TARGETS
    PhiloteCpp
    NAMESPACE PhiloteCpp::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/PhiloteCppConfig.cmake"
)
install(EXPORT
    PhiloteCppConfig
    DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake"
    NAMESPACE PhiloteCpp::
)