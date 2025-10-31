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
#include <unordered_map>
#include <utility>

#include <discipline.h>
#include <variable.h>
#include "discipline_client.h"

#include <disciplines.grpc.pb.h>
#include "discipline_server.h"

namespace philote
{
    // forward declaration
    class ExplicitDiscipline;

    /**
     * @brief Server base class for an explicit discipline.
     *
     * This class should be inherited from by analysis discipline developers to
     * create analysis servers.
     *
     * @note Thread Safety: gRPC may invoke RPC handlers concurrently on the same server
     * instance. While the server infrastructure itself is thread-safe, the linked
     * ExplicitDiscipline must also be thread-safe if concurrent RPC calls are expected.
     * User-defined Compute and ComputePartials methods should include appropriate
     * synchronization if they modify shared state.
     */
    class ExplicitServer : public ExplicitService::Service
    {
    public:
        //! Constructor
        ExplicitServer() = default;

        //! Destructor
        ~ExplicitServer() noexcept;

        /**
         * @brief Links the explicit server to the discipline server and
         * explicit discipline via pointers
         *
         * @param implementation Shared pointer to the explicit discipline instance
         */
        void LinkPointers(std::shared_ptr<philote::ExplicitDiscipline> implementation);

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
                                                             ::philote::Array> *stream) override;

        /**
         * @brief RPC that computes initiates gradient evaluation
         *
         * @param context
         * @param stream
         * @return grpc::Status
         */
        grpc::Status ComputeGradient(grpc::ServerContext *context,
                                     grpc::ServerReaderWriter<::philote::Array,
                                                             ::philote::Array> *stream) override;

        // Test helper methods that accept interface pointers for unit testing with mocks
        template<typename StreamType>
        grpc::Status ComputeFunctionImpl(grpc::ServerContext *context, StreamType *stream);

        template<typename StreamType>
        grpc::Status ComputeGradientImpl(grpc::ServerContext *context, StreamType *stream);

        // Public wrappers for tests
        grpc::Status ComputeFunctionForTesting(grpc::ServerContext *context,
                                               grpc::ServerReaderWriterInterface<::philote::Array,
                                                                                ::philote::Array> *stream) {
            return ComputeFunctionImpl(context, stream);
        }

        grpc::Status ComputeGradientForTesting(grpc::ServerContext *context,
                                               grpc::ServerReaderWriterInterface<::philote::Array,
                                                                                ::philote::Array> *stream) {
            return ComputeGradientImpl(context, stream);
        }

