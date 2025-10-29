/*
    Philote C++ Bindings

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
*/

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>

#include "explicit.h"
#include "test_helpers.h"

using namespace philote;
using namespace philote::test;

// ============================================================================
// Integration Test Fixture
// ============================================================================

class ExplicitIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_manager_ = std::make_unique<TestServerManager>();
    }

    void TearDown() override {
        if (server_manager_ && server_manager_->IsRunning()) {
            server_manager_->StopServer();
        }
        server_manager_.reset();
    }

    std::unique_ptr<TestServerManager> server_manager_;
};

// ============================================================================
// Basic Integration Tests
// ============================================================================

TEST_F(ExplicitIntegrationTest, ParaboloidFunctionComputation) {
    // Create and setup discipline
    auto discipline = std::make_unique<ParaboloidDiscipline>();

    // Start server
    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    // Create client and connect
    ExplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    // Get discipline info
    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();
    client.GetPartialDefinitions();

    // Prepare inputs: x=3.0, y=4.0
    Variables inputs;
    inputs["x"] = CreateScalarVariable(3.0);
    inputs["y"] = CreateScalarVariable(4.0);

    // Compute function: f = x^2 + y^2 = 9 + 16 = 25
    Variables outputs = client.ComputeFunction(inputs);

    // Verify results
    ASSERT_EQ(outputs.size(), 1);
    ASSERT_TRUE(outputs.count("f") > 0);
    EXPECT_DOUBLE_EQ(outputs["f"](0), 25.0);
}

TEST_F(ExplicitIntegrationTest, ParaboloidGradientComputation) {
    auto discipline = std::make_unique<ParaboloidDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ExplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();
    client.GetPartialDefinitions();

    // Prepare inputs: x=3.0, y=4.0
    Variables inputs;
    inputs["x"] = CreateScalarVariable(3.0);
    inputs["y"] = CreateScalarVariable(4.0);

    // Compute gradients: df/dx = 2x = 6, df/dy = 2y = 8
    Partials partials = client.ComputeGradient(inputs);

    // Verify results
    ASSERT_EQ(partials.size(), 2);
    EXPECT_DOUBLE_EQ((partials[{"f", "x"}](0)), 6.0);
    EXPECT_DOUBLE_EQ((partials[{"f", "y"}](0)), 8.0);
}

TEST_F(ExplicitIntegrationTest, MultiOutputDiscipline) {
    auto discipline = std::make_unique<MultiOutputDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ExplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Prepare inputs: x=5.0, y=3.0
    Variables inputs;
    inputs["x"] = CreateScalarVariable(5.0);
    inputs["y"] = CreateScalarVariable(3.0);

    // Compute: sum=8, product=15, difference=2
    Variables outputs = client.ComputeFunction(inputs);

    ASSERT_EQ(outputs.size(), 3);
    EXPECT_DOUBLE_EQ(outputs["sum"](0), 8.0);
    EXPECT_DOUBLE_EQ(outputs["product"](0), 15.0);
    EXPECT_DOUBLE_EQ(outputs["difference"](0), 2.0);
}

TEST_F(ExplicitIntegrationTest, MultiOutputGradients) {
    auto discipline = std::make_unique<MultiOutputDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ExplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();
    client.GetPartialDefinitions();

    Variables inputs;
    inputs["x"] = CreateScalarVariable(5.0);
    inputs["y"] = CreateScalarVariable(3.0);

    Partials partials = client.ComputeGradient(inputs);

    // Verify all 6 partials (3 outputs x 2 inputs)
    ASSERT_EQ(partials.size(), 6);

    // d(sum)/dx = 1, d(sum)/dy = 1
    EXPECT_DOUBLE_EQ((partials[{"sum", "x"}](0)), 1.0);
    EXPECT_DOUBLE_EQ((partials[{"sum", "y"}](0)), 1.0);

    // d(product)/dx = y = 3, d(product)/dy = x = 5
    EXPECT_DOUBLE_EQ((partials[{"product", "x"}](0)), 3.0);
    EXPECT_DOUBLE_EQ((partials[{"product", "y"}](0)), 5.0);

    // d(difference)/dx = 1, d(difference)/dy = -1
    EXPECT_DOUBLE_EQ((partials[{"difference", "x"}](0)), 1.0);
    EXPECT_DOUBLE_EQ((partials[{"difference", "y"}](0)), -1.0);
}

TEST_F(ExplicitIntegrationTest, VectorizedDiscipline) {
    // Create 3x2 system: z = A*x + b where A is 3x2, x is 2-vector, b is 3-vector
    auto discipline = std::make_unique<VectorizedDiscipline>(3, 2);

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ExplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Setup inputs
    Variables inputs;
    inputs["A"] = CreateMatrixVariable(3, 2, 0.0);
    // Set A = [[1, 2], [3, 4], [5, 6]]
    inputs["A"](0) = 1.0; inputs["A"](1) = 2.0;
    inputs["A"](2) = 3.0; inputs["A"](3) = 4.0;
    inputs["A"](4) = 5.0; inputs["A"](5) = 6.0;

    inputs["x"] = CreateVectorVariable({1.0, 2.0});
    inputs["b"] = CreateVectorVariable({1.0, 1.0, 1.0});

    // Compute: z = A*x + b
    // z[0] = 1*1 + 2*2 + 1 = 6
    // z[1] = 3*1 + 4*2 + 1 = 12
    // z[2] = 5*1 + 6*2 + 1 = 18
    Variables outputs = client.ComputeFunction(inputs);

    ASSERT_EQ(outputs.size(), 1);
    ASSERT_TRUE(outputs.count("z") > 0);
    ASSERT_EQ(outputs["z"].Shape()[0], 3);
    EXPECT_DOUBLE_EQ(outputs["z"](0), 6.0);
    EXPECT_DOUBLE_EQ(outputs["z"](1), 12.0);
    EXPECT_DOUBLE_EQ(outputs["z"](2), 18.0);
}

