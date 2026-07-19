#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "explicit.h"
#include "implicit.h"
#include "test_helpers.h"
#include "validation.h"

using namespace philote;
using namespace philote::test;

namespace
{
class DynamicDiscreteExplicit final : public ExplicitDiscipline
{
public:
    void Initialize() override
    {
        AddOption("config", "dict");
        AddOption("label", "str");
    }

    void Setup() override
    {
        AddInput("x", {}, "m", true);
        AddOutput("y", {}, "m", true);
        AddDiscreteInput("mode");
        AddDiscreteOutput("status");
    }

    void SetupPartials() override { DeclarePartials("y", "x"); }

    void Compute(const Variables &inputs,
                 Variables &outputs,
                 const DiscreteVariables &discrete_inputs,
                 DiscreteVariables &discrete_outputs) override
    {
        double scale = 2.0;
        const auto mode = discrete_inputs.find("mode");
        if (mode != discrete_inputs.end() && mode->second.string_value() == "triple")
            scale = 3.0;

        for (size_t i = 0; i < inputs.at("x").Size(); ++i)
            outputs.at("y")(i) = scale * inputs.at("x")(i);

        google::protobuf::Value status;
        auto *fields = status.mutable_struct_value()->mutable_fields();
        (*fields)["mode"].set_string_value(mode == discrete_inputs.end()
                                                      ? "default"
                                                      : mode->second.string_value());
        (*fields)["count"].set_number_value(
            static_cast<double>(inputs.at("x").Size()));
        discrete_outputs["status"] = status;
    }

    void ComputePartials(const Variables &inputs,
                         Partials &partials,
                         const DiscreteVariables &) override
    {
        auto &jac = partials.at({"y", "x"});
        const size_t n = inputs.at("x").Size();
        for (size_t i = 0; i < jac.Size(); ++i)
            jac(i) = 0.0;
        for (size_t i = 0; i < n; ++i)
            jac(i * n + i) = 3.0;
    }
};

class DynamicImplicit final : public ImplicitDiscipline
{
public:
    void Setup() override
    {
        AddInput("a", {1}, "");
        AddOutput("x", {}, "", true);
    }

    void SetupPartials() override
    {
        DeclarePartials("x", "a");
        DeclarePartials("x", "x");
    }

    void ComputeResiduals(const Variables &inputs,
                          const Variables &outputs,
                          Variables &residuals) override
    {
        for (size_t i = 0; i < outputs.at("x").Size(); ++i)
            residuals.at("x")(i) = outputs.at("x")(i) * outputs.at("x")(i) -
                                      inputs.at("a")(0);
    }

    void SolveResiduals(const Variables &inputs, Variables &outputs) override
    {
        const double value = std::sqrt(inputs.at("a")(0));
        for (size_t i = 0; i < outputs.at("x").Size(); ++i)
            outputs.at("x")(i) = value;
    }

    void ComputeResidualGradients(const Variables &,
                                  const Variables &outputs,
                                  Partials &partials) override
    {
        auto &d_a = partials.at({"x", "a"});
        for (size_t i = 0; i < d_a.Size(); ++i)
            d_a(i) = -1.0;

        auto &d_x = partials.at({"x", "x"});
        const size_t n = outputs.at("x").Size();
        for (size_t i = 0; i < d_x.Size(); ++i)
            d_x(i) = 0.0;
        for (size_t i = 0; i < n; ++i)
            d_x(i * n + i) = 2.0 * outputs.at("x")(i);
    }
};
} // namespace

