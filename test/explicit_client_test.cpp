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

#include "explicit.h"
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

class ExplicitClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        client_ = std::make_shared<ExplicitClient>();
        mock_explicit_stub_ = new MockExplicitServiceStub();

        // Inject mock explicit stub
        client_->SetStub(std::unique_ptr<ExplicitService::StubInterface>(mock_explicit_stub_));
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

    // Helper to set up partials metadata
    void SetupSimplePartialsMetadata() {
        std::vector<PartialsMetaData> partials;

        PartialsMetaData partial_meta;
        partial_meta.set_name("y");
        partial_meta.set_subname("x");
        partial_meta.add_shape(1);
        partials.push_back(partial_meta);

        client_->SetPartialsMetaData(partials);
    }

    MockExplicitServiceStub* mock_explicit_stub_;
    std::shared_ptr<ExplicitClient> client_;
};

// ============================================================================
// Constructor and Initialization Tests
// ============================================================================

TEST(ExplicitClientBasicTest, SimpleConstructor) {
    ExplicitClient client;
    EXPECT_TRUE(true);
}

TEST(ExplicitClientBasicTest, ConstructorInitialization) {
    ExplicitClient client;
    EXPECT_EQ(client.GetVariableNames().size(), 0);
    EXPECT_EQ(client.GetPartialsMeta().size(), 0);
}

TEST(ExplicitClientBasicTest, MetadataAccessors) {
    ExplicitClient client;
    EXPECT_EQ(client.GetVariableNames().size(), 0);
    EXPECT_EQ(client.GetPartialsMeta().size(), 0);
}

// ============================================================================
// ComputeFunction Tests
// ============================================================================

TEST_F(ExplicitClientTest, ComputeFunctionSimpleScalar) {
    SetupSimpleDisciplineMetadata();

    // Create mock stream
    auto mock_stream = new MockReaderWriter<Array, Array>();

    // Expect Write call for input 'x'
    EXPECT_CALL(*mock_stream, Write(_, _))
        .WillOnce(Return(true));

    // Expect WritesDone
    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Mock reading output 'y'
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_subname("");
            result->set_start(0);
            result->set_end(0);
            result->add_data(42.0);
            return true;
        }))
        .WillOnce(Return(false));  // End of stream

    // Expect Finish with OK status
    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    // Mock stub to return our mock stream
    EXPECT_CALL(*mock_explicit_stub_, ComputeFunctionRaw(_))
        .WillOnce(Return(mock_stream));

    // Create input
    Variables inputs;
    inputs["x"] = CreateScalarVariable(5.0);

    // Execute
    Variables outputs = client_->ComputeFunction(inputs);

    // Verify
    ASSERT_EQ(outputs.size(), 1);
    ASSERT_TRUE(outputs.count("y") > 0);
    EXPECT_DOUBLE_EQ(outputs["y"](0), 42.0);
}

TEST_F(ExplicitClientTest, ComputeFunctionMultipleInputsOutputs) {
    // Setup metadata for multiple variables
    std::vector<VariableMetaData> vars;

    VariableMetaData x_meta, y_meta, sum_meta, prod_meta;
    x_meta.set_name("x");
    x_meta.set_type(kInput);
    x_meta.add_shape(1);

    y_meta.set_name("y");
    y_meta.set_type(kInput);
    y_meta.add_shape(1);

    sum_meta.set_name("sum");
    sum_meta.set_type(kOutput);
    sum_meta.add_shape(1);

    prod_meta.set_name("product");
    prod_meta.set_type(kOutput);
    prod_meta.add_shape(1);

    vars.push_back(x_meta);
    vars.push_back(y_meta);
    vars.push_back(sum_meta);
    vars.push_back(prod_meta);
    client_->SetVariableMeta(vars);

    auto mock_stream = new MockReaderWriter<Array, Array>();

    // Expect 2 Write calls (for x and y)
    EXPECT_CALL(*mock_stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Mock reading 2 outputs
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("sum");
            result->set_start(0);
            result->set_end(0);
            result->add_data(8.0);
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("product");
            result->set_start(0);
            result->set_end(0);
            result->add_data(15.0);
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_explicit_stub_, ComputeFunctionRaw(_))
        .WillOnce(Return(mock_stream));

    Variables inputs;
    inputs["x"] = CreateScalarVariable(3.0);
    inputs["y"] = CreateScalarVariable(5.0);

    Variables outputs = client_->ComputeFunction(inputs);

    ASSERT_EQ(outputs.size(), 2);
    EXPECT_DOUBLE_EQ(outputs["sum"](0), 8.0);
    EXPECT_DOUBLE_EQ(outputs["product"](0), 15.0);
}

