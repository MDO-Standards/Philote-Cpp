#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "discipline_server.h"
#include "discipline.h"
#include <google/protobuf/struct.pb.h>
#include <grpcpp/grpcpp.h>

using namespace philote;
using namespace testing;

// Mock Discipline class for testing
class MockDiscipline : public Discipline
{
public:
    MOCK_METHOD0(Setup, void());
    MOCK_METHOD0(SetupPartials, void());
    MOCK_METHOD1(SetOptions, void(const google::protobuf::Struct &));
};

// Minimal mock writer for streaming RPCs
template <typename T>
class DummyWriter : public grpc::ServerWriterInterface<T>
{
public:
    bool Write(const T &, grpc::WriteOptions) override { return true; }
    void SendInitialMetadata() override {}
    void Finish() {}
    void SetNotifyOnWrite(bool) {}
};

// Test fixture for DisciplineServer class
class DisciplineServerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        server = std::make_unique<DisciplineServer>();
        mock_discipline = std::make_unique<MockDiscipline>();
        server->LinkPointers(mock_discipline.get());
    }

    void TearDown() override
    {
        server->UnlinkPointers();
        server.reset();
        mock_discipline.reset();
    }

    std::unique_ptr<DisciplineServer> server;
    std::unique_ptr<MockDiscipline> mock_discipline;
};

// Test constructor and pointer management
TEST_F(DisciplineServerTest, ConstructorAndPointerManagement)
{
    // Test initial state
    EXPECT_FALSE(server->DisciplinePointerNull());

    // Test unlinking pointers
    server->UnlinkPointers();
    EXPECT_TRUE(server->DisciplinePointerNull());

    // Test relinking pointers
    server->LinkPointers(mock_discipline.get());
    EXPECT_FALSE(server->DisciplinePointerNull());
}

// Test GetInfo RPC
TEST_F(DisciplineServerTest, GetInfo)
{
    grpc::ServerContext context;
    google::protobuf::Empty request;
    DisciplineProperties response;

    // Set up mock discipline properties
    auto &props = mock_discipline->properties();
    props.set_name("TestDiscipline");
    props.set_version("1.0.0");
    props.set_continuous(true);
    props.set_differentiable(true);
    props.set_provides_gradients(true);

    // Call GetInfo
    grpc::Status status = server->GetInfo(&context, &request, &response);

    // Verify response
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(response.name(), "TestDiscipline");
    EXPECT_EQ(response.version(), "1.0.0");
    EXPECT_TRUE(response.continuous());
    EXPECT_TRUE(response.differentiable());
    EXPECT_TRUE(response.provides_gradients());
}

// Test SetStreamOptions RPC
TEST_F(DisciplineServerTest, SetStreamOptions)
{
    grpc::ServerContext context;
    StreamOptions request;
    google::protobuf::Empty response;

    // Set up request
    request.set_num_double(10);

    // Call SetStreamOptions
    grpc::Status status = server->SetStreamOptions(&context, &request, &response);

    // Verify response
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(mock_discipline->stream_opts().num_double(), 10);
}

// Test SetOptions RPC
TEST_F(DisciplineServerTest, SetOptions)
{
    grpc::ServerContext context;
    DisciplineOptions request;
    google::protobuf::Empty response;

    // Set up request
    google::protobuf::Struct options_struct;
    (*options_struct.mutable_fields())["test_option"].set_number_value(42.0);
    request.mutable_options()->CopyFrom(options_struct);

    // Set up expectations
    EXPECT_CALL(*mock_discipline, SetOptions(Matcher<const google::protobuf::Struct &>(_)))
        .Times(1);

    // Call SetOptions
    grpc::Status status = server->SetOptions(&context, &request, &response);

    // Verify response
    EXPECT_TRUE(status.ok());
}

// Test GetAvailableOptions RPC
TEST_F(DisciplineServerTest, GetAvailableOptions)
{
    grpc::ServerContext context;
    google::protobuf::Empty request;
    StreamOptions response;

    // Set up mock discipline options
    auto &options = mock_discipline->options_list();
    options["option1"] = "double";
    options["option2"] = "double"; // Changed to double since num_int is not available

    // Call GetAvailableOptions
    grpc::Status status = server->GetAvailableOptions(&context, &request, &response);

    // Verify response
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(response.num_double(), 1000); // Default value from implementation
}

// Test GetVariableDefinitions RPC
TEST_F(DisciplineServerTest, GetVariableDefinitions)
{
    grpc::ServerContext context;
    google::protobuf::Empty request;
    // DummyWriter<VariableMetaData> writer;

    // Set up mock discipline variables
    mock_discipline->AddInput("x", {2, 3}, "m");
    mock_discipline->AddOutput("f", {4, 5}, "N");

    // Call GetVariableDefinitions
    grpc::Status status = server->GetVariableDefinitions(&context, &request, nullptr);

    // Verify response
    EXPECT_TRUE(status.ok());
}

// Test GetPartialDefinitions RPC
TEST_F(DisciplineServerTest, GetPartialDefinitions)
{
    grpc::ServerContext context;
    google::protobuf::Empty request;
    // DummyWriter<PartialsMetaData> writer;

    // Set up mock discipline partials
    mock_discipline->AddInput("x", {2}, "m");
    mock_discipline->AddOutput("f", {3}, "N");
    mock_discipline->DeclarePartials("f", "x");

    // Call GetPartialDefinitions
    grpc::Status status = server->GetPartialDefinitions(&context, &request, nullptr);

    // Verify response
    EXPECT_TRUE(status.ok());
}

// Test Setup RPC
TEST_F(DisciplineServerTest, Setup)
{
    grpc::ServerContext context;
    google::protobuf::Empty request;
    google::protobuf::Empty response;

    // Set up expectations
    EXPECT_CALL(*mock_discipline, Setup())
        .Times(1);

    // Call Setup
    grpc::Status status = server->Setup(&context, &request, &response);

    // Verify response
    EXPECT_TRUE(status.ok());
}