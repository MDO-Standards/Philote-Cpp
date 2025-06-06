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

#include <gtest/gtest.h>

#include <variable.h>
#include <disciplines.grpc.pb.h>

using std::vector;
using namespace philote;
// using grpc::testing::MockServerReaderWriter;
// using grpc::testing::MockClientReaderWriter;

/*
	Test the constructor.
*/
TEST(VariableTests, Constructor)
{
	// create a 3-dimensional array
	Variable array = Variable(kInput, {3, 4, 2});

	// check the shape of the array
	auto shape = array.Shape();

	// Expect equality.
	EXPECT_EQ(shape[0], 3);
	EXPECT_EQ(shape[1], 4);
	EXPECT_EQ(shape[2], 2);
}

/*
	Test the segment function.
*/
TEST(VariableTests, Segment)
{
	// create a 2-dimensional array
	Variable array = Variable(kInput, {2, 2});

	// assign some data
	std::vector<double> data = {1.0, 2.0, 3.0, 4.0};
	array.Segment(0, 3, data);

	// now replace values
	std::vector<double> data_seg = {1.0, 2.0, 3.0};
	array.Segment(1, 3, data_seg);

	// check the element (0,0)
	EXPECT_EQ(array(0), 1.0);

	// check the element (0,1)
	EXPECT_EQ(array(1), 1.0);

	// check the element (1,0)
	EXPECT_EQ(array(2), 2.0);

	// check the element (1,1)
	EXPECT_EQ(array(3), 3.0);

	// now replace only the last value (to see if single values can be set)
	data_seg = {1.0};
	array.Segment(3, 3, data_seg);

	// check the element (0,0)
	EXPECT_EQ(array(0), 1.0);

	// check the element (0,1)
	EXPECT_EQ(array(1), 1.0);

	// check the element (1,0)
	EXPECT_EQ(array(2), 2.0);

	// check the element (1,1)
	EXPECT_EQ(array(3), 1.0);
}

/*
	Test the size function.
*/
TEST(VariableTests, Size)
{
	// create a 3-dimensional array
	Variable array = Variable(kInput, {3, 4, 2});

	// check the shape of the array
	auto size = array.Size();

	EXPECT_EQ(size, 24);
}

/*
	Test the shape function.
*/
TEST(VariableTests, Shape)
{
	// create a 3-dimensional array
	Variable array = Variable(kInput, {3, 4, 2});

	// check the shape of the array
	auto shape = array.Shape();

	// Expect equality.
	EXPECT_EQ(shape[0], 3);
	EXPECT_EQ(shape[1], 4);
	EXPECT_EQ(shape[2], 2);
}

/*
	Test the element retrieval operator.
*/
TEST(VariableTests, ElementRetrieval)
{
	// create a 2-dimensional array
	Variable array = Variable(kInput, {2, 2});

	// assign some data
	std::vector<double> data = {1.0, 2.0, 3.0, 4.0};
	array.Segment(0, 3, data);

	// check the element (0,0)
	EXPECT_EQ(array(0), 1.0);

	// check the element (0,1)
	EXPECT_EQ(array(1), 2.0);

	// check the element (1,0)
	EXPECT_EQ(array(2), 3.0);

	// check the element (1,1)
	EXPECT_EQ(array(3), 4.0);
}

/*
	Test the element retrieval operator.
*/
TEST(VariableTests, Chunking)
{
	// create a 2-dimensional array
	Variable var = Variable(kInput, {4});

	// assign the test data
	std::vector<double> data = {1.0, 2.0, 3.0, 4.0};
	var.Segment(0, 3, data);

	// create the chunk
	Array chunk = var.CreateChunk(0, 2);

	// check the results
	EXPECT_EQ(chunk.data().at(0), 1.0);
	EXPECT_EQ(chunk.data().at(1), 2.0);
	EXPECT_EQ(chunk.data().at(2), 3.0);

	// check the chunk size
	EXPECT_EQ(chunk.data().size(), 3);
}

/*
	Test the element retrieval operator.
*/
TEST(VariableTests, AssignChunk)
{
	// create a 2-dimensional array
	Variable var = Variable(kInput, {4});

	// assign the test data
	std::vector<double> data = {1.0, 2.0, 3.0, 4.0};
	var.Segment(0, 3, data);

	// create the chunk
	vector<double> chunk_vector = {4.0, 3.0, 2.0};
	Array chunk;
	chunk.set_start(1);
	chunk.set_end(3);
	for (const double value : chunk_vector)
		chunk.add_data(value);
	var.AssignChunk(chunk);

	// check the results
	EXPECT_EQ(var(0), 1.0);
	EXPECT_EQ(var(1), 4.0);
	EXPECT_EQ(var(2), 3.0);
	EXPECT_EQ(var(3), 2.0);
}

/*
	Test the constructor with VariableMetaData.
*/
TEST(VariableTests, ConstructorWithMetaData)
{
	// Create metadata
	philote::VariableMetaData meta;
	meta.set_type(philote::kInput);
	meta.add_shape(3);
	meta.add_shape(2);

	// Create variable from metadata
	Variable var(meta);

	// Check shape
	auto shape = var.Shape();
	EXPECT_EQ(shape.size(), 2);
	EXPECT_EQ(shape[0], 3);
	EXPECT_EQ(shape[1], 2);

	// Check size
	EXPECT_EQ(var.Size(), 6);
}

/*
	Test the segment getter function.
*/
TEST(VariableTests, SegmentGetter)
{
	// create a 2-dimensional array
	Variable array = Variable(kInput, {2, 2});

	// assign some data
	std::vector<double> data = {1.0, 2.0, 3.0, 4.0};
	array.Segment(0, 3, data);

	// get a segment
	std::vector<double> segment = array.Segment(0, 2);

	// check the segment
	EXPECT_EQ(segment.size(), 3);
	EXPECT_EQ(segment[0], 1.0);
	EXPECT_EQ(segment[1], 2.0);
	EXPECT_EQ(segment[2], 3.0);
}

/*
	Test invalid indices in element access.
*/
TEST(VariableTests, InvalidIndices)
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
	Test edge cases for the segment function.
*/
TEST(VariableTests, SegmentEdgeCases)
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
	Test edge cases for chunking operations.
*/
TEST(VariableTests, ChunkingEdgeCases)
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
TEST(VariableTests, MetadataEdgeCases)
{
	// Test empty metadata
	philote::VariableMetaData empty_meta;
	EXPECT_NO_THROW((Variable(empty_meta)));

	// Test metadata with zero dimensions
	philote::VariableMetaData zero_dim_meta;
	zero_dim_meta.set_type(philote::kInput);
	zero_dim_meta.add_shape(0);
	EXPECT_NO_THROW((Variable(zero_dim_meta)));

	// Test metadata with negative dimensions
	philote::VariableMetaData neg_dim_meta;
	neg_dim_meta.set_type(philote::kInput);
	neg_dim_meta.add_shape(-1);
	EXPECT_THROW((Variable(neg_dim_meta)), std::length_error);
}