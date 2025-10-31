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
#include "discipline_client.h"

using google::protobuf::Empty;
using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::ClientReader;
using philote::DisciplineProperties;
using philote::VariableMetaData;
using std::string;
using std::vector;

using namespace philote;

DisciplineClient::DisciplineClient()
{
    // set default streaming options
    stream_options_.set_num_double(1000);
    stub_ = nullptr;
}

void DisciplineClient::ConnectChannel(const std::shared_ptr<grpc::ChannelInterface> &channel)
{
    stub_ = DisciplineService::NewStub(channel);
}

void DisciplineClient::GetInfo()
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + rpc_timeout_);
    Empty request;

    auto status = stub_->GetInfo(&context, request, &properties_);
    if (!status.ok())
    {
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            throw std::runtime_error("RPC timeout after " +
                                   std::to_string(rpc_timeout_.count()) +
                                   "ms: " + status.error_message());
        }
        std::string error_msg = "Failed to get discipline info [code=" +
                               std::to_string(status.error_code()) + "]: " +
                               status.error_message();
        throw std::runtime_error(error_msg);
    }
}

void DisciplineClient::SendStreamOptions()
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + rpc_timeout_);
    ::google::protobuf::Empty response;

    auto status = stub_->SetStreamOptions(&context, stream_options_, &response);
    if (!status.ok())
    {
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            throw std::runtime_error("RPC timeout after " +
                                   std::to_string(rpc_timeout_.count()) +
                                   "ms: " + status.error_message());
        }
        throw std::runtime_error("Failed to set stream options: " + status.error_message());
    }
}

void DisciplineClient::SendOptions(const DisciplineOptions &options)
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + rpc_timeout_);
    ::google::protobuf::Empty response;

    auto status = stub_->SetOptions(&context, options, &response);
    if (!status.ok())
    {
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            throw std::runtime_error("RPC timeout after " +
                                   std::to_string(rpc_timeout_.count()) +
                                   "ms: " + status.error_message());
        }
        throw std::runtime_error("Failed to set options: " + status.error_message());
    }
}

void DisciplineClient::Setup()
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + rpc_timeout_);
    ::google::protobuf::Empty request, response;

    auto status = stub_->Setup(&context, request, &response);
    if (!status.ok())
    {
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            throw std::runtime_error("RPC timeout after " +
                                   std::to_string(rpc_timeout_.count()) +
                                   "ms: " + status.error_message());
        }
        throw std::runtime_error("Failed to setup discipline: " + status.error_message());
    }
}

void DisciplineClient::GetVariableDefinitions()
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + rpc_timeout_);
    Empty request;
    std::unique_ptr<grpc::ClientReaderInterface<philote::VariableMetaData>> reactor;

    if (!var_meta_.empty())
    {
        // clear any existing meta data
        var_meta_.clear();
    }
    // get the meta data
    reactor = stub_->GetVariableDefinitions(&context, request);

    VariableMetaData meta;
    while (reactor->Read(&meta))
        var_meta_.push_back(meta);

    auto status = reactor->Finish();
    if (!status.ok())
    {
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            throw std::runtime_error("RPC timeout after " +
                                   std::to_string(rpc_timeout_.count()) +
                                   "ms: " + status.error_message());
        }
        throw std::runtime_error("Failed to get variable definitions: " + status.error_message());
    }
}

void DisciplineClient::GetPartialDefinitions()
{
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + rpc_timeout_);
    Empty request;
    std::unique_ptr<grpc::ClientReaderInterface<philote::PartialsMetaData>> reactor;

    if (!partials_meta_.empty())
    {
        // clear any existing meta data
        partials_meta_.clear();
    }
    // get the meta data
    reactor = stub_->GetPartialDefinitions(&context, request);

    PartialsMetaData meta;
    while (reactor->Read(&meta))
        partials_meta_.push_back(meta);

    auto status = reactor->Finish();
    if (!status.ok())
    {
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            throw std::runtime_error("RPC timeout after " +
                                   std::to_string(rpc_timeout_.count()) +
                                   "ms: " + status.error_message());
        }
        throw std::runtime_error("Failed to get partial definitions: " + status.error_message());
    }
}

vector<string> DisciplineClient::GetVariableNames()
{
    vector<string> keys;
    keys.reserve(var_meta_.size());
    for (const auto &var : var_meta_)
    {
        keys.push_back(var.name());
    }

    return keys;
}

VariableMetaData DisciplineClient::GetVariableMeta(const string &name)
{
    for (const auto &var : var_meta_)
    {
        if (var.name() == name)
        {
            return var;
        }
    }

    throw std::runtime_error("Variable not found: " + name);
}

std::vector<philote::PartialsMetaData> DisciplineClient::GetPartialsMeta()
{
    return partials_meta_;
}