TEST_F(ExplicitClientTest, ComputeFunctionVectorData) {
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
            result->add_data(2.0);
            result->add_data(3.0);
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_explicit_stub_, ComputeFunctionRaw(_))
        .WillOnce(Return(mock_stream));

    Variables inputs;
    inputs["x"] = CreateVectorVariable({1.0, 2.0, 3.0});

    Variables outputs = client_->ComputeFunction(inputs);

    ASSERT_EQ(outputs.size(), 1);
    ASSERT_EQ(outputs["y"].Shape()[0], 3);
    EXPECT_DOUBLE_EQ(outputs["y"](0), 1.0);
    EXPECT_DOUBLE_EQ(outputs["y"](1), 2.0);
    EXPECT_DOUBLE_EQ(outputs["y"](2), 3.0);
}

TEST_F(ExplicitClientTest, ComputeFunctionChunkedStreaming) {
    // Setup metadata for chunked test: y has shape {2} to receive 2 chunks
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

    // Simulate chunked response (2 chunks for single variable)
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

    EXPECT_CALL(*mock_explicit_stub_, ComputeFunctionRaw(_))
        .WillOnce(Return(mock_stream));

    Variables inputs;
    inputs["x"] = CreateScalarVariable(5.0);

    Variables outputs = client_->ComputeFunction(inputs);

    // Should have assembled chunks
    ASSERT_EQ(outputs.size(), 1);
    ASSERT_EQ(outputs["y"].Shape()[0], 2);
    EXPECT_DOUBLE_EQ(outputs["y"](0), 10.0);
    EXPECT_DOUBLE_EQ(outputs["y"](1), 20.0);
}

// ============================================================================
// ComputeGradient Tests
// ============================================================================

TEST_F(ExplicitClientTest, ComputeGradientSimple) {
    SetupSimpleDisciplineMetadata();
    SetupSimplePartialsMetadata();

    auto mock_stream = new MockReaderWriter<Array, Array>();

    // Expect Write for input
    EXPECT_CALL(*mock_stream, Write(_, _))
        .WillOnce(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Mock reading partial dy/dx
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("y");
            result->set_subname("x");
            result->set_start(0);
            result->set_end(0);
            result->add_data(2.0);
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_explicit_stub_, ComputeGradientRaw(_))
        .WillOnce(Return(mock_stream));

    Variables inputs;
    inputs["x"] = CreateScalarVariable(5.0);

    Partials partials = client_->ComputeGradient(inputs);

    ASSERT_EQ(partials.size(), 1);
    auto key = std::make_pair(std::string("y"), std::string("x"));
    ASSERT_TRUE(partials.count(key) > 0);
    EXPECT_DOUBLE_EQ(partials[key](0), 2.0);
}

