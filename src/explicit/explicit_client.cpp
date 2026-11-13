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
#include "explicit.h"
#include "validation.h"

#include <data.pb.h>
#include <disciplines.pb.h>
#include <disciplines.grpc.pb.h>

using grpc::ChannelInterface;

using philote::Array;
using philote::ExplicitClient;
using philote::VariableMessage;
using philote::VariableMetaData;

using std::make_pair;
using std::map;
using std::pair;
using std::string;
using std::vector;

void ExplicitClient::ConnectChannel(std::shared_ptr<ChannelInterface> channel)
{
    DisciplineClient::ConnectChannel(channel);
    stub_ = ExplicitService::NewStub(channel);
}

philote::Variables ExplicitClient::ComputeFunction(const Variables &inputs)
{
    DiscreteVariables discrete_outputs;
    return ComputeFunction(inputs, {}, discrete_outputs);
}

philote::Variables ExplicitClient::ComputeFunction(const Variables &inputs,
                                                   const DiscreteVariables &discrete_inputs,
                                                   DiscreteVariables &discrete_outputs)
{
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + GetRPCTimeout());
    std::unique_ptr<grpc::ClientReaderWriterInterface<VariableMessage, VariableMessage>>
        stream(stub_->ComputeFunction(&context));

    // send/assign continuous inputs and preallocate outputs
    Variables outputs;

    for (const VariableMetaData &var : GetVariableMetaAll())
    {
        const string &name = var.name();

        if (var.type() == kInput)
        {
            // Only send if the input was actually provided
            if (inputs.count(name) > 0)
                inputs.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kInput);
        }

        if (var.type() == kOutput)
            outputs[var.name()] = Variable(var);
    }

    // send discrete inputs
    for (const auto &[name, value] : discrete_inputs)
    {
        VariableMessage msg;
        DiscreteVariable *dv = msg.mutable_discrete();
        dv->set_name(name);
        dv->set_type(VariableType::kDiscreteInput);
        *dv->mutable_value() = value;
        if (!stream->Write(msg))
            throw PhiloteServerError("Failed to write discrete input '" + name + "'");
    }

    // finish streaming data to the server
    stream->WritesDone();

    VariableMessage result;
    while (stream->Read(&result))
    {
        if (result.has_continuous())
        {
            const Array &array = result.continuous();
            const string &name = array.name();
            if (outputs.find(name) == outputs.end())
                throw PhiloteServerError("Unexpected output variable received: " + name);
            outputs[name].AssignChunk(array);
        }
        else if (result.has_discrete())
        {
            const DiscreteVariable &dv = result.discrete();
            discrete_outputs[dv.name()] = dv.value();
        }
    }

    grpc::Status status = stream->Finish();
    if (!status.ok())
    {
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            throw PhiloteServerError("RPC timeout after " +
                                   std::to_string(GetRPCTimeout().count()) +
                                   "ms: " + status.error_message());
        }
        throw PhiloteServerError("ComputeFunction RPC failed: [" +
                                 std::to_string(status.error_code()) + "] " +
                                 status.error_message());
    }

    return outputs;
}

philote::Partials ExplicitClient::ComputeGradient(const Variables &inputs)
{
    return ComputeGradient(inputs, {});
}

philote::Partials ExplicitClient::ComputeGradient(const Variables &inputs,
                                                   const DiscreteVariables &discrete_inputs)
{
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + GetRPCTimeout());
    std::unique_ptr<grpc::ClientReaderWriterInterface<VariableMessage, VariableMessage>>
        stream(stub_->ComputeGradient(&context));

    // send/assign continuous inputs
    for (const VariableMetaData &var : GetVariableMetaAll())
    {
        const string name = var.name();

        if (var.type() == kInput)
        {
            // Only send if the input was actually provided
            if (inputs.count(name) > 0)
                inputs.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kInput);
        }
    }

    // send discrete inputs
    for (const auto &[name, value] : discrete_inputs)
    {
        VariableMessage msg;
        DiscreteVariable *dv = msg.mutable_discrete();
        dv->set_name(name);
        dv->set_type(VariableType::kDiscreteInput);
        *dv->mutable_value() = value;
        if (!stream->Write(msg))
            throw PhiloteServerError("Failed to write discrete input '" + name + "'");
    }

    // finish streaming data to the server
    stream->WritesDone();

    // preallocate partials
    Partials partials;
    for (const auto &par : GetPartialsMetaConst())
    {
        partials[make_pair(par.name(), par.subname())] = Variable(par);
    }

    // process messages from server
    VariableMessage result;
    while (stream->Read(&result))
    {
        if (result.has_continuous())
        {
            const Array &array = result.continuous();
            const string name = array.name();
            const string subname = array.subname();
            const auto key = make_pair(name, subname);
            if (partials.find(key) == partials.end())
                throw PhiloteServerError("Unexpected partial received: " + name + "/" + subname);
            partials[key].AssignChunk(array);
        }
        // discrete messages are not expected in gradient responses
    }

    grpc::Status status = stream->Finish();
    if (!status.ok())
    {
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            throw PhiloteServerError("RPC timeout after " +
                                   std::to_string(GetRPCTimeout().count()) +
                                   "ms: " + status.error_message());
        }
        throw PhiloteServerError("ComputeGradient RPC failed: [" +
                                 std::to_string(status.error_code()) + "] " +
                                 status.error_message());
    }

    return partials;
}
