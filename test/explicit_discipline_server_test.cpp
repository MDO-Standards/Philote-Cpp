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

// Mock ExplicitDiscipline for testing error conditions
class MockExplicitDiscipline : public ExplicitDiscipline {
public:
    MOCK_METHOD(void, Setup, (), (override));
    MOCK_METHOD(void, SetupPartials, (), (override));
    MOCK_METHOD(void, Compute, (const Variables&, Variables&), (override));
    MOCK_METHOD(void, ComputePartials, (const Variables&, Partials&), (override));
};

// ============================================================================
// Test Fixture
// ============================================================================

class ExplicitServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_ = std::make_unique<ExplicitServer>();
        context_ = std::make_unique<grpc::ServerContext>();
    }

    void TearDown() override {
        server_->UnlinkPointers();
        server_.reset();
        context_.reset();
    }

    // Helper to create a simple discipline with x input and y output
    std::shared_ptr<ParaboloidDiscipline> CreateSimpleDiscipline() {
        auto discipline = std::make_shared<ParaboloidDiscipline>();
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

    std::unique_ptr<ExplicitServer> server_;
    std::unique_ptr<grpc::ServerContext> context_;
};

// ============================================================================
// Initialization and Pointer Management Tests
// ============================================================================

TEST_F(ExplicitServerTest, Initialization) {
    EXPECT_TRUE(server_ != nullptr);
}

TEST_F(ExplicitServerTest, LinkAndUnlinkPointers) {
    auto discipline = CreateSimpleDiscipline();

    // Link pointers
    EXPECT_NO_THROW(server_->LinkPointers(discipline));

    // Unlink pointers
    EXPECT_NO_THROW(server_->UnlinkPointers());
}

TEST_F(ExplicitServerTest, MultipleLinkUnlink) {
    auto discipline1 = CreateSimpleDiscipline();
    auto discipline2 = CreateSimpleDiscipline();

    server_->LinkPointers(discipline1);
    server_->UnlinkPointers();
    server_->LinkPointers(discipline2);
    server_->UnlinkPointers();

    EXPECT_TRUE(true);  // Should not crash
}

// ============================================================================
// ComputeFunction - Error Condition Tests
// ============================================================================

TEST_F(ExplicitServerTest, ComputeFunctionUnlinkedPointers) {
    auto stream = std::make_unique<MockServerReaderWriter>();

    // Don't link any discipline
    EXPECT_CALL(*stream, Read(_)).Times(0);

    grpc::Status status = server_->ComputeFunctionForTesting(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
    EXPECT_THAT(status.error_message(), HasSubstr("not linked"));
}

TEST_F(ExplicitServerTest, ComputeFunctionNullStream) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline);

    grpc::Status status = server_->ComputeFunction(context_.get(), nullptr);

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_THAT(status.error_message(), HasSubstr("null"));
}

TEST_F(ExplicitServerTest, ComputeFunctionVariableNotFound) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline);

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving an unknown variable - server should fail immediately
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("unknown_variable");
            array->set_type(VariableType::kInput);
            array->add_data(1.0);
            return true;
        }));

    grpc::Status status = server_->ComputeFunctionForTesting(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_THAT(status.error_message(), HasSubstr("not found"));
}

TEST_F(ExplicitServerTest, ComputeFunctionInvalidVariableType) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline);

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving an output variable as input (invalid) - server should fail immediately
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("f");  // This is an output in ParaboloidDiscipline
            array->set_type(VariableType::kOutput);  // Wrong type
            array->add_data(1.0);
            return true;
        }));

    grpc::Status status = server_->ComputeFunctionForTesting(context_.get(), stream.get());

    // Should fail either at variable not found or invalid type
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

// ============================================================================
// ComputeFunction - Success Tests
// ============================================================================

TEST_F(ExplicitServerTest, ComputeFunctionSimpleScalar) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline);

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving inputs (x=3.0, y=4.0)
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("x");
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(0);  // For single element, end=0
            array->add_data(3.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("y");
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(0);  // For single element, end=0
            array->add_data(4.0);
            return true;
        }))
        .WillOnce(Return(false));  // End of input stream

    // Mock sending output (f = 3^2 + 4^2 = 25)
    EXPECT_CALL(*stream, Write(_, _))
        .WillOnce(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            EXPECT_EQ(array.name(), "f");
            EXPECT_EQ(array.data_size(), 1);
            EXPECT_DOUBLE_EQ(array.data(0), 25.0);
            return true;
        }));

    grpc::Status status = server_->ComputeFunctionForTesting(context_.get(), stream.get());
    EXPECT_TRUE(status.ok());
}

