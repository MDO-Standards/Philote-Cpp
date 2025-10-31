#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <set>
#include <memory>
#include <stdexcept>
#include <grpcpp/grpcpp.h>

#include "implicit.h"

using namespace philote;
using namespace testing;

// Test fixture for ImplicitDiscipline class
class ImplicitDisciplineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        discipline = std::make_shared<ImplicitDiscipline>();
    }

    void TearDown() override
    {
        discipline.reset();
    }

    std::shared_ptr<ImplicitDiscipline> discipline;
};

// Test constructor and basic initialization
TEST_F(ImplicitDisciplineTest, ConstructorInitialization)
{
    EXPECT_TRUE(discipline != nullptr);
    EXPECT_TRUE(discipline->var_meta().empty());
    EXPECT_TRUE(discipline->partials_meta().empty());
}

// Test a simple implicit discipline implementation
class SimpleImplicitDisciplineTest : public ImplicitDiscipline
{
public:
    void Setup() override
    {
        AddInput("x", {2}, "m");
        AddOutput("y", {2}, "m^2");
    }

    void SetupPartials() override
    {
        DeclarePartials("y", "x");
        DeclarePartials("y", "y");
    }

    void ComputeResiduals(const Variables &inputs, const Variables &outputs, Variables &residuals) override
    {
        // Residual: R = x^2 - y
        residuals.at("y")(0) = inputs.at("x")(0) * inputs.at("x")(0) - outputs.at("y")(0);
        residuals.at("y")(1) = inputs.at("x")(1) * inputs.at("x")(1) - outputs.at("y")(1);
    }

    void SolveResiduals(const Variables &inputs, Variables &outputs) override
    {
        // Solution: y = x^2
        outputs.at("y")(0) = inputs.at("x")(0) * inputs.at("x")(0);
        outputs.at("y")(1) = inputs.at("x")(1) * inputs.at("x")(1);
    }

    void ComputeResidualGradients(const Variables &inputs, const Variables &outputs, Partials &partials) override
    {
        // dR/dx = 2x, dR/dy = -1
        partials[std::make_pair("y", "x")](0) = 2.0 * inputs.at("x")(0);
        partials[std::make_pair("y", "x")](1) = 2.0 * inputs.at("x")(1);
        partials[std::make_pair("y", "y")](0) = -1.0;
        partials[std::make_pair("y", "y")](1) = -1.0;
    }
};

// Test a more complex implicit discipline implementation
class ComplexImplicitDisciplineTest : public ImplicitDiscipline
{
public:
    void Setup() override
    {
        AddInput("a", {2}, "");
        AddInput("b", {2}, "");
        AddInput("c", {2}, "");
        AddOutput("x", {2}, "");
    }

    void SetupPartials() override
    {
        DeclarePartials("x", "a");
        DeclarePartials("x", "b");
        DeclarePartials("x", "c");
        DeclarePartials("x", "x");
    }

    void ComputeResiduals(const Variables &inputs, const Variables &outputs, Variables &residuals) override
    {
        // Residual: R = ax^2 + bx + c
        for (int i = 0; i < 2; ++i)
        {
            double a = inputs.at("a")(i);
            double b = inputs.at("b")(i);
            double c = inputs.at("c")(i);
            double x = outputs.at("x")(i);
            residuals.at("x")(i) = a * x * x + b * x + c;
        }
    }

    void SolveResiduals(const Variables &inputs, Variables &outputs) override
    {
        // Quadratic formula: x = (-b + sqrt(b^2 - 4ac)) / 2a
        for (int i = 0; i < 2; ++i)
        {
            double a = inputs.at("a")(i);
            double b = inputs.at("b")(i);
            double c = inputs.at("c")(i);
            double discriminant = b * b - 4.0 * a * c;
            outputs.at("x")(i) = (-b + std::sqrt(discriminant)) / (2.0 * a);
        }
    }

    void ComputeResidualGradients(const Variables &inputs, const Variables &outputs, Partials &partials) override
    {
        // dR/da = x^2, dR/db = x, dR/dc = 1, dR/dx = 2ax + b
        for (int i = 0; i < 2; ++i)
        {
            double a = inputs.at("a")(i);
            double b = inputs.at("b")(i);
            double x = outputs.at("x")(i);

            partials[std::make_pair("x", "a")](i) = x * x;
            partials[std::make_pair("x", "b")](i) = x;
            partials[std::make_pair("x", "c")](i) = 1.0;
            partials[std::make_pair("x", "x")](i) = 2.0 * a * x + b;
        }
    }
};

