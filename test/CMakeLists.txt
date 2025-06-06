#===============================================================================
#    Philote C++ Bindings
#
#    Copyright 2022-2024 Christopher A. Lupp
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

# array tests
# add_executable(VariableTests variable_tests.cpp)
# target_link_libraries(VariableTests PhiloteCpp)
# target_link_libraries(VariableTests GTest::gtest_main)
# target_link_libraries(VariableTests protobuf::libprotobuf gRPC::grpc++)
# gtest_discover_tests(VariableTests)

# discipline tests
add_executable(DisciplineTests discipline_test.cpp)
target_link_libraries(DisciplineTests PhiloteCpp)
target_link_libraries(DisciplineTests GTest::gtest_main)
target_link_libraries(DisciplineTests protobuf::libprotobuf gRPC::grpc++)
gtest_discover_tests(DisciplineTests)

# # explicit server
# add_executable(DiscreteArrayTests discrete_array_tests.cpp)
# target_link_libraries(DiscreteArrayTests PhiloteCpp)
# target_link_libraries(DiscreteArrayTests GTest::gtest_main)
# gtest_discover_tests(DiscreteArrayTests)

# explicit client tests
add_executable(ExplicitClientTests explicit_client_tests.cpp)
target_link_libraries(ExplicitClientTests PhiloteCpp GTest::gtest_main GTest::gmock protobuf::libprotobuf gRPC::grpc++)
gtest_discover_tests(ExplicitClientTests)

# implicit server tests
add_executable(ImplicitServerTests implicit_server_tests.cpp)
target_link_libraries(ImplicitServerTests PhiloteCpp GTest::gtest_main GTest::gmock protobuf::libprotobuf gRPC::grpc++)
gtest_discover_tests(ImplicitServerTests)

# implicit client tests
add_executable(ImplicitClientTests implicit_client_tests.cpp)
target_link_libraries(ImplicitClientTests PhiloteCpp GTest::gtest_main GTest::gmock protobuf::libprotobuf gRPC::grpc++)
gtest_discover_tests(ImplicitClientTests)

# example disciplines
add_executable(ExampleDisciplineTests example_tests.cpp)
target_link_libraries(ExampleDisciplineTests PhiloteCpp GTest::gtest_main GTest::gmock protobuf::libprotobuf gRPC::grpc++)
gtest_discover_tests(ExampleDisciplineTests)