    private:
        //! Shared pointer to the implementation of the explicit discipline
        std::shared_ptr<philote::ExplicitDiscipline> implementation_;
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
     * Explicit disciplines compute direct function evaluations (f(x) = y) and
     * optionally their gradients. They are suitable for analyses where outputs
     * can be computed directly from inputs without solving implicit equations.
     *
     * @par Example: Simple Explicit Discipline
     * @code
     * #include <explicit.h>
     *
     * class SimpleFunction : public philote::ExplicitDiscipline {
     * private:
     *     void Setup() override {
     *         AddInput("x", {1}, "m");
     *         AddOutput("y", {1}, "m");
     *     }
     *
     *     void SetupPartials() override {
     *         DeclarePartials("y", "x");
     *     }
     *
     *     void Compute(const philote::Variables &inputs,
     *                  philote::Variables &outputs) override {
     *         double x = inputs.at("x")(0);
     *         outputs.at("y")(0) = 2.0 * x + 1.0;
     *     }
     *
     *     void ComputePartials(const philote::Variables &inputs,
     *                         philote::Partials &partials) override {
     *         partials[{"y", "x"}](0) = 2.0;
     *     }
     * };
     * @endcode
     *
     * @par Example: Multi-Input/Output Discipline
     * @code
     * class Paraboloid : public philote::ExplicitDiscipline {
     * private:
     *     void Setup() override {
     *         AddInput("x", {1}, "m");
     *         AddInput("y", {1}, "m");
     *         AddOutput("f_xy", {1}, "m**2");
     *     }
     *
     *     void SetupPartials() override {
     *         DeclarePartials("f_xy", "x");
     *         DeclarePartials("f_xy", "y");
     *     }
     *
     *     void Compute(const philote::Variables &inputs,
     *                  philote::Variables &outputs) override {
     *         double x = inputs.at("x")(0);
     *         double y = inputs.at("y")(0);
     *         outputs.at("f_xy")(0) = std::pow(x - 3.0, 2.0) + x * y +
     *                                 std::pow(y + 4.0, 2.0) - 3.0;
     *     }
     *
     *     void ComputePartials(const philote::Variables &inputs,
     *                         philote::Partials &partials) override {
     *         double x = inputs.at("x")(0);
     *         double y = inputs.at("y")(0);
     *         partials[{"f_xy", "x"}](0) = 2.0 * x - 6.0 + y;
     *         partials[{"f_xy", "y"}](0) = 2.0 * y + 8.0 + x;
     *     }
     * };
     * @endcode
     *
     * @par Example: Discipline with Options
     * @code
     * class ConfigurableDiscipline : public philote::ExplicitDiscipline {
     * private:
     *     double scale_factor_ = 1.0;
     *
     *     void Initialize() override {
     *         ExplicitDiscipline::Initialize();
     *         AddOption("scale_factor", "float");
     *     }
     *
     *     void Configure() override {
     *         // Options are set before Configure is called
     *         // Access via discipline properties if needed
     *     }
     *
     *     void Setup() override {
     *         AddInput("x", {1}, "m");
     *         AddOutput("y", {1}, "m");
     *     }
     *
     *     void Compute(const philote::Variables &inputs,
     *                  philote::Variables &outputs) override {
     *         outputs.at("y")(0) = scale_factor_ * inputs.at("x")(0);
     *     }
     * };
     * @endcode
     *
     * @par Example: Starting a Server
     * @code
     * #include <grpcpp/grpcpp.h>
     *
     * int main() {
     *     std::string address("localhost:50051");
     *     Paraboloid discipline;
     *
     *     grpc::ServerBuilder builder;
     *     builder.AddListeningPort(address, grpc::InsecureServerCredentials());
     *     discipline.RegisterServices(builder);
     *
     *     std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
     *     std::cout << "Server listening on " << address << std::endl;
     *     server->Wait();
     *
     *     return 0;
     * }
     * @endcode
     *
     * @note Thread Safety: This class is NOT inherently thread-safe. Concurrent calls
     * to Compute or ComputePartials from multiple RPC handlers will access the same
     * instance without synchronization. User-defined implementations should add
     * appropriate locks if they modify shared state or if thread safety is required.
     *
     * @see philote::ExplicitClient
     * @see philote::ImplicitDiscipline
     */
    class ExplicitDiscipline : public Discipline
    {
    public:
        /**
         * @brief Construct a new Explicit Discipline object
         */
        ExplicitDiscipline();