// Test fixture for SimpleImplicitDisciplineTest
class SimpleImplicitTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        discipline = std::make_shared<SimpleImplicitDisciplineTest>();
        discipline->Setup();
        discipline->SetupPartials();
    }

    void TearDown() override
    {
        discipline.reset();
    }

    std::shared_ptr<SimpleImplicitDisciplineTest> discipline;
};

// Test fixture for ComplexImplicitDisciplineTest
class ComplexImplicitTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        discipline = std::make_shared<ComplexImplicitDisciplineTest>();
        discipline->Setup();
        discipline->SetupPartials();
    }

    void TearDown() override
    {
        discipline.reset();
    }

    std::shared_ptr<ComplexImplicitDisciplineTest> discipline;
};

// Test variable setup
TEST_F(SimpleImplicitTest, VariableSetup)
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
    EXPECT_EQ(var_meta[1].units(), "m^2");
    ASSERT_EQ(var_meta[1].shape_size(), 1);
    EXPECT_EQ(var_meta[1].shape(0), 2);
}

// Test partials setup
TEST_F(SimpleImplicitTest, PartialsSetup)
{
    const auto &partials_meta = discipline->partials_meta();
    ASSERT_EQ(partials_meta.size(), 2);
    EXPECT_EQ(partials_meta[0].name(), "y");
    EXPECT_EQ(partials_meta[0].subname(), "x");
    EXPECT_EQ(partials_meta[1].name(), "y");
    EXPECT_EQ(partials_meta[1].subname(), "y");
}

// Test compute residuals function
TEST_F(SimpleImplicitTest, ComputeResidualsFunction)
{
    Variables inputs;
    inputs["x"] = Variable(kInput, {2});
    inputs.at("x")(0) = 2.0;
    inputs.at("x")(1) = 3.0;

    Variables outputs;
    outputs["y"] = Variable(kOutput, {2});
    outputs.at("y")(0) = 4.0;  // Correct value for x=2
    outputs.at("y")(1) = 9.0;  // Correct value for x=3

    Variables residuals;
    residuals["y"] = Variable(kOutput, {2});

    discipline->ComputeResiduals(inputs, outputs, residuals);

    // Residuals should be near zero for correct outputs
    EXPECT_NEAR(residuals.at("y")(0), 0.0, 1e-10);
    EXPECT_NEAR(residuals.at("y")(1), 0.0, 1e-10);
}

// Test compute residuals with wrong outputs
TEST_F(SimpleImplicitTest, ComputeResidualsWithWrongOutputs)
{
    Variables inputs;
    inputs["x"] = Variable(kInput, {2});
    inputs.at("x")(0) = 2.0;
    inputs.at("x")(1) = 3.0;

    Variables outputs;
    outputs["y"] = Variable(kOutput, {2});
    outputs.at("y")(0) = 5.0;  // Wrong value (should be 4.0)
    outputs.at("y")(1) = 10.0; // Wrong value (should be 9.0)

    Variables residuals;
    residuals["y"] = Variable(kOutput, {2});

    discipline->ComputeResiduals(inputs, outputs, residuals);

    // Residuals should be non-zero for incorrect outputs
    EXPECT_DOUBLE_EQ(residuals.at("y")(0), -1.0); // 4 - 5
    EXPECT_DOUBLE_EQ(residuals.at("y")(1), -1.0); // 9 - 10
}

// Test solve residuals function
TEST_F(SimpleImplicitTest, SolveResidualsFunction)
{
    Variables inputs;
    inputs["x"] = Variable(kInput, {2});
    inputs.at("x")(0) = 2.0;
    inputs.at("x")(1) = 3.0;

    Variables outputs;
    outputs["y"] = Variable(kOutput, {2});

    discipline->SolveResiduals(inputs, outputs);

    EXPECT_DOUBLE_EQ(outputs.at("y")(0), 4.0);  // 2^2
    EXPECT_DOUBLE_EQ(outputs.at("y")(1), 9.0);  // 3^2
}

// Test compute residual gradients
TEST_F(SimpleImplicitTest, ComputeResidualGradients)
{
    Variables inputs;
    inputs["x"] = Variable(kInput, {2});
    inputs.at("x")(0) = 2.0;
    inputs.at("x")(1) = 3.0;

    Variables outputs;
    outputs["y"] = Variable(kOutput, {2});
    outputs.at("y")(0) = 4.0;
    outputs.at("y")(1) = 9.0;

    Partials partials;
    partials[std::make_pair("y", "x")] = Variable(kOutput, {2});
    partials[std::make_pair("y", "y")] = Variable(kOutput, {2});

    discipline->ComputeResidualGradients(inputs, outputs, partials);

    // dR/dx = 2x
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y", "x")](0), 4.0);  // 2 * 2.0
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y", "x")](1), 6.0);  // 2 * 3.0

    // dR/dy = -1
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y", "y")](0), -1.0);
    EXPECT_DOUBLE_EQ(partials[std::make_pair("y", "y")](1), -1.0);
}