TEST_F(ExplicitClientTest, ComputeGradientMultiplePartials) {
    // Setup for f(x, y) with two partials
    std::vector<VariableMetaData> vars;

    VariableMetaData x_meta, y_meta, f_meta;
    x_meta.set_name("x");
    x_meta.set_type(kInput);
    x_meta.add_shape(1);

    y_meta.set_name("y");
    y_meta.set_type(kInput);
    y_meta.add_shape(1);

    f_meta.set_name("f");
    f_meta.set_type(kOutput);
    f_meta.add_shape(1);

    vars.push_back(x_meta);
    vars.push_back(y_meta);
    vars.push_back(f_meta);
    client_->SetVariableMeta(vars);

    // Setup partials df/dx and df/dy
    std::vector<PartialsMetaData> partials_meta;

    PartialsMetaData df_dx, df_dy;
    df_dx.set_name("f");
    df_dx.set_subname("x");
    df_dx.add_shape(1);

    df_dy.set_name("f");
    df_dy.set_subname("y");
    df_dy.add_shape(1);

    partials_meta.push_back(df_dx);
    partials_meta.push_back(df_dy);
    client_->SetPartialsMetaData(partials_meta);

    auto mock_stream = new MockReaderWriter<Array, Array>();

    // Expect 2 writes for inputs
    EXPECT_CALL(*mock_stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_stream, WritesDone())
        .WillOnce(Return(true));

    // Mock reading 2 partials
    EXPECT_CALL(*mock_stream, Read(_))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("f");
            result->set_subname("x");
            result->set_start(0);
            result->set_end(0);
            result->add_data(6.0);
            return true;
        }))
        .WillOnce(Invoke([](Array* result) {
            result->Clear();
            result->set_name("f");
            result->set_subname("y");
            result->set_start(0);
            result->set_end(0);
            result->add_data(8.0);
            return true;
        }))
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_explicit_stub_, ComputeGradientRaw(_))
        .WillOnce(Return(mock_stream));

    Variables inputs;
    inputs["x"] = CreateScalarVariable(3.0);
    inputs["y"] = CreateScalarVariable(4.0);

    Partials partials = client_->ComputeGradient(inputs);

    ASSERT_EQ(partials.size(), 2);
    EXPECT_DOUBLE_EQ((partials[{"f", "x"}](0)), 6.0);
    EXPECT_DOUBLE_EQ((partials[{"f", "y"}](0)), 8.0);
}

TEST_F(ExplicitClientTest, ComputeGradientChunkedStreaming) {
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

    // Partials metadata: dy/dx has shape {2} to receive 2 chunks
    std::vector<PartialsMetaData> partials_meta;

    PartialsMetaData partial_meta;
    partial_meta.set_name("y");
    partial_meta.set_subname("x");
    partial_meta.add_shape(2);  // Size 2 to accommodate 2 chunks

    partials_meta.push_back(partial_meta);
    client_->SetPartialsMetaData(partials_meta);

    auto mock_stream = new MockReaderWriter<Array, Array>();

    EXPECT_CALL(*mock_stream, Write(_, _))
        .WillOnce(Return(true));

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
        .WillOnce(Return(false));

    EXPECT_CALL(*mock_stream, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_CALL(*mock_explicit_stub_, ComputeGradientRaw(_))
        .WillOnce(Return(mock_stream));

    Variables inputs;
    inputs["x"] = CreateScalarVariable(5.0);

    Partials partials = client_->ComputeGradient(inputs);

    ASSERT_EQ(partials.size(), 1);
    auto key = std::make_pair(std::string("y"), std::string("x"));
    ASSERT_TRUE(partials.count(key) > 0);
    ASSERT_EQ(partials[key].Shape()[0], 2);
    EXPECT_DOUBLE_EQ(partials[key](0), 1.5);
    EXPECT_DOUBLE_EQ(partials[key](1), 2.5);
}

// ============================================================================
// Connection Tests
// ============================================================================

TEST(ExplicitClientConnectionTest, ConnectChannel) {
    ExplicitClient client;
    auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());

    // Should not throw
    EXPECT_NO_THROW(client.ConnectChannel(channel));
}

TEST(ExplicitClientConnectionTest, ConnectChannelMultipleTimes) {
    ExplicitClient client;
    auto channel1 = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    auto channel2 = grpc::CreateChannel("localhost:50052", grpc::InsecureChannelCredentials());

    // Should be able to reconnect
    EXPECT_NO_THROW(client.ConnectChannel(channel1));
    EXPECT_NO_THROW(client.ConnectChannel(channel2));
}