TEST(ProtocolParityIntegrationTest, DynamicShapesDiscreteValuesAndStructOptions)
{
    TestServerManager server;
    const auto discipline = std::make_shared<DynamicDiscreteExplicit>();
    const std::string address = server.StartServer(discipline);

    ExplicitClient client;
    client.ConnectChannel(CreateTestChannel(address));
    client.GetAvailableOptions();
    EXPECT_EQ(client.GetOptionsList().at("config"), "dict");
    EXPECT_EQ(client.GetOptionsList().at("label"), "str");

    client.Setup();
    client.GetVariableDefinitions();
    ASSERT_EQ(client.GetDynamicVariables().size(), 2);
    ASSERT_EQ(client.GetDiscreteVariableMetaAll().size(), 2);

    client.SendVariableShapes({
        client.CreateVariableShape("x", {3}, kInput),
        client.CreateVariableShape("y", {3}, kOutput),
    });
    client.GetPartialDefinitions();

    Variables inputs;
    inputs["x"] = Variable(kInput, {3});
    inputs["x"](0) = 1.0;
    inputs["x"](1) = 2.0;
    inputs["x"](2) = 4.0;

    DiscreteVariables discrete_inputs;
    discrete_inputs["mode"].set_string_value("triple");
    DiscreteVariables discrete_outputs;
    const Variables outputs =
        client.ComputeFunction(inputs, discrete_inputs, discrete_outputs);

    EXPECT_DOUBLE_EQ(outputs.at("y")(0), 3.0);
    EXPECT_DOUBLE_EQ(outputs.at("y")(1), 6.0);
    EXPECT_DOUBLE_EQ(outputs.at("y")(2), 12.0);
    ASSERT_TRUE(discrete_outputs.at("status").has_struct_value());
    EXPECT_DOUBLE_EQ(discrete_outputs.at("status")
                         .struct_value()
                         .fields()
                         .at("count")
                         .number_value(),
                     3.0);

    const Partials partials = client.ComputeGradient(inputs, discrete_inputs);
    ASSERT_EQ(partials.at({"y", "x"}).Shape(), (std::vector<size_t>{3, 3}));
    EXPECT_DOUBLE_EQ(partials.at({"y", "x"})(0), 3.0);
    EXPECT_DOUBLE_EQ(partials.at({"y", "x"})(4), 3.0);
    EXPECT_DOUBLE_EQ(partials.at({"y", "x"})(8), 3.0);
}

TEST(ProtocolParityIntegrationTest, ImplicitResidualMetadataAndDynamicJacobians)
{
    ImplicitTestServerManager server;
    const auto discipline = std::make_shared<DynamicImplicit>();
    const std::string address = server.StartServer(discipline);

    ImplicitClient client;
    client.ConnectChannel(CreateTestChannel(address));
    client.Setup();
    client.GetVariableDefinitions();

    const auto &metadata = client.GetVariableMetaAll();
    const auto residual = std::find_if(
        metadata.begin(), metadata.end(), [](const VariableMetaData &meta) {
            return meta.name() == "x" && meta.type() == kResidual;
        });
    ASSERT_NE(residual, metadata.end());
    EXPECT_TRUE(residual->dynamic_shape());

    client.SendVariableShapes({client.CreateVariableShape("x", {3}, kOutput)});
    client.GetPartialDefinitions();

    const auto updated_residual = std::find_if(
        client.GetVariableMetaAll().begin(), client.GetVariableMetaAll().end(),
        [](const VariableMetaData &meta) {
            return meta.name() == "x" && meta.type() == kResidual;
        });
    ASSERT_NE(updated_residual, client.GetVariableMetaAll().end());
    ASSERT_EQ(updated_residual->shape_size(), 1);
    EXPECT_EQ(updated_residual->shape(0), 3);

    Variables values;
    values["a"] = Variable(kInput, {1});
    values["a"](0) = 4.0;
    values["x"] = Variable(kOutput, {3});
    values["x"](0) = 2.0;
    values["x"](1) = 3.0;
    values["x"](2) = 1.0;

    const Variables residuals = client.ComputeResiduals(values);
    ASSERT_EQ(residuals.at("x").Shape(), (std::vector<size_t>{3}));
    EXPECT_DOUBLE_EQ(residuals.at("x")(0), 0.0);
    EXPECT_DOUBLE_EQ(residuals.at("x")(1), 5.0);
    EXPECT_DOUBLE_EQ(residuals.at("x")(2), -3.0);

    const Partials partials = client.ComputeResidualGradients(values);
    EXPECT_EQ(partials.at({"x", "a"}).Shape(), (std::vector<size_t>{3}));
    EXPECT_EQ(partials.at({"x", "x"}).Shape(), (std::vector<size_t>{3, 3}));
    EXPECT_DOUBLE_EQ(partials.at({"x", "x"})(0), 4.0);
    EXPECT_DOUBLE_EQ(partials.at({"x", "x"})(4), 6.0);
    EXPECT_DOUBLE_EQ(partials.at({"x", "x"})(8), 2.0);
}
