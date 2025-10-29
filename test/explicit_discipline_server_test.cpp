#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/create_channel.h>

#include "explicit.h"

using namespace philote;
using namespace testing;

// Minimal mock for grpc::ServerReaderWriterInterface
class MockServerReaderWriter : public grpc::ServerReaderWriterInterface<philote::Array, philote::Array>
{
public:
    MOCK_METHOD(bool, Read, (philote::Array *), (override));
    MOCK_METHOD(bool, Write, (const philote::Array &, grpc::WriteOptions), (override));
    MOCK_METHOD(bool, Write, (const philote::Array &), ());
    MOCK_METHOD(void, SendInitialMetadata, (), (override));
    MOCK_METHOD(bool, NextMessageSize, (uint32_t *), (override));
};

// Mock discipline for server tests with actual compute implementations
class MockExplicitDiscipline : public ExplicitDiscipline
{
public:
    void Compute(const Variables &inputs, Variables &outputs) override
    {
        // Simple computation: y = 2*x
        const auto& x = inputs.at("x");
        auto& y = outputs.at("y");
        for (size_t i = 0; i < y.Size(); ++i)
        {
            y(i) = 2.0 * x(i);
        }
    }

    void ComputePartials(const Variables &inputs, Partials &partials) override
    {
        // Simple partials: dy/dx = 2
        auto& partial = partials[std::make_pair("y", "x")];
        for (size_t i = 0; i < partial.Size(); ++i)
        {
            partial(i) = 2.0;
        }
    }
};

// Test fixture for ExplicitServer
class ExplicitServerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        server = std::make_unique<ExplicitServer>();
        discipline = std::make_unique<MockExplicitDiscipline>();

        // Add input variable
        std::vector<int64_t> input_shape = {2}; // 2-element vector
        discipline->AddInput("x", input_shape, "unit");

        // Add output variable
        std::vector<int64_t> output_shape = {2}; // 2-element vector
        discipline->AddOutput("y", output_shape, "unit");

        // Declare partials
        std::vector<int64_t> partials_shape = {2, 2}; // 2x2 matrix
        discipline->DeclarePartials("y", "x");

        discipline->Setup();
        discipline->SetupPartials();
        server->LinkPointers(discipline.get());
    }

    void TearDown() override
    {
        server->UnlinkPointers();
        server.reset();
        discipline.reset();
    }

    std::unique_ptr<ExplicitServer> server;
    std::unique_ptr<ExplicitDiscipline> discipline;
    grpc::ServerContext context;
    std::unique_ptr<MockServerReaderWriter> stream = std::make_unique<MockServerReaderWriter>();
};

// Test server initialization and pointer linking
TEST_F(ExplicitServerTest, Initialization)
{
    EXPECT_TRUE(server != nullptr);
    EXPECT_TRUE(discipline != nullptr);
}

// Test ComputeFunction with valid input
TEST_F(ExplicitServerTest, ComputeFunctionValidInput)
{
    // Setup input array
    philote::Array input_array;
    input_array.set_name("x");
    input_array.set_type(VariableType::kInput);
    input_array.set_start(0);
    input_array.set_end(1);
    input_array.add_data(1.0);
    input_array.add_data(2.0);

    // Setup stream expectations
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(DoAll(
            SetArgPointee<0>(input_array),
            Return(true)))
        .WillOnce(Return(false)); // Only one chunk, then end

    // Setup output expectations
    EXPECT_CALL(*stream, Write(_, _))
        .WillOnce(Return(true));

    // Call ComputeFunction
    grpc::Status status = server->ComputeFunction(&context, stream.get());
    EXPECT_TRUE(status.ok());
}

// Test ComputeFunction with invalid input
TEST_F(ExplicitServerTest, ComputeFunctionInvalidInput)
{
    // Setup invalid input array (wrong type)
    philote::Array input_array;
    input_array.set_name("x");
    input_array.set_type(VariableType::kOutput); // Wrong type
    input_array.set_start(0);
    input_array.set_end(1);
    input_array.add_data(1.0);
    input_array.add_data(2.0);

    // Setup stream expectations
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(DoAll(
            SetArgPointee<0>(input_array),
            Return(true)))
        .WillOnce(Return(false));

    // Call ComputeFunction
    grpc::Status status = server->ComputeFunction(&context, stream.get());
    EXPECT_TRUE(status.ok()); // Server should handle invalid input gracefully
}

// Test ComputeGradient with valid input
TEST_F(ExplicitServerTest, ComputeGradientValidInput)
{
    // Setup input array
    philote::Array input_array;
    input_array.set_name("x");
    input_array.set_type(VariableType::kInput);
    input_array.set_start(0);
    input_array.set_end(1);
    input_array.add_data(1.0);
    input_array.add_data(2.0);

    // Setup stream expectations
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(DoAll(
            SetArgPointee<0>(input_array),
            Return(true)))
        .WillOnce(Return(false)); // Only one chunk, then end

    // Setup output expectations
    EXPECT_CALL(*stream, Write(_, _))
        .WillOnce(Return(true));

    // Call ComputeGradient
    grpc::Status status = server->ComputeGradient(&context, stream.get());
    EXPECT_TRUE(status.ok());
}

// Test ComputeGradient with invalid input
TEST_F(ExplicitServerTest, ComputeGradientInvalidInput)
{
    // Setup invalid input array (wrong type)
    philote::Array input_array;
    input_array.set_name("x");
    input_array.set_type(VariableType::kOutput); // Wrong type
    input_array.set_start(0);
    input_array.set_end(1);
    input_array.add_data(1.0);
    input_array.add_data(2.0);

    // Setup stream expectations
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(DoAll(
            SetArgPointee<0>(input_array),
            Return(true)))
        .WillOnce(Return(false)); // Only one chunk, then end

    // Call ComputeGradient
    grpc::Status status = server->ComputeGradient(&context, stream.get());
    EXPECT_TRUE(status.ok()); // Server should handle invalid input gracefully
}

// Test server with unlinked pointers
TEST_F(ExplicitServerTest, UnlinkedPointers)
{
    server->UnlinkPointers();

    // Setup input array
    philote::Array input_array;
    input_array.set_name("x");
    input_array.set_type(VariableType::kInput);
    input_array.set_start(0);
    input_array.set_end(1);
    input_array.add_data(1.0);
    input_array.add_data(2.0);

    // Setup stream expectations
    EXPECT_CALL(*stream, Read(_)).Times(0);

    // Call ComputeFunction
    grpc::Status status = server->ComputeFunction(&context, stream.get());
    EXPECT_FALSE(status.ok());
}

// Test server with multiple inputs
TEST_F(ExplicitServerTest, MultipleInputs)
{
    // Setup input array
    philote::Array input_array;
    input_array.set_name("x");
    input_array.set_type(VariableType::kInput);
    input_array.set_start(0);
    input_array.set_end(1);
    input_array.add_data(1.0);
    input_array.add_data(2.0);

    // Setup stream expectations
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(DoAll(
            SetArgPointee<0>(input_array),
            Return(true)))
        .WillOnce(Return(false)); // Only one chunk, then end

    // Setup output expectations
    EXPECT_CALL(*stream, Write(_, _))
        .WillOnce(Return(true));

    // Call ComputeFunction
    grpc::Status status = server->ComputeFunction(&context, stream.get());
    EXPECT_TRUE(status.ok());
}