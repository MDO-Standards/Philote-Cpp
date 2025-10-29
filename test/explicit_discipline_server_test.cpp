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

// Test fixture for ExplicitServer
class ExplicitServerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        server = std::make_unique<ExplicitServer>();
        discipline = std::make_unique<ExplicitDiscipline>();
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