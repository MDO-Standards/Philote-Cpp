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

#include <google/protobuf/struct.pb.h>

#include <map>
#include <variable.h>

#include <data.pb.h>
#include <disciplines.grpc.pb.h>

namespace philote
{
    // forward declaration
    class DisciplineServer;

    /**
     * @brief Definition of the discipline base class
     *
     * This class is used as the base class for the Explicit and Implicit
     * Disciplines. It allows the discipline developers to overload the setup
     * and setup partials functions without code duplication within the explicit
     * and implicit classes.
     *
     */
    class Discipline
    {
    public:
        /**
         * @brief Construct a new Discipline object
         *
         */
        Discipline();

        /**
         * @brief Destroy the Discipline object
         *
         */
        ~Discipline();

        /**
         * Gets the options list.
         *
         * @return
         */
        std::map<std::string, std::string> &options_list();

        /**
         * @brief Accesses the variable meta data
         */
        std::vector<philote::VariableMetaData> &var_meta() { return var_meta_; }
        const std::vector<philote::VariableMetaData> &var_meta() const { return var_meta_; }

        /**
         * @brief Accesses the partials meta data
         */
        std::vector<philote::PartialsMetaData> &partials_meta() { return partials_meta_; }
        const std::vector<philote::PartialsMetaData> &partials_meta() const { return partials_meta_; }

        /**
         * @brief Gets the discipline properties
         *
         * @return DisciplineProperties
         */
        philote::DisciplineProperties &properties();

        /**
         * @brief Gets the stream options
         *
         * @return StreamOptions&
         */
        philote::StreamOptions &stream_opts() { return stream_opts_; }
        const philote::StreamOptions &stream_opts() const { return stream_opts_; }

        /**
         * @brief Declares an input
         *
         * @param var
         */
        void AddInput(const std::string &name,
                      const std::vector<int64_t> &shape,
                      const std::string &units);

        /**
         * @brief Declares an output
         *
         * @param var
         */
        void AddOutput(const std::string &name,
                       const std::vector<int64_t> &shape,
                       const std::string &units);

        /**
         * @brief Declare a (set of) partial(s) for the discipline
         *
         * @param f
         * @param x
         */
        void DeclarePartials(const std::string &f, const std::string &x);

        /**
         * @brief Add an option to the discipline
         *
         * @param name Option name
         * @param type Option type as string (e.g., "bool", "int", "float", "string")
         */
        void AddOption(const std::string &name, const std::string &type);

        /**
         * @brief Initialize function that sets up available options
         *
         * This function is called during discipline construction. It should be
         * used to define what option names and types are available. The
         * SetOptions function is used to actually set the option values.
         */
        virtual void Initialize();

        /**
         * @brief Configure function that is called after options are set
         *
         * This function is called after SetOptions but before Setup. It can be
         * used for any configuration that depends on the option values.
         */
        virtual void Configure();

        /**
         * @brief Sets up all discipline options based on a protobuf struct that
         * the server received from the client.
         *
         * @param options_struct
         */
        virtual void SetOptions(const google::protobuf::Struct &options_struct);

        /**
         * @brief Setup function that is called by the server when the client
         * calls the setup RPC.
         *
         */
        virtual void Setup();

        /**
         * @brief Setup function that is called by the server when the client
         * calls the setup RPC. This function is used to setup the partials.
         *
         */
        virtual void SetupPartials();

    protected:
        //! List of options that can be set by the client
        std::map<std::string, std::string> options_list_;

        //! List of variable meta data
        std::vector<philote::VariableMetaData> var_meta_;

        //! List of partials meta data
        std::vector<philote::PartialsMetaData> partials_meta_;

        //! Discipline properties
        philote::DisciplineProperties properties_;

        //! Stream options
        philote::StreamOptions stream_opts_;
    };

}
