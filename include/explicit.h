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

#include <string>
#include <map>
#include <utility>

#include <discipline.h>
#include <variable.h>

#include <disciplines.grpc.pb.h>

namespace philote
{
    // forward declaration
    class ExplicitDiscipline;

    /**
     * @brief Server base class for an explicit discipline.
     *
     * This class should be inherited from by analysis discipline developers to
     * create analysis servers.
     */
    class ExplicitServer : public ExplicitService::Service
    {
    public:
        //! Constructor
        ExplicitServer() = default;

        //! Destructor
        ~ExplicitServer();

        /**
         * @brief Links the explicit server to the discipline server and
         * explicit discipline via pointers
         *
         * @param discipline
         */
        void LinkPointers(philote::ExplicitDiscipline *implementation);

        /**
         * @brief Dereferences all pointers
         *
         */
        void UnlinkPointers();

        /**
         * @brief RPC that computes initiates function evaluation
         *
         * @param context
         * @param stream
         * @return grpc::Status
         */
        grpc::Status ComputeFunction(grpc::ServerContext *context,
                                     grpc::ServerReaderWriter<::philote::Array,
                                                              ::philote::Array> *stream);

        /**
         * @brief RPC that computes initiates gradient evaluation
         *
         * @param context
         * @param stream
         * @return grpc::Status
         */
        grpc::Status ComputeGradient(grpc::ServerContext *context,
                                     grpc::ServerReaderWriter<::philote::Array,
                                                              ::philote::Array> *stream);

    private:
        //! Pointer to the implementation of the explicit discipline
        philote::ExplicitDiscipline *implementation_;
    };

    /**
     * @brief Explicit discipline class
     *
     * This class should be overriden by discipline developers. The basic
     * discipline and explicit discipline servers are private member variables
     * of this class and are registered to a gRPC connection together via the
     * RegisterServers member function. The discipline developer should not have
     * to interact further with these services.
     *
     */
    class ExplicitDiscipline : public Discipline
    {
    public:
        /**
         * @brief Construct a new Explicit Discipline object
         */
        ExplicitDiscipline();

        /**
         *  @brief Destroy the Explicit Disipline object
         */
        ~ExplicitDiscipline();

        /**
         * @brief Registers all services with a gRPC channel
         *
         * @param builder
         */
        void RegisterServices(grpc::ServerBuilder &builder);

        /**
         * @brief Function evaluation for the discipline.
         *
         * This function should be overridden by the developer of the
         * discipline.
         *
         * @param inputs input variables for the discipline (continuous and
         * discrete)
         * @return philote::Variables
         */
        virtual void Compute(const philote::Variables &inputs, philote::Variables &outputs);

        /**
         * @brief Gradient evaluation for the discipline.
         *
         * This function should be overridden by the developer of the
         * discipline, if applicable (not every discipline can provide
         * partials).
         *
         * @param inputs input variables for the discipline (continuous and
         * discrete)
         */
        virtual void ComputePartials(const philote::Variables &inputs,
                                     Partials &partials);

    private:
        //! Explicit discipline server
        philote::ExplicitServer explicit_;
    };

    /**
     * @brief Client class for calling a remote explicit discipline.
     *
     * This class may be inherited from or used by MDO framework developers.
     * However, it is a fully functional Philote MDO client.
     */
    class ExplicitClient : public DisciplineClient
    {
    public:
        //! Constructor
        ExplicitClient() = default;

        //! Destructor
        ~ExplicitClient() = default;

        /**
         * @brief Connects the client stub to a gRPC channel
         *
         * @param channel
         */
        void ConnectChannel(std::shared_ptr<grpc::ChannelInterface> channel);

        /**
         * @brief Calls the remote analysis server function evaluation via gRPC.
         *
         * Unlike the analysis server, this function does not need to be
         * overridden, as it contains all logic necessary to retrieve the remote
         * function evaluation.
         *
         * @param inputs
         */
        Variables ComputeFunction(const Variables &inputs);

        /**
         * @brief Calls the remote analysis server gradient evaluation via gRPC
         *
         * @param inputs
         * @return Partials
         */
        Partials ComputeGradient(const Variables &inputs);

    private:
        //! explicit service stub
        std::unique_ptr<ExplicitService::Stub> stub_;
    };
}