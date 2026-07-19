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
#include "discipline_server.h"
#include "discipline.h"
#include "validation.h"

using std::string;
using std::vector;

using google::protobuf::Empty;
using google::protobuf::Struct;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

using philote::DisciplineServer;

DisciplineServer::~DisciplineServer() noexcept
{
    UnlinkPointers();
}

bool DisciplineServer::DisciplinePointerNull() noexcept
{
    if (discipline_ == nullptr)
        return true;
    else
        return false;
}

void DisciplineServer::LinkPointers(std::shared_ptr<philote::Discipline> discipline)
{
    discipline_ = discipline;
}

void DisciplineServer::UnlinkPointers()
{
    discipline_.reset();
}

grpc::Status DisciplineServer::GetInfo(ServerContext *context,
                                       const Empty *request,
                                       DisciplineProperties *response)
{
    if (discipline_ == nullptr)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                          "Discipline not linked to server");
    }

    *response = discipline_->properties();

    return Status::OK;
}

Status DisciplineServer::SetStreamOptions(ServerContext *context,
                                          const StreamOptions *request,
                                          Empty *response)
{
    if (discipline_ == nullptr)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                          "Discipline not linked to server");
    }

    if (request == nullptr || request->num_double() <= 0)
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "SetStreamOptions: num_double must be positive");
    }

    discipline_->stream_opts() = *request;

    return Status::OK;
}

grpc::Status DisciplineServer::SetOptions(ServerContext *context,
                                          const DisciplineOptions *request,
                                          Empty *response)
{
    if (discipline_ == nullptr)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                          "Discipline not linked to server");
    }

    if (request == nullptr)
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "SetOptions: request is null");

    try
    {
        discipline_->SetOptions(request->options());
    }
    catch (const PhiloteValidationError &e)
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    }
    catch (const std::exception &e)
    {
        return grpc::Status(grpc::StatusCode::INTERNAL,
                            "SetOptions failed: " + string(e.what()));
    }
    catch (...)
    {
        return grpc::Status(grpc::StatusCode::INTERNAL, "SetOptions failed");
    }

    return Status::OK;
}

Status DisciplineServer::GetVariableDefinitions(ServerContext *context,
                                                const Empty *request,
                                                ServerWriter<VariableMetaData> *writer)
{
    if (discipline_ == nullptr)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                          "Discipline not linked to server");
    }

    if (!writer)
        return Status::OK;
    for (const VariableMetaData &var : discipline_->var_meta())
    {
        if (!writer->Write(var))
        {
            return grpc::Status(grpc::StatusCode::INTERNAL,
                              "Failed to write variable metadata for '" + var.name() + "'");
        }
    }

    // Stream discrete variable metadata
    for (const VariableMetaData &var : discipline_->discrete_var_meta())
    {
        if (!writer->Write(var))
        {
            return grpc::Status(grpc::StatusCode::INTERNAL,
                              "Failed to write discrete variable metadata for '" + var.name() + "'");
        }
    }

    return Status::OK;
}

Status DisciplineServer::GetPartialDefinitions(ServerContext *context,
                                               const Empty *request,
                                               ServerWriter<PartialsMetaData> *writer)
{
    if (discipline_ == nullptr)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                          "Discipline not linked to server");
    }

    if (!writer)
        return Status::OK;
    for (const PartialsMetaData &partial : discipline_->partials_meta())
    {
        if (!writer->Write(partial))
        {
            return grpc::Status(grpc::StatusCode::INTERNAL,
                              "Failed to write partial metadata for ('" +
                              partial.name() + "', '" + partial.subname() + "')");
        }
    }

    return Status::OK;
}

