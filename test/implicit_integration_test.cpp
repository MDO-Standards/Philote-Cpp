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

#include "implicit.h"
#include "test_helpers.h"

using namespace philote;
using namespace philote::test;

// ============================================================================
// Integration Test Fixture
// ============================================================================

class ImplicitIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_manager_ = std::make_unique<ImplicitTestServerManager>();
    }

    void TearDown() override {
        if (server_manager_ && server_manager_->IsRunning()) {
            server_manager_->StopServer();
        }
        server_manager_.reset();
    }

    std::unique_ptr<ImplicitTestServerManager> server_manager_;
};

// ============================================================================
// Basic Integration Tests
// ============================================================================

TEST_F(ImplicitIntegrationTest, SimpleImplicitResidualComputation) {
    // Create and setup discipline: R = x^2 - y
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    // Start server
    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    // Create client and connect
    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    // Get discipline info
    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();
    client.GetPartialDefinitions();

    // Prepare inputs and outputs: x=3.0, y=9.0
    Variables vars;
    vars["x"] = CreateScalarVariable(3.0);
    vars["y"] = CreateScalarVariable(9.0);

    // Compute residuals: R = 3^2 - 9 = 0
    Variables residuals = client.ComputeResiduals(vars);

    // Verify results
    ASSERT_EQ(residuals.size(), 1);
    ASSERT_TRUE(residuals.count("y") > 0);
    EXPECT_NEAR(residuals["y"](0), 0.0, 1e-10);
}

TEST_F(ImplicitIntegrationTest, SimpleImplicitSolveResiduals) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Prepare inputs: x=4.0
    Variables inputs;
    inputs["x"] = CreateScalarVariable(4.0);

    // Solve residuals: y = x^2 = 16
    Variables outputs = client.SolveResiduals(inputs);

    // Verify results
    ASSERT_EQ(outputs.size(), 1);
    ASSERT_TRUE(outputs.count("y") > 0);
    EXPECT_DOUBLE_EQ(outputs["y"](0), 16.0);
}

TEST_F(ImplicitIntegrationTest, SimpleImplicitGradientComputation) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();
    client.GetPartialDefinitions();

    // Prepare inputs and outputs: x=5.0, y=25.0
    Variables vars;
    vars["x"] = CreateScalarVariable(5.0);
    vars["y"] = CreateScalarVariable(25.0);

    // Compute gradients: dR/dx = 2x = 10, dR/dy = -1
    Partials partials = client.ComputeResidualGradients(vars);

    // Verify results
    ASSERT_EQ(partials.size(), 2);
    EXPECT_DOUBLE_EQ((partials[{"y", "x"}](0)), 10.0);
    EXPECT_DOUBLE_EQ((partials[{"y", "y"}](0)), -1.0);
}

TEST_F(ImplicitIntegrationTest, QuadraticDiscipline) {
    auto discipline = std::make_unique<QuadraticDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Solve: x^2 - 5x + 6 = 0 (solutions: x = 2 or 3, returns positive root = 3)
    Variables inputs;
    inputs["a"] = CreateScalarVariable(1.0);
    inputs["b"] = CreateScalarVariable(-5.0);
    inputs["c"] = CreateScalarVariable(6.0);

    Variables outputs = client.SolveResiduals(inputs);

    ASSERT_EQ(outputs.size(), 1);
    EXPECT_DOUBLE_EQ(outputs["x"](0), 3.0);
}

TEST_F(ImplicitIntegrationTest, QuadraticGradients) {
    auto discipline = std::make_unique<QuadraticDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();
    client.GetPartialDefinitions();

    Variables vars;
    vars["a"] = CreateScalarVariable(1.0);
    vars["b"] = CreateScalarVariable(-5.0);
    vars["c"] = CreateScalarVariable(6.0);
    vars["x"] = CreateScalarVariable(3.0);

    Partials partials = client.ComputeResidualGradients(vars);

    // Verify all 4 partials
    ASSERT_EQ(partials.size(), 4);

    // dR/da = x^2 = 9
    EXPECT_DOUBLE_EQ((partials[{"x", "a"}](0)), 9.0);
    // dR/db = x = 3
    EXPECT_DOUBLE_EQ((partials[{"x", "b"}](0)), 3.0);
    // dR/dc = 1
    EXPECT_DOUBLE_EQ((partials[{"x", "c"}](0)), 1.0);
    // dR/dx = 2ax + b = 2*1*3 + (-5) = 1
    EXPECT_DOUBLE_EQ((partials[{"x", "x"}](0)), 1.0);
}

