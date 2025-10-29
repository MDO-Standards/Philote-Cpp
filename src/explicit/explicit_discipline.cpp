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

#include "explicit.h"

using grpc::Server;
using grpc::ServerBuilder;

using philote::ExplicitDiscipline;
using philote::Partials;
using philote::Variables;

ExplicitDiscipline::ExplicitDiscipline()
{
    // Link discipline server to this discipline instance
    discipline_server_.LinkPointers(this);

    // Link explicit server to this discipline instance
    explicit_.LinkPointers(this);
}

ExplicitDiscipline::~ExplicitDiscipline()
{
    explicit_.UnlinkPointers();
    discipline_server_.UnlinkPointers();
}

void ExplicitDiscipline::RegisterServices(ServerBuilder &builder)
{
    builder.RegisterService(&discipline_server_);
    builder.RegisterService(&explicit_);
}

void ExplicitDiscipline::Compute(const Variables &inputs,
                                 philote::Variables &outputs)
{
    if (inputs.empty())
    {
        throw std::invalid_argument("No input variables provided");
    }

    if (outputs.empty())
    {
        throw std::invalid_argument("No output variables provided");
    }

    // Validate that all required inputs are present
    for (const auto &var : var_meta())
    {
        if (var.type() == kInput && inputs.find(var.name()) == inputs.end())
        {
            throw std::runtime_error("Missing required input variable: " + var.name());
        }
    }

    // Validate that all required outputs are present
    for (const auto &var : var_meta())
    {
        if (var.type() == kOutput && outputs.find(var.name()) == outputs.end())
        {
            throw std::runtime_error("Missing required output variable: " + var.name());
        }
    }

    // Default implementation: copy input to output
    for (const auto &out : outputs)
    {
        const std::string &name = out.first;
        if (inputs.find(name) != inputs.end())
        {
            const auto &input = inputs.at(name);
            auto &output = outputs[name];

            // Copy input data to output
            for (size_t i = 0; i < input.Size(); ++i)
            {
                output(i) = input(i);
            }
        }
    }
}

void ExplicitDiscipline::ComputePartials(const Variables &inputs,
                                         Partials &partials)
{
    if (inputs.empty())
    {
        throw std::invalid_argument("No input variables provided");
    }

    if (partials.empty())
    {
        throw std::invalid_argument("No partial variables provided");
    }

    // Validate that all required inputs are present
    for (const auto &var : var_meta())
    {
        if (var.type() == kInput && inputs.find(var.name()) == inputs.end())
        {
            throw std::runtime_error("Missing required input variable: " + var.name());
        }
    }

    // Validate that all required partials are present
    for (const auto &par : partials_meta())
    {
        auto key = std::make_pair(par.name(), par.subname());
        if (partials.find(key) == partials.end())
        {
            throw std::runtime_error("Missing required partial: " + par.name() + "/" + par.subname());
        }
    }

    // Default implementation: set all partials to 1.0
    for (auto &par : partials)
    {
        auto &partial = par.second;
        for (size_t i = 0; i < partial.Size(); ++i)
        {
            partial(i) = 1.0;
        }
    }
}