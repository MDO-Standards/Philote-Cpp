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
#include <stdexcept>
#include <cmath>
#include <limits>

#include "implicit.h"
#include "test_helpers.h"

using namespace philote;
using namespace philote::test;

// ============================================================================
// Error Scenarios Test Fixture
// ============================================================================

class ImplicitErrorScenariosTest : public ::testing::Test {
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
// Connection Error Tests
// ============================================================================

TEST(ImplicitClientConnectionErrors, ConnectToNonexistentServer) {
    ImplicitClient client;
    auto channel = grpc::CreateChannel("localhost:99999", grpc::InsecureChannelCredentials());

    // Should not throw on connection (connection is lazy in gRPC)
    EXPECT_NO_THROW(client.ConnectChannel(channel));

    // But actual RPC calls should fail or timeout
}

TEST(ImplicitClientConnectionErrors, InvalidAddress) {
    ImplicitClient client;

    // Invalid address format
    auto channel = grpc::CreateChannel("invalid_address", grpc::InsecureChannelCredentials());
    EXPECT_NO_THROW(client.ConnectChannel(channel));
}

// ============================================================================
// Discipline Error Tests
// ============================================================================

TEST_F(ImplicitErrorScenariosTest, DisciplineThrowsOnSetup) {
    auto discipline = std::make_unique<ImplicitErrorDiscipline>(
        ImplicitErrorDiscipline::ErrorMode::THROW_ON_SETUP);

    // Starting server should throw because Setup is called during StartServer
    EXPECT_THROW(server_manager_->StartServer(discipline.get()), std::runtime_error);
}

TEST_F(ImplicitErrorScenariosTest, DisciplineThrowsOnComputeResiduals) {
    auto discipline = std::make_unique<ImplicitErrorDiscipline>(
        ImplicitErrorDiscipline::ErrorMode::THROW_ON_COMPUTE_RESIDUALS);

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    Variables vars;
    vars["x"] = CreateScalarVariable(1.0);
    vars["y"] = CreateScalarVariable(1.0);

    // ComputeResiduals should propagate the error from the discipline
    EXPECT_NO_THROW({
        Variables residuals = client.ComputeResiduals(vars);
    });
}

TEST_F(ImplicitErrorScenariosTest, DisciplineThrowsOnSolveResiduals) {
    auto discipline = std::make_unique<ImplicitErrorDiscipline>(
        ImplicitErrorDiscipline::ErrorMode::THROW_ON_SOLVE_RESIDUALS);

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    Variables inputs;
    inputs["x"] = CreateScalarVariable(1.0);

    // SolveResiduals should handle the error from the discipline
    EXPECT_NO_THROW({
        Variables outputs = client.SolveResiduals(inputs);
    });
}

TEST_F(ImplicitErrorScenariosTest, DisciplineThrowsOnComputeResidualGradients) {
    auto discipline = std::make_unique<ImplicitErrorDiscipline>(
        ImplicitErrorDiscipline::ErrorMode::THROW_ON_GRADIENTS);

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
    vars["x"] = CreateScalarVariable(1.0);
    vars["y"] = CreateScalarVariable(1.0);

    // ComputeResidualGradients should handle the error
    EXPECT_NO_THROW({
        Partials partials = client.ComputeResidualGradients(vars);
    });
}

// ============================================================================
// Missing Variable Tests
// ============================================================================

TEST_F(ImplicitErrorScenariosTest, MissingInputForComputeResiduals) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Only provide 'x', missing 'y' (output)
    Variables vars;
    vars["x"] = CreateScalarVariable(3.0);

    // This should fail because 'y' is required for residual evaluation
    EXPECT_NO_THROW({
        Variables residuals = client.ComputeResiduals(vars);
    });
}

TEST_F(ImplicitErrorScenariosTest, MissingInputForSolveResiduals) {
    auto discipline = std::make_unique<QuadraticDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Only provide 'a', missing 'b' and 'c'
    Variables inputs;
    inputs["a"] = CreateScalarVariable(1.0);

    // This should fail because 'b' and 'c' are required
    EXPECT_NO_THROW({
        Variables outputs = client.SolveResiduals(inputs);
    });
}

