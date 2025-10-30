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
#include "discipline_client.h"

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
     * Implicit disciplines define residuals R(x,y) that must be solved to find
     * outputs. They are suitable for analyses involving systems of equations,
     * iterative solvers, or problems where outputs cannot be computed directly.
     *
     * @par Example: Quadratic Equation Solver
     * @code
     * #include <implicit.h>
     * #include <cmath>
     *
     * class Quadratic : public philote::ImplicitDiscipline {
     * private:
     *     void Setup() override {
     *         // Coefficients as inputs
     *         AddInput("a", {1}, "m");
     *         AddInput("b", {1}, "m");
     *         AddInput("c", {1}, "m");
     *
     *         // Solution as output
     *         AddOutput("x", {1}, "m**2");
     *     }
     *
     *     void SetupPartials() override {
     *         DeclarePartials("x", "a");
     *         DeclarePartials("x", "b");
     *         DeclarePartials("x", "c");
     *         DeclarePartials("x", "x");  // Jacobian w.r.t. outputs
     *     }
     *
     *     void ComputeResiduals(const philote::Variables &inputs,
     *                          const philote::Variables &outputs,
     *                          philote::Variables &residuals) override {
     *         double a = inputs.at("a")(0);
     *         double b = inputs.at("b")(0);
     *         double c = inputs.at("c")(0);
     *         double x = outputs.at("x")(0);
     *
     *         // Residual: R = ax² + bx + c = 0
     *         residuals.at("x")(0) = a * std::pow(x, 2) + b * x + c;
     *     }
     *
     *     void SolveResiduals(const philote::Variables &inputs,
     *                        philote::Variables &outputs) override {
     *         double a = inputs.at("a")(0);
     *         double b = inputs.at("b")(0);
     *         double c = inputs.at("c")(0);
     *
     *         // Quadratic formula: x = (-b + √(b² - 4ac)) / 2a
     *         outputs.at("x")(0) = (-b + std::sqrt(std::pow(b, 2) - 4*a*c)) / (2*a);
     *     }
     *
     *     void ComputeResidualGradients(const philote::Variables &inputs,
     *                                  const philote::Variables &outputs,
     *                                  philote::Partials &partials) override {
     *         double a = inputs.at("a")(0);
     *         double b = inputs.at("b")(0);
     *         double x = outputs.at("x")(0);
     *
     *         // ∂R/∂a, ∂R/∂b, ∂R/∂c, ∂R/∂x
     *         partials[{"x", "a"}](0) = std::pow(x, 2);
     *         partials[{"x", "b"}](0) = x;
     *         partials[{"x", "c"}](0) = 1.0;
     *         partials[{"x", "x"}](0) = 2*a*x + b;
     *     }
     * };
     * @endcode
     *
     * @par Example: Simple Implicit Equation
     * @code
     * // Solve: x² - y = 0 for y
     * class SimpleImplicit : public philote::ImplicitDiscipline {
     * private:
     *     void Setup() override {
     *         AddInput("x", {1}, "m");
     *         AddOutput("y", {1}, "m**2");
     *     }
     *
     *     void SetupPartials() override {
     *         DeclarePartials("y", "x");
     *         DeclarePartials("y", "y");
     *     }
     *
     *     void ComputeResiduals(const philote::Variables &inputs,
     *                          const philote::Variables &outputs,
     *                          philote::Variables &residuals) override {
     *         double x = inputs.at("x")(0);
     *         double y = outputs.at("y")(0);
     *         residuals.at("y")(0) = std::pow(x, 2) - y;
     *     }
     *
     *     void SolveResiduals(const philote::Variables &inputs,
     *                        philote::Variables &outputs) override {
     *         double x = inputs.at("x")(0);
     *         outputs.at("y")(0) = std::pow(x, 2);
     *     }
     *
     *     void ComputeResidualGradients(const philote::Variables &inputs,
     *                                  const philote::Variables &outputs,
     *                                  philote::Partials &partials) override {
     *         double x = inputs.at("x")(0);
     *         partials[{"y", "x"}](0) = 2.0 * x;
     *         partials[{"y", "y"}](0) = -1.0;
     *     }
     * };
     * @endcode
     *
     * @par Example: Starting an Implicit Server
     * @code
     * #include <grpcpp/grpcpp.h>
     *
     * int main() {
     *     std::string address("localhost:50051");
     *     Quadratic discipline;
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
     * @note For implicit disciplines, you must implement both ComputeResiduals()
     *       and SolveResiduals(). ComputeResiduals evaluates R(x,y), while
     *       SolveResiduals finds y such that R(x,y) = 0.
     *
     * @see philote::ImplicitClient
     * @see philote::ExplicitDiscipline
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
     *
     * The ImplicitClient connects to a remote ImplicitDiscipline server via gRPC
     * and provides methods to compute residuals, solve for outputs, and evaluate
     * residual gradients.
     *
     * @par Example: Basic Client Usage
     * @code
     * #include <implicit.h>
     * #include <grpcpp/grpcpp.h>
     *
     * int main() {
     *     philote::ImplicitClient client;
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
     *     inputs["a"] = philote::Variable(philote::kInput, {1});
     *     inputs["b"] = philote::Variable(philote::kInput, {1});
     *     inputs["c"] = philote::Variable(philote::kInput, {1});
     *     inputs.at("a")(0) = 1.0;
     *     inputs.at("b")(0) = -5.0;
     *     inputs.at("c")(0) = 6.0;
     *
     *     // Solve for outputs
     *     philote::Variables outputs = client.SolveResiduals(inputs);
     *     std::cout << "Solution: x = " << outputs.at("x")(0) << std::endl;
     *
     *     return 0;
     * }
     * @endcode
     *
     * @par Example: Computing Residuals
     * @code
     * // After setting up client and inputs...
     *
     * // Prepare both inputs and outputs
     * philote::Variables vars;
     * vars["a"] = philote::Variable(philote::kInput, {1});
     * vars["b"] = philote::Variable(philote::kInput, {1});
     * vars["c"] = philote::Variable(philote::kInput, {1});
     * vars["x"] = philote::Variable(philote::kOutput, {1});
     *
     * vars.at("a")(0) = 1.0;
     * vars.at("b")(0) = -5.0;
     * vars.at("c")(0) = 6.0;
     * vars.at("x")(0) = 2.0;  // Guess
     *
     * // Compute residual
     * philote::Variables residuals = client.ComputeResiduals(vars);
     * std::cout << "R(x=2) = " << residuals.at("x")(0) << std::endl;
     * @endcode
     *
     * @par Example: Computing Residual Gradients
     * @code
     * // Get partial definitions
     * client.GetPartialDefinitions();
     *
     * // Compute residual gradients
     * philote::Partials gradients = client.ComputeResidualGradients(vars);
     *
     * // Access gradient values
     * double dR_da = gradients[{"x", "a"}](0);
     * double dR_db = gradients[{"x", "b"}](0);
     * double dR_dc = gradients[{"x", "c"}](0);
     * double dR_dx = gradients[{"x", "x"}](0);
     *
     * std::cout << "∂R/∂a = " << dR_da << std::endl;
     * std::cout << "∂R/∂b = " << dR_db << std::endl;
     * std::cout << "∂R/∂c = " << dR_dc << std::endl;
     * std::cout << "∂R/∂x = " << dR_dx << std::endl;
     * @endcode
     *
     * @par Example: Complete Implicit Client Workflow
     * @code
     * philote::ImplicitClient client;
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
     *
     * // Step 4: Prepare inputs
     * philote::Variables inputs;
     * inputs["a"] = philote::Variable(philote::kInput, {1});
     * inputs["a"](0) = 1.0;
     * // ... set other inputs
     *
     * // Step 5: Solve for outputs
     * philote::Variables outputs = client.SolveResiduals(inputs);
     *
     * // Step 6 (optional): Verify solution by computing residuals
     * philote::Variables combined = inputs;
     * for (const auto& [name, var] : outputs) {
     *     combined[name] = var;
     * }
     * philote::Variables residuals = client.ComputeResiduals(combined);
     * // residuals should be near zero
     *
     * // Step 7 (optional): Compute gradients
     * client.GetPartialDefinitions();
     * philote::Partials partials = client.ComputeResidualGradients(combined);
     * @endcode
     *
     * @note For implicit disciplines, variables passed to ComputeResiduals()
     *       must include both inputs and outputs. SolveResiduals() only requires
     *       inputs and returns the solved outputs.
     *
     * @see philote::ImplicitDiscipline
     * @see philote::ExplicitClient
     */
    class ImplicitClient : public ::philote::DisciplineClient
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

        /**
         * @brief Sets the stub for testing purposes (allows dependency injection)
         *
         * @param stub The stub to inject
         */
        void SetStub(std::unique_ptr<ImplicitService::StubInterface> stub)
        {
            stub_ = std::move(stub);
        }

    private:
        //! implicit service stub
        std::unique_ptr<ImplicitService::StubInterface> stub_;
    };
} // namespace philote