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
#include <gmock/gmock.h>
#include <memory>
#include <grpcpp/grpcpp.h>

#include "implicit.h"
#include "test_helpers.h"
#include "disciplines_mock.grpc.pb.h"
#include "data.pb.h"

using namespace philote;
using namespace philote::test;
using namespace testing;

// ============================================================================
// Mock Classes for Streaming
// ============================================================================

// Mock reader-writer for bidirectional streaming
template <typename W, typename R>
class MockReaderWriter : public grpc::ClientReaderWriterInterface<W, R> {
public:
    MOCK_METHOD(bool, Write, (const W&, grpc::WriteOptions), (override));
    MOCK_METHOD(bool, Read, (R*), (override));
    MOCK_METHOD(void, WaitForInitialMetadata, (), (override));
    MOCK_METHOD(bool, WritesDone, (), (override));
    MOCK_METHOD(grpc::Status, Finish, (), (override));
    MOCK_METHOD(bool, NextMessageSize, (uint32_t*), (override));
};

// ============================================================================
// Test Fixture
// ============================================================================

class ImplicitClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        client_ = std::make_shared<ImplicitClient>();
        mock_implicit_stub_ = new MockImplicitServiceStub();

        // Inject mock implicit stub
        client_->SetStub(std::unique_ptr<ImplicitService::StubInterface>(mock_implicit_stub_));
    }

    void TearDown() override {
        client_.reset();
    }

    // Helper to set up metadata for a simple discipline
    void SetupSimpleDisciplineMetadata() {
        std::vector<VariableMetaData> vars;

        VariableMetaData x_meta;
        x_meta.set_name("x");
        x_meta.set_type(kInput);
        x_meta.add_shape(1);
        vars.push_back(x_meta);

        VariableMetaData y_meta;
        y_meta.set_name("y");
        y_meta.set_type(kOutput);
        y_meta.add_shape(1);
        vars.push_back(y_meta);

        client_->SetVariableMeta(vars);
    }

    // Helper to set up partials metadata (includes output partials for implicit)
    void SetupSimplePartialsMetadata() {
        std::vector<PartialsMetaData> partials;

        PartialsMetaData partial_meta_x;
        partial_meta_x.set_name("y");
        partial_meta_x.set_subname("x");
        partial_meta_x.add_shape(1);
        partials.push_back(partial_meta_x);

        PartialsMetaData partial_meta_y;
        partial_meta_y.set_name("y");
        partial_meta_y.set_subname("y");
        partial_meta_y.add_shape(1);
        partials.push_back(partial_meta_y);

        client_->SetPartialsMetaData(partials);
    }

    MockImplicitServiceStub* mock_implicit_stub_;
    std::shared_ptr<ImplicitClient> client_;
};

// ============================================================================
// Constructor and Initialization Tests
// ============================================================================

TEST(ImplicitClientBasicTest, SimpleConstructor) {
    ImplicitClient client;
    EXPECT_TRUE(true);
}

TEST(ImplicitClientBasicTest, ConstructorInitialization) {
    ImplicitClient client;
    EXPECT_EQ(client.GetVariableNames().size(), 0);
    EXPECT_EQ(client.GetPartialsMeta().size(), 0);
}

TEST(ImplicitClientBasicTest, MetadataAccessors) {
    ImplicitClient client;
    EXPECT_EQ(client.GetVariableNames().size(), 0);
    EXPECT_EQ(client.GetPartialsMeta().size(), 0);
}

// ============================================================================
// ComputeResiduals Tests
// ============================================================================

