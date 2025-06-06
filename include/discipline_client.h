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

#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/status_code_enum.h>
#include <grpcpp/support/status.h>

#include <disciplines.grpc.pb.h>
#include <memory>
#include <string>
#include <vector>

namespace philote
{
    /**
     * @brief Client class for interacting with a discipline server
     *
     */
    class DisciplineClient
    {
    public:
        /**
         * @brief Construct a new Discipline Client object
         *
         */
        DisciplineClient();

        /**
         * @brief Connect to a gRPC channel
         *
         * @param channel
         */
        void ConnectChannel(const std::shared_ptr<grpc::ChannelInterface> &channel);

        /**
         * @brief Get the discipline info
         *
         */
        void GetInfo();

        /**
         * @brief Send the stream options to the server
         *
         */
        void SendStreamOptions();

        /**
         * @brief Send the discipline options to the server
         *
         * @param options
         */
        void SendOptions(const philote::DisciplineOptions &options);

        /**
         * @brief Setup the discipline
         *
         */
        void Setup();

        /**
         * @brief Get the variable definitions from the server
         *
         */
        void GetVariableDefinitions();

        /**
         * @brief Get the partial definitions from the server
         *
         */
        void GetPartialDefinitions();

        /**
         * @brief Get the variable names
         *
         * @return std::vector<std::string>
         */
        std::vector<std::string> GetVariableNames();

        /**
         * @brief Get the variable meta data
         *
         * @param name
         * @return VariableMetaData
         */
        VariableMetaData GetVariableMeta(const std::string &name);

        /**
         * @brief Get the partials meta data
         *
         * @return std::vector<PartialsMetaData>
         */
        std::vector<philote::PartialsMetaData> GetPartialsMeta();

        /**
         * @brief Set the stub
         *
         * @param stub
         */
        void SetStub(std::unique_ptr<philote::DisciplineService::StubInterface> stub)
        {
            stub_ = std::move(stub);
        }

    private:
        //! gRPC stub
        std::unique_ptr<philote::DisciplineService::StubInterface> stub_;

        //! Stream options
        philote::StreamOptions stream_options_;

        //! Discipline properties
        philote::DisciplineProperties properties_;

        //! Variable meta data
        std::vector<philote::VariableMetaData> var_meta_;

        //! Partials meta data
        std::vector<philote::PartialsMetaData> partials_meta_;
    };
} // namespace philote