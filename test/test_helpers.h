#ifndef PHILOTE_TEST_HELPERS_H
#define PHILOTE_TEST_HELPERS_H

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <grpcpp/grpcpp.h>

#include "explicit.h"
#include "implicit.h"
#include "variable.h"

namespace philote {
namespace test {

// ============================================================================
// Test Discipline Implementations - Explicit
// ============================================================================

/**
 * Simple paraboloid discipline: f(x, y) = x^2 + y^2
 * Partials: df/dx = 2x, df/dy = 2y
 */
class ParaboloidDiscipline : public ExplicitDiscipline {
public:
    void Setup() override;
    void SetupPartials() override;
    void Compute(const Variables &inputs, Variables &outputs) override;
    void ComputePartials(const Variables &inputs, Partials &partials) override;
};

/**
 * Multi-output discipline:
 *   f1(x, y) = x + y
 *   f2(x, y) = x * y
 *   f3(x, y) = x - y
 */
class MultiOutputDiscipline : public ExplicitDiscipline {
public:
    void Setup() override;
    void SetupPartials() override;
    void Compute(const Variables &inputs, Variables &outputs) override;
    void ComputePartials(const Variables &inputs, Partials &partials) override;
};

/**
 * Vectorized discipline: z = A * x + b
 * Where A is a matrix, x is a vector, b is a vector
 */
class VectorizedDiscipline : public ExplicitDiscipline {
public:
    VectorizedDiscipline(size_t n, size_t m);
    void Setup() override;
    void SetupPartials() override;
    void Compute(const Variables &inputs, Variables &outputs) override;
    void ComputePartials(const Variables &inputs, Partials &partials) override;

private:
    size_t n_;  // Output dimension
    size_t m_;  // Input dimension
};

/**
 * Error-throwing discipline for testing error handling
 */
class ErrorDiscipline : public ExplicitDiscipline {
public:
    enum class ErrorMode {
        NO_ERROR,
        THROW_ON_SETUP,
        THROW_ON_COMPUTE,
        THROW_ON_PARTIALS
    };

    ErrorDiscipline(ErrorMode mode = ErrorMode::NO_ERROR);
    void Setup() override;
    void SetupPartials() override;
    void Compute(const Variables &inputs, Variables &outputs) override;
    void ComputePartials(const Variables &inputs, Partials &partials) override;

private:
    ErrorMode mode_;
};

// ============================================================================
// Test Discipline Implementations - Implicit
// ============================================================================

/**
 * Simple implicit discipline: x^2 - y = 0
 * Residual: R(x, y) = x^2 - y
 * Solution: y = x^2
 * Partials: dR/dx = 2x, dR/dy = -1
 */
class SimpleImplicitDiscipline : public ImplicitDiscipline {
public:
    void Setup() override;
    void SetupPartials() override;
    void ComputeResiduals(const Variables &inputs, const Variables &outputs, Variables &residuals) override;
    void SolveResiduals(const Variables &inputs, Variables &outputs) override;
    void ComputeResidualGradients(const Variables &inputs, const Variables &outputs, Partials &partials) override;
};

/**
 * Quadratic equation discipline: ax^2 + bx + c = 0
 * Inputs: a, b, c (coefficients)
 * Output: x (solution using quadratic formula, positive root)
 * Residual: R(a,b,c,x) = ax^2 + bx + c
 * Partials: dR/da = x^2, dR/db = x, dR/dc = 1, dR/dx = 2ax + b
 */
class QuadraticDiscipline : public ImplicitDiscipline {
public:
    void Setup() override;
    void SetupPartials() override;
    void ComputeResiduals(const Variables &inputs, const Variables &outputs, Variables &residuals) override;
    void SolveResiduals(const Variables &inputs, Variables &outputs) override;
    void ComputeResidualGradients(const Variables &inputs, const Variables &outputs, Partials &partials) override;
};

/**
 * Multi-residual discipline with coupled equations:
 *   R1: x + y - sum = 0
 *   R2: x * y - product = 0
 * Inputs: sum, product
 * Outputs: x, y
 */
class MultiResidualDiscipline : public ImplicitDiscipline {
public:
    void Setup() override;
    void SetupPartials() override;
    void ComputeResiduals(const Variables &inputs, const Variables &outputs, Variables &residuals) override;
    void SolveResiduals(const Variables &inputs, Variables &outputs) override;
    void ComputeResidualGradients(const Variables &inputs, const Variables &outputs, Partials &partials) override;
};

/**
 * Vectorized implicit discipline for testing vector operations
 * System: A*x + b - y = 0
 * Inputs: A (matrix), b (vector)
 * Outputs: y (vector)
 * Solution: y = A*x + b (where x is initialized from inputs)
 */
class VectorizedImplicitDiscipline : public ImplicitDiscipline {
public:
    VectorizedImplicitDiscipline(size_t n, size_t m);
    void Setup() override;
    void SetupPartials() override;
    void ComputeResiduals(const Variables &inputs, const Variables &outputs, Variables &residuals) override;
    void SolveResiduals(const Variables &inputs, Variables &outputs) override;
    void ComputeResidualGradients(const Variables &inputs, const Variables &outputs, Partials &partials) override;

private:
    size_t n_;  // Output dimension
    size_t m_;  // Input dimension
};

/**
 * Error-throwing implicit discipline for testing error handling
 */
class ImplicitErrorDiscipline : public ImplicitDiscipline {
public:
    enum class ErrorMode {
        NO_ERROR,
        THROW_ON_SETUP,
        THROW_ON_COMPUTE_RESIDUALS,
        THROW_ON_SOLVE_RESIDUALS,
        THROW_ON_GRADIENTS
    };

