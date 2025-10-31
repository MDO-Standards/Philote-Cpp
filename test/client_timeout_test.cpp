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
#include <chrono>

#include "discipline_client.h"
#include "explicit.h"
#include "implicit.h"
#include "disciplines_mock.grpc.pb.h"

using ::testing::_;
using ::testing::Return;

// Test fixture for DisciplineClient timeout tests
class DisciplineClientTimeoutTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        client_ = std::make_shared<philote::DisciplineClient>();
        mock_stub_ = new philote::MockDisciplineServiceStub();
        client_->SetStub(std::unique_ptr<philote::DisciplineService::StubInterface>(mock_stub_));
    }

    philote::MockDisciplineServiceStub *mock_stub_ = nullptr;
    std::shared_ptr<philote::DisciplineClient> client_;
};

// Test fixture for ExplicitClient timeout tests
class ExplicitClientTimeoutTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        client_ = std::make_shared<philote::ExplicitClient>();
    }

    std::shared_ptr<philote::ExplicitClient> client_;
};

// Test fixture for ImplicitClient timeout tests
class ImplicitClientTimeoutTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        client_ = std::make_shared<philote::ImplicitClient>();
    }

    std::shared_ptr<philote::ImplicitClient> client_;
};

// ====================
// DisciplineClient Tests
// ====================

TEST_F(DisciplineClientTimeoutTest, DefaultTimeoutIs60Seconds)
{
    EXPECT_EQ(client_->GetRPCTimeout().count(), 60000);
}

TEST_F(DisciplineClientTimeoutTest, CanSetCustomTimeout)
{
    client_->SetRPCTimeout(std::chrono::milliseconds(30000));
    EXPECT_EQ(client_->GetRPCTimeout().count(), 30000);
}

TEST_F(DisciplineClientTimeoutTest, CanSetVeryShortTimeout)
{
    client_->SetRPCTimeout(std::chrono::milliseconds(100));
    EXPECT_EQ(client_->GetRPCTimeout().count(), 100);
}

TEST_F(DisciplineClientTimeoutTest, CanSetVeryLongTimeout)
{
    client_->SetRPCTimeout(std::chrono::milliseconds(300000));
    EXPECT_EQ(client_->GetRPCTimeout().count(), 300000);
}

TEST_F(DisciplineClientTimeoutTest, GetInfoTimeoutError)
{
    EXPECT_CALL(*mock_stub_, GetInfo(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "Timeout")));

    try
    {
        client_->GetInfo();
        FAIL() << "Expected std::runtime_error";
    }
    catch (const std::runtime_error &e)
    {
        std::string error_msg(e.what());
        EXPECT_TRUE(error_msg.find("RPC timeout after") != std::string::npos);
        EXPECT_TRUE(error_msg.find("60000ms") != std::string::npos);
    }
}

TEST_F(DisciplineClientTimeoutTest, SendStreamOptionsTimeoutError)
{
    EXPECT_CALL(*mock_stub_, SetStreamOptions(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "Timeout")));

    try
    {
        client_->SendStreamOptions();
        FAIL() << "Expected std::runtime_error";
    }
    catch (const std::runtime_error &e)
    {
        std::string error_msg(e.what());
        EXPECT_TRUE(error_msg.find("RPC timeout after") != std::string::npos);
        EXPECT_TRUE(error_msg.find("60000ms") != std::string::npos);
    }
}

TEST_F(DisciplineClientTimeoutTest, SendOptionsTimeoutError)
{
    philote::DisciplineOptions options;

    EXPECT_CALL(*mock_stub_, SetOptions(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "Timeout")));

    try
    {
        client_->SendOptions(options);
        FAIL() << "Expected std::runtime_error";
    }
    catch (const std::runtime_error &e)
    {
        std::string error_msg(e.what());
        EXPECT_TRUE(error_msg.find("RPC timeout after") != std::string::npos);
        EXPECT_TRUE(error_msg.find("60000ms") != std::string::npos);
    }
}

TEST_F(DisciplineClientTimeoutTest, SetupTimeoutError)
{
    EXPECT_CALL(*mock_stub_, Setup(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "Timeout")));

    try
    {
        client_->Setup();
        FAIL() << "Expected std::runtime_error";
    }
    catch (const std::runtime_error &e)
    {
        std::string error_msg(e.what());
        EXPECT_TRUE(error_msg.find("RPC timeout after") != std::string::npos);
        EXPECT_TRUE(error_msg.find("60000ms") != std::string::npos);
    }
}

TEST_F(DisciplineClientTimeoutTest, TimeoutErrorIncludesCustomTimeout)
{
    client_->SetRPCTimeout(std::chrono::milliseconds(5000));

    EXPECT_CALL(*mock_stub_, GetInfo(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "Timeout")));

    try
    {
        client_->GetInfo();
        FAIL() << "Expected std::runtime_error";
    }
    catch (const std::runtime_error &e)
    {
        std::string error_msg(e.what());
        EXPECT_TRUE(error_msg.find("RPC timeout after") != std::string::npos);
        EXPECT_TRUE(error_msg.find("5000ms") != std::string::npos);
    }
}

TEST_F(DisciplineClientTimeoutTest, NonTimeoutErrorsNotAffected)
{
    EXPECT_CALL(*mock_stub_, GetInfo(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::INTERNAL, "Internal error")));

    try
    {
        client_->GetInfo();
        FAIL() << "Expected std::runtime_error";
    }
    catch (const std::runtime_error &e)
    {
        std::string error_msg(e.what());
        EXPECT_TRUE(error_msg.find("RPC timeout") == std::string::npos);
        EXPECT_TRUE(error_msg.find("Failed to get discipline info") != std::string::npos);
    }
}

// ====================
// ExplicitClient Tests
// ====================

TEST_F(ExplicitClientTimeoutTest, InheritsDefaultTimeout)
{
    EXPECT_EQ(client_->GetRPCTimeout().count(), 60000);
}

TEST_F(ExplicitClientTimeoutTest, CanSetTimeout)
{
    client_->SetRPCTimeout(std::chrono::milliseconds(45000));
    EXPECT_EQ(client_->GetRPCTimeout().count(), 45000);
}

// ====================
// ImplicitClient Tests
// ====================

TEST_F(ImplicitClientTimeoutTest, InheritsDefaultTimeout)
{
    EXPECT_EQ(client_->GetRPCTimeout().count(), 60000);
}

TEST_F(ImplicitClientTimeoutTest, CanSetTimeout)
{
    client_->SetRPCTimeout(std::chrono::milliseconds(90000));
    EXPECT_EQ(client_->GetRPCTimeout().count(), 90000);
}
