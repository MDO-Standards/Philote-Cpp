#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "discipline.h"
#include <google/protobuf/struct.pb.h>

using namespace philote;
using namespace testing;

// Test fixture for Discipline class
class DisciplineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        discipline = std::make_unique<Discipline>();
    }

    void TearDown() override
    {
        discipline.reset();
    }

    std::unique_ptr<Discipline> discipline;
};

// Test constructor and basic initialization
TEST_F(DisciplineTest, ConstructorInitialization)
{
    EXPECT_TRUE(discipline != nullptr);
    EXPECT_TRUE(discipline->var_meta().empty());
    EXPECT_TRUE(discipline->partials_meta().empty());
}

// Test AddInput method
TEST_F(DisciplineTest, AddInput)
{
    std::string name = "test_input";
    std::vector<int64_t> shape = {2, 3};
    std::string units = "m";

    discipline->AddInput(name, shape, units);

    ASSERT_EQ(discipline->var_meta().size(), 1);
    const auto &var = discipline->var_meta()[0];
    EXPECT_EQ(var.name(), name);
    EXPECT_EQ(var.units(), units);
    EXPECT_EQ(var.type(), kInput);
    ASSERT_EQ(var.shape_size(), 2);
    EXPECT_EQ(var.shape(0), 2);
    EXPECT_EQ(var.shape(1), 3);
}

// Test AddOutput method
TEST_F(DisciplineTest, AddOutput)
{
    std::string name = "test_output";
    std::vector<int64_t> shape = {4, 5};
    std::string units = "kg";

    discipline->AddOutput(name, shape, units);

    ASSERT_EQ(discipline->var_meta().size(), 1);
    const auto &var = discipline->var_meta()[0];
    EXPECT_EQ(var.name(), name);
    EXPECT_EQ(var.units(), units);
    EXPECT_EQ(var.type(), kOutput);
    ASSERT_EQ(var.shape_size(), 2);
    EXPECT_EQ(var.shape(0), 4);
    EXPECT_EQ(var.shape(1), 5);
}

// Test DeclarePartials method with scalar variables
TEST_F(DisciplineTest, DeclarePartialsScalar)
{
    // Add input and output variables
    discipline->AddInput("x", {1}, "m");
    discipline->AddOutput("f", {1}, "N");

    // Declare partials
    discipline->DeclarePartials("f", "x");

    ASSERT_EQ(discipline->partials_meta().size(), 1);
    const auto &partial = discipline->partials_meta()[0];
    EXPECT_EQ(partial.name(), "f");
    EXPECT_EQ(partial.subname(), "x");
    ASSERT_EQ(partial.shape_size(), 1);
    EXPECT_EQ(partial.shape(0), 1);
}

// Test DeclarePartials method with vector variables
TEST_F(DisciplineTest, DeclarePartialsVector)
{
    // Add input and output variables
    discipline->AddInput("x", {3}, "m");
    discipline->AddOutput("f", {2}, "N");

    // Declare partials
    discipline->DeclarePartials("f", "x");

    ASSERT_EQ(discipline->partials_meta().size(), 1);
    const auto &partial = discipline->partials_meta()[0];
    EXPECT_EQ(partial.name(), "f");
    EXPECT_EQ(partial.subname(), "x");
    ASSERT_EQ(partial.shape_size(), 2);
    // Note: The current implementation concatenates shapes in the order they are found
    // First the output shape (2), then the input shape (3)
    EXPECT_EQ(partial.shape(0), 2);
    EXPECT_EQ(partial.shape(1), 3);
}

// Test SetOptions method
TEST_F(DisciplineTest, SetOptions)
{
    google::protobuf::Struct options_struct;
    // The base class implementation is empty, but we can test it doesn't crash
    EXPECT_NO_THROW(discipline->SetOptions(options_struct));
}

// Test Setup method
TEST_F(DisciplineTest, Setup)
{
    // The base class implementation is empty, but we can test it doesn't crash
    EXPECT_NO_THROW(discipline->Setup());
}

// Test SetupPartials method
TEST_F(DisciplineTest, SetupPartials)
{
    // The base class implementation is empty, but we can test it doesn't crash
    EXPECT_NO_THROW(discipline->SetupPartials());
}

// Test properties access
TEST_F(DisciplineTest, PropertiesAccess)
{
    auto &props = discipline->properties();
    EXPECT_FALSE(props.continuous());
    EXPECT_FALSE(props.differentiable());
    EXPECT_FALSE(props.provides_gradients());
    EXPECT_TRUE(props.name().empty());
    EXPECT_TRUE(props.version().empty());
}

// Test stream options access
TEST_F(DisciplineTest, StreamOptionsAccess)
{
    auto &opts = discipline->stream_opts();
    // Default is 1000 (set in Discipline constructor to prevent division by zero)
    EXPECT_EQ(opts.num_double(), 1000);
}

// Test options list access
TEST_F(DisciplineTest, OptionsListAccess)
{
    auto &options = discipline->options_list();
    EXPECT_TRUE(options.empty());

    // Test adding an option
    options["test_option"] = "double";
    EXPECT_EQ(options.size(), 1);
    EXPECT_EQ(options["test_option"], "double");
}

