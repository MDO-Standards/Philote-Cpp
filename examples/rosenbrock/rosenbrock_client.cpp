/*
    Philote C++ Bindings

    Copyright 2022-2023 Christopher A. Lupp

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
#include <iostream>

#include <grpcpp/grpcpp.h>

#include <Philote/variable.h>
#include <Philote/explicit.h>

using std::cout;
using std::endl;
using std::make_pair;
using std::string;
using std::vector;

using grpc::Channel;

using philote::DisciplineOptions;
using philote::Partials;
using philote::Variable;
using philote::Variables;


int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <number>" << std::endl;
		return 1;
	}

	// Read the command line argument as a string
	std::string input = argv[1];
	char *endptr;
	unsigned long dimension = std::strtoll(input.c_str(), &endptr, 10);


	// create the gRPC channel
	std::shared_ptr<Channel> channel = grpc::CreateChannel("localhost:50051",
			grpc::InsecureChannelCredentials());
	philote::ExplicitClient client;
	client.ConnectChannel(channel);

	// send stream options to the analysis server
	client.SendStreamOptions();

	// create and send discipline options
	google::protobuf::Struct options;
	(*options.mutable_fields())["dimension"] = google::protobuf::Value();
	(*options.mutable_fields())["dimension"].set_number_value(dimension);
	philote::DisciplineOptions options_message;
	options_message.mutable_options()->CopyFrom(options);

	client.SendOptions(options_message);

	// call the discipline server setup RPC
	client.Setup();

	// get the variable meta data from the server
	client.GetVariableDefinitions();

	vector<string> vars = client.GetVariableNames();
	cout << "Variable List" << endl;
	for (auto &name : vars)
	{
		cout << name << endl;
	}
	cout << endl
		 << endl;

	for (auto &name : vars)
	{
		auto var = client.GetVariableMeta(name);
		cout << "name: " << var.name() << ", ";

		if (var.type() == philote::kInput)
			cout << "input";
		else if (var.type() == philote::kOutput)
		{
			cout << "output";
		}
		cout << endl;
	}

	// get the partials meta data from the server
	client.GetPartialDefinitions();

	auto partials_meta = client.partials_meta();

	cout << endl;
	cout << "Partials Meta" << endl
		 << "-------------" << endl;
	for (const auto &par : partials_meta)
	{
		cout << "d" << par.name() << "/d" << par.subname() << ", size: " << par.shape()[0] << endl;
	}

	// define the inputs and run a function evaluation
	Variables inputs;
	inputs["x"] = Variable(philote::kInput, {dimension});

	for (int i = 0; i < dimension; i++)
		inputs["x"](i) = 1.0;

	Variables outputs = client.ComputeFunction(inputs);

	cout << endl
		 << endl;
	cout << "Outputs" << endl;
	cout << "-------" << endl;
	for (auto &var : outputs)
	{
		cout << "name: " << var.first << ", " << var.second(0) << endl;
	}

	// run a gradient evaluation
	Partials partials = client.ComputeGradient(inputs);

	cout << endl
		 << endl
		 << "Partials" << endl
		 << "--------" << endl;
	cout << "df_dx: " << partials[make_pair("f_xy", "x")](0) << endl;
	cout << "df_dy: " << partials[make_pair("f_xy", "y")](0) << endl;

    return 0;
}