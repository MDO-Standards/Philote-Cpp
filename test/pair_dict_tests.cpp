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

#include <gtest/gtest.h>
#include <variable.h>

using namespace philote;

/*
    Test PairDict basic construction and assignment
*/
TEST(PairDictTests, BasicConstruction)
{
    PairDict<double> dict;
    
    // Initially should be empty
    EXPECT_TRUE(dict.empty());
    EXPECT_EQ(dict.size(), 0);
    
    // Test basic assignment
    dict("output", "input") = 42.0;
    
    EXPECT_FALSE(dict.empty());
    EXPECT_EQ(dict.size(), 1);
    EXPECT_EQ(dict("output", "input"), 42.0);
}

/*
    Test PairDict with Variable objects
*/
TEST(PairDictTests, VariablePairDict)
{
    PairDict<Variable> var_dict;
    
    // Create test variables
    Variable var1(kInput, {2, 2});
    Variable var2(kOutput, {3});
    
    // Fill variables with test data
    for (size_t i = 0; i < var1.Size(); ++i) {
        var1(i) = static_cast<double>(i * 10);
    }
    
    for (size_t i = 0; i < var2.Size(); ++i) {
        var2(i) = static_cast<double>(i * 100);
    }
    
    // Add to dictionary
    var_dict("f1", "x1") = var1;
    var_dict("f2", "x2") = var2;
    
    EXPECT_EQ(var_dict.size(), 2);
    
    // Verify values
    EXPECT_EQ(var_dict("f1", "x1").Size(), 4);
    EXPECT_EQ(var_dict("f2", "x2").Size(), 3);
    
    // Test data integrity
    for (size_t i = 0; i < var_dict("f1", "x1").Size(); ++i) {
        EXPECT_EQ(var_dict("f1", "x1")(i), static_cast<double>(i * 10));
    }
}

/*
    Test const access methods
*/
TEST(PairDictTests, ConstAccess)
{
    PairDict<int> dict;
    dict("key1", "key2") = 100;
    dict("foo", "bar") = 200;
    
    const PairDict<int>& const_dict = dict;
    
    // Test const access
    EXPECT_EQ(const_dict("key1", "key2"), 100);
    EXPECT_EQ(const_dict("foo", "bar"), 200);
    
    // Test contains method
    EXPECT_TRUE(const_dict.contains("key1", "key2"));
    EXPECT_TRUE(const_dict.contains("foo", "bar"));
    EXPECT_FALSE(const_dict.contains("nonexistent", "key"));
    EXPECT_FALSE(const_dict.contains("key1", "wrong"));
}

/*
    Test container operations
*/
TEST(PairDictTests, ContainerOperations)
{
    PairDict<std::string> dict;
    
    // Test empty state
    EXPECT_TRUE(dict.empty());
    EXPECT_EQ(dict.size(), 0);
    
    // Add some entries
    dict("name", "first") = "John";
    dict("name", "last") = "Doe";
    dict("age", "current") = "30";
    
    EXPECT_FALSE(dict.empty());
    EXPECT_EQ(dict.size(), 3);
    
    // Test clear
    dict.clear();
    EXPECT_TRUE(dict.empty());
    EXPECT_EQ(dict.size(), 0);
    
    // Verify contains returns false after clear
    EXPECT_FALSE(dict.contains("name", "first"));
}

/*
    Test iterator functionality
*/
TEST(PairDictTests, IteratorFunctionality)
{
    PairDict<double> dict;
    dict("f1", "x1") = 1.0;
    dict("f1", "x2") = 2.0;
    dict("f2", "x1") = 3.0;
    
    // Test non-const iterators
    size_t count = 0;
    for (auto it = dict.begin(); it != dict.end(); ++it) {
        count++;
        EXPECT_GT(it->second, 0.0);  // All values should be positive
    }
    EXPECT_EQ(count, 3);
    
    // Test const iterators
    const PairDict<double>& const_dict = dict;
    count = 0;
    for (auto it = const_dict.begin(); it != const_dict.end(); ++it) {
        count++;
        EXPECT_GT(it->second, 0.0);
    }
    EXPECT_EQ(count, 3);
    
    // Test range-based for loop
    count = 0;
    for (const auto& entry : dict) {
        count++;
        EXPECT_GT(entry.second, 0.0);
        
        // Verify key structure
        EXPECT_FALSE(entry.first.first.empty());   // First key not empty
        EXPECT_FALSE(entry.first.second.empty());  // Second key not empty
    }
    EXPECT_EQ(count, 3);
}

