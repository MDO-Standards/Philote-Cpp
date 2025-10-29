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

# Test helpers library
add_library(PhiloteTestHelpers test_helpers.cpp)
target_link_libraries(PhiloteTestHelpers PUBLIC PhiloteCpp GTest::gtest GTest::gmock)
target_include_directories(PhiloteTestHelpers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
enable_coverage(PhiloteTestHelpers)

# array tests
add_executable(VariableTests variable_tests.cpp)
target_link_libraries(VariableTests PhiloteCpp GTest::gtest_main GTest::gmock)
enable_coverage(VariableTests)
gtest_discover_tests(VariableTests)

# pair dict tests
add_executable(PairDictTests pair_dict_tests.cpp)
target_link_libraries(PairDictTests PhiloteCpp GTest::gtest_main GTest::gmock)
enable_coverage(PairDictTests)
gtest_discover_tests(PairDictTests)

# discipline tests
add_executable(DisciplineTests discipline_test.cpp)
target_link_libraries(DisciplineTests PhiloteCpp GTest::gtest_main GTest::gmock)
enable_coverage(DisciplineTests)
gtest_discover_tests(DisciplineTests)

# discipline server tests
add_executable(DisciplineServerTests discipline_server_test.cpp)
target_link_libraries(DisciplineServerTests PhiloteCpp GTest::gtest_main GTest::gmock)
enable_coverage(DisciplineServerTests)
gtest_discover_tests(DisciplineServerTests)

# discipline client tests
add_executable(DisciplineClientTests discipline_client_test.cpp)
target_link_libraries(DisciplineClientTests PhiloteCpp GTest::gtest_main GTest::gmock)
enable_coverage(DisciplineClientTests)
target_include_directories(DisciplineClientTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/discipline
)
gtest_discover_tests(DisciplineClientTests)

# explicit discipline tests
add_executable(ExplicitDisciplineTests explicit_discipline_test.cpp)
target_link_libraries(ExplicitDisciplineTests
    PhiloteCpp
    GTest::gtest_main
    GTest::gmock
)
enable_coverage(ExplicitDisciplineTests)
target_include_directories(ExplicitDisciplineTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/explicit
)
gtest_discover_tests(ExplicitDisciplineTests)

# explicit discipline server tests
add_executable(ExplicitDisciplineServerTests explicit_discipline_server_test.cpp)
target_link_libraries(ExplicitDisciplineServerTests
    PhiloteCpp
    GTest::gtest_main
    GTest::gmock
)
enable_coverage(ExplicitDisciplineServerTests)
target_include_directories(ExplicitDisciplineServerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/explicit
)
gtest_discover_tests(ExplicitDisciplineServerTests)

# explicit client tests
add_executable(ExplicitClientTests explicit_client_test.cpp)
target_link_libraries(ExplicitClientTests
    PhiloteCpp
    GTest::gtest_main
    GTest::gmock
)
enable_coverage(ExplicitClientTests)
target_include_directories(ExplicitClientTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/explicit
)
gtest_discover_tests(ExplicitClientTests)

# explicit integration tests
add_executable(ExplicitIntegrationTests explicit_integration_test.cpp)
target_link_libraries(ExplicitIntegrationTests
    PhiloteCpp
    PhiloteTestHelpers
    GTest::gtest_main
    GTest::gmock
)
enable_coverage(ExplicitIntegrationTests)
target_include_directories(ExplicitIntegrationTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/explicit
)
gtest_discover_tests(ExplicitIntegrationTests)

# explicit error scenarios tests
add_executable(ExplicitErrorScenariosTests explicit_error_scenarios_test.cpp)
target_link_libraries(ExplicitErrorScenariosTests
    PhiloteCpp
    PhiloteTestHelpers
    GTest::gtest_main
    GTest::gmock
)
enable_coverage(ExplicitErrorScenariosTests)
target_include_directories(ExplicitErrorScenariosTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/explicit
)
gtest_discover_tests(ExplicitErrorScenariosTests)

# Set test discovery timeout
set(GTEST_DISCOVER_TESTS_DISCOVERY_TIMEOUT 60)
