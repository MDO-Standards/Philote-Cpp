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

using namespace philote;
using namespace philote::test;
using namespace testing;

// ============================================================================
// Mock Classes
// ============================================================================

// Mock for grpc::ServerReaderWriterInterface
class MockServerReaderWriter : public grpc::ServerReaderWriterInterface<philote::Array, philote::Array> {
public:
    MOCK_METHOD(bool, Read, (philote::Array*), (override));
    MOCK_METHOD(bool, Write, (const philote::Array&, grpc::WriteOptions), (override));
    MOCK_METHOD(void, SendInitialMetadata, (), (override));
    MOCK_METHOD(bool, NextMessageSize, (uint32_t*), (override));
};

// Mock ImplicitDiscipline for testing error conditions
class MockImplicitDiscipline : public ImplicitDiscipline {
public:
    MOCK_METHOD(void, Setup, (), (override));
    MOCK_METHOD(void, SetupPartials, (), (override));
    MOCK_METHOD(void, ComputeResiduals, (const Variables&, const Variables&, Variables&), (override));
    MOCK_METHOD(void, SolveResiduals, (const Variables&, Variables&), (override));
    MOCK_METHOD(void, ComputeResidualGradients, (const Variables&, const Variables&, Partials&), (override));
};

// ============================================================================
// Test Fixture
// ============================================================================

class ImplicitServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_ = std::make_unique<ImplicitServer>();
        context_ = std::make_unique<grpc::ServerContext>();
    }

    void TearDown() override {
        server_->UnlinkPointers();
        server_.reset();
        context_.reset();
    }

    // Helper to create a simple discipline with x input and y output
    std::unique_ptr<SimpleImplicitDiscipline> CreateSimpleDiscipline() {
        auto discipline = std::make_unique<SimpleImplicitDiscipline>();
        discipline->Initialize();
        discipline->Configure();
        discipline->Setup();
        discipline->SetupPartials();
        return discipline;
    }

    // Helper to create an input Array message
    philote::Array CreateInputArray(const std::string& name, const std::vector<double>& data) {
        philote::Array array;
        array.set_name(name);
        array.set_type(VariableType::kInput);
        array.set_start(0);
        array.set_end(data.empty() ? 0 : data.size() - 1);  // end is inclusive, so size-1
        for (double val : data) {
            array.add_data(val);
        }
        return array;
    }

    // Helper to create an output Array message
    philote::Array CreateOutputArray(const std::string& name, const std::vector<double>& data) {
        philote::Array array;
        array.set_name(name);
        array.set_type(VariableType::kOutput);
        array.set_start(0);
        array.set_end(data.empty() ? 0 : data.size() - 1);
        for (double val : data) {
            array.add_data(val);
        }
        return array;
    }

    std::unique_ptr<ImplicitServer> server_;
    std::unique_ptr<grpc::ServerContext> context_;
};

// ============================================================================
// Initialization and Pointer Management Tests
// ============================================================================

TEST_F(ImplicitServerTest, Initialization) {
    EXPECT_TRUE(server_ != nullptr);
}

TEST_F(ImplicitServerTest, LinkAndUnlinkPointers) {
    auto discipline = CreateSimpleDiscipline();

    // Link pointers
    EXPECT_NO_THROW(server_->LinkPointers(discipline.get()));

    // Unlink pointers
    EXPECT_NO_THROW(server_->UnlinkPointers());
}

TEST_F(ImplicitServerTest, MultipleLinkUnlink) {
    auto discipline1 = CreateSimpleDiscipline();
    auto discipline2 = CreateSimpleDiscipline();

    server_->LinkPointers(discipline1.get());
    server_->UnlinkPointers();
    server_->LinkPointers(discipline2.get());
    server_->UnlinkPointers();

    EXPECT_TRUE(true);  // Should not crash
}

// ============================================================================
// ComputeResiduals - Error Condition Tests
// ============================================================================

TEST_F(ImplicitServerTest, ComputeResidualsUnlinkedPointers) {
    auto stream = std::make_unique<MockServerReaderWriter>();

    // Don't link any discipline
    EXPECT_CALL(*stream, Read(_)).Times(0);

    grpc::Status status = server_->ComputeResiduals(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
    EXPECT_THAT(status.error_message(), HasSubstr("not linked"));
}

TEST_F(ImplicitServerTest, ComputeResidualsNullStream) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline.get());

    grpc::Status status = server_->ComputeResiduals(context_.get(), nullptr);

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_THAT(status.error_message(), HasSubstr("null"));
}

