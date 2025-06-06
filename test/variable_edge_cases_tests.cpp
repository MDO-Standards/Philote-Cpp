/*
    Philote C++ Bindings

    Copyright 2022-2024 Christopher A. Lupp

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
#include <iostream>
#include <vector>
#include <stdexcept>

#include <gtest/gtest.h>

#include <variable.h>
#include <disciplines.grpc.pb.h>

using std::vector;
using namespace philote;

/*
    Test edge cases for the segment function.
*/
TEST(VariableEdgeCases, SegmentEdgeCases)
{
    // Test empty array
    Variable empty_array = Variable(kInput, {0});
    std::vector<double> empty_data;
    EXPECT_THROW(empty_array.Segment(0, 0, empty_data), std::out_of_range);

    // Test single element array
    Variable single_array = Variable(kInput, {1});
    std::vector<double> single_data = {1.0};
    EXPECT_NO_THROW(single_array.Segment(0, 0, single_data));

    // Test invalid start index
    Variable array = Variable(kInput, {2, 2});
    std::vector<double> data = {1.0, 2.0};
    EXPECT_THROW(array.Segment(-1, 1, data), std::invalid_argument);
    EXPECT_THROW(array.Segment(4, 5, data), std::out_of_range);

    // Test invalid end index
    EXPECT_THROW(array.Segment(0, 5, data), std::out_of_range);
    EXPECT_THROW(array.Segment(2, 1, data), std::invalid_argument);

    // Test empty data vector
    EXPECT_THROW(array.Segment(0, 1, std::vector<double>()), std::length_error);

    // Test data size mismatch
    std::vector<double> large_data = {1.0, 2.0, 3.0, 4.0, 5.0};
    EXPECT_THROW(array.Segment(0, 1, large_data), std::length_error);
}

/*
    Test invalid indices in element access.
*/
TEST(VariableEdgeCases, InvalidIndices)
{
    Variable array = Variable(kInput, {2, 2});
    std::vector<double> data = {1.0, 2.0, 3.0, 4.0};
    array.Segment(0, 3, data);

    // Test negative index
    EXPECT_THROW(array(-1), std::out_of_range);

    // Test index beyond array size
    EXPECT_THROW(array(4), std::out_of_range);
    EXPECT_THROW(array(5), std::out_of_range);

    // Test zero-sized array
    Variable empty_array = Variable(kInput, {0});
    EXPECT_THROW(empty_array(0), std::out_of_range);
}

/*
    Test edge cases for chunking operations.
*/
TEST(VariableEdgeCases, ChunkingEdgeCases)
{
    // Test empty array chunking
    Variable empty_array = Variable(kInput, {0});
    EXPECT_THROW(empty_array.CreateChunk(0, 0), std::out_of_range);

    // Test invalid chunk indices
    Variable array = Variable(kInput, {4});
    std::vector<double> data = {1.0, 2.0, 3.0, 4.0};
    array.Segment(0, 3, data);

    EXPECT_THROW(array.CreateChunk(-1, 2), std::invalid_argument);
    EXPECT_THROW(array.CreateChunk(0, 5), std::out_of_range);
    EXPECT_THROW(array.CreateChunk(3, 2), std::invalid_argument);

    // Test invalid chunk assignment
    Array invalid_chunk;
    invalid_chunk.set_start(-1);
    invalid_chunk.set_end(2);
    EXPECT_THROW(array.AssignChunk(invalid_chunk), std::invalid_argument);

    invalid_chunk.set_start(0);
    invalid_chunk.set_end(5);
    EXPECT_THROW(array.AssignChunk(invalid_chunk), std::out_of_range);
}

/*
    Test edge cases for metadata operations.
*/
TEST(VariableEdgeCases, MetadataEdgeCases)
{
    // Test empty metadata
    philote::VariableMetaData empty_meta;
    EXPECT_NO_THROW(Variable(empty_meta));

    // Test metadata with zero dimensions
    philote::VariableMetaData zero_dim_meta;
    zero_dim_meta.set_type(philote::kInput);
    zero_dim_meta.add_shape(0);
    EXPECT_NO_THROW(Variable(zero_dim_meta));

    // Test metadata with negative dimensions
    philote::VariableMetaData neg_dim_meta;
    neg_dim_meta.set_type(philote::kInput);
    neg_dim_meta.add_shape(-1);
    EXPECT_NO_THROW(Variable(neg_dim_meta));
}