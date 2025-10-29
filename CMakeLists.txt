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
set(
    ENABLE_COVERAGE
    FALSE
    CACHE BOOL "Enable code coverage analysis"
)

# add cmake scripts
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# set the C++ standard
set(CMAKE_CXX_STANDARD 17)

# Coverage configuration
if(ENABLE_COVERAGE)
    message("Enabling code coverage...")
    
    # Check if we have the required tools
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    
    if(NOT GCOV_PATH)
        message(FATAL_ERROR "gcov not found! Install gcov and try again.")
    endif()
    
    # Set coverage flags
    set(COVERAGE_COMPILER_FLAGS "-g -O0 --coverage -fprofile-arcs -ftest-coverage")
    set(COVERAGE_LINKER_FLAGS "--coverage")
    
    # Apply flags to all targets
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILER_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_COMPILER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_LINKER_FLAGS}")
    
    message("Coverage flags: ${COVERAGE_COMPILER_FLAGS}")
endif()

# Find gRPC and its dependencies
# Try CONFIG first, fall back to pkg-config for system packages
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
    # Use MODULE mode for system installations
    find_package(Protobuf REQUIRED)

    # Create compatibility targets if they don't exist
    if(NOT TARGET protobuf::libprotobuf AND TARGET protobuf::protobuf)
        add_library(protobuf::libprotobuf ALIAS protobuf::protobuf)
    elseif(NOT TARGET protobuf::libprotobuf)
        add_library(protobuf::libprotobuf INTERFACE IMPORTED)
        set_target_properties(protobuf::libprotobuf PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Protobuf_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${Protobuf_LIBRARIES}"
        )
    endif()

    # Ensure protoc executable target exists
    if(NOT TARGET protobuf::protoc)
        add_executable(protobuf::protoc IMPORTED)
        set_target_properties(protobuf::protoc PROPERTIES
            IMPORTED_LOCATION "${Protobuf_PROTOC_EXECUTABLE}"
        )
    endif()
endif()

find_package(gRPC CONFIG QUIET)
if(NOT gRPC_FOUND)
    # Use pkg-config as fallback for system installations
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GRPC REQUIRED grpc++)
    pkg_check_modules(GRPC_UNSECURE REQUIRED grpc)

    # Create imported targets for consistency
    if(NOT TARGET gRPC::grpc++)
        add_library(gRPC::grpc++ INTERFACE IMPORTED)
        set_target_properties(gRPC::grpc++ PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GRPC_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${GRPC_LINK_LIBRARIES}"
        )
    endif()

    # Find grpc_cpp_plugin
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
    if(NOT GRPC_CPP_PLUGIN)
        message(FATAL_ERROR "grpc_cpp_plugin not found")
    endif()
endif()

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

    # Verify that GTest::gmock target is available
    if(NOT TARGET GTest::gmock)
        message(FATAL_ERROR "GTest::gmock target not found. Please install googletest with gmock support.")
    endif()

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

# Coverage targets
if(ENABLE_COVERAGE AND BUILD_TESTS)
    
    # List of source directories to include in coverage
    set(COVERAGE_EXCLUDES
        '/usr/include/*'
        '*/test/*'
        '*/tests/*'
        '*/gtest/*'
        '*/gmock/*'
        '*/src/generated/*'
        '*/cmake-build-*/*'
        '*/build/*'
        '*/cmake/*'
        '*/examples/*'
    )
    
    # Coverage target that runs tests and generates coverage
    add_custom_target(coverage
        COMMENT "Generating code coverage report"
        
        # Clean previous coverage data
        COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
        
        # Run tests to generate coverage data
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        
        # Generate coverage info files for each source file
        COMMAND find ${CMAKE_BINARY_DIR}/src -name "*.gcno" -exec ${GCOV_PATH} -p -b {} + || true
        
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS PhiloteCpp
    )
    
    # If lcov is available, create HTML reports
    if(LCOV_PATH AND GENHTML_PATH)
        add_custom_target(coverage-html
            COMMENT "Generating HTML coverage report"
            
            # Clean previous coverage data
            COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
            COMMAND ${LCOV_PATH} --zerocounters --directory ${CMAKE_BINARY_DIR}
            
            # Run tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            
            # Capture coverage data
            COMMAND ${LCOV_PATH} --capture --directory ${CMAKE_BINARY_DIR} --output-file coverage.info
            
            # Remove unwanted files from coverage
            COMMAND ${LCOV_PATH} --remove coverage.info ${COVERAGE_EXCLUDES} --output-file coverage_filtered.info
            
            # Generate HTML report
            COMMAND ${GENHTML_PATH} coverage_filtered.info --output-directory coverage_html
            
            # Print summary
            COMMAND ${LCOV_PATH} --summary coverage_filtered.info
            
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS PhiloteCpp
        )
        
        message("Coverage targets added: 'coverage' and 'coverage-html'")
        message("  Run: make coverage        # for basic gcov reports")
        message("  Run: make coverage-html   # for HTML reports")
    else()
        add_custom_target(coverage-html
            COMMAND ${CMAKE_COMMAND} -E echo "lcov and genhtml are required for HTML coverage reports"
            COMMAND ${CMAKE_COMMAND} -E echo "Install lcov: apt-get install lcov (Ubuntu) or brew install lcov (macOS)"
        )
        message("Coverage target added: 'coverage' (lcov not found, HTML reports unavailable)")
    endif()
    
    # Codecov-friendly target that generates coverage.xml
    add_custom_target(coverage-xml
        COMMENT "Generating XML coverage report for Codecov"
        
        # Clean previous coverage data
        COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
        
        # Run tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        
        # Generate individual .gcov files for each source file
        COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -exec ${GCOV_PATH} -p -b -o {} + || true
        
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS PhiloteCpp
    )
    
    message("Codecov target added: 'coverage-xml'")
    message("  CI Usage: cmake -DENABLE_COVERAGE=ON .. && make coverage-xml")
    
endif()