/*
    Test edge cases and error conditions
*/
TEST(PairDictTests, EdgeCases)
{
    PairDict<int> dict;
    
    // Test access to non-existent key (should create entry)
    dict("new", "key") = 42;
    EXPECT_TRUE(dict.contains("new", "key"));
    EXPECT_EQ(dict("new", "key"), 42);
    
    // Test overwriting existing values
    dict("test", "value") = 100;
    EXPECT_EQ(dict("test", "value"), 100);
    
    dict("test", "value") = 200;
    EXPECT_EQ(dict("test", "value"), 200);
    
    // Test with empty string keys
    dict("", "") = 999;
    EXPECT_TRUE(dict.contains("", ""));
    EXPECT_EQ(dict("", ""), 999);
    
    // Test with one empty key
    dict("valid", "") = 111;
    dict("", "valid") = 222;
    EXPECT_TRUE(dict.contains("valid", ""));
    EXPECT_TRUE(dict.contains("", "valid"));
}

/*
    Test specialized typedef PartialsPairDict
*/
TEST(PairDictTests, PartialsPairDictTypedef)
{
    PartialsPairDict partials;
    
    // Create test variables for partials
    Variable partial1(kPartial, {2, 3});
    Variable partial2(kPartial, {1});
    
    // Fill with test data
    for (size_t i = 0; i < partial1.Size(); ++i) {
        partial1(i) = static_cast<double>(i) * 0.1;
    }
    partial2(0) = 5.0;
    
    // Add to partials dictionary
    partials("output1", "input1") = partial1;
    partials("output2", "input2") = partial2;
    
    EXPECT_EQ(partials.size(), 2);
    EXPECT_TRUE(partials.contains("output1", "input1"));
    EXPECT_TRUE(partials.contains("output2", "input2"));
    
    // Verify values
    EXPECT_EQ(partials("output1", "input1").Size(), 6);
    EXPECT_EQ(partials("output2", "input2").Size(), 1);
    EXPECT_EQ(partials("output2", "input2")(0), 5.0);
}

/*
    Test copy and assignment operations
*/
TEST(PairDictTests, CopyAndAssignment)
{
    PairDict<double> dict1;
    dict1("a", "b") = 1.0;
    dict1("c", "d") = 2.0;
    
    // Test copy constructor
    PairDict<double> dict2 = dict1;
    EXPECT_EQ(dict2.size(), 2);
    EXPECT_EQ(dict2("a", "b"), 1.0);
    EXPECT_EQ(dict2("c", "d"), 2.0);
    
    // Test assignment operator
    PairDict<double> dict3;
    dict3 = dict1;
    EXPECT_EQ(dict3.size(), 2);
    EXPECT_EQ(dict3("a", "b"), 1.0);
    EXPECT_EQ(dict3("c", "d"), 2.0);
    
    // Modify original and verify copies are independent
    dict1("a", "b") = 99.0;
    EXPECT_EQ(dict1("a", "b"), 99.0);
    EXPECT_EQ(dict2("a", "b"), 1.0);  // Should be unchanged
    EXPECT_EQ(dict3("a", "b"), 1.0);  // Should be unchanged
}

/*
    Test performance with larger datasets
*/
TEST(PairDictTests, PerformanceAndScale)
{
    PairDict<int> dict;
    
    // Add many entries
    const size_t num_entries = 100;
    for (size_t i = 0; i < num_entries; ++i) {
        std::string key1 = "f" + std::to_string(i);
        std::string key2 = "x" + std::to_string(i);
        dict(key1, key2) = static_cast<int>(i);
    }
    
    EXPECT_EQ(dict.size(), num_entries);
    
    // Verify all entries exist and have correct values
    for (size_t i = 0; i < num_entries; ++i) {
        std::string key1 = "f" + std::to_string(i);
        std::string key2 = "x" + std::to_string(i);
        EXPECT_TRUE(dict.contains(key1, key2));
        EXPECT_EQ(dict(key1, key2), static_cast<int>(i));
    }
    
    // Test that non-existent keys return false
    EXPECT_FALSE(dict.contains("nonexistent", "key"));
    EXPECT_FALSE(dict.contains("f0", "nonexistent"));
}