// ============================================================================
// Multiple Sequential Calls
// ============================================================================

TEST_F(ExplicitIntegrationTest, MultipleSequentialFunctionCalls) {
    auto discipline = std::make_unique<ParaboloidDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ExplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Make multiple calls with different inputs
    for (int i = 1; i <= 5; ++i) {
        Variables inputs;
        inputs["x"] = CreateScalarVariable(static_cast<double>(i));
        inputs["y"] = CreateScalarVariable(static_cast<double>(i));

        Variables outputs = client.ComputeFunction(inputs);

        ASSERT_EQ(outputs.size(), 1);
        // f = x^2 + y^2 = i^2 + i^2 = 2*i^2
        EXPECT_DOUBLE_EQ(outputs["f"](0), 2.0 * i * i);
    }
}

TEST_F(ExplicitIntegrationTest, InterleavedFunctionAndGradientCalls) {
    auto discipline = std::make_unique<ParaboloidDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ExplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();
    client.GetPartialDefinitions();

    Variables inputs;
    inputs["x"] = CreateScalarVariable(2.0);
    inputs["y"] = CreateScalarVariable(3.0);

    // Call function
    Variables outputs1 = client.ComputeFunction(inputs);
    EXPECT_DOUBLE_EQ(outputs1["f"](0), 13.0);  // 4 + 9

    // Call gradient
    Partials partials1 = client.ComputeGradient(inputs);
    EXPECT_DOUBLE_EQ((partials1[{"f", "x"}](0)), 4.0);  // 2*2
    EXPECT_DOUBLE_EQ((partials1[{"f", "y"}](0)), 6.0);  // 2*3

    // Change inputs and call function again
    inputs["x"] = CreateScalarVariable(1.0);
    inputs["y"] = CreateScalarVariable(1.0);

    Variables outputs2 = client.ComputeFunction(inputs);
    EXPECT_DOUBLE_EQ(outputs2["f"](0), 2.0);  // 1 + 1

    // Call gradient again
    Partials partials2 = client.ComputeGradient(inputs);
    EXPECT_DOUBLE_EQ((partials2[{"f", "x"}](0)), 2.0);
    EXPECT_DOUBLE_EQ((partials2[{"f", "y"}](0)), 2.0);
}

// ============================================================================
// Concurrent Client Tests
// ============================================================================

// NOTE: MultipleConcurrentClients test removed - concurrent access not currently supported
// NOTE: ConcurrentFunctionAndGradientCalls test removed - concurrent RPC calls not currently supported

// ============================================================================
// Data Integrity Tests
// ============================================================================

TEST_F(ExplicitIntegrationTest, LargeVectorDataIntegrity) {
    // Test with larger vectors to ensure data integrity through full pipeline
    const size_t n = 100;
    const size_t m = 50;

    auto discipline = std::make_unique<VectorizedDiscipline>(n, m);

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ExplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Create large inputs
    Variables inputs;
    inputs["A"] = CreateMatrixVariable(n, m, 1.0);  // All ones
    inputs["x"] = CreateVectorVariable(std::vector<double>(m, 2.0));  // All 2s
    inputs["b"] = CreateVectorVariable(std::vector<double>(n, 3.0));  // All 3s

    // Compute: z = A*x + b
    // Each z[i] = sum(A[i,:] * x) + b[i] = m*1*2 + 3 = 2m + 3
    Variables outputs = client.ComputeFunction(inputs);

    ASSERT_EQ(outputs.size(), 1);
    ASSERT_EQ(outputs["z"].Shape()[0], n);

    double expected = 2.0 * m + 3.0;
    for (size_t i = 0; i < n; ++i) {
        EXPECT_DOUBLE_EQ(outputs["z"](i), expected) << "Mismatch at index " << i;
    }
}

TEST_F(ExplicitIntegrationTest, NegativeAndZeroValues) {
    auto discipline = std::make_unique<ParaboloidDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ExplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Test with negative values
    Variables inputs1;
    inputs1["x"] = CreateScalarVariable(-3.0);
    inputs1["y"] = CreateScalarVariable(-4.0);

    Variables outputs1 = client.ComputeFunction(inputs1);
    EXPECT_DOUBLE_EQ(outputs1["f"](0), 25.0);  // (-3)^2 + (-4)^2 = 25

    // Test with zero values
    Variables inputs2;
    inputs2["x"] = CreateScalarVariable(0.0);
    inputs2["y"] = CreateScalarVariable(0.0);

    Variables outputs2 = client.ComputeFunction(inputs2);
    EXPECT_DOUBLE_EQ(outputs2["f"](0), 0.0);

    // Test with mixed signs
    Variables inputs3;
    inputs3["x"] = CreateScalarVariable(-2.0);
    inputs3["y"] = CreateScalarVariable(2.0);

    Variables outputs3 = client.ComputeFunction(inputs3);
    EXPECT_DOUBLE_EQ(outputs3["f"](0), 8.0);  // 4 + 4 = 8
}