TEST_F(ExplicitServerTest, ComputeFunctionMultiOutput) {
    auto discipline = std::make_shared<MultiOutputDiscipline>();
    discipline->Initialize();
    discipline->Configure();
    discipline->Setup();
    discipline->SetupPartials();

    server_->LinkPointers(discipline);

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving inputs (x=5.0, y=3.0)
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("x");
            array->set_type(VariableType::kInput);
            array->add_data(5.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("y");
            array->set_type(VariableType::kInput);
            array->add_data(3.0);
            return true;
        }))
        .WillOnce(Return(false));

    // Mock sending 3 outputs: sum=8, product=15, difference=2
    EXPECT_CALL(*stream, Write(_, _))
        .Times(3)
        .WillRepeatedly(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            if (array.name() == "sum") {
                EXPECT_DOUBLE_EQ(array.data(0), 8.0);
            } else if (array.name() == "product") {
                EXPECT_DOUBLE_EQ(array.data(0), 15.0);
            } else if (array.name() == "difference") {
                EXPECT_DOUBLE_EQ(array.data(0), 2.0);
            }
            return true;
        }));

    grpc::Status status = server_->ComputeFunctionForTesting(context_.get(), stream.get());

    EXPECT_TRUE(status.ok());
}

TEST_F(ExplicitServerTest, ComputeFunctionVectorData) {
    auto discipline = std::make_shared<VectorizedDiscipline>(2, 3);
    discipline->Initialize();
    discipline->Configure();
    discipline->Setup();
    discipline->SetupPartials();

    server_->LinkPointers(discipline);

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving vector/matrix inputs
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("A");  // 2x3 matrix = 6 elements
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(5);  // 6 elements: end = size - 1 = 5
            array->add_data(1.0);
            array->add_data(2.0);
            array->add_data(3.0);
            array->add_data(4.0);
            array->add_data(5.0);
            array->add_data(6.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("x");  // 3-vector = 3 elements
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(2);  // 3 elements: end = size - 1 = 2
            array->add_data(1.0);
            array->add_data(1.0);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("b");  // 2-vector = 2 elements
            array->set_type(VariableType::kInput);
            array->set_start(0);
            array->set_end(1);  // 2 elements: end = size - 1 = 1
            array->add_data(1.0);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Return(false));

    // Mock sending output z = A*x + b (sent in one chunk with default chunk_size=1000)
    EXPECT_CALL(*stream, Write(_, _))
        .WillOnce(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            EXPECT_EQ(array.name(), "z");
            EXPECT_EQ(array.data_size(), 2);
            // z[0] = 1*1 + 2*1 + 3*1 + 1 = 7
            EXPECT_DOUBLE_EQ(array.data(0), 7.0);
            // z[1] = 4*1 + 5*1 + 6*1 + 1 = 16
            EXPECT_DOUBLE_EQ(array.data(1), 16.0);
            return true;
        }));

    grpc::Status status = server_->ComputeFunctionForTesting(context_.get(), stream.get());

    EXPECT_TRUE(status.ok());
}

TEST_F(ExplicitServerTest, ComputeFunctionComputeThrows) {
    auto mock_discipline = std::make_shared<MockExplicitDiscipline>();

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

    // Mock Compute to throw
    EXPECT_CALL(*mock_discipline, Compute(_, _))
        .WillOnce(Throw(std::runtime_error("Compute failed")));

    server_->LinkPointers(mock_discipline);

    auto stream = std::make_unique<MockServerReaderWriter>();

    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("x");
            array->set_type(VariableType::kInput);
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Return(false));

    grpc::Status status = server_->ComputeFunctionForTesting(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);
    EXPECT_THAT(status.error_message(), HasSubstr("compute"));
}

// ============================================================================
// ComputeGradient - Error Condition Tests
// ============================================================================

TEST_F(ExplicitServerTest, ComputeGradientUnlinkedPointers) {
    auto stream = std::make_unique<MockServerReaderWriter>();

    EXPECT_CALL(*stream, Read(_)).Times(0);

    grpc::Status status = server_->ComputeGradientForTesting(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
}

TEST_F(ExplicitServerTest, ComputeGradientNullStream) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline);

    grpc::Status status = server_->ComputeGradient(context_.get(), nullptr);

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ExplicitServerTest, ComputeGradientVariableNotFound) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline);

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving an invalid variable - server should fail immediately
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("invalid_var");
            array->set_type(VariableType::kInput);
            array->add_data(1.0);
            return true;
        }));

    grpc::Status status = server_->ComputeGradientForTesting(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

// ============================================================================
// ComputeGradient - Success Tests
// ============================================================================

TEST_F(ExplicitServerTest, ComputeGradientSimpleScalar) {
    auto discipline = CreateSimpleDiscipline();
    server_->LinkPointers(discipline);

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving inputs (x=3.0, y=4.0)
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("x");
            array->set_type(VariableType::kInput);
            array->add_data(3.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("y");
            array->set_type(VariableType::kInput);
            array->add_data(4.0);
            return true;
        }))
        .WillOnce(Return(false));

    // Mock sending 2 partials: df/dx = 2*3 = 6, df/dy = 2*4 = 8
    EXPECT_CALL(*stream, Write(_, _))
        .Times(2)
        .WillRepeatedly(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            if (array.name() == "f" && array.subname() == "x") {
                EXPECT_DOUBLE_EQ(array.data(0), 6.0);
            } else if (array.name() == "f" && array.subname() == "y") {
                EXPECT_DOUBLE_EQ(array.data(0), 8.0);
            }
            return true;
        }));

    grpc::Status status = server_->ComputeGradientForTesting(context_.get(), stream.get());

    EXPECT_TRUE(status.ok());
}

