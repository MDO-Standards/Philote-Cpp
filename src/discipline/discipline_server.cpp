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
#include "discipline.h"

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

DisciplineServer::~DisciplineServer()
{
    UnlinkPointers();
}

bool DisciplineServer::DisiplinePointerNull()
{
    if (discipline_ == nullptr)
        return true;
    else
        return false;
}

void DisciplineServer::LinkPointers(philote::Discipline *discipline)
{
    discipline_ = discipline;
}

void DisciplineServer::UnlinkPointers()
{
    discipline_ = nullptr;
}

grpc::Status DisciplineServer::GetInfo(ServerContext *context,
                                       Empty *request,
                                       DisciplineProperties *response)
{
    *response = discipline_->properties();

    return Status::OK;
}

Status DisciplineServer::SetStreamOptions(ServerContext *context,
                                          const StreamOptions *request,
                                          Empty *response)
{
    discipline_->stream_opts() = *request;

    return Status::OK;
}

grpc::Status DisciplineServer::SetOptions(ServerContext *context,
                                          const DisciplineOptions *request,
                                          Empty *response)
{
    const Struct &options = request->options();

    discipline_->SetOptions(options);

    return Status::OK;
}

Status DisciplineServer::GetVariableDefinitions(ServerContext *context,
                                                const Empty *request,
                                                ServerWriter<VariableMetaData> *writer)
{
    if (!writer)
        return Status::OK;
    for (const VariableMetaData &var : discipline_->var_meta())
        writer->Write(var);

    return Status::OK;
}

Status DisciplineServer::GetPartialDefinitions(ServerContext *context,
                                               const Empty *request,
                                               ServerWriter<PartialsMetaData> *writer)
{
    if (!writer)
        return Status::OK;
    for (const PartialsMetaData &partial : discipline_->partials_meta())
        writer->Write(partial);

    return Status::OK;
}

grpc::Status DisciplineServer::Setup(grpc::ServerContext *context,
                                     const Empty *request,
                                     Empty *response)
{
    if (!discipline_->var_meta().empty() or !discipline_->partials_meta().empty())
    {
        // clear any existing meta data
        discipline_->var_meta().clear();
        discipline_->partials_meta().clear();
    }

    // run the developer-defined setup functions
    try
    {
        discipline_->Setup();
    }
    catch (const std::exception &e)
    {
        std::cout << "Setup failed" << e.what() << std::endl;
        return Status(grpc::INTERNAL, "Internal server error during Setup call: exception.");
    }
    catch (...)
    {
        std::cout << "Internal server error during Setup call." << std::endl;
    }

    try
    {
        discipline_->SetupPartials();
    }
    catch (const std::exception &e)
    {
        std::cout << "Setup failed" << e.what() << std::endl;
        return Status(grpc::INTERNAL, "Internal server error during SetupPartials call: exception.");
    }
    catch (...)
    {
        std::cout << "Internal server error during SetupPartials call." << std::endl;
    }

    return Status::OK;
}

::grpc::Status philote::DisciplineServer::GetAvailableOptions(::grpc::ServerContext *context,
                                                              const ::google::protobuf::Empty *request,
                                                              ::philote::StreamOptions *response)
{
    // StreamOptions only contains num_double field
    // Set a default value or get it from the discipline if available
    response->set_num_double(1000); // Default value, adjust as needed

    return Status::OK;
}
