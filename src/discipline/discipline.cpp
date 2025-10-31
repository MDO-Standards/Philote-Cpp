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
    // Initialize stream options with default chunk size
    stream_opts_.set_num_double(1000);
}

std::map<std::string, std::string> &Discipline::options_list()
{
    return options_list_;
}

DisciplineProperties &Discipline::properties()
{
    return properties_;
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

vector<int64_t> Discipline::ComputePartialShape(const string &f,
                                                 const string &x,
                                                 bool allow_output_as_x)
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

    // Then find the input (or output for implicit) variable shape
    for (const auto &var : var_meta_)
    {
        if (allow_output_as_x)
        {
            // For implicit disciplines, x can be either input or output
            if (var.name() == x and (var.type() == kInput or var.type() == kOutput))
            {
                for (const auto &dim : var.shape())
                    shape_x.push_back(dim);
                found_x = true;
            }
        }
        else
        {
            // For explicit disciplines, x must be an input
            if (var.name() == x and var.type() == kInput)
            {
                for (const auto &dim : var.shape())
                    shape_x.push_back(dim);
                found_x = true;
            }
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

    return shape;
}

void Discipline::DeclarePartials(const string &f, const string &x)
{
    // Compute the shape using the helper method
    vector<int64_t> shape = ComputePartialShape(f, x, false);

    // assign the meta data
    PartialsMetaData meta;
    meta.set_name(f);
    meta.set_subname(x);
    for (const auto &dim : shape)
        meta.add_shape(dim);

    partials_meta().push_back(meta);
}

void Discipline::AddOption(const string &name, const string &type)
{
    options_list_[name] = type;
}

void Discipline::Initialize()
{
    // Default implementation does nothing
    // Should be overridden by derived classes to set up available options
}

void Discipline::Configure()
{
    // Default implementation does nothing
    // Should be overridden by derived classes for post-option configuration
}

void Discipline::SetOptions(const google::protobuf::Struct &options_struct)
{
    // Base implementation simply calls Configure() after options should be set.
    // Derived classes should override this method to extract option values from
    // the protobuf Struct and store them in strongly-typed member variables.
    //
    // Example pattern for derived classes:
    //   void MyDiscipline::SetOptions(const google::protobuf::Struct &options_struct) {
    //       // Extract option values into member variables
    //       auto it = options_struct.fields().find("my_option");
    //       if (it != options_struct.fields().end()) {
    //           my_option_ = it->second.number_value();  // or string_value(), bool_value()
    //       }
    //
    //       // Call parent implementation to invoke Configure()
    //       ExplicitDiscipline::SetOptions(options_struct);  // or ImplicitDiscipline
    //   }
    //
    // Call configure after options are set
    Configure();
}

void Discipline::Setup()
{
}

void Discipline::SetupPartials()
{
}

void Discipline::SetContext(grpc::ServerContext* context) const noexcept
{
    current_context_ = context;
}

void Discipline::ClearContext() const noexcept
{
    current_context_ = nullptr;
}

bool Discipline::IsCancelled() const noexcept
{
    return current_context_ != nullptr && current_context_->IsCancelled();
}

philote::Discipline::~Discipline() noexcept = default;