// Test AddOption method
TEST_F(DisciplineTest, AddOptionMethod)
{
    // Initially options list should be empty
    EXPECT_TRUE(discipline->options_list().empty());

    // Add options using the AddOption method
    discipline->AddOption("scale_factor", "float");
    discipline->AddOption("enable_scaling", "bool");
    discipline->AddOption("max_iterations", "int");

    // Verify options were added correctly
    auto &options = discipline->options_list();
    EXPECT_EQ(options.size(), 3);
    EXPECT_EQ(options["scale_factor"], "float");
    EXPECT_EQ(options["enable_scaling"], "bool");
    EXPECT_EQ(options["max_iterations"], "int");
}

// Test DeclarePartials error conditions (uncovered error paths)
TEST_F(DisciplineTest, DeclarePartialsErrorConditions)
{
    // Add some inputs and outputs for testing
    discipline->AddInput("input1", {2}, "m");
    discipline->AddInput("input2", {1}, "m");
    discipline->AddOutput("output1", {3}, "m");
    discipline->AddOutput("output2", {1}, "m");

    // Test missing output variable error
    EXPECT_THROW(discipline->DeclarePartials("nonexistent_output", "input1"), std::runtime_error);
    
    // Test missing input variable error
    EXPECT_THROW(discipline->DeclarePartials("output1", "nonexistent_input"), std::runtime_error);
    
    // Test both missing
    EXPECT_THROW(discipline->DeclarePartials("nonexistent_output", "nonexistent_input"), std::runtime_error);
}

// Test DeclarePartials edge cases for different shape combinations (uncovered paths)
TEST_F(DisciplineTest, DeclarePartialsShapeCombinations)
{
    // Add variables with different shapes
    discipline->AddInput("scalar_input", {1}, "m");      // scalar
    discipline->AddInput("vector_input", {3}, "m");      // vector
    discipline->AddOutput("scalar_output", {1}, "m");    // scalar
    discipline->AddOutput("vector_output", {2}, "m");    // vector

    // Test output scalar, input vector (uncovered line 133)
    EXPECT_NO_THROW(discipline->DeclarePartials("scalar_output", "vector_input"));
    
    // Test input scalar, output vector (uncovered line 138) 
    EXPECT_NO_THROW(discipline->DeclarePartials("vector_output", "scalar_input"));
    
    // Test scalar-scalar (this should work and is already covered)
    EXPECT_NO_THROW(discipline->DeclarePartials("scalar_output", "scalar_input"));
    
    // Test vector-vector (this should work and is already covered)
    EXPECT_NO_THROW(discipline->DeclarePartials("vector_output", "vector_input"));
    
    // Verify that the partials were added
    EXPECT_EQ(discipline->partials_meta().size(), 4);
}

// Test Initialize and Configure method behavior
TEST_F(DisciplineTest, InitializeConfigureBehavior)
{
    // Test that Initialize is called during construction (already tested in constructor)
    // and Configure is called during SetOptions
    
    // Verify the default Initialize does nothing but doesn't crash
    EXPECT_NO_THROW(discipline->Initialize());
    
    // Verify the default Configure does nothing but doesn't crash  
    EXPECT_NO_THROW(discipline->Configure());
    
    // Test that Configure is called during SetOptions
    google::protobuf::Struct empty_options;
    EXPECT_NO_THROW(discipline->SetOptions(empty_options));
    
    // Test adding options manually after construction
    discipline->AddOption("manual_option", "string");
    auto &options = discipline->options_list();
    EXPECT_TRUE(options.find("manual_option") != options.end());
    EXPECT_EQ(options["manual_option"], "string");
}

// Test discipline that overrides SetOptions to demonstrate proper pattern
class ConfigurableDiscipline : public Discipline
{
public:
    double scale_factor_ = 1.0;
    int dimension_ = 2;
    bool enable_feature_ = false;
    std::string mode_ = "default";
    bool configure_called_ = false;

    void Initialize() override
    {
        Discipline::Initialize();
        AddOption("scale_factor", "float");
        AddOption("dimension", "int");
        AddOption("enable_feature", "bool");
        AddOption("mode", "string");
    }

    void SetOptions(const google::protobuf::Struct &options_struct) override
    {
        // Extract scale_factor
        auto it = options_struct.fields().find("scale_factor");
        if (it != options_struct.fields().end())
        {
            scale_factor_ = it->second.number_value();
        }

        // Extract dimension
        it = options_struct.fields().find("dimension");
        if (it != options_struct.fields().end())
        {
            dimension_ = static_cast<int>(it->second.number_value());
        }

        // Extract enable_feature
        it = options_struct.fields().find("enable_feature");
        if (it != options_struct.fields().end())
        {
            enable_feature_ = it->second.bool_value();
        }

        // Extract mode
        it = options_struct.fields().find("mode");
        if (it != options_struct.fields().end())
        {
            mode_ = it->second.string_value();
        }

        // Call parent implementation which invokes Configure()
        Discipline::SetOptions(options_struct);
    }

