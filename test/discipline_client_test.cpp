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
        client_ = std::make_unique<philote::DisciplineClient>();
        mock_stub_ = new philote::MockDisciplineServiceStub();
        // Inject the mock stub into the client
        client_->SetStub(std::unique_ptr<philote::DisciplineService::StubInterface>(mock_stub_));
    }

    philote::MockDisciplineServiceStub *mock_stub_ = nullptr;
    std::unique_ptr<philote::DisciplineClient> client_;
};

// Test constructor and initial state
TEST_F(DisciplineClientTest, ConstructorInitialization)
{
    philote::DisciplineClient client;
    EXPECT_EQ(client.GetVariableNames().size(), 0);
    EXPECT_EQ(client.GetPartialsMeta().size(), 0);
}

// Test GetInfo
TEST_F(DisciplineClientTest, GetInfo)
{
    philote::DisciplineProperties properties;
    properties.set_name("TestDiscipline");
    properties.set_continuous(true);

    EXPECT_CALL(*mock_stub_, GetInfo(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(DoAll(SetArgPointee<2>(properties), Return(grpc::Status::OK)));

    client_->GetInfo();
}

// Test SendStreamOptions
TEST_F(DisciplineClientTest, SendStreamOptions)
{
    EXPECT_CALL(*mock_stub_, SetStreamOptions(_, _, _))
        .WillOnce(Return(grpc::Status::OK));

    client_->SendStreamOptions();
}

// Test SendOptions
TEST_F(DisciplineClientTest, SendOptions)
{
    philote::DisciplineOptions options;

    EXPECT_CALL(*mock_stub_, SetOptions(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(Return(grpc::Status::OK));

    client_->SendOptions(options);
}

// Test Setup
TEST_F(DisciplineClientTest, Setup)
{
    EXPECT_CALL(*mock_stub_, Setup(_, _, _))
        .WillOnce(Return(grpc::Status::OK));

    client_->Setup();
}

// Test GetVariableDefinitions
TEST_F(DisciplineClientTest, GetVariableDefinitions)
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
    auto vars = client_->GetVariableNames();
    EXPECT_EQ(vars.size(), 1);
    EXPECT_EQ(vars[0], "test_var");
}

// Test GetPartialDefinitions
TEST_F(DisciplineClientTest, GetPartialDefinitions)
{
    auto mock_reader_ptr = new MockReader<philote::PartialsMetaData>();
    philote::PartialsMetaData partial;
    partial.set_name("test_partial");
    partial.set_subname("test_var");

    EXPECT_CALL(*mock_stub_, GetPartialDefinitionsRaw(_, _))
        .WillOnce(Return(mock_reader_ptr));
    EXPECT_CALL(*mock_reader_ptr, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(partial), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_reader_ptr, Finish())
        .WillOnce(Return(grpc::Status::OK));

    client_->GetPartialDefinitions();
    auto partials = client_->GetPartialsMeta();
    EXPECT_EQ(partials.size(), 1);
    EXPECT_EQ(partials[0].name(), "test_partial");
    EXPECT_EQ(partials[0].subname(), "test_var");
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