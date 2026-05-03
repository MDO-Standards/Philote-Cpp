#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "validation.h"

using namespace philote;
using namespace testing;

// ============================================================================
// PhiloteError hierarchy tests
// ============================================================================

TEST(PhiloteErrorTest, PhiloteErrorInheritsFromRuntimeError)
{
    PhiloteError error("test error");
    EXPECT_STREQ(error.what(), "test error");

    // Verify it can be caught as std::runtime_error
    EXPECT_THROW(throw PhiloteError("msg"), std::runtime_error);
}

TEST(PhiloteErrorTest, PhiloteValidationErrorInheritsFromPhiloteError)
{
    PhiloteValidationError error("validation error");
    EXPECT_STREQ(error.what(), "validation error");

    // Verify it can be caught as PhiloteError and std::runtime_error
    EXPECT_THROW(throw PhiloteValidationError("msg"), PhiloteError);
    EXPECT_THROW(throw PhiloteValidationError("msg"), std::runtime_error);
}

TEST(PhiloteErrorTest, PhiloteServerErrorInheritsFromPhiloteError)
{
    PhiloteServerError error("server error");
    EXPECT_STREQ(error.what(), "server error");

    EXPECT_THROW(throw PhiloteServerError("msg"), PhiloteError);
    EXPECT_THROW(throw PhiloteServerError("msg"), std::runtime_error);
}

// ============================================================================
// ValidateName tests
// ============================================================================

TEST(ValidateNameTest, ValidNameDoesNotThrow)
{
    EXPECT_NO_THROW(ValidateName("x", "test_context"));
    EXPECT_NO_THROW(ValidateName("my_variable", "test_context"));
    EXPECT_NO_THROW(ValidateName("a", "test_context"));
}

TEST(ValidateNameTest, EmptyNameThrows)
{
    EXPECT_THROW(ValidateName("", "add_input"), PhiloteValidationError);
}

TEST(ValidateNameTest, EmptyNameErrorMessageContainsContext)
{
    try
    {
        ValidateName("", "add_input");
        FAIL() << "Expected PhiloteValidationError";
    }
    catch (const PhiloteValidationError &e)
    {
        EXPECT_THAT(std::string(e.what()), HasSubstr("add_input"));
        EXPECT_THAT(std::string(e.what()), HasSubstr("must not be empty"));
    }
}

// ============================================================================
// ValidateShape tests
// ============================================================================

TEST(ValidateShapeTest, ValidShapeDoesNotThrow)
{
    EXPECT_NO_THROW(ValidateShape({1}, "test"));
    EXPECT_NO_THROW(ValidateShape({2, 3}, "test"));
    EXPECT_NO_THROW(ValidateShape({10, 20, 30}, "test"));
}

TEST(ValidateShapeTest, EmptyShapeDoesNotThrow)
{
    EXPECT_NO_THROW(ValidateShape({}, "test"));
}

TEST(ValidateShapeTest, ZeroDimensionThrows)
{
    EXPECT_THROW(ValidateShape({0}, "test"), PhiloteValidationError);
    EXPECT_THROW(ValidateShape({2, 0}, "test"), PhiloteValidationError);
    EXPECT_THROW(ValidateShape({0, 3}, "test"), PhiloteValidationError);
}

TEST(ValidateShapeTest, NegativeDimensionThrows)
{
    EXPECT_THROW(ValidateShape({-1}, "test"), PhiloteValidationError);
    EXPECT_THROW(ValidateShape({2, -5}, "test"), PhiloteValidationError);
}

TEST(ValidateShapeTest, ErrorMessageContainsDetails)
{
    try
    {
        ValidateShape({3, -2}, "add_input");
        FAIL() << "Expected PhiloteValidationError";
    }
    catch (const PhiloteValidationError &e)
    {
        std::string msg(e.what());
        EXPECT_THAT(msg, HasSubstr("add_input"));
        EXPECT_THAT(msg, HasSubstr("positive"));
        EXPECT_THAT(msg, HasSubstr("1"));  // element index
        EXPECT_THAT(msg, HasSubstr("-2")); // the bad value
    }
}

// ============================================================================
// ValidateOptionType tests
// ============================================================================

TEST(ValidateOptionTypeTest, AllValidTypesAccepted)
{
    EXPECT_NO_THROW(ValidateOptionType("bool", "opt"));
    EXPECT_NO_THROW(ValidateOptionType("int", "opt"));
    EXPECT_NO_THROW(ValidateOptionType("float", "opt"));
    EXPECT_NO_THROW(ValidateOptionType("double", "opt"));
    EXPECT_NO_THROW(ValidateOptionType("string", "opt"));
    EXPECT_NO_THROW(ValidateOptionType("dict", "opt"));
}

TEST(ValidateOptionTypeTest, InvalidTypeThrows)
{
    EXPECT_THROW(ValidateOptionType("list", "opt"), PhiloteValidationError);
    EXPECT_THROW(ValidateOptionType("", "opt"), PhiloteValidationError);
    EXPECT_THROW(ValidateOptionType("BOOL", "opt"), PhiloteValidationError);
    EXPECT_THROW(ValidateOptionType("integer", "opt"), PhiloteValidationError);
}

TEST(ValidateOptionTypeTest, ErrorMessageContainsDetails)
{
    try
    {
        ValidateOptionType("vector", "my_option");
        FAIL() << "Expected PhiloteValidationError";
    }
    catch (const PhiloteValidationError &e)
    {
        std::string msg(e.what());
        EXPECT_THAT(msg, HasSubstr("vector"));
        EXPECT_THAT(msg, HasSubstr("my_option"));
        EXPECT_THAT(msg, HasSubstr("Allowed types"));
    }
}
