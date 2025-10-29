#include "test_helpers.h"
#include <cmath>
#include <random>
#include <stdexcept>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace philote {
namespace test {

// ============================================================================
// ParaboloidDiscipline Implementation
// ============================================================================

void ParaboloidDiscipline::Setup() {
    AddInput("x", {1}, "m");
    AddInput("y", {1}, "m");
    AddOutput("f", {1}, "m^2");
}

void ParaboloidDiscipline::SetupPartials() {
    DeclarePartials("f", "x");
    DeclarePartials("f", "y");
}

void ParaboloidDiscipline::Compute(const Variables &inputs, Variables &outputs) {
    double x = inputs.at("x").data()[0];
    double y = inputs.at("y").data()[0];
    double f = x * x + y * y;

    outputs["f"] = CreateScalarVariable(f);
}

void ParaboloidDiscipline::ComputePartials(const Variables &inputs, Partials &partials) {
    double x = inputs.at("x").data()[0];
    double y = inputs.at("y").data()[0];

    partials[{"f", "x"}] = CreateScalarVariable(2.0 * x);
    partials[{"f", "y"}] = CreateScalarVariable(2.0 * y);
}

// ============================================================================
// MultiOutputDiscipline Implementation
// ============================================================================

void MultiOutputDiscipline::Setup() {
    AddInput("x", {1}, "m");
    AddInput("y", {1}, "m");
    AddOutput("sum", {1}, "m");
    AddOutput("product", {1}, "m");
    AddOutput("difference", {1}, "m");
}

void MultiOutputDiscipline::SetupPartials() {
    DeclarePartials("sum", "x");
    DeclarePartials("sum", "y");
    DeclarePartials("product", "x");
    DeclarePartials("product", "y");
    DeclarePartials("difference", "x");
    DeclarePartials("difference", "y");
}

void MultiOutputDiscipline::Compute(const Variables &inputs, Variables &outputs) {
    double x = inputs.at("x").data()[0];
    double y = inputs.at("y").data()[0];

    outputs["sum"] = CreateScalarVariable(x + y);
    outputs["product"] = CreateScalarVariable(x * y);
    outputs["difference"] = CreateScalarVariable(x - y);
}

void MultiOutputDiscipline::ComputePartials(const Variables &inputs, Partials &partials) {
    double x = inputs.at("x").data()[0];
    double y = inputs.at("y").data()[0];

    // d(sum)/dx = 1, d(sum)/dy = 1
    partials[{"sum", "x"}] = CreateScalarVariable(1.0);
    partials[{"sum", "y"}] = CreateScalarVariable(1.0);

    // d(product)/dx = y, d(product)/dy = x
    partials[{"product", "x"}] = CreateScalarVariable(y);
    partials[{"product", "y"}] = CreateScalarVariable(x);

    // d(difference)/dx = 1, d(difference)/dy = -1
    partials[{"difference", "x"}] = CreateScalarVariable(1.0);
    partials[{"difference", "y"}] = CreateScalarVariable(-1.0);
}

// ============================================================================
// VectorizedDiscipline Implementation
// ============================================================================

VectorizedDiscipline::VectorizedDiscipline(size_t n, size_t m) : n_(n), m_(m) {}

void VectorizedDiscipline::Setup() {
    AddInput("A", {n_, m_}, "");
    AddInput("x", {m_}, "");
    AddInput("b", {n_}, "");
    AddOutput("z", {n_}, "");
}

void VectorizedDiscipline::SetupPartials() {
    DeclarePartials("z", "A");
    DeclarePartials("z", "x");
    DeclarePartials("z", "b");
}

void VectorizedDiscipline::Compute(const Variables &inputs, Variables &outputs) {
    const double *A = inputs.at("A").data();
    const double *x = inputs.at("x").data();
    const double *b = inputs.at("b").data();

    std::vector<double> z(n_, 0.0);

    // z = A * x + b
    for (size_t i = 0; i < n_; ++i) {
        z[i] = b[i];
        for (size_t j = 0; j < m_; ++j) {
            z[i] += A[i * m_ + j] * x[j];
        }
    }

    outputs["z"] = CreateVectorVariable(z);
}

void VectorizedDiscipline::ComputePartials(const Variables &inputs, Partials &partials) {
    const double *A = inputs.at("A").data();
    const double *x = inputs.at("x").data();

    // dz/dA: Each z[i] depends on A[i,:], with derivatives equal to x
    std::vector<double> dz_dA(n_ * m_);
    for (size_t i = 0; i < n_; ++i) {
        for (size_t j = 0; j < m_; ++j) {
            dz_dA[i * m_ + j] = x[j];
        }
    }
    partials[{"z", "A"}] = CreateMatrixVariable(n_, m_, 0.0);
    std::copy(dz_dA.begin(), dz_dA.end(), const_cast<double*>(partials[{"z", "A"}].data()));

    // dz/dx: Each z[i] depends on all x[j], with derivatives equal to A[i,j]
    std::vector<double> dz_dx(n_ * m_);
    for (size_t i = 0; i < n_; ++i) {
        for (size_t j = 0; j < m_; ++j) {
            dz_dx[i * m_ + j] = A[i * m_ + j];
        }
    }
    partials[{"z", "x"}] = CreateMatrixVariable(n_, m_, 0.0);
    std::copy(dz_dx.begin(), dz_dx.end(), const_cast<double*>(partials[{"z", "x"}].data()));

    // dz/db: Identity matrix (dz[i]/db[j] = delta_ij)
    std::vector<double> dz_db(n_ * n_, 0.0);
    for (size_t i = 0; i < n_; ++i) {
        dz_db[i * n_ + i] = 1.0;
    }
    partials[{"z", "b"}] = CreateMatrixVariable(n_, n_, 0.0);
    std::copy(dz_db.begin(), dz_db.end(), const_cast<double*>(partials[{"z", "b"}].data()));
}

// ============================================================================
// ErrorDiscipline Implementation
// ============================================================================

ErrorDiscipline::ErrorDiscipline(ErrorMode mode) : mode_(mode) {}

void ErrorDiscipline::Setup() {
    if (mode_ == ErrorMode::THROW_ON_SETUP) {
        throw std::runtime_error("ErrorDiscipline: Error in Setup()");
    }
    AddInput("x", {1}, "");
    AddOutput("y", {1}, "");
}

void ErrorDiscipline::SetupPartials() {
    DeclarePartials("y", "x");
}

void ErrorDiscipline::Compute(const Variables &inputs, Variables &outputs) {
    if (mode_ == ErrorMode::THROW_ON_COMPUTE) {
        throw std::runtime_error("ErrorDiscipline: Error in Compute()");
    }
    double x = inputs.at("x").data()[0];
    outputs["y"] = CreateScalarVariable(x);
}

void ErrorDiscipline::ComputePartials(const Variables &inputs, Partials &partials) {
    if (mode_ == ErrorMode::THROW_ON_PARTIALS) {
        throw std::runtime_error("ErrorDiscipline: Error in ComputePartials()");
    }
    partials[{"y", "x"}] = CreateScalarVariable(1.0);
}

// ============================================================================
// Variable Creation Helpers
// ============================================================================

Variable CreateScalarVariable(double value) {
    Variable var({1});
    var.data()[0] = value;
    return var;
}

Variable CreateVectorVariable(const std::vector<double> &values) {
    Variable var({values.size()});
    std::copy(values.begin(), values.end(), var.data());
    return var;
}

Variable CreateMatrixVariable(size_t rows, size_t cols, double fill_value) {
    Variable var({rows, cols});
    std::fill(var.data(), var.data() + rows * cols, fill_value);
    return var;
}

Variables CreateVariables(const std::map<std::string, std::vector<double>> &data) {
    Variables vars;
    for (const auto &[name, values] : data) {
        vars[name] = CreateVectorVariable(values);
    }
    return vars;
}

Partials CreatePartials(const std::map<std::pair<std::string, std::string>, std::vector<double>> &data) {
    Partials partials;
    for (const auto &[key, values] : data) {
        partials[key] = CreateVectorVariable(values);
    }
    return partials;
}

// ============================================================================
// Assertion Helpers
// ============================================================================

void ExpectVariablesEqual(const Variable &expected, const Variable &actual, double tolerance) {
    ASSERT_EQ(expected.shape().size(), actual.shape().size())
        << "Variable shapes have different dimensions";

    for (size_t i = 0; i < expected.shape().size(); ++i) {
        ASSERT_EQ(expected.shape()[i], actual.shape()[i])
            << "Variable shapes differ at dimension " << i;
    }

    size_t total_size = 1;
    for (size_t dim : expected.shape()) {
        total_size *= dim;
    }

    for (size_t i = 0; i < total_size; ++i) {
        EXPECT_NEAR(expected.data()[i], actual.data()[i], tolerance)
            << "Variables differ at index " << i;
    }
}

void ExpectVariablesMapsEqual(const Variables &expected, const Variables &actual, double tolerance) {
    ASSERT_EQ(expected.size(), actual.size()) << "Variables maps have different sizes";

    for (const auto &[name, expected_var] : expected) {
        ASSERT_TRUE(actual.count(name) > 0) << "Variable '" << name << "' not found in actual map";
        ExpectVariablesEqual(expected_var, actual.at(name), tolerance);
    }
}

void ExpectPartialsMapsEqual(const Partials &expected, const Partials &actual, double tolerance) {
    ASSERT_EQ(expected.size(), actual.size()) << "Partials maps have different sizes";

    for (const auto &[key, expected_var] : expected) {
        const auto &[output, input] = key;
        ASSERT_TRUE(actual.count(key) > 0)
            << "Partial (" << output << ", " << input << ") not found in actual map";
        ExpectVariablesEqual(expected_var, actual.at(key), tolerance);
    }
}

void ExpectVariableShape(const Variable &var, const std::vector<size_t> &expected_shape) {
    ASSERT_EQ(var.shape().size(), expected_shape.size())
        << "Variable has different number of dimensions than expected";

    for (size_t i = 0; i < expected_shape.size(); ++i) {
        EXPECT_EQ(var.shape()[i], expected_shape[i])
            << "Variable shape differs at dimension " << i;
    }
}

// ============================================================================
// Server Management Helpers
// ============================================================================

TestServerManager::TestServerManager() : running_(false), discipline_(nullptr) {}

TestServerManager::~TestServerManager() {
    if (running_) {
        StopServer();
    }
}

std::string TestServerManager::StartServer(ExplicitDiscipline *discipline) {
    if (running_) {
        throw std::runtime_error("Server is already running");
    }

    discipline_ = discipline;

    // Find an available port
    int port = FindAvailablePort();
    address_ = "localhost:" + std::to_string(port);

    // Initialize discipline
    discipline_->Initialize();
    discipline_->Configure();
    discipline_->Setup();
    discipline_->SetupPartials();

    // Build and start server
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
    discipline_->RegisterServices(builder);

    server_ = builder.BuildAndStart();
    if (!server_) {
        throw std::runtime_error("Failed to start test server");
    }

    running_ = true;
    return address_;
}

void TestServerManager::StopServer() {
    if (server_) {
        server_->Shutdown();
        server_->Wait();
        server_.reset();
    }
    running_ = false;
    discipline_ = nullptr;
}

std::shared_ptr<grpc::Channel> CreateTestChannel(const std::string &address) {
    return grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
}

int FindAvailablePort() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;  // Let OS choose port

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        throw std::runtime_error("Failed to bind socket");
    }

    socklen_t addr_len = sizeof(addr);
    if (getsockname(sock, (sockaddr*)&addr, &addr_len) < 0) {
        close(sock);
        throw std::runtime_error("Failed to get socket name");
    }

    int port = ntohs(addr.sin_port);
    close(sock);

    return port;
}

// ============================================================================
// Data Generation Helpers
// ============================================================================

std::vector<double> GenerateRandomData(size_t size, double min, double max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(min, max);

    std::vector<double> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = dist(gen);
    }
    return data;
}

std::vector<double> GenerateSequentialData(size_t size, double start) {
    std::vector<double> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = start + static_cast<double>(i);
    }
    return data;
}

} // namespace test
} // namespace philote
