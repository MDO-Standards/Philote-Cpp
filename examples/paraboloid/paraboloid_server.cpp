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

    This work has been cleared for public release, distribution unlimited, case
    number: AFRL-2023-5716.

    The views expressed are those of the authors and do not reflect the
    official guidance or position of the United States Government, the
    Department of Defense or of the United States Air Force.

    Statement from DoD: The Appearance of external hyperlinks does not
    constitute endorsement by the United States Department of Defense (DoD) of
    the linked websites, of the information, products, or services contained
    therein. The DoD does not exercise any editorial, security, or other
    control over the information you may find at these locations.
*/
#include <cmath>

#include <grpcpp/grpcpp.h>

#include <explicit.h>

using grpc::Server;
using grpc::ServerBuilder;

using philote::ExplicitDiscipline;
using philote::Partials;
using philote::Variables;

using std::make_pair;
using std::pow;

class Paraboloid : public ExplicitDiscipline
{
public:
    // Constructor
    Paraboloid() = default;

    // Destructor
    ~Paraboloid() = default;

private:
    // Initialize function to set up available options
    void Initialize() override
    {
        // Call parent initialize first
        ExplicitDiscipline::Initialize();
        
        // Add discipline-specific options
        AddOption("scale_factor", "float");
        AddOption("enable_scaling", "bool");
    }

    // Configure function called after options are set
    void Configure() override
    {
        // Any configuration based on options would go here
        // For this example, we'll just demonstrate the capability
    }

    // Defines the variables for the discipline
    void Setup() override
    {
        AddInput("x", {1}, "m");
        AddInput("y", {1}, "m");

        AddOutput("f_xy", {1}, "m**2");
    }

    // Defines the partials for the discipline
    void SetupPartials() override
    {
        DeclarePartials("f_xy", "x");
        DeclarePartials("f_xy", "y");
    }

    // Computes
    void Compute(const philote::Variables &inputs, philote::Variables &outputs) override
    {
        double x = inputs.at("x")(0);
        double y = inputs.at("y")(0);

        outputs.at("f_xy")(0) = pow(x - 3.0, 2.0) + x * y +
                                pow(y + 4.0, 2.0) - 3.0;
    }

    void ComputePartials(const philote::Variables &inputs, Partials &jac) override
    {
        double x = inputs.at("x")(0);
        double y = inputs.at("y")(0);

        // Approach 1: Traditional std::map with pair keys using std::make_pair
        jac[make_pair("f_xy", "x")](0) = 2.0 * x - 6.0 + y;
        jac[make_pair("f_xy", "y")](0) = 2.0 * y + 8.0 + x;

        // Approach 2: Alternative cleaner syntax using PartialsPairDict
        // This provides the same functionality with more readable syntax
        // To use this approach, uncomment the following lines:
        //
        // philote::PartialsPairDict pair_jac;
        // pair_jac("f_xy", "x")(0) = 2.0 * x - 6.0 + y;
        // pair_jac("f_xy", "y")(0) = 2.0 * y + 8.0 + x;
    }
};

int main()
{
    std::string address("localhost:50051");
    auto service = std::make_shared<Paraboloid>();

    ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    service->RegisterServices(builder);

    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "Server listening on port: " << address << std::endl;

    server->Wait();

    return 0;
}