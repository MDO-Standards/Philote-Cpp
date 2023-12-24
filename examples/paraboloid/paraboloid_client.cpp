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
#include <iostream>

#include <grpcpp/grpcpp.h>

#include <variable.h>
#include <explicit.h>

using std::cout;
using std::endl;
using std::make_pair;
using std::string;
using std::vector;

using grpc::Channel;

using philote::Partials;
using philote::Variable;
using philote::Variables;

int main()
{
    std::shared_ptr<Channel> channel = grpc::CreateChannel("localhost:50051",
                                                           grpc::InsecureChannelCredentials());
    philote::ExplicitClient client;
    client.ConnectChannel(channel);

    // send stream options to the analysis server
    client.SendStreamOptions();

    // send stream options to the analysis server
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
    inputs["x"] = Variable(philote::kInput, {1});
    inputs["y"] = Variable(philote::kInput, {1});

    inputs["x"](0) = 1.0;
    inputs["y"](0) = 2.0;

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