TEST_F(ImplicitClientTest, ComputeResidualsSimpleScalar) {
    SetupSimpleDisciplineMetadata();

    // Create mock stream
    auto mock_stream = new MockReaderWriter<Array, Array>();

    // Expect Write calls for both input 'x' and output 'y'
    EXPECT_CALL(*mock_stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Return(true));

    // Expect WritesDone
    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Mock reading residual 'y'
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_subname("");
            result->set_start(0);
            result->set_end(0);
            result->add_data(0.5);  // Some residual value
            return true;
        }))
        .WillOnce(Return(false));  // End of stream

    // Expect Finish with OK status
    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    // Mock stub to return our mock stream
    EXPECT_CALL(*mock_implicit_stub_, ComputeResidualsRaw(_))
        .WillOnce(Return(mock_stream));

    // Create inputs and outputs
    Variables vars;
    vars["x"] = CreateScalarVariable(2.0);
    vars["y"] = CreateScalarVariable(3.5);  // Output guess

    // Execute
    Variables residuals = client_->ComputeResiduals(vars);

    // Verify
    ASSERT_EQ(residuals.size(), 1);
    ASSERT_TRUE(residuals.count("y") > 0);
    EXPECT_DOUBLE_EQ(residuals["y"](0), 0.5);
}

TEST_F(ImplicitClientTest, ComputeResidualsMultipleOutputs) {
    // Setup metadata for multiple variables
    std::vector<VariableMetaData> vars;

    VariableMetaData sum_meta, prod_meta, x_meta, y_meta;
    sum_meta.set_name("sum");
    sum_meta.set_type(kInput);
    sum_meta.add_shape(1);

    prod_meta.set_name("product");
    prod_meta.set_type(kInput);
    prod_meta.add_shape(1);

    x_meta.set_name("x");
    x_meta.set_type(kOutput);
    x_meta.add_shape(1);

    y_meta.set_name("y");
    y_meta.set_type(kOutput);
    y_meta.add_shape(1);

    vars.push_back(sum_meta);
    vars.push_back(prod_meta);
    vars.push_back(x_meta);
    vars.push_back(y_meta);
    client_->SetVariableMeta(vars);

    auto mock_stream = new MockReaderWriter<Array, Array>();

    // Expect 4 Write calls (2 inputs + 2 outputs)
    EXPECT_CALL(*mock_stream, Write(_, _))
        .Times(4)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Mock reading 2 residuals
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("x");
            result->set_start(0);
            result->set_end(0);
            result->add_data(0.1);
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_start(0);
            result->set_end(0);
            result->add_data(-0.2);
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_implicit_stub_, ComputeResidualsRaw(_))
        .WillOnce(Return(mock_stream));

    Variables inputs;
    inputs["sum"] = CreateScalarVariable(8.0);
    inputs["product"] = CreateScalarVariable(15.0);
    inputs["x"] = CreateScalarVariable(5.1);  // Guess
    inputs["y"] = CreateScalarVariable(2.8);  // Guess

    Variables residuals = client_->ComputeResiduals(inputs);

    ASSERT_EQ(residuals.size(), 2);
    EXPECT_DOUBLE_EQ(residuals["x"](0), 0.1);
    EXPECT_DOUBLE_EQ(residuals["y"](0), -0.2);
}

TEST_F(ImplicitClientTest, ComputeResidualsVectorData) {
    // Setup for vector input/output
    std::vector<VariableMetaData> vars;

    VariableMetaData x_meta, y_meta;
    x_meta.set_name("x");
    x_meta.set_type(kInput);
    x_meta.add_shape(3);

    y_meta.set_name("y");
    y_meta.set_type(kOutput);
    y_meta.add_shape(3);

    vars.push_back(x_meta);
    vars.push_back(y_meta);
    client_->SetVariableMeta(vars);

    auto mock_stream = new MockReaderWriter<Array, Array>();

    // Expect 2 Write calls (input + output)
    EXPECT_CALL(*mock_stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_start(0);
            result->set_end(2);  // 3 elements: end = size - 1
            result->add_data(0.1);
            result->add_data(0.2);
            result->add_data(0.3);
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_implicit_stub_, ComputeResidualsRaw(_))
        .WillOnce(Return(mock_stream));

    Variables vars_input;
    vars_input["x"] = CreateVectorVariable({1.0, 2.0, 3.0});
    vars_input["y"] = CreateVectorVariable({1.1, 4.2, 9.3});  // Guesses

    Variables residuals = client_->ComputeResiduals(vars_input);

    ASSERT_EQ(residuals.size(), 1);
    ASSERT_EQ(residuals["y"].Shape()[0], 3);
    EXPECT_DOUBLE_EQ(residuals["y"](0), 0.1);
    EXPECT_DOUBLE_EQ(residuals["y"](1), 0.2);
    EXPECT_DOUBLE_EQ(residuals["y"](2), 0.3);
}

