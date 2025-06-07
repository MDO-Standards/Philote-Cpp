#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <set>
#include <memory>
#include <stdexcept>

#include "explicit.h"

using namespace philote;
using namespace testing;

// Test fixture for ExplicitDiscipline class
class ExplicitDisciplineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        discipline = std::make_unique<ExplicitDiscipline>();
    }

    void TearDown() override
    {
        discipline.reset();
    }

    std::unique_ptr<ExplicitDiscipline> discipline;
};

// Test constructor and basic initialization
TEST_F(ExplicitDisciplineTest, ConstructorInitialization)
{
    EXPECT_TRUE(discipline != nullptr);
    EXPECT_TRUE(discipline->var_meta().empty());
    EXPECT_TRUE(discipline->partials_meta().empty());
}

// Test a simple explicit discipline implementation
class SimpleExplicitDiscipline : public ExplicitDiscipline
{
public:
    void Setup() override
    {
        AddInput("x", {2}, "m");
        AddOutput("y", {2}, "m");
    }

    void SetupPartials() override
    {
        DeclarePartials("y", "x");
    }

    void Compute(const Variables &inputs, Variables &outputs) override
    {
        // Simple computation: y = 2*x
        outputs.at("y")(0) = 2.0 * inputs.at("x")(0);
        outputs.at("y")(1) = 2.0 * inputs.at("x")(1);
    }

    void ComputePartials(const Variables &inputs, Partials &partials) override
    {
        // Simple partials: dy/dx = 2
        partials[std::make_pair("y", "x")](0) = 2.0;
        partials[std::make_pair("y", "x")](1) = 2.0;
    }
};

// Test a more complex explicit discipline implementation
class ComplexExplicitDiscipline : public ExplicitDiscipline
{
public:
    void Setup() override
    {
        AddInput("x1", {2}, "m");
        AddInput("x2", {3}, "kg");
        AddOutput("y1", {2}, "m");
        AddOutput("y2", {3}, "kg");
    }

    void SetupPartials() override
    {
        DeclarePartials("y1", "x1");
        DeclarePartials("y1", "x2");
        DeclarePartials("y2", "x1");
        DeclarePartials("y2", "x2");
    }

    void Compute(const Variables &inputs, Variables &outputs) override
    {
        // y1 = x1 + x2[0:1]
        outputs.at("y1")(0) = inputs.at("x1")(0) + inputs.at("x2")(0);
        outputs.at("y1")(1) = inputs.at("x1")(1) + inputs.at("x2")(1);

        // y2 = x2 * 2
        for (int i = 0; i < 3; ++i)
        {
            outputs.at("y2")(i) = 2.0 * inputs.at("x2")(i);
        }
    }

    void ComputePartials(const Variables &inputs, Partials &partials) override
    {
        // dy1/dx1 = 1
        partials[std::make_pair("y1", "x1")](0) = 1.0;
        partials[std::make_pair("y1", "x1")](1) = 1.0;

        // dy1/dx2 = 1 for first two elements
        partials[std::make_pair("y1", "x2")](0) = 1.0;
        partials[std::make_pair("y1", "x2")](1) = 1.0;
        partials[std::make_pair("y1", "x2")](2) = 0.0;

        // dy2/dx1 = 0
        partials[std::make_pair("y2", "x1")](0) = 0.0;
        partials[std::make_pair("y2", "x1")](1) = 0.0;

        // dy2/dx2 = 2
        partials[std::make_pair("y2", "x2")](0) = 2.0;
        partials[std::make_pair("y2", "x2")](1) = 2.0;
        partials[std::make_pair("y2", "x2")](2) = 2.0;
    }
};

// Test fixture for SimpleExplicitDiscipline
class SimpleExplicitDisciplineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        discipline = std::make_unique<SimpleExplicitDiscipline>();
        discipline->Setup();
        discipline->SetupPartials();
    }

    void TearDown() override
    {
        discipline.reset();
    }

    std::unique_ptr<SimpleExplicitDiscipline> discipline;
};

// Test fixture for ComplexExplicitDiscipline
class ComplexExplicitDisciplineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        discipline = std::make_unique<ComplexExplicitDiscipline>();
        discipline->Setup();
        discipline->SetupPartials();
    }

    void TearDown() override
    {
        discipline.reset();
    }

    std::unique_ptr<ComplexExplicitDiscipline> discipline;
};

