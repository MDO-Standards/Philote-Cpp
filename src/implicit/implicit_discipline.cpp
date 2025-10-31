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
#include <grpcpp/grpcpp.h>

#include "implicit.h"

using std::string;
using std::vector;

using grpc::Server;
using grpc::ServerBuilder;

using philote::ImplicitDiscipline;
using philote::Partials;
using philote::Variables;

ImplicitDiscipline::ImplicitDiscipline()
{
    // Note: LinkPointers cannot be called here because shared_from_this()
    // requires the object to be managed by a shared_ptr first.
    // RegisterServices() will handle the linking when the discipline
    // is ready to be used.
}

ImplicitDiscipline::~ImplicitDiscipline() noexcept
{
    implicit_.UnlinkPointers();
    discipline_server_.UnlinkPointers();
}

void ImplicitDiscipline::RegisterServices(ServerBuilder &builder)
{
    // Link servers to this discipline instance using shared_from_this()
    // This must be done when the discipline is managed by a shared_ptr
    auto self = std::dynamic_pointer_cast<ImplicitDiscipline>(shared_from_this());
    discipline_server_.LinkPointers(shared_from_this());
    implicit_.LinkPointers(self);

    builder.RegisterService(&discipline_server_);
    builder.RegisterService(&implicit_);
}

void ImplicitDiscipline::DeclarePartials(const string &f, const string &x)
{
    // Compute the shape using the base class helper method
    // Pass true to allow outputs as the x parameter (for implicit disciplines)
    vector<int64_t> shape = ComputePartialShape(f, x, true);

    // assign the meta data
    PartialsMetaData meta;
    meta.set_name(f);
    meta.set_subname(x);
    for (const auto &dim : shape)
        meta.add_shape(dim);

    partials_meta_.push_back(meta);
}

void ImplicitDiscipline::ComputeResiduals(const Variables &inputs,
                                          const philote::Variables &outputs,
                                          philote::Variables &residuals)
{
}

void ImplicitDiscipline::SolveResiduals(const Variables &inputs,
                                        philote::Variables &outputs)
{
}

void ImplicitDiscipline::ComputeResidualGradients(const Variables &inputs,
                                                  const philote::Variables &outputs,
                                                  Partials &partials)
{
}