TEST_F(ImplicitClientTest, ComputeResidualsChunkedStreaming) {
    // Setup metadata for chunked test
    std::vector<VariableMetaData> vars;

    VariableMetaData x_meta, y_meta;
    x_meta.set_name("x");
    x_meta.set_type(kInput);
    x_meta.add_shape(1);

    y_meta.set_name("y");
    y_meta.set_type(kOutput);
    y_meta.add_shape(2);  // Size 2 to accommodate 2 chunks

    vars.push_back(x_meta);
    vars.push_back(y_meta);
    client_->SetVariableMeta(vars);

    auto mock_stream = new MockReaderWriter<Array, Array>();

    EXPECT_CALL(*mock_stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Simulate chunked response (2 chunks for single residual variable)
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_start(0);
            result->set_end(0);  // First chunk: element 0
            result->add_data(0.5);
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_start(1);
            result->set_end(1);  // Second chunk: element 1
            result->add_data(0.7);
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_implicit_stub_, ComputeResidualsRaw(_))
        .WillOnce(Return(mock_stream));

    Variables vars_input;
    vars_input["x"] = CreateScalarVariable(2.0);
    vars_input["y"] = CreateVectorVariable({4.5, 5.3});

    Variables residuals = client_->ComputeResiduals(vars_input);

    // Should have assembled chunks
    ASSERT_EQ(residuals.size(), 1);
    ASSERT_EQ(residuals["y"].Shape()[0], 2);
    EXPECT_DOUBLE_EQ(residuals["y"](0), 0.5);
    EXPECT_DOUBLE_EQ(residuals["y"](1), 0.7);
}

// ============================================================================
// SolveResiduals Tests
// ============================================================================

TEST_F(ImplicitClientTest, SolveResidualsSimpleScalar) {
    SetupSimpleDisciplineMetadata();

    auto mock_stream = new MockReaderWriter<Array, Array>();

    // Expect Write call for input 'x' only (not outputs for solve)
    EXPECT_CALL(*mock_stream, Write(_, _))
        .WillOnce(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Mock reading solved output 'y'
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_subname("");
            result->set_start(0);
            result->set_end(0);
            result->add_data(4.0);  // Solved value for x^2
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_implicit_stub_, SolveResidualsRaw(_))
        .WillOnce(Return(mock_stream));

    // Create input only
    Variables inputs;
    inputs["x"] = CreateScalarVariable(2.0);

    // Execute
    Variables outputs = client_->SolveResiduals(inputs);

    // Verify
    ASSERT_EQ(outputs.size(), 1);
    ASSERT_TRUE(outputs.count("y") > 0);
    EXPECT_DOUBLE_EQ(outputs["y"](0), 4.0);
}

TEST_F(ImplicitClientTest, SolveResidualsMultipleOutputs) {
    // Setup metadata for multiple variables
    std::vector<VariableMetaData> vars;

    VariableMetaData sum_meta, prod_meta, x_meta, y_meta;
    sum_meta.set_name("sum");
    sum_meta.set_type(kInput);
    sum_meta.add_shape(1);

    prod_meta.set_name("product");
    prod_meta.set_type(kInput);
    prod_meta.add_shape(1);

    x_meta.set_name("x");
    x_meta.set_type(kOutput);
    x_meta.add_shape(1);

    y_meta.set_name("y");
    y_meta.set_type(kOutput);
    y_meta.add_shape(1);

    vars.push_back(sum_meta);
    vars.push_back(prod_meta);
    vars.push_back(x_meta);
    vars.push_back(y_meta);
    client_->SetVariableMeta(vars);

    auto mock_stream = new MockReaderWriter<Array, Array>();

    // Expect 2 Write calls (for inputs only)
    EXPECT_CALL(*mock_stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Mock reading 2 outputs
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("x");
            result->set_start(0);
            result->set_end(0);
            result->add_data(5.0);
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_start(0);
            result->set_end(0);
            result->add_data(3.0);
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_implicit_stub_, SolveResidualsRaw(_))
        .WillOnce(Return(mock_stream));

    Variables inputs;
    inputs["sum"] = CreateScalarVariable(8.0);
    inputs["product"] = CreateScalarVariable(15.0);

    Variables outputs = client_->SolveResiduals(inputs);

    ASSERT_EQ(outputs.size(), 2);
    EXPECT_DOUBLE_EQ(outputs["x"](0), 5.0);
    EXPECT_DOUBLE_EQ(outputs["y"](0), 3.0);
}

