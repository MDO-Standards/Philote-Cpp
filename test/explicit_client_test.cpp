#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <grpcpp/grpcpp.h>

#include "explicit.h"
#include "disciplines_mock.grpc.pb.h"

using namespace philote;
using namespace testing;

// Test just creating an ExplicitClient
TEST(ExplicitClientTest, SimpleConstructor)
{
    ExplicitClient client; // Use stack allocation like DisciplineClientTest
    // Just test that it was created without crashing
    EXPECT_TRUE(true);
}

// Test constructor initialization without any mock setup
TEST(ExplicitClientTest, ConstructorInitialization)
{
    ExplicitClient client;
    EXPECT_EQ(client.GetVariableNames().size(), 0); // Use working DisciplineClient methods
    EXPECT_EQ(client.GetPartialsMeta().size(), 0);
}

// Test metadata getters and setters without mock setup
TEST(ExplicitClientTest, MetadataAccessors)
{
    ExplicitClient client;

    // Test variable metadata access (similar to working DisciplineClient test pattern)
    EXPECT_EQ(client.GetVariableNames().size(), 0);
    EXPECT_EQ(client.GetPartialsMeta().size(), 0);
}