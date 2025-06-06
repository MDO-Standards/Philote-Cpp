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
    Empty request;

    stub_->GetInfo(&context, request, &properties_);
}

void DisciplineClient::SendStreamOptions()
{
    ClientContext context;
    ::google::protobuf::Empty response;

    stub_->SetStreamOptions(&context, stream_options_, &response);
}

void DisciplineClient::SendOptions(const DisciplineOptions &options)
{
    ClientContext context;
    ::google::protobuf::Empty response;

    stub_->SetOptions(&context, options, &response);
}

void DisciplineClient::Setup()
{
    ClientContext context;
    ::google::protobuf::Empty request, response;

    stub_->Setup(&context, request, &response);
}

void DisciplineClient::GetVariableDefinitions()
{
    ClientContext context;
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
    reactor->Finish();
}

void DisciplineClient::GetPartialDefinitions()
{
    ClientContext context;
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
    reactor->Finish();
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
    VariableMetaData out;

    for (const auto &var : var_meta_)
    {
        if (var.name() == name)
        {
            out = var;
            break;
        }
    }

    return out;
}

std::vector<philote::PartialsMetaData> DisciplineClient::GetPartialsMeta()
{
    return partials_meta_;
}