grpc::Status DisciplineServer::SetVariableShapes(ServerContext *context,
                                                  grpc::ServerReader<VariableMetaData> *reader,
                                                  Empty *response)
{
    if (discipline_ == nullptr)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                          "Discipline not linked to server");
    }

    if (reader == nullptr)
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "SetVariableShapes: reader is null");

    VariableMetaData meta;
    while (reader->Read(&meta))
    {
        if (meta.name().empty())
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                "SetVariableShapes: variable name must not be empty");
        if (meta.type() != kInput && meta.type() != kOutput)
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                "SetVariableShapes: variable type must be kInput or kOutput");

        // validate shape has positive dimensions
        for (int i = 0; i < meta.shape_size(); i++)
        {
            if (meta.shape(i) <= 0)
            {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                  "SetVariableShapes: shape dimensions must be positive for variable '" +
                                  meta.name() + "'");
            }
        }

        // find the matching variable and update its shape
        bool found = false;
        for (auto &var : discipline_->var_meta())
        {
            if (var.name() == meta.name() && var.type() == meta.type())
            {
                if (!var.dynamic_shape())
                {
                    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                      "Variable '" + meta.name() + "' does not allow dynamic shapes.");
                }
                var.clear_shape();
                for (int i = 0; i < meta.shape_size(); i++)
                    var.add_shape(meta.shape(i));
                found = true;
                break;
            }
        }

        if (!found)
        {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                              "SetVariableShapes: variable '" + meta.name() + "' not found.");
        }

        // Implicit outputs have a matching residual metadata entry.
        if (meta.type() == kOutput)
        {
            for (auto &var : discipline_->var_meta())
            {
                if (var.name() == meta.name() && var.type() == kResidual &&
                    var.dynamic_shape())
                {
                    var.clear_shape();
                    for (const auto dim : meta.shape())
                        var.add_shape(dim);
                    break;
                }
            }
        }
    }


    try
    {
        discipline_->RefreshPartialShapes();
    }
    catch (const PhiloteValidationError &e)
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    }
    catch (const std::exception &e)
    {
        return grpc::Status(grpc::StatusCode::INTERNAL,
                            "SetVariableShapes failed: " + string(e.what()));
    }

    return Status::OK;
}

grpc::Status DisciplineServer::Setup(grpc::ServerContext *context,
                                     const Empty *request,
                                     Empty *response)
{
    if (discipline_ == nullptr)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                          "Discipline not linked to server");
    }

    if (!discipline_->var_meta().empty() or !discipline_->partials_meta().empty()
        or !discipline_->discrete_var_meta().empty())
    {
        // clear any existing meta data
        discipline_->var_meta().clear();
        discipline_->discrete_var_meta().clear();
        discipline_->partials_meta().clear();
    }

    // run the developer-defined setup functions
    try
    {
        discipline_->Setup();
    }
    catch (const PhiloteValidationError &e)
    {
        return Status(grpc::INVALID_ARGUMENT, e.what());
    }
    catch (const std::exception &e)
    {
        return Status(grpc::INTERNAL, "Setup failed: " + string(e.what()));
    }
    catch (...)
    {
        return Status(grpc::INTERNAL, "Internal server error during Setup call.");
    }

    try
    {
        discipline_->SetupPartials();
    }
    catch (const PhiloteValidationError &e)
    {
        return Status(grpc::INVALID_ARGUMENT, e.what());
    }
    catch (const std::exception &e)
    {
        return Status(grpc::INTERNAL, "SetupPartials failed: " + string(e.what()));
    }
    catch (...)
    {
        return Status(grpc::INTERNAL, "Internal server error during SetupPartials call.");
    }

    return Status::OK;
}

::grpc::Status philote::DisciplineServer::GetAvailableOptions(::grpc::ServerContext *context,
                                                              const ::google::protobuf::Empty *request,
                                                              ::philote::OptionsList *response)
{
    if (discipline_ == nullptr)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                          "Discipline not linked to server");
    }

    // Populate the options list from the discipline's available options
    for (const auto &option : discipline_->options_list())
    {
        response->add_options(option.first);

        // Map string type names to DataType enum
        if (option.second == "bool")
            response->add_type(philote::DataType::kBool);
        else if (option.second == "int")
            response->add_type(philote::DataType::kInt);
        else if (option.second == "double" || option.second == "float")
            response->add_type(philote::DataType::kDouble);
        else if (option.second == "str" || option.second == "string")
            response->add_type(philote::DataType::kString);
        else if (option.second == "dict" || option.second == "struct")
            response->add_type(philote::DataType::kStruct);
        else
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                "Invalid value for discipline option '" + option.first + "'");
    }

    return Status::OK;
}
