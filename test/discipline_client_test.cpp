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
#include <grpcpp/impl/channel_interface.h>
#include <grpcpp/support/sync_stream.h>

#include <google/protobuf/struct.pb.h>
#include <grpcpp/support/async_stream.h>

#include "discipline_client.h"
#include "disciplines_mock.grpc.pb.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;

// Mock gRPC reader
template <typename T>
class MockReader : public grpc::ClientReaderInterface<T>
{
public:
    MOCK_METHOD(bool, NextMessageSize, (uint32_t *), (override));
    MOCK_METHOD(bool, Read, (T *), (override));
    MOCK_METHOD(void, WaitForInitialMetadata, (), (override));
    MOCK_METHOD(grpc::Status, Finish, (), (override));
};

class DisciplineClientTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        client_ = std::make_shared<philote::DisciplineClient>();
        mock_stub_ = new philote::MockDisciplineServiceStub();
        // Inject the mock stub into the client
        client_->SetStub(std::unique_ptr<philote::DisciplineService::StubInterface>(mock_stub_));
    }

    philote::MockDisciplineServiceStub *mock_stub_ = nullptr;
    std::shared_ptr<philote::DisciplineClient> client_;
};

// Test constructor and initial state
TEST_F(DisciplineClientTest, ConstructorInitialization)
{
    philote::DisciplineClient client;
    EXPECT_EQ(client.GetVariableNames().size(), 0);
    EXPECT_EQ(client.GetPartialsMeta().size(), 0);
}

// Test GetInfo with error handling
TEST_F(DisciplineClientTest, GetInfo)
{
    philote::DisciplineProperties properties;
    properties.set_name("TestDiscipline");
    properties.set_continuous(true);

    EXPECT_CALL(*mock_stub_, GetInfo(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(DoAll(SetArgPointee<2>(properties), Return(grpc::Status::OK)));

    client_->GetInfo();
}

// Test GetInfo with error
TEST_F(DisciplineClientTest, GetInfoError)
{
    EXPECT_CALL(*mock_stub_, GetInfo(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::INTERNAL, "Internal error")));

    EXPECT_THROW(client_->GetInfo(), std::runtime_error);
}

// Test SendStreamOptions
TEST_F(DisciplineClientTest, SendStreamOptions)
{
    EXPECT_CALL(*mock_stub_, SetStreamOptions(_, _, _))
        .WillOnce(Return(grpc::Status::OK));

    client_->SendStreamOptions();
}

// Test SendStreamOptions with error
TEST_F(DisciplineClientTest, SendStreamOptionsError)
{
    EXPECT_CALL(*mock_stub_, SetStreamOptions(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::INTERNAL, "Internal error")));

    EXPECT_THROW(client_->SendStreamOptions(), std::runtime_error);
}

// Test SendOptions
TEST_F(DisciplineClientTest, SendOptions)
{
    philote::DisciplineOptions options;
    (*options.mutable_options()->mutable_fields())["test_key"].set_string_value("test_value");

    EXPECT_CALL(*mock_stub_, SetOptions(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(Return(grpc::Status::OK));

    client_->SendOptions(options);
}

// Test SendOptions with error
TEST_F(DisciplineClientTest, SendOptionsError)
{
    philote::DisciplineOptions options;
    EXPECT_CALL(*mock_stub_, SetOptions(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::INTERNAL, "Internal error")));

    EXPECT_THROW(client_->SendOptions(options), std::runtime_error);
}

// Test Setup
TEST_F(DisciplineClientTest, Setup)
{
    EXPECT_CALL(*mock_stub_, Setup(_, _, _))
        .WillOnce(Return(grpc::Status::OK));

    client_->Setup();
}

// Test Setup with error
TEST_F(DisciplineClientTest, SetupError)
{
    EXPECT_CALL(*mock_stub_, Setup(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::INTERNAL, "Internal error")));

    EXPECT_THROW(client_->Setup(), std::runtime_error);
}

// Test GetVariableDefinitions
TEST_F(DisciplineClientTest, GetVariableDefinitions)
{
    auto mock_reader_ptr = new MockReader<philote::VariableMetaData>();
    philote::VariableMetaData var1, var2;
    var1.set_name("test_var1");
    var1.set_type(philote::kInput);
    var2.set_name("test_var2");
    var2.set_type(philote::kOutput);

    EXPECT_CALL(*mock_stub_, GetVariableDefinitionsRaw(_, _))
        .WillOnce(Return(mock_reader_ptr));
    EXPECT_CALL(*mock_reader_ptr, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(var1), Return(true)))
        .WillOnce(DoAll(SetArgPointee<0>(var2), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_reader_ptr, Finish())
        .WillOnce(Return(grpc::Status::OK));

    client_->GetVariableDefinitions();
    auto vars = client_->GetVariableNames();
    EXPECT_EQ(vars.size(), 2);
    EXPECT_EQ(vars[0], "test_var1");
    EXPECT_EQ(vars[1], "test_var2");
}