        /**
         *  @brief Destroy the Explicit Discipline object
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
        //! Discipline server
        philote::DisciplineServer discipline_server_;
    };

    /**
     * @brief Client class for calling a remote explicit discipline.
     *
     * This class may be inherited from or used by MDO framework developers.
     * However, it is a fully functional Philote MDO client.
     *
     * The ExplicitClient connects to a remote ExplicitDiscipline server via gRPC
     * and provides methods to perform function and gradient evaluations.
     *
     * @par Example: Basic Client Usage
     * @code
     * #include <explicit.h>
     * #include <grpcpp/grpcpp.h>
     *
     * int main() {
     *     philote::ExplicitClient client;
     *
     *     // Connect to server
     *     auto channel = grpc::CreateChannel("localhost:50051",
     *                                       grpc::InsecureChannelCredentials());
     *     client.ConnectChannel(channel);
     *
     *     // Initialize client
     *     client.GetInfo();
     *     client.Setup();
     *     client.GetVariableDefinitions();
     *
     *     // Prepare inputs
     *     philote::Variables inputs;
     *     inputs["x"] = philote::Variable(philote::kInput, {1});
     *     inputs["y"] = philote::Variable(philote::kInput, {1});
     *     inputs.at("x")(0) = 5.0;
     *     inputs.at("y")(0) = -2.0;
     *
     *     // Compute function
     *     philote::Variables outputs = client.ComputeFunction(inputs);
     *     std::cout << "f(5, -2) = " << outputs.at("f_xy")(0) << std::endl;
     *
     *     return 0;
     * }
     * @endcode
     *
     * @par Example: Computing Gradients
     * @code
     * // After setting up client and inputs as above...
     *
     * // Get partial definitions
     * client.GetPartialDefinitions();
     *
     * // Compute gradients
     * philote::Partials gradients = client.ComputeGradient(inputs);
     *
     * // Access gradient values
     * double df_dx = gradients[{"f_xy", "x"}](0);
     * double df_dy = gradients[{"f_xy", "y"}](0);
     *
     * std::cout << "∂f/∂x = " << df_dx << std::endl;
     * std::cout << "∂f/∂y = " << df_dy << std::endl;
     * @endcode
     *
     * @par Example: Complete Client-Server Workflow
     * @code
     * philote::ExplicitClient client;
     * auto channel = grpc::CreateChannel("localhost:50051",
     *                                   grpc::InsecureChannelCredentials());
     * client.ConnectChannel(channel);
     *
     * // Step 1: Get discipline information
     * client.GetInfo();
     *
     * // Step 2: Setup the discipline
     * client.Setup();
     *
     * // Step 3: Get variable definitions
     * client.GetVariableDefinitions();
     * std::vector<std::string> var_names = client.GetVariableNames();
     *
     * // Step 4: Get partial definitions (if computing gradients)
     * client.GetPartialDefinitions();
     *
     * // Step 5: Prepare inputs and compute
     * philote::Variables inputs;
     * for (const auto& name : var_names) {
     *     auto meta = client.GetVariableMeta(name);
     *     if (meta.type() == philote::kInput) {
     *         inputs[name] = philote::Variable(meta);
     *         inputs[name](0) = 1.0;  // Set your values
     *     }
     * }
     *
     * philote::Variables outputs = client.ComputeFunction(inputs);
     * philote::Partials partials = client.ComputeGradient(inputs);
     * @endcode
     *
     * @note Thread Safety: This class is NOT thread-safe. Each thread should create
     * its own ExplicitClient instance. Concurrent calls to ComputeFunction or
     * ComputeGradient on the same instance will cause data races. The underlying gRPC
     * stub is thread-safe, so multiple ExplicitClient instances can safely share a
     * channel.
     *
     * @see philote::ExplicitDiscipline
     * @see philote::ImplicitClient
     */
    class ExplicitClient : public ::philote::DisciplineClient
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

        /**
         * @brief Set the stub (for testing purposes)
         *
         * @param stub
         */
        void SetStub(std::unique_ptr<ExplicitService::StubInterface> stub)
        {
            stub_ = std::move(stub);
        }

    protected:
        //! explicit service stub
        std::unique_ptr<ExplicitService::StubInterface> stub_;
    };
}
// Template implementations must be in header
#include <algorithm>

namespace philote {

template<typename StreamType>
grpc::Status ExplicitServer::ComputeFunctionImpl(grpc::ServerContext *context, StreamType *stream)
{
    if (!implementation_)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Discipline implementation not linked");
    }