// Test complex discipline variable setup
TEST_F(ComplexImplicitTest, VariableSetup)
{
    const auto &var_meta = discipline->var_meta();
    ASSERT_EQ(var_meta.size(), 4);

    // Check input variables
    EXPECT_EQ(var_meta[0].name(), "a");
    EXPECT_EQ(var_meta[0].type(), kInput);
    EXPECT_EQ(var_meta[0].units(), "");
    ASSERT_EQ(var_meta[0].shape_size(), 1);
    EXPECT_EQ(var_meta[0].shape(0), 2);

    EXPECT_EQ(var_meta[1].name(), "b");
    EXPECT_EQ(var_meta[1].type(), kInput);
    EXPECT_EQ(var_meta[1].units(), "");
    ASSERT_EQ(var_meta[1].shape_size(), 1);
    EXPECT_EQ(var_meta[1].shape(0), 2);

    EXPECT_EQ(var_meta[2].name(), "c");
    EXPECT_EQ(var_meta[2].type(), kInput);
    EXPECT_EQ(var_meta[2].units(), "");
    ASSERT_EQ(var_meta[2].shape_size(), 1);
    EXPECT_EQ(var_meta[2].shape(0), 2);

    // Check output variable
    EXPECT_EQ(var_meta[3].name(), "x");
    EXPECT_EQ(var_meta[3].type(), kOutput);
    EXPECT_EQ(var_meta[3].units(), "");
    ASSERT_EQ(var_meta[3].shape_size(), 1);
    EXPECT_EQ(var_meta[3].shape(0), 2);
}

// Test complex discipline partials setup
TEST_F(ComplexImplicitTest, PartialsSetup)
{
    const auto &partials_meta = discipline->partials_meta();
    ASSERT_EQ(partials_meta.size(), 4);

    // Check all declared partials
    EXPECT_EQ(partials_meta[0].name(), "x");
    EXPECT_EQ(partials_meta[0].subname(), "a");

    EXPECT_EQ(partials_meta[1].name(), "x");
    EXPECT_EQ(partials_meta[1].subname(), "b");

    EXPECT_EQ(partials_meta[2].name(), "x");
    EXPECT_EQ(partials_meta[2].subname(), "c");

    EXPECT_EQ(partials_meta[3].name(), "x");
    EXPECT_EQ(partials_meta[3].subname(), "x");
}

// Test complex discipline compute residuals
TEST_F(ComplexImplicitTest, ComputeResidualsFunction)
{
    Variables inputs;
    inputs["a"] = Variable(kInput, {2});
    inputs["b"] = Variable(kInput, {2});
    inputs["c"] = Variable(kInput, {2});

    // First equation: x^2 - 5x + 6 = 0, solution x = 2 or 3
    inputs.at("a")(0) = 1.0;
    inputs.at("b")(0) = -5.0;
    inputs.at("c")(0) = 6.0;

    // Second equation: x^2 - 7x + 12 = 0, solution x = 3 or 4
    inputs.at("a")(1) = 1.0;
    inputs.at("b")(1) = -7.0;
    inputs.at("c")(1) = 12.0;

    Variables outputs;
    outputs["x"] = Variable(kOutput, {2});
    outputs.at("x")(0) = 3.0;  // One solution of first equation
    outputs.at("x")(1) = 4.0;  // One solution of second equation

    Variables residuals;
    residuals["x"] = Variable(kOutput, {2});

    discipline->ComputeResiduals(inputs, outputs, residuals);

    // Residuals should be near zero for correct outputs
    EXPECT_NEAR(residuals.at("x")(0), 0.0, 1e-10);
    EXPECT_NEAR(residuals.at("x")(1), 0.0, 1e-10);
}

// Test complex discipline solve residuals
TEST_F(ComplexImplicitTest, SolveResidualsFunction)
{
    Variables inputs;
    inputs["a"] = Variable(kInput, {2});
    inputs["b"] = Variable(kInput, {2});
    inputs["c"] = Variable(kInput, {2});

    // First equation: x^2 - 5x + 6 = 0, solution x = 3 (positive root)
    inputs.at("a")(0) = 1.0;
    inputs.at("b")(0) = -5.0;
    inputs.at("c")(0) = 6.0;

    // Second equation: x^2 - 7x + 12 = 0, solution x = 4 (positive root)
    inputs.at("a")(1) = 1.0;
    inputs.at("b")(1) = -7.0;
    inputs.at("c")(1) = 12.0;

    Variables outputs;
    outputs["x"] = Variable(kOutput, {2});

    discipline->SolveResiduals(inputs, outputs);

    EXPECT_DOUBLE_EQ(outputs.at("x")(0), 3.0);
    EXPECT_DOUBLE_EQ(outputs.at("x")(1), 4.0);
}

