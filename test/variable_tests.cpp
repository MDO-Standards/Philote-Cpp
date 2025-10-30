/*
    Philote C++ Bindings

    Copyright 2022-2023 Christopher A. Lupp

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
#include <Philote/variable.h>

#include <gtest/gtest.h>
// #include <gmock/gmock.h>
// #include <grpcpp/test/mock_stream.h>

#include <disciplines.grpc.pb.h>

#include <iostream>
#include <vector>

using std::vector;

using namespace philote;

/*
	Test the constructor.
*/
TEST(VariableTests, Constructor)
{
	// create a 3-dimensional array
	Variable array = Variable(kInput, { 3, 4, 2 });

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
	Variable array = Variable(kInput, { 2, 2 });

	// assign some data
	std::vector<double> data = { 1.0, 2.0, 3.0, 4.0 };
	array.Segment(0, 3, data);

	// now replace values
	std::vector<double> data_seg = { 1.0, 2.0 , 3.0};
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
	data_seg = { 1.0 };
	array.Segment(3, 3, data_seg);

	// check the element (0,0)
	EXPECT_EQ(array(0), 1.0);

	// check the element (0,1)
	EXPECT_EQ(array(1), 1.0);

	// check the element (1,0)
	EXPECT_EQ(array(2), 2.0);

	// check the element (1,1)
	EXPECT_EQ(array(3), 1.0);

	// now replace only the last value (to see if single values can be set)
	data_seg = { 1.0 };
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
	Variable array = Variable(kInput, { 3, 4, 2 });

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
	Variable array = Variable(kInput, { 3, 4, 2 });

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
	Variable array = Variable(kInput, { 2, 2 });

	// assign some data
	std::vector<double> data = { 1.0, 2.0, 3.0, 4.0 };
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
	Variable var = Variable(kInput, { 4 });

	// assign the test data
	std::vector<double> data = { 1.0, 2.0, 3.0, 4.0 };
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
	Variable var = Variable(kInput, { 4 });

	// assign the test data
	std::vector<double> data = { 1.0, 2.0, 3.0, 4.0 };
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



// /*
// 	Test the element retrieval operator.
// */
// using MockClientReaderWriter = grpc::testing::MockClientReaderWriter<Array, Array>;

// TEST(VariableTest, SendTest) {
// 	// Create a mock gRPC client stream
// 	testing::NiceMock<MockClientReaderWriter> mockClient;

// 	// Create an instance of your Variable class
// 	Variable variable;

// 	// Set up your test data
// 	std::string name = "test_name";
// 	std::string subname = "test_subname";
// 	size_t chunk_size = 10;

// 	// Call the Send method with the mock gRPC client stream
// 	variable.Send(name, subname, &mockClient, chunk_size);

// 	// Add your assertions and expectations here to verify the behavior of the Send method
// }