// Test variable setup
TEST_F(SimpleExplicitDisciplineTest, VariableSetup)
{
    const auto &var_meta = discipline->var_meta();
    ASSERT_EQ(var_meta.size(), 2);

    // Check input variable
    EXPECT_EQ(var_meta[0].name(), "x");
    EXPECT_EQ(var_meta[0].type(), kInput);
    EXPECT_EQ(var_meta[0].units(), "m");
    ASSERT_EQ(var_meta[0].shape_size(), 1);
    EXPECT_EQ(var_meta[0].shape(0), 2);

    // Check output variable
    EXPECT_EQ(var_meta[1].name(), "y");
    EXPECT_EQ(var_meta[1].type(), kOutput);
    EXPECT_EQ(var_meta[1].units(), "m");
    ASSERT_EQ(var_meta[1].shape_size(), 1);
    EXPECT_EQ(var_meta[1].shape(0), 2);
}

// Test partials setup
TEST_F(SimpleExplicitDisciplineTest, PartialsSetup)
{
    const auto &partials_meta = discipline->partials_meta();
    ASSERT_EQ(partials_meta.size(), 1);
    EXPECT_EQ(partials_meta[0].name(), "y");
    EXPECT_EQ(partials_meta[0].subname(), "x");
}

// Test compute function
TEST_F(SimpleExplicitDisciplineTest, ComputeFunction)
{
    Variables inputs;
    inputs["x"] = Variable(kInput, {2});
    inputs.at("x")(0) = 1.0;
    inputs.at("x")(1) = 2.0;

    Variables outputs;
    outputs["y"] = Variable(kOutput, {2});

    discipline->Compute(inputs, outputs);

    EXPECT_DOUBLE_EQ(outputs.at("y")(0), 2.0); // 2 * 1.0
    EXPECT_DOUBLE_EQ(outputs.at("y")(1), 4.0); // 2 * 2.0
}

// Test compute partials
TEST_F(SimpleExplicitDisciplineTest, ComputePartials)
{
    Variables inputs;
    inputs["x"] = Variable(kInput, {2});
    inputs.at("x")(0) = 1.0;
    inputs.at("x")(1) = 2.0;

    Partials partials;
    partials[std::make_pair("y", "x")] = Variable(kOutput, {2});

    discipline->ComputePartials(inputs, partials);

    EXPECT_DOUBLE_EQ(partials[std::make_pair("y", "x")](0), 2.0);
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y", "x")](1), 2.0);
}

// Test complex discipline variable setup
TEST_F(ComplexExplicitDisciplineTest, VariableSetup)
{
    const auto &var_meta = discipline->var_meta();
    ASSERT_EQ(var_meta.size(), 4);

    // Check input variables
    EXPECT_EQ(var_meta[0].name(), "x1");
    EXPECT_EQ(var_meta[0].type(), kInput);
    EXPECT_EQ(var_meta[0].units(), "m");
    ASSERT_EQ(var_meta[0].shape_size(), 1);
    EXPECT_EQ(var_meta[0].shape(0), 2);

    EXPECT_EQ(var_meta[1].name(), "x2");
    EXPECT_EQ(var_meta[1].type(), kInput);
    EXPECT_EQ(var_meta[1].units(), "kg");
    ASSERT_EQ(var_meta[1].shape_size(), 1);
    EXPECT_EQ(var_meta[1].shape(0), 3);

    // Check output variables
    EXPECT_EQ(var_meta[2].name(), "y1");
    EXPECT_EQ(var_meta[2].type(), kOutput);
    EXPECT_EQ(var_meta[2].units(), "m");
    ASSERT_EQ(var_meta[2].shape_size(), 1);
    EXPECT_EQ(var_meta[2].shape(0), 2);

    EXPECT_EQ(var_meta[3].name(), "y2");
    EXPECT_EQ(var_meta[3].type(), kOutput);
    EXPECT_EQ(var_meta[3].units(), "kg");
    ASSERT_EQ(var_meta[3].shape_size(), 1);
    EXPECT_EQ(var_meta[3].shape(0), 3);
}

// Test complex discipline partials setup
TEST_F(ComplexExplicitDisciplineTest, PartialsSetup)
{
    const auto &partials_meta = discipline->partials_meta();
    ASSERT_EQ(partials_meta.size(), 4);

    // Check all declared partials
    EXPECT_EQ(partials_meta[0].name(), "y1");
    EXPECT_EQ(partials_meta[0].subname(), "x1");

    EXPECT_EQ(partials_meta[1].name(), "y1");
    EXPECT_EQ(partials_meta[1].subname(), "x2");

    EXPECT_EQ(partials_meta[2].name(), "y2");
    EXPECT_EQ(partials_meta[2].subname(), "x1");

    EXPECT_EQ(partials_meta[3].name(), "y2");
    EXPECT_EQ(partials_meta[3].subname(), "x2");
}

