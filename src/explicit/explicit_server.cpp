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
#include <vector>
#include "explicit.h"

using std::make_pair;
using std::string;
using std::vector;

using google::protobuf::Empty;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriterInterface;
using grpc::ServerWriter;
using grpc::Status;

using philote::ExplicitServer;
using philote::Partials;
using philote::Variables;

ExplicitServer::~ExplicitServer()
{
    UnlinkPointers();
}

void ExplicitServer::LinkPointers(philote::ExplicitDiscipline *implementation)
{
    implementation_ = implementation;
}

void ExplicitServer::UnlinkPointers()
{
    implementation_ = nullptr;
}

Status ExplicitServer::ComputeFunction(ServerContext *context,
                                       grpc::ServerReaderWriterInterface<::philote::Array,
                                                                         ::philote::Array> *stream)
{
    if (!implementation_)
    {
        return Status(grpc::StatusCode::FAILED_PRECONDITION, "Discipline implementation not linked");
    }

    if (!stream)
    {
        return Status(grpc::StatusCode::INVALID_ARGUMENT, "Stream is null");
    }

    philote::Array array;

    // preallocate the inputs based on meta data
    Variables inputs;
    const auto *discipline = static_cast<philote::Discipline *>(implementation_);
    if (!discipline)
    {
        return Status(grpc::StatusCode::INTERNAL, "Failed to cast implementation to Discipline");
    }

    for (const auto &var : discipline->var_meta())
    {
        string name = var.name();
        if (var.type() == kInput)
            inputs[name] = Variable(var);
    }

    while (stream->Read(&array))
    {
        // get variables from the stream message
        const string &name = array.name();

        // get the variable corresponding to the current message
        const auto &var = std::find_if(discipline->var_meta().begin(),
                                       discipline->var_meta().end(),
                                       [&name](const VariableMetaData &var)
                                       { return var.name() == name; });

        if (var == discipline->var_meta().end())
        {
            return Status(grpc::StatusCode::INVALID_ARGUMENT, "Variable not found: " + name);
        }

        // obtain the inputs and discrete inputs from the stream
        if (var->type() == VariableType::kInput)
        {
            try
            {
                // set the variable slice
                inputs[name].AssignChunk(array);
            }
            catch (const std::exception &e)
            {
                return Status(grpc::StatusCode::INVALID_ARGUMENT,
                              "Failed to assign chunk for variable " + name + ": " + e.what());
            }
        }
        else
        {
            return Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid variable type for input: " + name);
        }
    }

    // preallocate outputs
    Variables outputs;
    for (const VariableMetaData &var : discipline->var_meta())
    {
        if (var.type() == kOutput)
            outputs[var.name()] = Variable(var);
    }

    // call the discipline developer-defined Compute function
    try
    {
        implementation_->Compute(inputs, outputs);
    }
    catch (const std::exception &e)
    {
        return Status(grpc::StatusCode::INTERNAL,
                      "Failed to compute outputs: " + std::string(e.what()));
    }

    // iterate through continuous outputs
    for (const auto &out : outputs)
    {
        const string &name = out.first;
        try
        {
            out.second.Send(name, "", stream, discipline->stream_opts().num_double());
        }
        catch (const std::exception &e)
        {
            return Status(grpc::StatusCode::INTERNAL,
                          "Failed to send output " + name + ": " + e.what());
        }
    }

    return Status::OK;
}

Status ExplicitServer::ComputeGradient(ServerContext *context,
                                       grpc::ServerReaderWriterInterface<::philote::Array,
                                                                         ::philote::Array> *stream)
{
    if (!implementation_)
    {
        return Status(grpc::StatusCode::FAILED_PRECONDITION, "Discipline implementation not linked");
    }

    if (!stream)
    {
        return Status(grpc::StatusCode::INVALID_ARGUMENT, "Stream is null");
    }

    philote::Array array;

    // preallocate the inputs based on meta data
    Variables inputs;
    const auto *discipline = static_cast<philote::Discipline *>(implementation_);
    if (!discipline)
    {
        return Status(grpc::StatusCode::INTERNAL, "Failed to cast implementation to Discipline");
    }

    for (const auto &var : discipline->var_meta())
    {
        string name = var.name();
        if (var.type() == kInput)
            inputs[name] = Variable(var);
    }

    while (stream->Read(&array))
    {
        // get variables from the stream message
        const string &name = array.name();
        const auto &start = array.start();
        const auto &end = array.end();

        // get the variable corresponding to the current message
        const auto &var = std::find_if(discipline->var_meta().begin(),
                                       discipline->var_meta().end(),
                                       [&name](const VariableMetaData &var)
                                       { return var.name() == name; });

        if (var == discipline->var_meta().end())
        {
            return Status(grpc::StatusCode::INVALID_ARGUMENT, "Variable not found: " + name);
        }

        // obtain the inputs and discrete inputs from the stream
        if (var->type() == VariableType::kInput)
        {
            try
            {
                // set the variable slice
                inputs[name].AssignChunk(array);
            }
            catch (const std::exception &e)
            {
                return Status(grpc::StatusCode::INVALID_ARGUMENT,
                              "Failed to assign chunk for variable " + name + ": " + e.what());
            }
        }
        else
        {
            return Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid variable type for input: " + name);
        }
    }

    // preallocate outputs
    Partials partials;
    for (const PartialsMetaData &par : discipline->partials_meta())
    {
        vector<size_t> shape;
        for (const int64_t &dim : par.shape())
            shape.push_back(dim);

        partials[make_pair(par.name(), par.subname())] = Variable(kOutput, shape);
    }

    // call the discipline developer-defined Compute function
    try
    {
        implementation_->ComputePartials(inputs, partials);
    }
    catch (const std::exception &e)
    {
        return Status(grpc::StatusCode::INTERNAL,
                      "Failed to compute partials: " + std::string(e.what()));
    }

    // iterate through continuous outputs
    for (const auto &par : partials)
    {
        const string &name = par.first.first;
        const string &subname = par.first.second;
        try
        {
            par.second.Send(name, subname, stream, discipline->stream_opts().num_double());
        }
        catch (const std::exception &e)
        {
            return Status(grpc::StatusCode::INTERNAL,
                          "Failed to send partial " + name + "/" + subname + ": " + e.what());
        }
    }

    return Status::OK;
}