TEST_F(ImplicitIntegrationTest, MultiResidualDiscipline) {
    auto discipline = std::make_unique<MultiResidualDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Solve: x + y = 8, x*y = 15 (solutions: x=5, y=3 or x=3, y=5, returns x=5, y=3)
    Variables inputs;
    inputs["sum"] = CreateScalarVariable(8.0);
    inputs["product"] = CreateScalarVariable(15.0);

    Variables outputs = client.SolveResiduals(inputs);

    ASSERT_EQ(outputs.size(), 2);
    EXPECT_DOUBLE_EQ(outputs["x"](0), 5.0);
    EXPECT_DOUBLE_EQ(outputs["y"](0), 3.0);
}

TEST_F(ImplicitIntegrationTest, MultiResidualGradients) {
    auto discipline = std::make_unique<MultiResidualDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();
    client.GetPartialDefinitions();

    Variables vars;
    vars["sum"] = CreateScalarVariable(8.0);
    vars["product"] = CreateScalarVariable(15.0);
    vars["x"] = CreateScalarVariable(5.0);
    vars["y"] = CreateScalarVariable(3.0);

    Partials partials = client.ComputeResidualGradients(vars);

    // Verify all 8 partials (2 residuals x 4 variables)
    ASSERT_EQ(partials.size(), 8);

    // R1 = x + y - sum, so dR1/dsum = -1, dR1/dproduct = 0, dR1/dx = 1, dR1/dy = 1
    EXPECT_DOUBLE_EQ((partials[{"x", "sum"}](0)), -1.0);
    EXPECT_DOUBLE_EQ((partials[{"x", "product"}](0)), 0.0);
    EXPECT_DOUBLE_EQ((partials[{"x", "x"}](0)), 1.0);
    EXPECT_DOUBLE_EQ((partials[{"x", "y"}](0)), 1.0);

    // R2 = x*y - product, so dR2/dsum = 0, dR2/dproduct = -1, dR2/dx = y, dR2/dy = x
    EXPECT_DOUBLE_EQ((partials[{"y", "sum"}](0)), 0.0);
    EXPECT_DOUBLE_EQ((partials[{"y", "product"}](0)), -1.0);
    EXPECT_DOUBLE_EQ((partials[{"y", "x"}](0)), 3.0);
    EXPECT_DOUBLE_EQ((partials[{"y", "y"}](0)), 5.0);
}

TEST_F(ImplicitIntegrationTest, VectorizedImplicitDiscipline) {
    // Create 3x2 system: R = A*x + b - y where A is 3x2, x is 2-vector, b is 3-vector, y is 3-vector
    auto discipline = std::make_unique<VectorizedImplicitDiscipline>(3, 2);

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
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

    // Solve: y = A*x + b
    // y[0] = 1*1 + 2*2 + 1 = 6
    // y[1] = 3*1 + 4*2 + 1 = 12
    // y[2] = 5*1 + 6*2 + 1 = 18
    Variables outputs = client.SolveResiduals(inputs);

    ASSERT_EQ(outputs.size(), 1);
    ASSERT_TRUE(outputs.count("y") > 0);
    ASSERT_EQ(outputs["y"].Shape()[0], 3);
    EXPECT_DOUBLE_EQ(outputs["y"](0), 6.0);
    EXPECT_DOUBLE_EQ(outputs["y"](1), 12.0);
    EXPECT_DOUBLE_EQ(outputs["y"](2), 18.0);
}

// ============================================================================
// Multiple Sequential Calls
// ============================================================================

TEST_F(ImplicitIntegrationTest, MultipleSequentialSolveCalls) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Make multiple calls with different inputs
    for (int i = 1; i <= 5; ++i) {
        Variables inputs;
        inputs["x"] = CreateScalarVariable(static_cast<double>(i));

        Variables outputs = client.SolveResiduals(inputs);

        ASSERT_EQ(outputs.size(), 1);
        // y = x^2 = i^2
        EXPECT_DOUBLE_EQ(outputs["y"](0), static_cast<double>(i * i));
    }
}