    void Configure() override
    {
        configure_called_ = true;
    }
};

// Test fixture for ConfigurableDiscipline
class ConfigurableDisciplineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        discipline = std::make_unique<ConfigurableDiscipline>();
    }

    void TearDown() override
    {
        discipline.reset();
    }

    std::unique_ptr<ConfigurableDiscipline> discipline;
};

// Test that Initialize properly declares options
TEST_F(ConfigurableDisciplineTest, InitializeDeclareOptions)
{
    const auto &options = discipline->options_list();
    EXPECT_EQ(options.size(), 4);
    EXPECT_EQ(options.at("scale_factor"), "float");
    EXPECT_EQ(options.at("dimension"), "int");
    EXPECT_EQ(options.at("enable_feature"), "bool");
    EXPECT_EQ(options.at("mode"), "string");
}

// Test extracting float option value
TEST_F(ConfigurableDisciplineTest, SetOptionsExtractsFloatValue)
{
    google::protobuf::Struct options_struct;
    (*options_struct.mutable_fields())["scale_factor"].set_number_value(2.5);

    EXPECT_DOUBLE_EQ(discipline->scale_factor_, 1.0);  // Default value
    discipline->SetOptions(options_struct);
    EXPECT_DOUBLE_EQ(discipline->scale_factor_, 2.5);  // Updated value
}

// Test extracting int option value
TEST_F(ConfigurableDisciplineTest, SetOptionsExtractsIntValue)
{
    google::protobuf::Struct options_struct;
    (*options_struct.mutable_fields())["dimension"].set_number_value(5);

    EXPECT_EQ(discipline->dimension_, 2);  // Default value
    discipline->SetOptions(options_struct);
    EXPECT_EQ(discipline->dimension_, 5);  // Updated value
}

// Test extracting bool option value
TEST_F(ConfigurableDisciplineTest, SetOptionsExtractsBoolValue)
{
    google::protobuf::Struct options_struct;
    (*options_struct.mutable_fields())["enable_feature"].set_bool_value(true);

    EXPECT_FALSE(discipline->enable_feature_);  // Default value
    discipline->SetOptions(options_struct);
    EXPECT_TRUE(discipline->enable_feature_);  // Updated value
}

// Test extracting string option value
TEST_F(ConfigurableDisciplineTest, SetOptionsExtractsStringValue)
{
    google::protobuf::Struct options_struct;
    (*options_struct.mutable_fields())["mode"].set_string_value("advanced");

    EXPECT_EQ(discipline->mode_, "default");  // Default value
    discipline->SetOptions(options_struct);
    EXPECT_EQ(discipline->mode_, "advanced");  // Updated value
}

// Test extracting multiple option values at once
TEST_F(ConfigurableDisciplineTest, SetOptionsExtractsMultipleValues)
{
    google::protobuf::Struct options_struct;
    (*options_struct.mutable_fields())["scale_factor"].set_number_value(3.14);
    (*options_struct.mutable_fields())["dimension"].set_number_value(10);
    (*options_struct.mutable_fields())["enable_feature"].set_bool_value(true);
    (*options_struct.mutable_fields())["mode"].set_string_value("turbo");

    discipline->SetOptions(options_struct);

    EXPECT_DOUBLE_EQ(discipline->scale_factor_, 3.14);
    EXPECT_EQ(discipline->dimension_, 10);
    EXPECT_TRUE(discipline->enable_feature_);
    EXPECT_EQ(discipline->mode_, "turbo");
}

// Test that Configure is called after SetOptions
TEST_F(ConfigurableDisciplineTest, SetOptionsCallsConfigure)
{
    google::protobuf::Struct options_struct;
    EXPECT_FALSE(discipline->configure_called_);
    discipline->SetOptions(options_struct);
    EXPECT_TRUE(discipline->configure_called_);
}

// Test that missing options don't change default values
TEST_F(ConfigurableDisciplineTest, SetOptionsMissingOptionsKeepDefaults)
{
    google::protobuf::Struct options_struct;
    // Only set scale_factor, leave others unset
    (*options_struct.mutable_fields())["scale_factor"].set_number_value(5.0);

    discipline->SetOptions(options_struct);

    EXPECT_DOUBLE_EQ(discipline->scale_factor_, 5.0);  // Updated
    EXPECT_EQ(discipline->dimension_, 2);  // Default unchanged
    EXPECT_FALSE(discipline->enable_feature_);  // Default unchanged
    EXPECT_EQ(discipline->mode_, "default");  // Default unchanged
}

// Test with empty options struct
TEST_F(ConfigurableDisciplineTest, SetOptionsEmptyStruct)
{
    google::protobuf::Struct options_struct;

    discipline->SetOptions(options_struct);

    // All values should remain at defaults
    EXPECT_DOUBLE_EQ(discipline->scale_factor_, 1.0);
    EXPECT_EQ(discipline->dimension_, 2);
    EXPECT_FALSE(discipline->enable_feature_);
    EXPECT_EQ(discipline->mode_, "default");
    EXPECT_TRUE(discipline->configure_called_);
}