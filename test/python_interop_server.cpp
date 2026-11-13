#include <grpcpp/grpcpp.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "explicit.h"

using namespace philote;

namespace
{
class InteropDiscipline final : public ExplicitDiscipline
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
        const bool triple = discrete_inputs.at("mode").string_value() == "triple";
        const double scale = triple ? 3.0 : 2.0;
        for (size_t i = 0; i < inputs.at("x").Size(); ++i)
            outputs.at("y")(i) = scale * inputs.at("x")(i);

        auto &status = discrete_outputs["status"];
        auto *fields = status.mutable_struct_value()->mutable_fields();
        (*fields)["ok"].set_bool_value(true);
        (*fields)["mode"].set_string_value(triple ? "triple" : "double");
    }

    void ComputePartials(const Variables &inputs,
                         Partials &partials,
                         const DiscreteVariables &discrete_inputs) override
    {
        const double scale = discrete_inputs.at("mode").string_value() == "triple"
                                 ? 3.0
                                 : 2.0;
        auto &jac = partials.at({"y", "x"});
        const size_t n = inputs.at("x").Size();
        for (size_t i = 0; i < jac.Size(); ++i)
            jac(i) = 0.0;
        for (size_t i = 0; i < n; ++i)
            jac(i * n + i) = scale;
    }
};
} // namespace

int main(int argc, char **argv)
{
    if (argc != 2)
        return EXIT_FAILURE;

    const std::string address = "127.0.0.1:" + std::string(argv[1]);
    auto discipline = std::make_shared<InteropDiscipline>();
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    discipline->RegisterServices(builder);
    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server)
        return EXIT_FAILURE;

    std::cout << "READY" << std::endl;
    server->Wait();
    return EXIT_SUCCESS;
}