// Test complex discipline compute residual gradients
TEST_F(ComplexImplicitTest, ComputeResidualGradients)
{
    Variables inputs;
    inputs["a"] = Variable(kInput, {2});
    inputs["b"] = Variable(kInput, {2});
    inputs["c"] = Variable(kInput, {2});

    inputs.at("a")(0) = 1.0;
    inputs.at("b")(0) = -5.0;
    inputs.at("c")(0) = 6.0;
    inputs.at("a")(1) = 1.0;
    inputs.at("b")(1) = -7.0;
    inputs.at("c")(1) = 12.0;

    Variables outputs;
    outputs["x"] = Variable(kOutput, {2});
    outputs.at("x")(0) = 3.0;
    outputs.at("x")(1) = 4.0;

    Partials partials;
    partials[std::make_pair("x", "a")] = Variable(kOutput, {2});
    partials[std::make_pair("x", "b")] = Variable(kOutput, {2});
    partials[std::make_pair("x", "c")] = Variable(kOutput, {2});
    partials[std::make_pair("x", "x")] = Variable(kOutput, {2});

    discipline->ComputeResidualGradients(inputs, outputs, partials);

    // dR/da = x^2
    EXPECT_DOUBLE_EQ(partials[std::make_pair("x", "a")](0), 9.0);   // 3^2
    EXPECT_DOUBLE_EQ(partials[std::make_pair("x", "a")](1), 16.0);  // 4^2

    // dR/db = x
    EXPECT_DOUBLE_EQ(partials[std::make_pair("x", "b")](0), 3.0);
    EXPECT_DOUBLE_EQ(partials[std::make_pair("x", "b")](1), 4.0);

    // dR/dc = 1
    EXPECT_DOUBLE_EQ(partials[std::make_pair("x", "c")](0), 1.0);
    EXPECT_DOUBLE_EQ(partials[std::make_pair("x", "c")](1), 1.0);

    // dR/dx = 2ax + b
    EXPECT_DOUBLE_EQ(partials[std::make_pair("x", "x")](0), 1.0);  // 2*1*3 + (-5) = 1
    EXPECT_DOUBLE_EQ(partials[std::make_pair("x", "x")](1), 1.0);  // 2*1*4 + (-7) = 1
}

// Test error handling for missing variables
TEST_F(SimpleImplicitTest, ErrorHandlingMissingVariables)
{
    Variables inputs;
    Variables outputs;
    outputs["y"] = Variable(kOutput, {2});
    Variables residuals;
    residuals["y"] = Variable(kOutput, {2});

    // Just verify the exception is thrown, don't check compiler-specific message
    EXPECT_THROW(discipline->ComputeResiduals(inputs, outputs, residuals), std::out_of_range);
}

// Test error handling for wrong shapes
TEST_F(SimpleImplicitTest, ErrorHandlingWrongShapes)
{
    Variables inputs;
    inputs["x"] = Variable(kInput, {3}); // Wrong shape, should be {2}
    Variables outputs;
    outputs["y"] = Variable(kOutput, {2});
    Variables residuals;
    residuals["y"] = Variable(kOutput, {2});

    // The current implementation doesn't check shapes, so we expect no exception
    EXPECT_NO_THROW(discipline->ComputeResiduals(inputs, outputs, residuals));
}

// Test error handling for missing partials
TEST_F(SimpleImplicitTest, ErrorHandlingMissingPartials)
{
    Variables inputs;
    inputs["x"] = Variable(kInput, {2});
    Variables outputs;
    outputs["y"] = Variable(kOutput, {2});
    Partials partials;

    EXPECT_THROW({
        try {
            discipline->ComputeResidualGradients(inputs, outputs, partials);
        } catch (const std::out_of_range& e) {
            EXPECT_STREQ(e.what(), "Index out of range in Variable::operator() non-const");
            throw;
        } }, std::out_of_range);
}

// ============================================================================
// RegisterServices Tests
// ============================================================================

// Test RegisterServices() registers both discipline and implicit servers
TEST_F(ImplicitDisciplineTest, RegisterServicesRegistersServers)
{
    // Create a ServerBuilder
    grpc::ServerBuilder builder;

    // Register services
    discipline->RegisterServices(builder);

    // Build the server - this will fail if services aren't properly registered
    // We're just verifying the registration doesn't crash
    EXPECT_NO_THROW({
        std::shared_ptr<grpc::Server> server = builder.BuildAndStart();
        if (server) {
            server->Shutdown();
        }
    });
}
