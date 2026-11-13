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
#include "validation.h"

using std::shared_ptr;
using std::string;

using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::ClientReaderWriter;

using philote::Array;
using philote::DiscreteVariable;
using philote::DiscreteVariables;
using philote::ImplicitClient;
using philote::Partials;
using philote::VariableMessage;
using philote::Variables;

namespace
{
Variables PrepareResidualCall(const std::vector<philote::VariableMetaData> &metadata,
                              const Variables &vars)
{
    Variables residuals;
    for (const philote::VariableMetaData &var : metadata)
    {
        const string &name = var.name();
        if ((var.type() == philote::kInput || var.type() == philote::kOutput) &&
            vars.find(name) == vars.end())
        {
            throw philote::PhiloteValidationError(
                "Missing required variable for ComputeResiduals: " + name);
        }

        // Output metadata is a compatibility fallback for pre-0.8 servers.
        // Philote 0.8 residual metadata supersedes it when present.
        if (var.type() == philote::kOutput || var.type() == philote::kResidual)
            residuals[name] = philote::Variable(var);
    }
    return residuals;
}
} // namespace

void ImplicitClient::ConnectChannel(shared_ptr<ChannelInterface> channel)
{
    DisciplineClient::ConnectChannel(channel);
    stub_ = ImplicitService::NewStub(channel);
}

Variables ImplicitClient::ComputeResiduals(const Variables &vars)
{
    // Validate and allocate before opening a bidirectional stream. This keeps
    // local argument errors from abandoning an active gRPC stream.
    Variables res = PrepareResidualCall(GetVariableMetaAll(), vars);

    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + GetRPCTimeout());
    std::unique_ptr<grpc::ClientReaderWriterInterface<VariableMessage, VariableMessage>>
        stream(stub_->ComputeResiduals(&context));

    // send inputs and output guesses
    for (const VariableMetaData &var : GetVariableMetaAll())
    {
        const string &name = var.name();

        if (var.type() == kInput)
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kInput);

        if (var.type() == kOutput)
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kOutput);
    }

    // finish streaming data to the server
    stream->WritesDone();

    VariableMessage var_msg;
    while (stream->Read(&var_msg))
    {
        if (var_msg.has_continuous())
        {
            const Array &result = var_msg.continuous();
            const string &name = result.name();
            if (res.find(name) == res.end())
                throw PhiloteServerError("Unexpected residual variable received: " + name);
            res.at(name).AssignChunk(result);
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
        throw PhiloteServerError("ComputeResiduals RPC failed: [" +
                                 std::to_string(status.error_code()) + "] " +
                                 status.error_message());
    }

    return res;
}

Variables ImplicitClient::ComputeResiduals(const Variables &vars,
                                           const DiscreteVariables &discrete_inputs,
                                           DiscreteVariables &discrete_outputs)
{
    Variables res = PrepareResidualCall(GetVariableMetaAll(), vars);

    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + GetRPCTimeout());
    std::unique_ptr<grpc::ClientReaderWriterInterface<VariableMessage, VariableMessage>>
        stream(stub_->ComputeResiduals(&context));

    // send inputs and output guesses
    for (const VariableMetaData &var : GetVariableMetaAll())
    {
        const string &name = var.name();

        if (var.type() == kInput)
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kInput);

        if (var.type() == kOutput)
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kOutput);
    }

    // send discrete inputs
    for (const auto &dv : discrete_inputs)
    {
        VariableMessage msg;
        DiscreteVariable *disc = msg.mutable_discrete();
        disc->set_name(dv.first);
        disc->set_type(VariableType::kDiscreteInput);
        *disc->mutable_value() = dv.second;
        if (!stream->Write(msg))
            throw PhiloteServerError("Failed to write discrete input '" + dv.first + "'");
    }

    // Discrete output values are state for implicit evaluations and are sent
    // to the server just like continuous output guesses.
    for (const auto &dv : discrete_outputs)
    {
        VariableMessage msg;
        DiscreteVariable *disc = msg.mutable_discrete();
        disc->set_name(dv.first);
        disc->set_type(VariableType::kDiscreteOutput);
        *disc->mutable_value() = dv.second;
        if (!stream->Write(msg))
            throw PhiloteServerError("Failed to write discrete output '" + dv.first + "'");
    }

    // finish streaming data to the server
    stream->WritesDone();

    VariableMessage var_msg;
    while (stream->Read(&var_msg))
    {
        if (var_msg.has_continuous())
        {
            const Array &result = var_msg.continuous();
            const string &name = result.name();
            if (res.find(name) == res.end())
                throw PhiloteServerError("Unexpected residual variable received: " + name);
            res.at(name).AssignChunk(result);
        }
        else if (var_msg.has_discrete())
        {
            const DiscreteVariable &dv = var_msg.discrete();
            if (dv.type() == VariableType::kDiscreteOutput)
            {
                discrete_outputs[dv.name()] = dv.value();
            }
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
        throw PhiloteServerError("ComputeResiduals RPC failed: [" +
                                 std::to_string(status.error_code()) + "] " +
                                 status.error_message());
    }

    return res;
}

Variables ImplicitClient::SolveResiduals(const Variables &vars)
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + GetRPCTimeout());
    std::unique_ptr<grpc::ClientReaderWriterInterface<VariableMessage, VariableMessage>>
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
                vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kInput);
        }

        if (var.type() == kOutput)
        {
            // Preallocate output (do not send)
            out[name] = Variable(var);
        }
    }

    // finish streaming data to the server
    stream->WritesDone();

    VariableMessage var_msg;
    while (stream->Read(&var_msg))
    {
        if (var_msg.has_continuous())
        {
            const Array &result = var_msg.continuous();
            const string &name = result.name();
            if (out.find(name) == out.end())
                throw PhiloteServerError("Unexpected output variable received: " + name);
            out.at(name).AssignChunk(result);
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
        throw PhiloteServerError("SolveResiduals RPC failed: [" +
                                 std::to_string(status.error_code()) + "] " +
                                 status.error_message());
    }

    return out;
}

