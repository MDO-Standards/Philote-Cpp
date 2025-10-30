
# array tests
add_executable(VariableTests variable_tests.cpp)
target_link_libraries(VariableTests PhiloteCpp)
target_link_libraries(VariableTests GTest::gtest_main)
target_link_libraries(VariableTests  protobuf::libprotobuf gRPC::grpc++)
gtest_discover_tests(VariableTests)



# # explicit server
# add_executable(DiscreteArrayTests discrete_array_tests.cpp)
# target_link_libraries(DiscreteArrayTests PhiloteCpp)
# target_link_libraries(DiscreteArrayTests GTest::gtest_main)
# gtest_discover_tests(DiscreteArrayTests)

# # explicit client
# add_executable(DiscreteArrayTests discrete_array_tests.cpp)
# target_link_libraries(DiscreteArrayTests PhiloteCpp)
# target_link_libraries(DiscreteArrayTests GTest::gtest_main)
# gtest_discover_tests(DiscreteArrayTests)

# # implicit server
# add_executable(DiscreteArrayTests discrete_array_tests.cpp)
# target_link_libraries(DiscreteArrayTests PhiloteCpp)
# target_link_libraries(DiscreteArrayTests GTest::gtest_main)
# gtest_discover_tests(DiscreteArrayTests)

# # implicit client
# add_executable(DiscreteArrayTests discrete_array_tests.cpp)
# target_link_libraries(DiscreteArrayTests PhiloteCpp)
# target_link_libraries(DiscreteArrayTests GTest::gtest_main)
# gtest_discover_tests(DiscreteArrayTests)