TEST_F(ImplicitServerTest, ComputeResidualsVariableNotFound) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving an unknown variable - server should fail immediately
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("unknown_variable");
            array->set_type(VariableType::kInput);
            array->add_data(1.0);
            return true;
        }));

    grpc::Status status = server_->ComputeResiduals(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_THAT(status.error_message(), HasSubstr("not found"));
}

TEST_F(ImplicitServerTest, ComputeResidualsInvalidVariableType) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving wrong type
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("x");  // This is an input
            array->set_type(VariableType::kOutput);  // Wrong type
            array->add_data(1.0);
            return true;
        }));

    grpc::Status status = server_->ComputeResiduals(context_.get(), stream.get());

    // Should fail at invalid type or not found
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ImplicitServerTest, ComputeResidualsComputeThrows) {
    auto mock_discipline = std::make_unique<MockImplicitDiscipline>();

    // Setup metadata
    VariableMetaData x_meta, y_meta;
    x_meta.set_name("x");
    x_meta.set_type(kInput);
    x_meta.add_shape(1);
    y_meta.set_name("y");
    y_meta.set_type(kOutput);
    y_meta.add_shape(1);

    mock_discipline->var_meta().push_back(x_meta);
    mock_discipline->var_meta().push_back(y_meta);

    // Mock ComputeResiduals to throw
    EXPECT_CALL(*mock_discipline, ComputeResiduals(_, _, _))
        .WillOnce(Throw(std::runtime_error("ComputeResiduals failed")));

    server_->LinkPointers(mock_discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("x");
            array->set_type(VariableType::kInput);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("y");
            array->set_type(VariableType::kOutput);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Return(false));

    grpc::Status status = server_->ComputeResiduals(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);
    EXPECT_THAT(status.error_message(), HasSubstr("residual"));
}

// ============================================================================
// ComputeResiduals - Success Tests
// ============================================================================

TEST_F(ImplicitServerTest, ComputeResidualsSimpleScalar) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving inputs and outputs (x=2.0, y=4.0)
    // Residual: R = x^2 - y = 4 - 4 = 0
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("x");
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(0);
            array->add_data(2.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("y");
            array->set_type(VariableType::kOutput);
            array->set_start(0);
            array->set_end(0);
            array->add_data(4.0);
            return true;
        }))
        .WillOnce(Return(false));  // End of input stream

    // Mock sending residual (should be near 0)
    EXPECT_CALL(*stream, Write(_, _))
        .WillOnce(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            EXPECT_EQ(array.name(), "y");
            EXPECT_EQ(array.data_size(), 1);
            EXPECT_NEAR(array.data(0), 0.0, 1e-10);
            return true;
        }));

    grpc::Status status = server_->ComputeResiduals(context_.get(), stream.get());
    EXPECT_TRUE(status.ok());
}

TEST_F(ImplicitServerTest, ComputeResidualsMultiOutput) {
    auto discipline = std::make_unique<MultiResidualDiscipline>();
    discipline->Initialize();
    discipline->Configure();
    discipline->Setup();
    discipline->SetupPartials();

    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving inputs (sum=8.0, product=15.0) and outputs (x=5.0, y=3.0)
    // R1 = x + y - sum = 5 + 3 - 8 = 0
    // R2 = x * y - product = 5 * 3 - 15 = 0
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("sum");
            array->set_type(VariableType::kInput);
            array->add_data(8.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("product");
            array->set_type(VariableType::kInput);
            array->add_data(15.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("x");
            array->set_type(VariableType::kOutput);
            array->add_data(5.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("y");
            array->set_type(VariableType::kOutput);
            array->add_data(3.0);
            return true;
        }))
        .WillOnce(Return(false));

    // Mock sending 2 residuals (both should be near 0)
    EXPECT_CALL(*stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            if (array.name() == "x") {
                EXPECT_NEAR(array.data(0), 0.0, 1e-10);
            } else if (array.name() == "y") {
                EXPECT_NEAR(array.data(0), 0.0, 1e-10);
            }
            return true;
        }));

    grpc::Status status = server_->ComputeResiduals(context_.get(), stream.get());

    EXPECT_TRUE(status.ok());
}