Variables ImplicitClient::SolveResiduals(const Variables &vars,
                                         const DiscreteVariables &discrete_inputs)
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + GetRPCTimeout());
    std::unique_ptr<grpc::ClientReaderWriterInterface<VariableMessage, VariableMessage>>
        stream(stub_->SolveResiduals(&context));

    // send inputs only (outputs are solved by the server)
    Variables out;
    for (const VariableMetaData &var : GetVariableMetaAll())
    {
        const string &name = var.name();

        if (var.type() == kInput)
        {
            if (vars.count(name) > 0)
                vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kInput);
        }

        if (var.type() == kOutput)
        {
            out[name] = Variable(var);
        }
    }

    // send discrete inputs
    for (const auto &dv : discrete_inputs)
    {
        VariableMessage msg;
        DiscreteVariable *disc = msg.mutable_discrete();
        disc->set_name(dv.first);
        disc->set_type(VariableType::kDiscreteInput);
        *disc->mutable_value() = dv.second;
        if (!stream->Write(msg))
            throw PhiloteServerError("Failed to write discrete input '" + dv.first + "'");
    }

    // finish streaming data to the server
    stream->WritesDone();

    VariableMessage var_msg;
    while (stream->Read(&var_msg))
    {
        if (var_msg.has_continuous())
        {
            const Array &result = var_msg.continuous();
            const string &name = result.name();
            if (out.find(name) == out.end())
                throw PhiloteServerError("Unexpected output variable received: " + name);
            out.at(name).AssignChunk(result);
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
        throw PhiloteServerError("SolveResiduals RPC failed: [" +
                                 std::to_string(status.error_code()) + "] " +
                                 status.error_message());
    }

    return out;
}

Partials ImplicitClient::ComputeResidualGradients(const Variables &vars)
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + GetRPCTimeout());
    std::unique_ptr<grpc::ClientReaderWriterInterface<VariableMessage, VariableMessage>>
        stream(stub_->ComputeResidualGradients(&context));

    // send/assign inputs and outputs, preallocate residuals
    Variables out;
    for (const VariableMetaData &var : GetVariableMetaAll())
    {
        const string &name = var.name();

        if (var.type() == kInput)
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kInput);

        if (var.type() == kOutput)
        {
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kOutput);
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
    VariableMessage var_msg;
    while (stream->Read(&var_msg))
    {
        if (var_msg.has_continuous())
        {
            const Array &result = var_msg.continuous();
            const string name = result.name();
            const string subname = result.subname();

            const auto key = make_pair(name, subname);
            if (partials.find(key) == partials.end())
                throw PhiloteServerError("Unexpected partial received: " + name + "/" + subname);
            partials.at(key).AssignChunk(result);
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
        throw PhiloteServerError("ComputeResidualGradients RPC failed: [" +
                                 std::to_string(status.error_code()) + "] " +
                                 status.error_message());
    }

    return partials;
}

Partials ImplicitClient::ComputeResidualGradients(const Variables &vars,
                                                   const DiscreteVariables &discrete_inputs,
                                                   DiscreteVariables &discrete_outputs)
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + GetRPCTimeout());
    std::unique_ptr<grpc::ClientReaderWriterInterface<VariableMessage, VariableMessage>>
        stream(stub_->ComputeResidualGradients(&context));

    // send/assign inputs and outputs
    Variables out;
    for (const VariableMetaData &var : GetVariableMetaAll())
    {
        const string &name = var.name();

        if (var.type() == kInput)
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kInput);

        if (var.type() == kOutput)
        {
            vars.at(name).Send(name, "", stream.get(), GetStreamOptions().num_double(), kOutput);
            out[name] = Variable(var);
        }
    }

    // send discrete inputs
    for (const auto &dv : discrete_inputs)
    {
        VariableMessage msg;
        DiscreteVariable *disc = msg.mutable_discrete();
        disc->set_name(dv.first);
        disc->set_type(VariableType::kDiscreteInput);
        *disc->mutable_value() = dv.second;
        if (!stream->Write(msg))
            throw PhiloteServerError("Failed to write discrete input '" + dv.first + "'");
    }

    for (const auto &dv : discrete_outputs)
    {
        VariableMessage msg;
        DiscreteVariable *disc = msg.mutable_discrete();
        disc->set_name(dv.first);
        disc->set_type(VariableType::kDiscreteOutput);
        *disc->mutable_value() = dv.second;
        if (!stream->Write(msg))
            throw PhiloteServerError("Failed to write discrete output '" + dv.first + "'");
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
    VariableMessage var_msg;
    while (stream->Read(&var_msg))
    {
        if (var_msg.has_continuous())
        {
            const Array &result = var_msg.continuous();
            const string name = result.name();
            const string subname = result.subname();

            const auto key = make_pair(name, subname);
            if (partials.find(key) == partials.end())
                throw PhiloteServerError("Unexpected partial received: " + name + "/" + subname);
            partials.at(key).AssignChunk(result);
        }
        else if (var_msg.has_discrete())
        {
            const DiscreteVariable &dv = var_msg.discrete();
            if (dv.type() == VariableType::kDiscreteOutput)
            {
                discrete_outputs[dv.name()] = dv.value();
            }
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
        throw PhiloteServerError("ComputeResidualGradients RPC failed: [" +
                                 std::to_string(status.error_code()) + "] " +
                                 status.error_message());
    }

    return partials;
}