    ImplicitErrorDiscipline(ErrorMode mode = ErrorMode::NO_ERROR);
    void Setup() override;
    void SetupPartials() override;
    void ComputeResiduals(const Variables &inputs, const Variables &outputs, Variables &residuals) override;
    void SolveResiduals(const Variables &inputs, Variables &outputs) override;
    void ComputeResidualGradients(const Variables &inputs, const Variables &outputs, Partials &partials) override;

private:
    ErrorMode mode_;
};

// ============================================================================
// Server Management Helpers - Implicit
// ============================================================================

/**
 * Test server wrapper for implicit discipline integration testing
 * Manages server lifecycle and provides connection info
 */
class ImplicitTestServerManager {
public:
    ImplicitTestServerManager();
    ~ImplicitTestServerManager();

    /**
     * Start server with given discipline on a random available port
     * Returns the server address (e.g., "localhost:12345")
     */
    std::string StartServer(std::shared_ptr<ImplicitDiscipline> discipline);

    /**
     * Stop the server and clean up
     */
    void StopServer();

    /**
     * Get the server address
     */
    std::string GetAddress() const { return address_; }

    /**
     * Check if server is running
     */
    bool IsRunning() const { return running_; }

private:
    std::unique_ptr<grpc::Server> server_;
    std::string address_;
    bool running_;
    std::shared_ptr<ImplicitDiscipline> discipline_;
};

// ============================================================================
// Variable Creation Helpers
// ============================================================================

/**
 * Create a scalar Variable with a single value
 */
Variable CreateScalarVariable(double value);

/**
 * Create a vector Variable from a vector of values
 */
Variable CreateVectorVariable(const std::vector<double> &values);

/**
 * Create a matrix Variable with given dimensions and fill value
 */
Variable CreateMatrixVariable(size_t rows, size_t cols, double fill_value = 0.0);

/**
 * Create a Variables map with specific entries
 */
Variables CreateVariables(const std::map<std::string, std::vector<double>> &data);

/**
 * Create a Partials map with specific entries
 */
Partials CreatePartials(const std::map<std::pair<std::string, std::string>, std::vector<double>> &data);

// ============================================================================
// Assertion Helpers
// ============================================================================

/**
 * Compare two Variables for equality within tolerance
 */
void ExpectVariablesEqual(const Variable &expected, const Variable &actual, double tolerance = 1e-10);

/**
 * Compare two Variables maps for equality
 */
void ExpectVariablesMapsEqual(const Variables &expected, const Variables &actual, double tolerance = 1e-10);

/**
 * Compare two Partials maps for equality
 */
void ExpectPartialsMapsEqual(const Partials &expected, const Partials &actual, double tolerance = 1e-10);

/**
 * Assert that a Variable has expected shape
 */
void ExpectVariableShape(const Variable &var, const std::vector<size_t> &expected_shape);

// ============================================================================
// Server Management Helpers (for integration tests)
// ============================================================================

/**
 * Test server wrapper for integration testing
 * Manages server lifecycle and provides connection info
 */
class TestServerManager {
public:
    TestServerManager();
    ~TestServerManager();

    /**
     * Start server with given discipline on a random available port
     * Returns the server address (e.g., "localhost:12345")
     */
    std::string StartServer(std::shared_ptr<ExplicitDiscipline> discipline);

    /**
     * Stop the server and clean up
     */
    void StopServer();

    /**
     * Get the server address
     */
    std::string GetAddress() const { return address_; }

    /**
     * Check if server is running
     */
    bool IsRunning() const { return running_; }

private:
    std::unique_ptr<grpc::Server> server_;
    std::string address_;
    bool running_;
    std::shared_ptr<ExplicitDiscipline> discipline_;
};

/**
 * Create a gRPC channel to the given address with appropriate options
 */
std::shared_ptr<grpc::Channel> CreateTestChannel(const std::string &address);

/**
 * Find an available port for testing
 */
int FindAvailablePort();

// ============================================================================
// Data Generation Helpers
// ============================================================================

/**
 * Generate random data for testing
 */
std::vector<double> GenerateRandomData(size_t size, double min = -10.0, double max = 10.0);

/**
 * Generate sequential data (1, 2, 3, ..., n)
 */
std::vector<double> GenerateSequentialData(size_t size, double start = 1.0);

} // namespace test
} // namespace philote

#endif // PHILOTE_TEST_HELPERS_H