TEST_F(ImplicitClientTest, SolveResidualsVectorData) {
    // Setup for vector input/output
    std::vector<VariableMetaData> vars;

    VariableMetaData x_meta, y_meta;
    x_meta.set_name("x");
    x_meta.set_type(kInput);
    x_meta.add_shape(3);

    y_meta.set_name("y");
    y_meta.set_type(kOutput);
    y_meta.add_shape(3);

    vars.push_back(x_meta);
    vars.push_back(y_meta);
    client_->SetVariableMeta(vars);

    auto mock_stream = new MockReaderWriter<Array, Array>();

    EXPECT_CALL(*mock_stream, Write(_, _))
        .WillOnce(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_start(0);
            result->set_end(2);  // 3 elements: end = size - 1
            result->add_data(1.0);
            result->add_data(4.0);
            result->add_data(9.0);
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_implicit_stub_, SolveResidualsRaw(_))
        .WillOnce(Return(mock_stream));

    Variables inputs;
    inputs["x"] = CreateVectorVariable({1.0, 2.0, 3.0});

    Variables outputs = client_->SolveResiduals(inputs);

    ASSERT_EQ(outputs.size(), 1);
    ASSERT_EQ(outputs["y"].Shape()[0], 3);
    EXPECT_DOUBLE_EQ(outputs["y"](0), 1.0);
    EXPECT_DOUBLE_EQ(outputs["y"](1), 4.0);
    EXPECT_DOUBLE_EQ(outputs["y"](2), 9.0);
}

TEST_F(ImplicitClientTest, SolveResidualsChunkedStreaming) {
    // Setup metadata for chunked test
    std::vector<VariableMetaData> vars;

    VariableMetaData x_meta, y_meta;
    x_meta.set_name("x");
    x_meta.set_type(kInput);
    x_meta.add_shape(1);

    y_meta.set_name("y");
    y_meta.set_type(kOutput);
    y_meta.add_shape(2);  // Size 2 to accommodate 2 chunks

    vars.push_back(x_meta);
    vars.push_back(y_meta);
    client_->SetVariableMeta(vars);

    auto mock_stream = new MockReaderWriter<Array, Array>();

    EXPECT_CALL(*mock_stream, Write(_, _))
        .WillOnce(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Simulate chunked response (2 chunks for output variable)
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_start(0);
            result->set_end(0);  // First chunk: element 0
            result->add_data(10.0);
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_start(1);
            result->set_end(1);  // Second chunk: element 1
            result->add_data(20.0);
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_implicit_stub_, SolveResidualsRaw(_))
        .WillOnce(Return(mock_stream));

    Variables inputs;
    inputs["x"] = CreateScalarVariable(5.0);

    Variables outputs = client_->SolveResiduals(inputs);

    // Should have assembled chunks
    ASSERT_EQ(outputs.size(), 1);
    ASSERT_EQ(outputs["y"].Shape()[0], 2);
    EXPECT_DOUBLE_EQ(outputs["y"](0), 10.0);
    EXPECT_DOUBLE_EQ(outputs["y"](1), 20.0);
}

// ============================================================================
// ComputeResidualGradients Tests
// ============================================================================