TEST_F(ImplicitServerTest, ComputeResidualsVectorData) {
    auto discipline = std::make_unique<VectorizedImplicitDiscipline>(2, 3);
    discipline->Initialize();
    discipline->Configure();
    discipline->Setup();
    discipline->SetupPartials();

    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving vector/matrix inputs
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("A");  // 2x3 matrix = 6 elements
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(5);
            array->add_data(1.0);
            array->add_data(2.0);
            array->add_data(3.0);
            array->add_data(4.0);
            array->add_data(5.0);
            array->add_data(6.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("x");  // 3-vector
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(2);
            array->add_data(1.0);
            array->add_data(1.0);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("b");  // 2-vector
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(1);
            array->add_data(1.0);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("y");  // 2-vector output
            array->set_type(VariableType::kOutput);
            array->set_start(0);
            array->set_end(1);
            // Correct values: y = A*x + b = [7, 16]
            array->add_data(7.0);
            array->add_data(16.0);
            return true;
        }))
        .WillOnce(Return(false));

    // Mock sending residuals (should be near 0 for correct y)
    EXPECT_CALL(*stream, Write(_, _))
        .WillOnce(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            EXPECT_EQ(array.name(), "y");
            EXPECT_EQ(array.data_size(), 2);
            EXPECT_NEAR(array.data(0), 0.0, 1e-10);
            EXPECT_NEAR(array.data(1), 0.0, 1e-10);
            return true;
        }));

    grpc::Status status = server_->ComputeResiduals(context_.get(), stream.get());

    EXPECT_TRUE(status.ok());
}

// ============================================================================
// SolveResiduals - Error Condition Tests
// ============================================================================

TEST_F(ImplicitServerTest, SolveResidualsUnlinkedPointers) {
    auto stream = std::make_unique<MockServerReaderWriter>();

    EXPECT_CALL(*stream, Read(_)).Times(0);

    grpc::Status status = server_->SolveResiduals(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
}

TEST_F(ImplicitServerTest, SolveResidualsNullStream) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline.get());

    grpc::Status status = server_->SolveResiduals(context_.get(), nullptr);

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ImplicitServerTest, SolveResidualsVariableNotFound) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("invalid_var");
            array->set_type(VariableType::kInput);
            array->add_data(1.0);
            return true;
        }));

    grpc::Status status = server_->SolveResiduals(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ImplicitServerTest, SolveResidualsSolveThrows) {
    auto mock_discipline = std::make_unique<MockImplicitDiscipline>();

    // Setup metadata
    VariableMetaData x_meta, y_meta;
    x_meta.set_name("x");
    x_meta.set_type(kInput);
    x_meta.add_shape(1);
    y_meta.set_name("y");
    y_meta.set_type(kOutput);
    y_meta.add_shape(1);

    mock_discipline->var_meta().push_back(x_meta);
    mock_discipline->var_meta().push_back(y_meta);

    // Mock SolveResiduals to throw
    EXPECT_CALL(*mock_discipline, SolveResiduals(_, _))
        .WillOnce(Throw(std::runtime_error("SolveResiduals failed")));

    server_->LinkPointers(mock_discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("x");
            array->set_type(VariableType::kInput);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Return(false));

    grpc::Status status = server_->SolveResiduals(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);
    EXPECT_THAT(status.error_message(), HasSubstr("solve"));
}

// ============================================================================
// SolveResiduals - Success Tests
// ============================================================================

TEST_F(ImplicitServerTest, SolveResidualsSimpleScalar) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving input only (x=3.0)
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("x");
            array->set_type(VariableType::kInput);
            array->add_data(3.0);
            return true;
        }))
        .WillOnce(Return(false));

    // Mock sending solved output (y = x^2 = 9.0)
    EXPECT_CALL(*stream, Write(_, _))
        .WillOnce(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            EXPECT_EQ(array.name(), "y");
            EXPECT_EQ(array.data_size(), 1);
            EXPECT_DOUBLE_EQ(array.data(0), 9.0);
            return true;
        }));

    grpc::Status status = server_->SolveResiduals(context_.get(), stream.get());
    EXPECT_TRUE(status.ok());
}