    if (!stream)
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Stream is null");
    }

    philote::Array array;

    // preallocate the inputs based on meta data
    Variables inputs;
    const auto *discipline = static_cast<philote::Discipline *>(implementation_.get());
    if (!discipline)
    {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to cast implementation to Discipline");
    }

    for (const auto &var : discipline->var_meta())
    {
        std::string name = var.name();
        if (var.type() == kInput)
            inputs[name] = Variable(var);
    }

    // Build O(1) lookup map for variable metadata
    std::unordered_map<std::string, const VariableMetaData*> var_lookup;
    for (const auto &var : discipline->var_meta())
    {
        var_lookup[var.name()] = &var;
    }

    while (stream->Read(&array))
    {
        // get variables from the stream message
        const std::string &name = array.name();

        // get the variable corresponding to the current message using O(1) lookup
        auto var_it = var_lookup.find(name);
        if (var_it == var_lookup.end())
        {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Variable not found: " + name);
        }
        const VariableMetaData* var = var_it->second;

        // obtain the inputs and discrete inputs from the stream
        if (var->type() == VariableType::kInput)
        {
            try
            {
                // set the variable slice
                inputs[name].AssignChunk(array);
            }
            catch (const std::exception &e)
            {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                              "Failed to assign chunk for variable " + name + ": " + e.what());
            }
        }
        else
        {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid variable type for input: " + name);
        }
    }

    // preallocate outputs
    Variables outputs;
    for (const VariableMetaData &var : discipline->var_meta())
    {
        if (var.type() == kOutput)
            outputs[var.name()] = Variable(var);
    }

    // call the discipline developer-defined Compute function
    try
    {
        implementation_->Compute(inputs, outputs);
    }
    catch (const std::exception &e)
    {
        return grpc::Status(grpc::StatusCode::INTERNAL,
                      "Failed to compute outputs: " + std::string(e.what()));
    }

    // iterate through continuous outputs
    for (const auto &out : outputs)
    {
        const std::string &name = out.first;
        try
        {
            out.second.Send(name, "", stream, discipline->stream_opts().num_double());
        }
        catch (const std::exception &e)
        {
            return grpc::Status(grpc::StatusCode::INTERNAL,
                          "Failed to send output " + name + ": " + e.what());
        }
    }

    return grpc::Status::OK;
}

template<typename StreamType>
grpc::Status ExplicitServer::ComputeGradientImpl(grpc::ServerContext *context, StreamType *stream)
{
    if (!implementation_)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Discipline implementation not linked");
    }

    if (!stream)
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Stream is null");
    }

    philote::Array array;

    // preallocate the inputs based on meta data
    Variables inputs;
    const auto *discipline = static_cast<philote::Discipline *>(implementation_.get());
    if (!discipline)
    {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to cast implementation to Discipline");
    }

    for (const auto &var : discipline->var_meta())
    {
        std::string name = var.name();
        if (var.type() == kInput)
            inputs[name] = Variable(var);
    }

    // Build O(1) lookup map for variable metadata
    std::unordered_map<std::string, const VariableMetaData*> var_lookup;
    for (const auto &var : discipline->var_meta())
    {
        var_lookup[var.name()] = &var;
    }

    while (stream->Read(&array))
    {
        // get variables from the stream message
        const std::string &name = array.name();

        // get the variable corresponding to the current message using O(1) lookup
        auto var_it = var_lookup.find(name);
        if (var_it == var_lookup.end())
        {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Variable not found: " + name);
        }
        const VariableMetaData* var = var_it->second;

        // obtain the inputs and discrete inputs from the stream
        if (var->type() == VariableType::kInput)
        {
            try
            {
                // set the variable slice
                inputs[name].AssignChunk(array);
            }
            catch (const std::exception &e)
            {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                              "Failed to assign chunk for variable " + name + ": " + e.what());
            }
        }
        else
        {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid variable type for input: " + name);
        }
    }

    // preallocate outputs
    Partials partials;
    for (const PartialsMetaData &par : discipline->partials_meta())
    {
        std::vector<size_t> shape;
        for (const int64_t &dim : par.shape())
            shape.push_back(dim);

        partials[std::make_pair(par.name(), par.subname())] = Variable(kOutput, shape);
    }

    // call the discipline developer-defined Compute function
    try
    {
        implementation_->ComputePartials(inputs, partials);
    }
    catch (const std::exception &e)
    {
        return grpc::Status(grpc::StatusCode::INTERNAL,
                      "Failed to compute partials: " + std::string(e.what()));
    }

    // iterate through continuous outputs
    for (const auto &par : partials)
    {
        const std::string &name = par.first.first;
        const std::string &subname = par.first.second;
        try
        {
            par.second.Send(name, subname, stream, discipline->stream_opts().num_double());
        }
        catch (const std::exception &e)
        {
            return grpc::Status(grpc::StatusCode::INTERNAL,
                          "Failed to send partial " + name + "/" + subname + ": " + e.what());
        }
    }

    return grpc::Status::OK;
}

} // namespace philote