TEST_F(ImplicitClientTest, ComputeResidualGradientsSimple) {
    SetupSimpleDisciplineMetadata();
    SetupSimplePartialsMetadata();

    auto mock_stream = new MockReaderWriter<Array, Array>();

    // Expect Write for both input and output (2 writes)
    EXPECT_CALL(*mock_stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Mock reading partials dR/dx and dR/dy
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_subname("x");
            result->set_start(0);
            result->set_end(0);
            result->add_data(4.0);  // dR/dx = 2x for x=2
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_subname("y");
            result->set_start(0);
            result->set_end(0);
            result->add_data(-1.0);  // dR/dy = -1
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_implicit_stub_, ComputeResidualGradientsRaw(_))
        .WillOnce(Return(mock_stream));

    Variables vars;
    vars["x"] = CreateScalarVariable(2.0);
    vars["y"] = CreateScalarVariable(4.0);

    Partials partials = client_->ComputeResidualGradients(vars);

    ASSERT_EQ(partials.size(), 2);
    auto key_x = std::make_pair(std::string("y"), std::string("x"));
    auto key_y = std::make_pair(std::string("y"), std::string("y"));
    ASSERT_TRUE(partials.count(key_x) > 0);
    ASSERT_TRUE(partials.count(key_y) > 0);
    EXPECT_DOUBLE_EQ(partials[key_x](0), 4.0);
    EXPECT_DOUBLE_EQ(partials[key_y](0), -1.0);
}

TEST_F(ImplicitClientTest, ComputeResidualGradientsMultiplePartials) {
    // Setup for quadratic equation: ax^2 + bx + c = 0
    std::vector<VariableMetaData> vars;

    VariableMetaData a_meta, b_meta, c_meta, x_meta;
    a_meta.set_name("a");
    a_meta.set_type(kInput);
    a_meta.add_shape(1);

    b_meta.set_name("b");
    b_meta.set_type(kInput);
    b_meta.add_shape(1);

    c_meta.set_name("c");
    c_meta.set_type(kInput);
    c_meta.add_shape(1);

    x_meta.set_name("x");
    x_meta.set_type(kOutput);
    x_meta.add_shape(1);

    vars.push_back(a_meta);
    vars.push_back(b_meta);
    vars.push_back(c_meta);
    vars.push_back(x_meta);
    client_->SetVariableMeta(vars);

    // Setup partials dR/da, dR/db, dR/dc, dR/dx
    std::vector<PartialsMetaData> partials_meta;

    PartialsMetaData dR_da, dR_db, dR_dc, dR_dx;
    dR_da.set_name("x");
    dR_da.set_subname("a");
    dR_da.add_shape(1);

    dR_db.set_name("x");
    dR_db.set_subname("b");
    dR_db.add_shape(1);

    dR_dc.set_name("x");
    dR_dc.set_subname("c");
    dR_dc.add_shape(1);

    dR_dx.set_name("x");
    dR_dx.set_subname("x");
    dR_dx.add_shape(1);

    partials_meta.push_back(dR_da);
    partials_meta.push_back(dR_db);
    partials_meta.push_back(dR_dc);
    partials_meta.push_back(dR_dx);
    client_->SetPartialsMetaData(partials_meta);

    auto mock_stream = new MockReaderWriter<Array, Array>();

    // Expect 4 writes for all variables (3 inputs + 1 output)
    EXPECT_CALL(*mock_stream, Write(_, _))
        .Times(4)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Mock reading 4 partials
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("x");
            result->set_subname("a");
            result->set_start(0);
            result->set_end(0);
            result->add_data(9.0);  // x^2 for x=3
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("x");
            result->set_subname("b");
            result->set_start(0);
            result->set_end(0);
            result->add_data(3.0);  // x for x=3
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("x");
            result->set_subname("c");
            result->set_start(0);
            result->set_end(0);
            result->add_data(1.0);
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("x");
            result->set_subname("x");
            result->set_start(0);
            result->set_end(0);
            result->add_data(1.0);  // 2ax + b for a=1, x=3, b=-5
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_implicit_stub_, ComputeResidualGradientsRaw(_))
        .WillOnce(Return(mock_stream));

    Variables vars_input;
    vars_input["a"] = CreateScalarVariable(1.0);
    vars_input["b"] = CreateScalarVariable(-5.0);
    vars_input["c"] = CreateScalarVariable(6.0);
    vars_input["x"] = CreateScalarVariable(3.0);

    Partials partials = client_->ComputeResidualGradients(vars_input);

    ASSERT_EQ(partials.size(), 4);
    EXPECT_DOUBLE_EQ((partials[{"x", "a"}](0)), 9.0);
    EXPECT_DOUBLE_EQ((partials[{"x", "b"}](0)), 3.0);
    EXPECT_DOUBLE_EQ((partials[{"x", "c"}](0)), 1.0);
    EXPECT_DOUBLE_EQ((partials[{"x", "x"}](0)), 1.0);
}