// Test GetVariableDefinitions with error
TEST_F(DisciplineClientTest, GetVariableDefinitionsError)
{
    auto mock_reader_ptr = new MockReader<philote::VariableMetaData>();
    EXPECT_CALL(*mock_stub_, GetVariableDefinitionsRaw(_, _))
        .WillOnce(Return(mock_reader_ptr));
    EXPECT_CALL(*mock_reader_ptr, Read(_))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_reader_ptr, Finish())
        .WillOnce(Return(grpc::Status(grpc::StatusCode::INTERNAL, "Internal error")));

    EXPECT_THROW(client_->GetVariableDefinitions(), std::runtime_error);
}

// Test GetPartialDefinitions
TEST_F(DisciplineClientTest, GetPartialDefinitions)
{
    auto mock_reader_ptr = new MockReader<philote::PartialsMetaData>();
    philote::PartialsMetaData partial1, partial2;
    partial1.set_name("test_partial1");
    partial1.set_subname("test_var1");
    partial2.set_name("test_partial2");
    partial2.set_subname("test_var2");

    EXPECT_CALL(*mock_stub_, GetPartialDefinitionsRaw(_, _))
        .WillOnce(Return(mock_reader_ptr));
    EXPECT_CALL(*mock_reader_ptr, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(partial1), Return(true)))
        .WillOnce(DoAll(SetArgPointee<0>(partial2), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_reader_ptr, Finish())
        .WillOnce(Return(grpc::Status::OK));

    client_->GetPartialDefinitions();
    auto partials = client_->GetPartialsMeta();
    EXPECT_EQ(partials.size(), 2);
    EXPECT_EQ(partials[0].name(), "test_partial1");
    EXPECT_EQ(partials[0].subname(), "test_var1");
    EXPECT_EQ(partials[1].name(), "test_partial2");
    EXPECT_EQ(partials[1].subname(), "test_var2");
}

// Test GetPartialDefinitions with error
TEST_F(DisciplineClientTest, GetPartialDefinitionsError)
{
    auto mock_reader_ptr = new MockReader<philote::PartialsMetaData>();
    EXPECT_CALL(*mock_stub_, GetPartialDefinitionsRaw(_, _))
        .WillOnce(Return(mock_reader_ptr));
    EXPECT_CALL(*mock_reader_ptr, Read(_))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_reader_ptr, Finish())
        .WillOnce(Return(grpc::Status(grpc::StatusCode::INTERNAL, "Internal error")));

    EXPECT_THROW(client_->GetPartialDefinitions(), std::runtime_error);
}

// Test GetVariableMeta
TEST_F(DisciplineClientTest, GetVariableMeta)
{
    auto mock_reader_ptr = new MockReader<philote::VariableMetaData>();
    philote::VariableMetaData var;
    var.set_name("test_var");
    var.set_type(philote::kInput);

    EXPECT_CALL(*mock_stub_, GetVariableDefinitionsRaw(_, _))
        .WillOnce(Return(mock_reader_ptr));
    EXPECT_CALL(*mock_reader_ptr, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(var), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_reader_ptr, Finish())
        .WillOnce(Return(grpc::Status::OK));

    client_->GetVariableDefinitions();
    auto meta = client_->GetVariableMeta("test_var");
    EXPECT_EQ(meta.name(), "test_var");
    EXPECT_EQ(meta.type(), philote::kInput);
}

// Test GetVariableMeta with non-existent variable
TEST_F(DisciplineClientTest, GetVariableMetaNonExistent)
{
    auto mock_reader_ptr = new MockReader<philote::VariableMetaData>();
    philote::VariableMetaData var;
    var.set_name("test_var");
    var.set_type(philote::kInput);

    EXPECT_CALL(*mock_stub_, GetVariableDefinitionsRaw(_, _))
        .WillOnce(Return(mock_reader_ptr));
    EXPECT_CALL(*mock_reader_ptr, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(var), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_reader_ptr, Finish())
        .WillOnce(Return(grpc::Status::OK));

    client_->GetVariableDefinitions();
    EXPECT_THROW(client_->GetVariableMeta("non_existent_var"), std::runtime_error);
}