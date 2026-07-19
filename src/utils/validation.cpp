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
#include "validation.h"

#include <set>

using philote::PhiloteValidationError;

static const std::set<std::string> VALID_OPTION_TYPES = {
    "bool", "int", "float", "double", "str", "string", "dict", "struct"};

void philote::ValidateName(const std::string &name, const std::string &context)
{
    if (name.empty())
    {
        throw PhiloteValidationError(context + ": 'name' must not be empty.");
    }
}

void philote::ValidateShape(const std::vector<int64_t> &shape, const std::string &context)
{
    for (size_t i = 0; i < shape.size(); i++)
    {
        if (shape[i] <= 0)
        {
            throw PhiloteValidationError(
                context + ": all elements of 'shape' must be positive, but element " +
                std::to_string(i) + " is " + std::to_string(shape[i]) + ".");
        }
    }
}

void philote::ValidateOptionType(const std::string &type, const std::string &name)
{
    if (VALID_OPTION_TYPES.find(type) == VALID_OPTION_TYPES.end())
    {
        std::string allowed = "{";
        bool first = true;
        for (const auto &t : VALID_OPTION_TYPES)
        {
            if (!first)
                allowed += ", ";
            allowed += "\"" + t + "\"";
            first = false;
        }
        allowed += "}";
        throw PhiloteValidationError(
            "Invalid type '" + type + "' for option '" + name +
            "'. Allowed types are: " + allowed + ".");
    }
}