TEST_F(ImplicitServerTest, SolveResidualsMultiOutput) {
    auto discipline = std::make_unique<MultiResidualDiscipline>();
    discipline->Initialize();
    discipline->Configure();
    discipline->Setup();
    discipline->SetupPartials();

    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving inputs (sum=8.0, product=15.0)
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("sum");
            array->set_type(VariableType::kInput);
            array->add_data(8.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("product");
            array->set_type(VariableType::kInput);
            array->add_data(15.0);
            return true;
        }))
        .WillOnce(Return(false));

    // Mock sending 2 outputs
    EXPECT_CALL(*stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            if (array.name() == "x") {
                EXPECT_DOUBLE_EQ(array.data(0), 5.0);
            } else if (array.name() == "y") {
                EXPECT_DOUBLE_EQ(array.data(0), 3.0);
            }
            return true;
        }));

    grpc::Status status = server_->SolveResiduals(context_.get(), stream.get());

    EXPECT_TRUE(status.ok());
}

TEST_F(ImplicitServerTest, SolveResidualsVectorData) {
    auto discipline = std::make_unique<VectorizedImplicitDiscipline>(2, 3);
    discipline->Initialize();
    discipline->Configure();
    discipline->Setup();
    discipline->SetupPartials();

    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving vector/matrix inputs
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("A");
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(5);
            array->add_data(1.0);
            array->add_data(2.0);
            array->add_data(3.0);
            array->add_data(4.0);
            array->add_data(5.0);
            array->add_data(6.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("x");
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(2);
            array->add_data(1.0);
            array->add_data(1.0);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("b");
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(1);
            array->add_data(1.0);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Return(false));

    // Mock sending output y = A*x + b
    EXPECT_CALL(*stream, Write(_, _))
        .WillOnce(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            EXPECT_EQ(array.name(), "y");
            EXPECT_EQ(array.data_size(), 2);
            // y[0] = 1*1 + 2*1 + 3*1 + 1 = 7
            EXPECT_DOUBLE_EQ(array.data(0), 7.0);
            // y[1] = 4*1 + 5*1 + 6*1 + 1 = 16
            EXPECT_DOUBLE_EQ(array.data(1), 16.0);
            return true;
        }));

    grpc::Status status = server_->SolveResiduals(context_.get(), stream.get());

    EXPECT_TRUE(status.ok());
}

// ============================================================================
// ComputeResidualGradients - Error Condition Tests
// ============================================================================

TEST_F(ImplicitServerTest, ComputeResidualGradientsUnlinkedPointers) {
    auto stream = std::make_unique<MockServerReaderWriter>();

    EXPECT_CALL(*stream, Read(_)).Times(0);

    grpc::Status status = server_->ComputeResidualGradients(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
}

TEST_F(ImplicitServerTest, ComputeResidualGradientsNullStream) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline.get());

    grpc::Status status = server_->ComputeResidualGradients(context_.get(), nullptr);

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ImplicitServerTest, ComputeResidualGradientsVariableNotFound) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("invalid_var");
            array->set_type(VariableType::kInput);
            array->add_data(1.0);
            return true;
        }));

    grpc::Status status = server_->ComputeResidualGradients(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ImplicitServerTest, ComputeResidualGradientsComputeThrows) {
    auto mock_discipline = std::make_unique<MockImplicitDiscipline>();

    // Setup metadata
    VariableMetaData x_meta, y_meta;
    x_meta.set_name("x");
    x_meta.set_type(kInput);
    x_meta.add_shape(1);
    y_meta.set_name("y");
    y_meta.set_type(kOutput);
    y_meta.add_shape(1);

    mock_discipline->var_meta().push_back(x_meta);
    mock_discipline->var_meta().push_back(y_meta);

    // Setup partials metadata
    PartialsMetaData p1, p2;
    p1.set_name("y");
    p1.set_subname("x");
    p2.set_name("y");
    p2.set_subname("y");
    mock_discipline->partials_meta().push_back(p1);
    mock_discipline->partials_meta().push_back(p2);

    // Mock ComputeResidualGradients to throw
    EXPECT_CALL(*mock_discipline, ComputeResidualGradients(_, _, _))
        .WillOnce(Throw(std::runtime_error("ComputeResidualGradients failed")));

    server_->LinkPointers(mock_discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("x");
            array->set_type(VariableType::kInput);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("y");
            array->set_type(VariableType::kOutput);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Return(false));

    grpc::Status status = server_->ComputeResidualGradients(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);
    EXPECT_THAT(status.error_message(), HasSubstr("gradient"));
}