TEST_F(ImplicitClientTest, ComputeResidualGradientsChunkedStreaming) {
    // Setup metadata for chunked test
    std::vector<VariableMetaData> vars;

    VariableMetaData x_meta, y_meta;
    x_meta.set_name("x");
    x_meta.set_type(kInput);
    x_meta.add_shape(1);

    y_meta.set_name("y");
    y_meta.set_type(kOutput);
    y_meta.add_shape(1);

    vars.push_back(x_meta);
    vars.push_back(y_meta);
    client_->SetVariableMeta(vars);

    // Partials metadata: dR/dx has shape {2} to receive 2 chunks
    std::vector<PartialsMetaData> partials_meta;

    PartialsMetaData partial_meta_x, partial_meta_y;
    partial_meta_x.set_name("y");
    partial_meta_x.set_subname("x");
    partial_meta_x.add_shape(2);  // Size 2 to accommodate 2 chunks

    partial_meta_y.set_name("y");
    partial_meta_y.set_subname("y");
    partial_meta_y.add_shape(2);

    partials_meta.push_back(partial_meta_x);
    partials_meta.push_back(partial_meta_y);
    client_->SetPartialsMetaData(partials_meta);

    auto mock_stream = new MockReaderWriter<Array, Array>();

    EXPECT_CALL(*mock_stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Simulate chunked partial response
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_subname("x");
            result->set_start(0);
            result->set_end(0);  // First chunk: element 0
            result->add_data(1.5);
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_subname("x");
            result->set_start(1);
            result->set_end(1);  // Second chunk: element 1
            result->add_data(2.5);
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_subname("y");
            result->set_start(0);
            result->set_end(0);
            result->add_data(-1.0);
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_subname("y");
            result->set_start(1);
            result->set_end(1);
            result->add_data(-1.0);
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_implicit_stub_, ComputeResidualGradientsRaw(_))
        .WillOnce(Return(mock_stream));

    Variables vars_input;
    vars_input["x"] = CreateScalarVariable(2.0);
    vars_input["y"] = CreateScalarVariable(4.0);

    Partials partials = client_->ComputeResidualGradients(vars_input);

    ASSERT_EQ(partials.size(), 2);
    auto key_x = std::make_pair(std::string("y"), std::string("x"));
    auto key_y = std::make_pair(std::string("y"), std::string("y"));
    ASSERT_TRUE(partials.count(key_x) > 0);
    ASSERT_TRUE(partials.count(key_y) > 0);
    ASSERT_EQ(partials[key_x].Shape()[0], 2);
    ASSERT_EQ(partials[key_y].Shape()[0], 2);
    EXPECT_DOUBLE_EQ(partials[key_x](0), 1.5);
    EXPECT_DOUBLE_EQ(partials[key_x](1), 2.5);
    EXPECT_DOUBLE_EQ(partials[key_y](0), -1.0);
    EXPECT_DOUBLE_EQ(partials[key_y](1), -1.0);
}

// ============================================================================
// Connection Tests
// ============================================================================

TEST(ImplicitClientConnectionTest, ConnectChannel) {
    ImplicitClient client;
    auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());

    // Should not throw
    EXPECT_NO_THROW(client.ConnectChannel(channel));
}

TEST(ImplicitClientConnectionTest, ConnectChannelMultipleTimes) {
    ImplicitClient client;
    auto channel1 = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    auto channel2 = grpc::CreateChannel("localhost:50052", grpc::InsecureChannelCredentials());

    // Should be able to reconnect
    EXPECT_NO_THROW(client.ConnectChannel(channel1));
    EXPECT_NO_THROW(client.ConnectChannel(channel2));
}
