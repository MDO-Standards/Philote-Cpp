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
#include "implicit.h"

using std::shared_ptr;
using std::string;

using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::ClientReaderWriter;

using philote::ImplicitClient;
using philote::Partials;
using philote::Variables;

void ImplicitClient::ConnectChannel(shared_ptr<ChannelInterface> channel)
{
    DisciplineClient::ConnectChannel(channel);
    stub_ = ImplicitService::NewStub(channel);
}

Variables ImplicitClient::ComputeResiduals(const Variables &vars)
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + GetRPCTimeout());
    std::unique_ptr<grpc::ClientReaderWriterInterface<Array, Array>>
        stream(stub_->ComputeResiduals(&context));

    // send/assign inputs and outputs, preallocate residuals
    Variables res;
    for (const VariableMetaData &var : GetVariableMetaAll())
    {
        const string &name = var.name();

        if (var.type() == kInput)
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double());

        if (var.type() == kOutput)
        {
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double());
            res[name] = Variable(var);
        }
    }

    // finish streaming data to the server
    stream->WritesDone();

    Array result;
    while (stream->Read(&result))
    {
        const string &name = result.name();
        res[name].AssignChunk(result);
    }

    grpc::Status status = stream->Finish();
    if (!status.ok())
    {
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            throw std::runtime_error("RPC timeout after " +
                                   std::to_string(GetRPCTimeout().count()) +
                                   "ms: " + status.error_message());
        }
        throw std::runtime_error("ComputeResiduals RPC failed: [" +
                                 std::to_string(status.error_code()) + "] " +
                                 status.error_message());
    }

    return res;
}

Variables ImplicitClient::SolveResiduals(const Variables &vars)
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + GetRPCTimeout());
    std::unique_ptr<grpc::ClientReaderWriterInterface<Array, Array>>
        stream(stub_->SolveResiduals(&context));

    // send inputs only (outputs are solved by the server)
    Variables out;
    for (const VariableMetaData &var : GetVariableMetaAll())
    {
        const string &name = var.name();

        if (var.type() == kInput)
        {
            // Only send if the input was actually provided
            if (vars.count(name) > 0)
                vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double());
        }

        if (var.type() == kOutput)
        {
            // Preallocate output (do not send)
            out[name] = Variable(var);
        }
    }

    // finish streaming data to the server
    stream->WritesDone();

    Array result;
    while (stream->Read(&result))
    {
        const string &name = result.name();
        out[name].AssignChunk(result);
    }

    grpc::Status status = stream->Finish();
    if (!status.ok())
    {
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            throw std::runtime_error("RPC timeout after " +
                                   std::to_string(GetRPCTimeout().count()) +
                                   "ms: " + status.error_message());
        }
        throw std::runtime_error("SolveResiduals RPC failed: [" +
                                 std::to_string(status.error_code()) + "] " +
                                 status.error_message());
    }

    return out;
}

Partials ImplicitClient::ComputeResidualGradients(const Variables &vars)
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + GetRPCTimeout());
    std::unique_ptr<grpc::ClientReaderWriterInterface<Array, Array>>
        stream(stub_->ComputeResidualGradients(&context));

    // send/assign inputs and outputs, preallocate residuals
    Variables out;
    for (const VariableMetaData &var : GetVariableMetaAll())
    {
        const string &name = var.name();

        if (var.type() == kInput)
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double());

        if (var.type() == kOutput)
        {
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double());
            out[name] = Variable(var);
        }
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
    Array result;
    while (stream->Read(&result))
    {
        const string name = result.name();
        const string subname = result.subname();

        partials[make_pair(name, subname)].AssignChunk(result);
    }

    grpc::Status status = stream->Finish();
    if (!status.ok())
    {
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            throw std::runtime_error("RPC timeout after " +
                                   std::to_string(GetRPCTimeout().count()) +
                                   "ms: " + status.error_message());
        }
        throw std::runtime_error("ComputeResidualGradients RPC failed: [" +
                                 std::to_string(status.error_code()) + "] " +
                                 status.error_message());
    }

    return partials;
}
