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
#include "discipline.h"

using std::string;
using std::vector;

using google::protobuf::Struct;

using philote::Discipline;
using philote::DisciplineProperties;
using philote::StreamOptions;

Discipline::Discipline()
{
    discipline_server_.LinkPointers(this);
}

std::map<std::string, std::string> &Discipline::options_list()
{
    return options_list_;
}

std::vector<philote::VariableMetaData> &Discipline::var_meta()
{
    return var_meta_;
}

std::vector<philote::PartialsMetaData> &Discipline::partials_meta()
{
    return partials_meta_;
}

DisciplineProperties &Discipline::properties()
{
    return properties_;
}

StreamOptions &Discipline::stream_opts()
{
    return stream_opts_;
}

void Discipline::AddInput(const string &name,
                          const vector<int64_t> &shape,
                          const string &units)
{
    VariableMetaData var;
    var.set_name(name);
    for (const int64_t dim : shape)
        var.add_shape(dim);
    var.set_units(units);
    var.set_type(philote::kInput);

    var_meta().push_back(var);
}

void Discipline::AddOutput(const string &name,
                           const vector<int64_t> &shape,
                           const string &units)
{
    VariableMetaData var;
    var.set_name(name);
    for (const int64_t dim : shape)
        var.add_shape(dim);
    var.set_units(units);
    var.set_type(philote::kOutput);

    var_meta().push_back(var);
}

void Discipline::DeclarePartials(const string &f, const string &x)
{
    // determine and assign the shape of the partials array
    vector<int64_t> shape_f, shape_x;
    bool found_f = false, found_x = false;

    // First find the output variable shape
    for (const auto &var : var_meta_)
    {
        if (var.name() == f and var.type() == kOutput)
        {
            for (const auto &dim : var.shape())
                shape_f.push_back(dim);
            found_f = true;
        }
    }

    // Then find the input variable shape
    for (const auto &var : var_meta_)
    {
        if (var.name() == x and var.type() == kInput)
        {
            for (const auto &dim : var.shape())
                shape_x.push_back(dim);
            found_x = true;
        }
    }

    if (!found_f || !found_x)
    {
        throw std::runtime_error("Could not find both input and output variables for partials declaration");
    }

    // create the combined shape array
    vector<int64_t> shape;

    if (shape_f.size() == 1 and shape_f[0] == 1 and shape_x.size() == 1 and shape_x[0] == 1)
    {
        // Both are scalars
        shape.resize(1);
        shape[0] = 1;
    }
    else if (shape_f.size() == 1 and shape_f[0] == 1)
    {
        // Output is scalar, input is vector/matrix
        shape = shape_x;
    }
    else if (shape_x.size() == 1 and shape_x[0] == 1)
    {
        // Input is scalar, output is vector/matrix
        shape = shape_f;
    }
    else
    {
        // Both are vectors/matrices
        // For partial derivatives, we want output dimensions first, then input dimensions
        shape.reserve(shape_f.size() + shape_x.size());
        shape.insert(shape.end(), shape_f.begin(), shape_f.end());
        shape.insert(shape.end(), shape_x.begin(), shape_x.end());
    }

    // assign the meta data
    PartialsMetaData meta;
    meta.set_name(f);
    meta.set_subname(x);
    for (const auto &dim : shape)
        meta.add_shape(dim);

    partials_meta_.push_back(meta);
}

void Discipline::SetOptions(const google::protobuf::Struct &options_struct)
{
}

void Discipline::Setup()
{
}

void Discipline::SetupPartials()
{
}

philote::Discipline::~Discipline() = default;