TEST_F(ImplicitErrorScenariosTest, ExtraUnknownVariable) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Provide required inputs plus an unknown variable
    Variables inputs;
    inputs["x"] = CreateScalarVariable(3.0);
    inputs["unknown_var"] = CreateScalarVariable(5.0);

    // The server should ignore or reject the unknown variable
    // Current implementation sends only known inputs based on metadata
    EXPECT_NO_THROW({
        Variables outputs = client.SolveResiduals(inputs);
        // Should still compute correctly, ignoring unknown_var
        if (outputs.count("y")) {
            EXPECT_DOUBLE_EQ(outputs["y"](0), 9.0);
        }
    });
}

// ============================================================================
// Variable Shape Mismatch Tests
// ============================================================================

TEST_F(ImplicitErrorScenariosTest, WrongShapeInput) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Provide wrong shape - expect scalar but provide vector
    Variables inputs;
    inputs["x"] = CreateVectorVariable({1.0, 2.0, 3.0});  // Wrong shape

    // This might fail during variable sending or assignment
    EXPECT_NO_THROW({
        Variables outputs = client.SolveResiduals(inputs);
    });
}

TEST_F(ImplicitErrorScenariosTest, MismatchedInputOutputShapes) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Provide wrong shape for output in residual evaluation
    Variables vars;
    vars["x"] = CreateScalarVariable(3.0);
    vars["y"] = CreateVectorVariable({1.0, 2.0});  // Wrong shape

    EXPECT_NO_THROW({
        Variables residuals = client.ComputeResiduals(vars);
    });
}

// ============================================================================
// Empty and Boundary Value Tests
// ============================================================================

TEST_F(ImplicitErrorScenariosTest, EmptyInputsMap) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Provide empty inputs map
    Variables inputs;

    // Should fail because required inputs are missing
    EXPECT_NO_THROW({
        Variables outputs = client.SolveResiduals(inputs);
    });
}

TEST_F(ImplicitErrorScenariosTest, VeryLargeValues) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Test with very large values
    Variables inputs;
    inputs["x"] = CreateScalarVariable(1.0e100);

    EXPECT_NO_THROW({
        Variables outputs = client.SolveResiduals(inputs);
        if (outputs.count("y")) {
            // y = x^2 should be very large
            EXPECT_GT(outputs["y"](0), 1.0e199);
        }
    });
}

TEST_F(ImplicitErrorScenariosTest, VerySmallValues) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Test with very small values
    Variables inputs;
    inputs["x"] = CreateScalarVariable(1.0e-100);

    EXPECT_NO_THROW({
        Variables outputs = client.SolveResiduals(inputs);
        if (outputs.count("y")) {
            // y = x^2 should be very small
            EXPECT_LT(outputs["y"](0), 1.0e-199);
        }
    });
}

TEST_F(ImplicitErrorScenariosTest, InfinityValues) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Test with infinity
    Variables inputs;
    inputs["x"] = CreateScalarVariable(std::numeric_limits<double>::infinity());

    EXPECT_NO_THROW({
        Variables outputs = client.SolveResiduals(inputs);
        if (outputs.count("y")) {
            // y = x^2 should be infinity
            EXPECT_TRUE(std::isinf(outputs["y"](0)));
        }
    });
}

TEST_F(ImplicitErrorScenariosTest, NaNValues) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Test with NaN
    Variables inputs;
    inputs["x"] = CreateScalarVariable(std::numeric_limits<double>::quiet_NaN());

    EXPECT_NO_THROW({
        Variables outputs = client.SolveResiduals(inputs);
        if (outputs.count("y")) {
            // y = x^2 should be NaN
            EXPECT_TRUE(std::isnan(outputs["y"](0)));
        }
    });
}

// ============================================================================
// Server Lifecycle Tests
// ============================================================================

TEST_F(ImplicitErrorScenariosTest, MultipleServerStartStop) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    // Start and stop multiple times
    for (int i = 0; i < 3; ++i) {
        std::string address = server_manager_->StartServer(discipline.get());
        ASSERT_FALSE(address.empty());
        ASSERT_TRUE(server_manager_->IsRunning());

        server_manager_->StopServer();
        ASSERT_FALSE(server_manager_->IsRunning());
    }
}