TEST_F(ImplicitIntegrationTest, InterleavedResidualSolveAndGradientCalls) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();
    client.GetPartialDefinitions();

    Variables inputs;
    inputs["x"] = CreateScalarVariable(2.0);

    // Call solve
    Variables outputs1 = client.SolveResiduals(inputs);
    EXPECT_DOUBLE_EQ(outputs1["y"](0), 4.0);  // 2^2

    // Call residual computation with correct output
    Variables vars1;
    vars1["x"] = CreateScalarVariable(2.0);
    vars1["y"] = CreateScalarVariable(4.0);
    Variables residuals1 = client.ComputeResiduals(vars1);
    EXPECT_NEAR(residuals1["y"](0), 0.0, 1e-10);

    // Call gradient
    Partials partials1 = client.ComputeResidualGradients(vars1);
    EXPECT_DOUBLE_EQ((partials1[{"y", "x"}](0)), 4.0);  // 2*2
    EXPECT_DOUBLE_EQ((partials1[{"y", "y"}](0)), -1.0);

    // Change inputs and call solve again
    inputs["x"] = CreateScalarVariable(3.0);

    Variables outputs2 = client.SolveResiduals(inputs);
    EXPECT_DOUBLE_EQ(outputs2["y"](0), 9.0);  // 3^2

    // Call gradient again
    Variables vars2;
    vars2["x"] = CreateScalarVariable(3.0);
    vars2["y"] = CreateScalarVariable(9.0);
    Partials partials2 = client.ComputeResidualGradients(vars2);
    EXPECT_DOUBLE_EQ((partials2[{"y", "x"}](0)), 6.0);  // 2*3
    EXPECT_DOUBLE_EQ((partials2[{"y", "y"}](0)), -1.0);
}

TEST_F(ImplicitIntegrationTest, ResidualEvaluationWithWrongOutputs) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Prepare inputs with wrong output guess: x=3.0, y=10.0
    // Correct would be y = 9.0
    Variables vars;
    vars["x"] = CreateScalarVariable(3.0);
    vars["y"] = CreateScalarVariable(10.0);

    // Compute residuals: R = 3^2 - 10 = -1
    Variables residuals = client.ComputeResiduals(vars);

    ASSERT_EQ(residuals.size(), 1);
    EXPECT_DOUBLE_EQ(residuals["y"](0), -1.0);
}

// ============================================================================
// Data Integrity Tests
// ============================================================================

TEST_F(ImplicitIntegrationTest, LargeVectorDataIntegrity) {
    // Test with larger vectors to ensure data integrity through full pipeline
    const size_t n = 100;
    const size_t m = 50;

    auto discipline = std::make_unique<VectorizedImplicitDiscipline>(n, m);

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
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

    // Solve: y = A*x + b
    // Each y[i] = sum(A[i,:] * x) + b[i] = m*1*2 + 3 = 2m + 3
    Variables outputs = client.SolveResiduals(inputs);

    ASSERT_EQ(outputs.size(), 1);
    ASSERT_EQ(outputs["y"].Shape()[0], n);

    double expected = 2.0 * m + 3.0;
    for (size_t i = 0; i < n; ++i) {
        EXPECT_DOUBLE_EQ(outputs["y"](i), expected) << "Mismatch at index " << i;
    }
}

TEST_F(ImplicitIntegrationTest, NegativeAndZeroValues) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Test with negative values
    Variables inputs1;
    inputs1["x"] = CreateScalarVariable(-4.0);

    Variables outputs1 = client.SolveResiduals(inputs1);
    EXPECT_DOUBLE_EQ(outputs1["y"](0), 16.0);  // (-4)^2 = 16

    // Test with zero values
    Variables inputs2;
    inputs2["x"] = CreateScalarVariable(0.0);

    Variables outputs2 = client.SolveResiduals(inputs2);
    EXPECT_DOUBLE_EQ(outputs2["y"](0), 0.0);

    // Test residual with negative output
    Variables vars3;
    vars3["x"] = CreateScalarVariable(3.0);
    vars3["y"] = CreateScalarVariable(-1.0);

    Variables residuals3 = client.ComputeResiduals(vars3);
    // R = 3^2 - (-1) = 9 + 1 = 10
    EXPECT_DOUBLE_EQ(residuals3["y"](0), 10.0);
}

TEST_F(ImplicitIntegrationTest, VerifySolutionSatisfiesResidual) {
    // Solve the residual and then verify it evaluates to near-zero
    auto discipline = std::make_unique<QuadraticDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Solve: 2x^2 - 8x + 6 = 0 (solutions: x = 1 or 3, returns positive root = 3)
    Variables inputs;
    inputs["a"] = CreateScalarVariable(2.0);
    inputs["b"] = CreateScalarVariable(-8.0);
    inputs["c"] = CreateScalarVariable(6.0);

    Variables outputs = client.SolveResiduals(inputs);
    ASSERT_EQ(outputs.size(), 1);

    // Now verify the solution satisfies the residual
    Variables vars;
    vars["a"] = CreateScalarVariable(2.0);
    vars["b"] = CreateScalarVariable(-8.0);
    vars["c"] = CreateScalarVariable(6.0);
    vars["x"] = outputs["x"];  // Use the solved value

    Variables residuals = client.ComputeResiduals(vars);

    // Residual should be near zero
    EXPECT_NEAR(residuals["x"](0), 0.0, 1e-10);
}
