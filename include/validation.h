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
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace philote
{
    /**
     * @brief Base class for all Philote-specific errors.
     */
    class PhiloteError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /**
     * @brief Raised when an input fails validation.
     */
    class PhiloteValidationError : public PhiloteError
    {
    public:
        using PhiloteError::PhiloteError;
    };

    /**
     * @brief Raised on the client side when a gRPC server call fails.
     */
    class PhiloteServerError : public PhiloteError
    {
    public:
        using PhiloteError::PhiloteError;
    };

    /**
     * @brief Validate that a name is a non-empty string.
     *
     * @param name The name to validate
     * @param context Human-readable context for error messages (e.g., "add_input")
     * @throws PhiloteValidationError if name is empty
     */
    void ValidateName(const std::string &name, const std::string &context);

    /**
     * @brief Validate that a shape contains only positive dimensions.
     *
     * @param shape The shape to validate
     * @param context Human-readable context for error messages
     * @throws PhiloteValidationError if any dimension is not positive
     */
    void ValidateShape(const std::vector<int64_t> &shape, const std::string &context);

    /**
     * @brief Validate that an option type string is one of the allowed types.
     *
     * Allowed types: "bool", "int", "float", "double", "string", "dict"
     *
     * @param type The option type string to validate
     * @param name The option name (for error messages)
     * @throws PhiloteValidationError if type is not in the allowed set
     */
    void ValidateOptionType(const std::string &type, const std::string &name);

} // namespace philote