TEST_F(ExplicitServerTest, ComputeGradientMultiplePartials) {
    auto discipline = std::make_shared<MultiOutputDiscipline>();
    discipline->Initialize();
    discipline->Configure();
    discipline->Setup();
    discipline->SetupPartials();

    server_->LinkPointers(discipline);

    auto stream = std::make_unique<MockServerReaderWriter>();

    // Mock receiving inputs (x=5.0, y=3.0)
    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("x");
            array->add_data(5.0);
            return true;
        }))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("y");
            array->add_data(3.0);
            return true;
        }))
        .WillOnce(Return(false));

    // Mock sending 6 partials (3 outputs x 2 inputs)
    EXPECT_CALL(*stream, Write(_, _))
        .Times(6)
        .WillRepeatedly(Invoke([](const philote::Array& array, grpc::WriteOptions) {
            // Verify expected partial values based on MultiOutputDiscipline
            if (array.name() == "sum") {
                EXPECT_DOUBLE_EQ(array.data(0), 1.0);  // d(sum)/dx = 1, d(sum)/dy = 1
            } else if (array.name() == "product") {
                if (array.subname() == "x") {
                    EXPECT_DOUBLE_EQ(array.data(0), 3.0);  // d(product)/dx = y = 3
                } else {
                    EXPECT_DOUBLE_EQ(array.data(0), 5.0);  // d(product)/dy = x = 5
                }
            } else if (array.name() == "difference") {
                if (array.subname() == "x") {
                    EXPECT_DOUBLE_EQ(array.data(0), 1.0);   // d(diff)/dx = 1
                } else {
                    EXPECT_DOUBLE_EQ(array.data(0), -1.0);  // d(diff)/dy = -1
                }
            }
            return true;
        }));

    grpc::Status status = server_->ComputeGradientForTesting(context_.get(), stream.get());

    EXPECT_TRUE(status.ok());
}

TEST_F(ExplicitServerTest, ComputeGradientComputePartialsThrows) {
    auto mock_discipline = std::make_shared<MockExplicitDiscipline>();

    // Setup metadata
    VariableMetaData x_meta, y_meta;
    x_meta.set_name("x");
    x_meta.set_type(kInput);
    x_meta.add_shape(1);
    y_meta.set_name("y");
    y_meta.set_type(kOutput);
    y_meta.add_shape(1);

    PartialsMetaData partial_meta;
    partial_meta.set_name("y");
    partial_meta.set_subname("x");
    partial_meta.add_shape(1);

    mock_discipline->var_meta().push_back(x_meta);
    mock_discipline->var_meta().push_back(y_meta);
    mock_discipline->partials_meta().push_back(partial_meta);

    // Mock ComputePartials to throw
    EXPECT_CALL(*mock_discipline, ComputePartials(_, _))
        .WillOnce(Throw(std::runtime_error("ComputePartials failed")));

    server_->LinkPointers(mock_discipline);

    auto stream = std::make_unique<MockServerReaderWriter>();

    EXPECT_CALL(*stream, Read(_))
        .WillOnce(Invoke([](philote::Array* array) {
            array->Clear();  // Clear any previous data
            array->set_name("x");
            array->add_data(1.0);
            return true;
        }))
        .WillOnce(Return(false));

    grpc::Status status = server_->ComputeGradientForTesting(context_.get(), stream.get());

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);
    EXPECT_THAT(status.error_message(), HasSubstr("partials"));
}

// ============================================================================
// Destructor Test
// ============================================================================

TEST_F(ExplicitServerTest, DestructorUnlinksPointers) {
    auto discipline = CreateSimpleDiscipline();
    auto server = std::make_unique<ExplicitServer>();

    server->LinkPointers(discipline);

    // Destructor should be called and unlink pointers without issues
    EXPECT_NO_THROW(server.reset());
}
