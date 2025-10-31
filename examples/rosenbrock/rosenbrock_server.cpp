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
#include <vector>

#include <grpcpp/grpcpp.h>

#include <explicit.h>

using google::protobuf::Struct;

using grpc::Server;
using grpc::ServerBuilder;

using philote::ExplicitDiscipline;
using philote::Partials;
using philote::Variables;

using std::cout;
using std::endl;
using std::make_pair;
using std::pow;
using std::vector;

class Rosenbrock : public ExplicitDiscipline
{
public:
    // Constructor
    Rosenbrock()
	{
		// default dimensions
		n_ = 2;
	}

    // Destructor
    ~Rosenbrock() = default;

private:
    // dimension of the Rosenbrock function
    int64_t n_;

    // Declare available options
    void Initialize() override
    {
        ExplicitDiscipline::Initialize();
        AddOption("dimension", "int");
    }

    // Extract option values from protobuf Struct
    void SetOptions(const Struct &options_struct) override
    {
        auto it = options_struct.fields().find("dimension");
        if (it != options_struct.fields().end())
        {
            n_ = static_cast<int64_t>(it->second.number_value());
            cout << "Dimension set to: " << n_ << endl;
        }

        // Call parent implementation to invoke Configure()
        ExplicitDiscipline::SetOptions(options_struct);
    }

    // Defines the variables for the discipline
    void Setup() override
    {
        AddInput("x", {n_}, "");

        AddOutput("f", {1}, "");
    }

    // Defines the partials for the discipline
    void SetupPartials() override
    {
        DeclarePartials("f", "x");
    }

    // Computes
    void Compute(const philote::Variables &inputs, philote::Variables &outputs) override
    {
		// preallocate and assign the inputs
        vector<double> x(n_);
		for (int i = 0; i < inputs.at("x").Size(); i++)
		{
			x.at(i) = inputs.at("x")(i);
		}

		// compute the function
		double f = 0.0;
		for (int i = 0; i < n_ - 1; ++i)
		{
			double x_i = x[i];
			double x_i1 = x[i + 1];
			f += 100.0 * std::pow(x_i1 - x_i * x_i, 2) + std::pow(1.0 - x_i, 2);
		}

        outputs.at("f")(0) = f;
    }

    void ComputePartials(const philote::Variables &inputs, Partials &jac) override
    {
		// preallocate and assign the inputs
		vector<double> x(n_);
		for (int i = 0; i < inputs.at("x").Size(); i++)
		{
			x.at(i) = inputs.at("x")(i);
		}

		std::vector<double> gradient(n_, 0.0);

		for (int i = 0; i < n_ - 1; ++i)
		{
			double x_i = x.at(i);
			double x_i1 = x.at(i + 1);

			double dx_i = -400.0 * x_i * (x_i1 - x_i * x_i) - 2.0 * (1.0 - x_i);
			double dx_i1 = 200.0 * (x_i1 - x_i * x_i);

			gradient.at(i) += dx_i;
			gradient.at(i + 1) += dx_i1;
		}

		for (int i = 0; i < n_; ++i)
			jac[make_pair("f", "x")](i) = gradient.at(i);
    }
};

int main()
{
    std::string address("localhost:50051");
    auto service = std::make_shared<Rosenbrock>();

    ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    service->RegisterServices(builder);

    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "Server listening on port: " << address << std::endl;

    server->Wait();

    return 0;
}