// Test complex discipline compute function
TEST_F(ComplexExplicitDisciplineTest, ComputeFunction)
{
    Variables inputs;
    inputs["x1"] = Variable(kInput, {2});
    inputs["x2"] = Variable(kInput, {3});

    inputs.at("x1")(0) = 1.0;
    inputs.at("x1")(1) = 2.0;
    inputs.at("x2")(0) = 3.0;
    inputs.at("x2")(1) = 4.0;
    inputs.at("x2")(2) = 5.0;

    Variables outputs;
    outputs["y1"] = Variable(kOutput, {2});
    outputs["y2"] = Variable(kOutput, {3});

    discipline->Compute(inputs, outputs);

    // Check y1 = x1 + x2[0:1]
    EXPECT_DOUBLE_EQ(outputs.at("y1")(0), 4.0); // 1.0 + 3.0
    EXPECT_DOUBLE_EQ(outputs.at("y1")(1), 6.0); // 2.0 + 4.0

    // Check y2 = x2 * 2
    EXPECT_DOUBLE_EQ(outputs.at("y2")(0), 6.0);  // 2 * 3.0
    EXPECT_DOUBLE_EQ(outputs.at("y2")(1), 8.0);  // 2 * 4.0
    EXPECT_DOUBLE_EQ(outputs.at("y2")(2), 10.0); // 2 * 5.0
}

// Test complex discipline compute partials
TEST_F(ComplexExplicitDisciplineTest, ComputePartials)
{
    Variables inputs;
    inputs["x1"] = Variable(kInput, {2});
    inputs["x2"] = Variable(kInput, {3});

    inputs.at("x1")(0) = 1.0;
    inputs.at("x1")(1) = 2.0;
    inputs.at("x2")(0) = 3.0;
    inputs.at("x2")(1) = 4.0;
    inputs.at("x2")(2) = 5.0;

    Partials partials;
    partials[std::make_pair("y1", "x1")] = Variable(kOutput, {2});
    partials[std::make_pair("y1", "x2")] = Variable(kOutput, {3});
    partials[std::make_pair("y2", "x1")] = Variable(kOutput, {2});
    partials[std::make_pair("y2", "x2")] = Variable(kOutput, {3});

    discipline->ComputePartials(inputs, partials);

    // Check dy1/dx1 = 1
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y1", "x1")](0), 1.0);
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y1", "x1")](1), 1.0);

    // Check dy1/dx2 = 1 for first two elements, 0 for third
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y1", "x2")](0), 1.0);
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y1", "x2")](1), 1.0);
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y1", "x2")](2), 0.0);

    // Check dy2/dx1 = 0
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y2", "x1")](0), 0.0);
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y2", "x1")](1), 0.0);

    // Check dy2/dx2 = 2
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y2", "x2")](0), 2.0);
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y2", "x2")](1), 2.0);
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y2", "x2")](2), 2.0);
}

// Test error handling for missing variables
TEST_F(SimpleExplicitDisciplineTest, ErrorHandlingMissingVariables)
{
    Variables inputs;
    Variables outputs;
    outputs["y"] = Variable(kOutput, {2});

    EXPECT_THROW({
        try {
            discipline->Compute(inputs, outputs);
        } catch (const std::out_of_range& e) {
            EXPECT_STREQ(e.what(), "map::at:  key not found");
            throw;
        } }, std::out_of_range);
}

// Test error handling for wrong shapes
TEST_F(SimpleExplicitDisciplineTest, ErrorHandlingWrongShapes)
{
    Variables inputs;
    inputs["x"] = Variable(kInput, {3}); // Wrong shape, should be {2}
    Variables outputs;
    outputs["y"] = Variable(kOutput, {2});

    // The current implementation doesn't check shapes, so we expect no exception
    EXPECT_NO_THROW(discipline->Compute(inputs, outputs));
}

// Test error handling for missing partials
TEST_F(SimpleExplicitDisciplineTest, ErrorHandlingMissingPartials)
{
    Variables inputs;
    inputs["x"] = Variable(kInput, {2});
    Partials partials;

    EXPECT_THROW({
        try {
            discipline->ComputePartials(inputs, partials);
        } catch (const std::out_of_range& e) {
            EXPECT_STREQ(e.what(), "Index out of range in Variable::operator() non-const");
            throw;
        } }, std::out_of_range);
}