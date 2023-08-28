/*
    Philote C++ Bindings

    Copyright 2022-2023 Christopher A. Lupp

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#pragma once

#include <disciplines.grpc.pb.h>

#include <Philote/discipline.h>

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
        grpc::Status ComputeResidual(grpc::ServerContext *context,
                                     grpc::ServerReaderWriter<::philote::Array,
                                                              ::philote::Array> *stream);

        /**
         * @brief RPC that computes the residual evaluation
         *
         * @param context
         * @param stream
         * @return grpc::Status
         */
        grpc::Status SolveResidual(grpc::ServerContext *context,
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
         * @brief Declares an output. This is a specialization of the add output
         * function for the implicit discipline. It is necessary to overload
         * this function, as the implicit discipline adds residuals to the
         * variable vector.
         *
         * @param name
         * @param shape
         * @param units
         */
        void AddOutput(const std::string &name,
                       const std::vector<int64_t> &shape,
                       const std::string &units);

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
         * @param inputs input variables for the discipline (continuous and
         * discrete)
         * @return philote::Variables
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
    };

    /**
     * @brief Client class for calling a remote implicit discipline.
     *
     * This class may be inherited from or used by MDO framework developers.
     * However, it is a fully functional Philote MDO client.
     */
    class ImplicitClient : public DisciplineClient
    {
    public:
        //! Constructor
        ImplicitClient();

        //! Destructor
        ~ImplicitClient();

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
        //! host name of the analysis server
        std::string host_;

        std::shared_ptr<grpc::Channel> channel_;
        std::unique_ptr<ExplicitService::Stub> stub_;
    };
} // namespace philote