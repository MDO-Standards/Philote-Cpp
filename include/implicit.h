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

#include <disciplines.grpc.pb.h>
#include "discipline_server.h"

#include <discipline.h>

namespace philote
{
    // forward declaration
    class ImplicitDiscipline;

    /**
     * @brief Implicit server class
     *
     */
    class ImplicitServer : public ImplicitService::Service
    {
    public:
        //! Constructor
        ImplicitServer() = default;

        //! Destructor
        ~ImplicitServer();

        /**
         * @brief Links the explicit server to the discipline server and
         * explicit discipline via pointers
         *
         * @param discipline
         */
        void LinkPointers(philote::ImplicitDiscipline *implementation);

        /**
         * @brief Dereferences all pointers
         *
         */
        void UnlinkPointers();

        /**
         * @brief RPC that computes the residual evaluation
         *
         * @param context
         * @param stream
         * @return grpc::Status
         */
        grpc::Status ComputeResiduals(grpc::ServerContext *context,
                                      grpc::ServerReaderWriter<::philote::Array,
                                                               ::philote::Array> *stream);

        /**
         * @brief RPC that computes the residual evaluation
         *
         * @param context
         * @param stream
         * @return grpc::Status
         */
        grpc::Status SolveResiduals(grpc::ServerContext *context,
                                    grpc::ServerReaderWriter<::philote::Array,
                                                             ::philote::Array> *stream);

        /**
         * @brief RPC that computes the residual evaluation
         *
         * @param context
         * @param stream
         * @return grpc::Status
         */
        grpc::Status ComputeResidualGradients(grpc::ServerContext *context,
                                              grpc::ServerReaderWriter<::philote::Array,
                                                                       ::philote::Array> *stream);

    private:
        //! Pointer to the implementation of the implicit discipline
        philote::ImplicitDiscipline *implementation_;
    };

    /**
     * @brief Implicit discipline class.
     *
     * This class should be overriden by discipline developers. The basic
     * discipline an implicit discipline servers are private member variables
     * of this class and are registered to a gRPC connection together via the
     * RegisterServers member function. The discipline developer should not have
     * to interact further with these services.
     *
     */
    class ImplicitDiscipline : public Discipline
    {
    public:
        /**
         * @brief Construct a new Implicit Discipline object
         *
         */
        ImplicitDiscipline();

        /**
         * @brief Destroy the Implicit Discipline object
         *
         */
        ~ImplicitDiscipline();

        /**
         * @brief Registers all services with a gRPC channel
         *
         * @param builder
         */
        void RegisterServices(grpc::ServerBuilder &builder);

        /**
         * @brief Declare a (set of) partial(s) for the discipline
         *
         * @param f
         * @param x
         */
        void DeclarePartials(const std::string &f, const std::string &x);

        /**
         * @brief Computes the residual for the discipline.
         *
         * This function should be overridden by the developer of the
         * discipline.
         *
         * @param inputs input variables for the discipline (continuous and
         * discrete)
         * @return philote::Variables
         */
        virtual void ComputeResiduals(const philote::Variables &inputs,
                                      const philote::Variables &outputs,
                                      philote::Variables &residuals);

        /**
         * @brief Solves the residuals to obtain the outputs for the discipline.
         *
         * This function should be overridden by the developer of the
         * discipline.
         *
         * @param inputs input variables for the discipline
         * @param outputs output variables for the discipline (will be assigned
         * during the function call)
         */
        virtual void SolveResiduals(const philote::Variables &inputs, philote::Variables &outputs);

        /**
         * @brief Computes the gradients of the residuals evaluation for the
         * discipline.
         *
         * This function should be overridden by the developer of the
         * discipline, if applicable (not every discipline can provide
         * partials).
         *
         * @param inputs input variables for the discipline (continuous and
         * discrete)
         */
        virtual void ComputeResidualGradients(const philote::Variables &inputs,
                                              const philote::Variables &outputs,
                                              Partials &partials);

    private:
        //! Implicit discipline server
        philote::ImplicitServer implicit_;
        //! Discipline server
        philote::DisciplineServer discipline_server_;
    };

    /**
     * @brief Client class for calling a remote implicit discipline.
     *
     * This class may be inherited from or used by MDO framework developers.
     * However, it is a fully functional Philote MDO client.
     */
    class ImplicitClient : public BaseDisciplineClient
    {
    public:
        //! Constructor
        ImplicitClient() = default;

        //! Destructor
        ~ImplicitClient() = default;

        /**
         * @brief Connects the client stub to a gRPC channel
         *
         * @param channel
         */
        void ConnectChannel(std::shared_ptr<grpc::ChannelInterface> channel);

        /**
         * @brief Calls the remote analysis server residuals evaluation via gRPC.
         *
         * Unlike the analysis server, this function does not need to be
         * overridden, as it contains all logic necessary to retrieve the remote
         * function evaluation.
         *
         * @param vars inputs and outputs for the discipline
         */
        Variables ComputeResiduals(const Variables &vars);

        /**
         * @brief Calls the remote analysis server to solve  via gRPC.
         *
         * Unlike the analysis server, this function does not need to be
         * overridden, as it contains all logic necessary to retrieve the remote
         * function evaluation.
         *
         * @param vars inputs and outputs for the discipline
         */
        Variables SolveResiduals(const Variables &vars);

        /**
         * @brief Calls the remote analysis server gradient evaluation via gRPC
         *
         * @param vars inputs and outputs for the discipline
         * @return Partials
         */
        Partials ComputeResidualGradients(const Variables &vars);

    private:
        //! explicit service stub
        std::unique_ptr<ImplicitService::Stub> stub_;
    };
} // namespace philote