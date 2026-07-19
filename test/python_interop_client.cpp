#include <grpcpp/grpcpp.h>

#include <cstdlib>
#include <string>

#include "explicit.h"

using namespace philote;

int main(int argc, char **argv)
{
    if (argc != 2)
        return EXIT_FAILURE;

    ExplicitClient client;
    client.ConnectChannel(grpc::CreateChannel(
        "127.0.0.1:" + std::string(argv[1]), grpc::InsecureChannelCredentials()));
    client.GetAvailableOptions();
    if (client.GetOptionsList().at("config") != "dict" ||
        client.GetOptionsList().at("label") != "str")
        return EXIT_FAILURE;

    client.Setup();
    client.GetVariableDefinitions();
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
    if (outputs.at("y")(0) != 3.0 || outputs.at("y")(1) != 6.0 ||
        outputs.at("y")(2) != 12.0)
        return EXIT_FAILURE;
    if (!discrete_outputs.at("status").struct_value().fields().at("ok").bool_value())
        return EXIT_FAILURE;

    const Partials partials = client.ComputeGradient(inputs, discrete_inputs);
    const auto &jac = partials.at({"y", "x"});
    if (jac.Shape() != std::vector<size_t>({3, 3}) || jac(0) != 3.0 ||
        jac(4) != 3.0 || jac(8) != 3.0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

