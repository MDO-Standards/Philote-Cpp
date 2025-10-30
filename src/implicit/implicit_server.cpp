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

using std::string;
using std::vector;

using google::protobuf::Empty;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

using philote::ImplicitServer;
using philote::Partials;
using philote::Variables;

ImplicitServer::~ImplicitServer()
{
    UnlinkPointers();
}

void ImplicitServer::LinkPointers(philote::ImplicitDiscipline *implementation)
{
    implementation_ = implementation;
}

void ImplicitServer::UnlinkPointers()
{
    implementation_ = nullptr;
}

grpc::Status ImplicitServer::ComputeResiduals(grpc::ServerContext *context,
                                              grpc::ServerReaderWriter<::philote::Array,
                                                                       ::philote::Array> *stream)
{
    return ComputeResidualsImpl(context, stream);
}

grpc::Status ImplicitServer::SolveResiduals(grpc::ServerContext *context,
                                            grpc::ServerReaderWriter<::philote::Array,
                                                                     ::philote::Array> *stream)
{
    return SolveResidualsImpl(context, stream);
}

grpc::Status ImplicitServer::ComputeResidualGradients(grpc::ServerContext *context,
                                                      grpc::ServerReaderWriter<::philote::Array,
                                                                               ::philote::Array> *stream)
{
    return ComputeResidualGradientsImpl(context, stream);
}