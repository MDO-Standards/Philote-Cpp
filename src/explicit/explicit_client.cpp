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
#include <Philote/explicit.h>

#include <data.pb.h>
#include <disciplines.pb.h>
#include <disciplines.grpc.pb.h>

using grpc::ChannelInterface;

using philote::Array;
using philote::ExplicitClient;
using philote::VariableMetaData;

using std::make_pair;
using std::map;
using std::pair;
using std::string;
using std::vector;

ExplicitClient::ExplicitClient()
{
    // default host name
    host_ = "localhost:50051";
}

ExplicitClient::ExplicitClient(const std::string &host)
{
    host_ = host;
}

ExplicitClient::~ExplicitClient() {}

void ExplicitClient::ConnectChannel(std::shared_ptr<ChannelInterface> channel)
{
    DisciplineClient::ConnectChannel(channel);
    stub_ = ExplicitService::NewStub(channel);
}

philote::Variables ExplicitClient::ComputeFunction(const Variables &inputs)
{
    grpc::ClientContext context;
    std::shared_ptr<grpc::ClientReaderWriter<Array, Array>>
        stream(stub_->ComputeFunction(&context));

    // send/assign inputs and preallocate outputs
    Variables outputs;

    for (const VariableMetaData &var : var_meta_)
    {
        const string name = var.name();

        if (var.type() == kInput)
            inputs.at(name).Send(name, "", stream, stream_options_.num_double());

        if (var.type() == kOutput)
            outputs[var.name()] = Variable(var);
    }

    // finish streaming data to the server
    stream->WritesDone();

    Array result;
    while (stream->Read(&result))
    {
        const string name = result.name();
        outputs[name].AssignChunk(result);
    }

    grpc::Status status = stream->Finish();

    return outputs;
}

philote::Partials ExplicitClient::ComputeGradient(const Variables &inputs)
{
    grpc::ClientContext context;
    std::shared_ptr<grpc::ClientReaderWriter<Array, Array>>
        stream(stub_->ComputeGradient(&context));

    // send/assign inputs
    for (const VariableMetaData &var : var_meta_)
    {
        const string name = var.name();
        const string subname = var.name();

        if (var.type() == kInput)
            inputs.at(name).Send(name, subname, stream, stream_options_.num_double());
    }

    // finish streaming data to the server
    stream->WritesDone();

    // preallocate partials
    Partials partials;
    for (const auto &par : partials_meta_)
    {
        partials[make_pair(par.name(), par.subname())] = Variable(par);
    }

    // process messages from server
    Array result;
    while (stream->Read(&result))
    {
        const string name = result.name();
        const string subname = result.subname();

        partials[make_pair(name, subname)].AssignChunk(result);
    }

    grpc::Status status = stream->Finish();

    return partials;
}