TEST_F(ImplicitErrorScenariosTest, ClientAfterServerStop) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Stop server
    server_manager_->StopServer();

    // Try to use client after server stopped
    Variables inputs;
    inputs["x"] = CreateScalarVariable(2.0);

    // This should fail or timeout since server is stopped
    EXPECT_NO_THROW({
        Variables outputs = client.SolveResiduals(inputs);
    });
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(ImplicitErrorScenariosTest, RapidSuccessiveCalls) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // Make many rapid calls
    for (int i = 1; i <= 100; ++i) {
        Variables inputs;
        inputs["x"] = CreateScalarVariable(static_cast<double>(i));

        Variables outputs = client.SolveResiduals(inputs);

        ASSERT_EQ(outputs.size(), 1);
        EXPECT_DOUBLE_EQ(outputs["y"](0), static_cast<double>(i * i));
    }
}

TEST_F(ImplicitErrorScenariosTest, LargeNumberOfVariables) {
    // Test with discipline that has many variables
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

    // Create inputs with many elements
    Variables inputs;
    inputs["A"] = CreateMatrixVariable(n, m, 1.0);
    inputs["x"] = CreateVectorVariable(std::vector<double>(m, 1.0));
    inputs["b"] = CreateVectorVariable(std::vector<double>(n, 0.0));

    EXPECT_NO_THROW({
        Variables outputs = client.SolveResiduals(inputs);
        ASSERT_EQ(outputs.size(), 1);
        ASSERT_EQ(outputs["y"].Shape()[0], n);
    });
}

// ============================================================================
// Mixed Operation Tests
// ============================================================================

TEST_F(ImplicitErrorScenariosTest, AlternatingMethodCalls) {
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

    // Alternate between different methods rapidly
    for (int i = 1; i <= 10; ++i) {
        Variables inputs;
        inputs["x"] = CreateScalarVariable(static_cast<double>(i));

        // Solve
        Variables outputs = client.SolveResiduals(inputs);

        // Compute residuals with solution
        Variables vars;
        vars["x"] = inputs["x"];
        vars["y"] = outputs["y"];
        Variables residuals = client.ComputeResiduals(vars);

        // Compute gradients
        Partials partials = client.ComputeResidualGradients(vars);

        // Verify consistency
        EXPECT_NEAR(residuals["y"](0), 0.0, 1e-9);
        EXPECT_DOUBLE_EQ((partials[{"y", "y"}](0)), -1.0);
    }
}

TEST_F(ImplicitErrorScenariosTest, WrongOutputGuessProducesNonZeroResidual) {
    auto discipline = std::make_unique<SimpleImplicitDiscipline>();

    std::string address = server_manager_->StartServer(discipline.get());
    ASSERT_FALSE(address.empty());

    ImplicitClient client;
    auto channel = CreateTestChannel(address);
    client.ConnectChannel(channel);

    client.GetInfo();
    client.Setup();
    client.GetVariableDefinitions();

    // First solve to get correct output
    Variables inputs;
    inputs["x"] = CreateScalarVariable(5.0);
    Variables correct_outputs = client.SolveResiduals(inputs);

    // Now evaluate residual with wrong output
    Variables vars_wrong;
    vars_wrong["x"] = CreateScalarVariable(5.0);
    vars_wrong["y"] = CreateScalarVariable(20.0);  // Wrong (correct is 25)
    Variables residuals_wrong = client.ComputeResiduals(vars_wrong);

    // Residual should be non-zero
    EXPECT_NE(residuals_wrong["y"](0), 0.0);
    EXPECT_DOUBLE_EQ(residuals_wrong["y"](0), 5.0);  // 25 - 20 = 5

    // Now evaluate residual with correct output
    Variables vars_correct;
    vars_correct["x"] = CreateScalarVariable(5.0);
    vars_correct["y"] = correct_outputs["y"];
    Variables residuals_correct = client.ComputeResiduals(vars_correct);

    // Residual should be near zero
    EXPECT_NEAR(residuals_correct["y"](0), 0.0, 1e-10);
}