// ============================================================================
// ComputeResidualGradients - Success Tests
// ============================================================================

TEST_F(ImplicitServerTest, ComputeResidualGradientsSimpleScalar) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving inputs and outputs (x=3.0, y=9.0)
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("x");
            array->set_type(VariableType::kInput);
            array->add_data(3.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("y");
            array->set_type(VariableType::kOutput);
            array->add_data(9.0);
            return true;
        }))
        .WillOnce(Return(false));

    // Mock sending 2 partials: dR/dx = 2*3 = 6, dR/dy = -1
    EXPECT_CALL(*stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            if (array.name() == "y" && array.subname() == "x") {
                EXPECT_DOUBLE_EQ(array.data(0), 6.0);
            } else if (array.name() == "y" && array.subname() == "y") {
                EXPECT_DOUBLE_EQ(array.data(0), -1.0);
            }
            return true;
        }));

    grpc::Status status = server_->ComputeResidualGradients(context_.get(), stream.get());

    EXPECT_TRUE(status.ok());
}

TEST_F(ImplicitServerTest, ComputeResidualGradientsMultiplePartials) {
    auto discipline = std::make_unique<MultiResidualDiscipline>();
    discipline->Initialize();
    discipline->Configure();
    discipline->Setup();
    discipline->SetupPartials();

    server_->LinkPointers(discipline.get());

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving inputs and outputs
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("sum");
            array->add_data(8.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("product");
            array->add_data(15.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("x");
            array->set_type(VariableType::kOutput);
            array->add_data(5.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();
            array->set_name("y");
            array->set_type(VariableType::kOutput);
            array->add_data(3.0);
            return true;
        }))
        .WillOnce(Return(false));

    // Mock sending 8 partials (2 residuals x 4 variables each)
    EXPECT_CALL(*stream, Write(_, _))
        .Times(8)
        .WillRepeatedly(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            // Verify expected partial values based on MultiResidualDiscipline
            if (array.name() == "x" && array.subname() == "sum") {
                EXPECT_DOUBLE_EQ(array.data(0), -1.0);
            } else if (array.name() == "x" && array.subname() == "product") {
                EXPECT_DOUBLE_EQ(array.data(0), 0.0);
            } else if (array.name() == "x" && array.subname() == "x") {
                EXPECT_DOUBLE_EQ(array.data(0), 1.0);
            } else if (array.name() == "x" && array.subname() == "y") {
                EXPECT_DOUBLE_EQ(array.data(0), 1.0);
            } else if (array.name() == "y" && array.subname() == "sum") {
                EXPECT_DOUBLE_EQ(array.data(0), 0.0);
            } else if (array.name() == "y" && array.subname() == "product") {
                EXPECT_DOUBLE_EQ(array.data(0), -1.0);
            } else if (array.name() == "y" && array.subname() == "x") {
                EXPECT_DOUBLE_EQ(array.data(0), 3.0);  // dR2/dx = y = 3
            } else if (array.name() == "y" && array.subname() == "y") {
                EXPECT_DOUBLE_EQ(array.data(0), 5.0);  // dR2/dy = x = 5
            }
            return true;
        }));

    grpc::Status status = server_->ComputeResidualGradients(context_.get(), stream.get());

    EXPECT_TRUE(status.ok());
}

// ============================================================================
// Destructor Test
// ============================================================================

TEST_F(ImplicitServerTest, DestructorUnlinksPointers) {
    auto discipline = CreateSimpleDiscipline();

    {
        auto temp_server = std::make_unique<ImplicitServer>();
        temp_server->LinkPointers(discipline.get());
        // temp_server goes out of scope here
    }

    // If destructor didn't properly unlink, subsequent use would be dangerous
    // This test just ensures no crash occurs
    EXPECT_TRUE(true);
}
