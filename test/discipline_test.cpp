#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "discipline.h"
#include <google/protobuf/struct.pb.h>

using namespace philote;
using namespace testing;

// Test fixture for Discipline class
class DisciplineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        discipline = std::make_unique<Discipline>();
    }

    void TearDown() override
    {
        discipline.reset();
    }

    std::unique_ptr<Discipline> discipline;
};

// Test constructor and basic initialization
TEST_F(DisciplineTest, ConstructorInitialization)
{
    EXPECT_TRUE(discipline != nullptr);
    EXPECT_TRUE(discipline->var_meta().empty());
    EXPECT_TRUE(discipline->partials_meta().empty());
}

// Test AddInput method
TEST_F(DisciplineTest, AddInput)
{
    std::string name = "test_input";
    std::vector<int64_t> shape = {2, 3};
    std::string units = "m";

    discipline->AddInput(name, shape, units);

    ASSERT_EQ(discipline->var_meta().size(), 1);
    const auto &var = discipline->var_meta()[0];
    EXPECT_EQ(var.name(), name);
    EXPECT_EQ(var.units(), units);
    EXPECT_EQ(var.type(), kInput);
    ASSERT_EQ(var.shape_size(), 2);
    EXPECT_EQ(var.shape(0), 2);
    EXPECT_EQ(var.shape(1), 3);
}

// Test AddOutput method
TEST_F(DisciplineTest, AddOutput)
{
    std::string name = "test_output";
    std::vector<int64_t> shape = {4, 5};
    std::string units = "kg";

    discipline->AddOutput(name, shape, units);

    ASSERT_EQ(discipline->var_meta().size(), 1);
    const auto &var = discipline->var_meta()[0];
    EXPECT_EQ(var.name(), name);
    EXPECT_EQ(var.units(), units);
    EXPECT_EQ(var.type(), kOutput);
    ASSERT_EQ(var.shape_size(), 2);
    EXPECT_EQ(var.shape(0), 4);
    EXPECT_EQ(var.shape(1), 5);
}

// Test DeclarePartials method with scalar variables
TEST_F(DisciplineTest, DeclarePartialsScalar)
{
    // Add input and output variables
    discipline->AddInput("x", {1}, "m");
    discipline->AddOutput("f", {1}, "N");

    // Declare partials
    discipline->DeclarePartials("f", "x");

    ASSERT_EQ(discipline->partials_meta().size(), 1);
    const auto &partial = discipline->partials_meta()[0];
    EXPECT_EQ(partial.name(), "f");
    EXPECT_EQ(partial.subname(), "x");
    ASSERT_EQ(partial.shape_size(), 1);
    EXPECT_EQ(partial.shape(0), 1);
}

// Test DeclarePartials method with vector variables
TEST_F(DisciplineTest, DeclarePartialsVector)
{
    // Add input and output variables
    discipline->AddInput("x", {3}, "m");
    discipline->AddOutput("f", {2}, "N");

    // Declare partials
    discipline->DeclarePartials("f", "x");

    ASSERT_EQ(discipline->partials_meta().size(), 1);
    const auto &partial = discipline->partials_meta()[0];
    EXPECT_EQ(partial.name(), "f");
    EXPECT_EQ(partial.subname(), "x");
    ASSERT_EQ(partial.shape_size(), 2);
    // Note: The current implementation concatenates shapes in the order they are found
    // First the output shape (2), then the input shape (3)
    EXPECT_EQ(partial.shape(0), 2);
    EXPECT_EQ(partial.shape(1), 3);
}

// Test SetOptions method
TEST_F(DisciplineTest, SetOptions)
{
    google::protobuf::Struct options_struct;
    // The base class implementation is empty, but we can test it doesn't crash
    EXPECT_NO_THROW(discipline->SetOptions(options_struct));
}

// Test Setup method
TEST_F(DisciplineTest, Setup)
{
    // The base class implementation is empty, but we can test it doesn't crash
    EXPECT_NO_THROW(discipline->Setup());
}

// Test SetupPartials method
TEST_F(DisciplineTest, SetupPartials)
{
    // The base class implementation is empty, but we can test it doesn't crash
    EXPECT_NO_THROW(discipline->SetupPartials());
}

// Test properties access
TEST_F(DisciplineTest, PropertiesAccess)
{
    auto &props = discipline->properties();
    EXPECT_FALSE(props.continuous());
    EXPECT_FALSE(props.differentiable());
    EXPECT_FALSE(props.provides_gradients());
    EXPECT_TRUE(props.name().empty());
    EXPECT_TRUE(props.version().empty());
}

// Test stream options access
TEST_F(DisciplineTest, StreamOptionsAccess)
{
    auto &opts = discipline->stream_opts();
    EXPECT_EQ(opts.num_double(), 0);
}

// Test options list access
TEST_F(DisciplineTest, OptionsListAccess)
{
    auto &options = discipline->options_list();
    EXPECT_TRUE(options.empty());

    // Test adding an option
    options["test_option"] = "double";
    EXPECT_EQ(options.size(), 1);
    EXPECT_EQ(options["test_option"], "double");
}