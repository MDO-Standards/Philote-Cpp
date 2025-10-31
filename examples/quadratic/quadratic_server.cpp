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
#include <iostream>

#include <grpcpp/grpcpp.h>

#include <implicit.h>

using grpc::Server;
using grpc::ServerBuilder;

using philote::ImplicitDiscipline;
using philote::Partials;
using philote::Variables;

using std::cout;
using std::endl;
using std::make_pair;
using std::pow;
using std::sqrt;
using std::string;
using std::unique_ptr;

class Quadratic : public ImplicitDiscipline
{
public:
    // Constructor
    Quadratic() = default;

    // Destructor
    ~Quadratic() = default;

private:
    void Setup() override
    {
        AddInput("a", {1}, "m");
        AddInput("b", {1}, "m");
        AddInput("c", {1}, "m");

        AddOutput("x", {1}, "m**2");
    }

    void SetupPartials() override
    {
        DeclarePartials("x", "a");
        DeclarePartials("x", "b");
        DeclarePartials("x", "c");
        DeclarePartials("x", "x");
    }

    void ComputeResiduals(const philote::Variables &inputs,
                          const philote::Variables &outputs,
                          philote::Variables &residuals) override
    {
        double a = inputs.at("a")(0);
        double b = inputs.at("b")(0);
        double c = inputs.at("c")(0);
        double x = outputs.at("x")(0);

        residuals.at("x")(0) = a * pow(x, 2) + b * x + c;
    }

    void SolveResiduals(const philote::Variables &inputs,
                        philote::Variables &outputs) override
    {
        double a = inputs.at("a")(0);
        double b = inputs.at("b")(0);
        double c = inputs.at("c")(0);

        outputs.at("x")(0) = (-b + sqrt(pow(b, 2) - 4 * a * c)) / (2 * a);
    }

    void ComputeResidualGradients(const philote::Variables &inputs,
                                  const philote::Variables &outputs,
                                  Partials &jac) override
    {
        double a = inputs.at("a")(0);
        double b = inputs.at("b")(0);
        double x = outputs.at("x")(0);

        jac[make_pair("x", "a")](0) = pow(x, 2);
        jac[make_pair("x", "b")](0) = x;
        jac[make_pair("x", "c")](0) = 1.0;
        jac[make_pair("x", "x")](0) = 2 * a * x + b;
    }
};

int main()
{
    string address("localhost:50051");
    auto service = std::make_shared<Quadratic>();

    ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    service->RegisterServices(builder);

    unique_ptr<Server> server(builder.BuildAndStart());

    cout << "Server listening on port: " << address << endl;

    server->Wait();

    return 0;
}