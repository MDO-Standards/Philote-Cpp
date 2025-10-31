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
#include "variable.h"

using grpc::ClientReaderWriter;
using grpc::ClientReaderWriterInterface;
using grpc::ServerReaderWriter;
using grpc::ServerReaderWriterInterface;
using std::map;
using std::shared_ptr;
using std::string;
using std::vector;

using namespace philote;

Variable::Variable(const philote::VariableType &type,
                   const std::vector<size_t> &shape)
{
    type_ = type;
    shape_ = shape;

    // serialized array size
    size_t size = 1;
    for (unsigned long i : shape_)
        size *= i;

    // initialize the array from the shape input
    data_.resize(size);
}

Variable::Variable(const philote::VariableMetaData &meta)
{
    for (auto &val : meta.shape())
        shape_.push_back(val);

    type_ = meta.type();

    // serialized array size
    size_t size = 1;
    for (unsigned long i : shape_)
        size *= i;

    // initialize the array from the shape input
    data_.resize(size);
}

Variable::Variable(const philote::PartialsMetaData &meta)
{
    for (auto &val : meta.shape())
        shape_.push_back(val);

    type_ = kPartial;

    // serialized array size
    size_t size = 1;
    for (unsigned long i : shape_)
        size *= i;

    // initialize the array from the shape input
    data_.resize(size);
}

void Variable::Segment(const size_t &start, const size_t &end,
                       const std::vector<double> &data)
{
    if (start > end)
        throw std::invalid_argument("Start index greater than end index in Variable::Segment");
    if (end >= data_.size())
        throw std::out_of_range("End index out of range in Variable::Segment");
    if (data.size() == 0 && start > end)
        return; // allow empty segment for start > end as a no-op
    if ((end - start) + 1 != data.size())
    {
        std::string expected = std::to_string((end - start) + 1);
        std::string actual = std::to_string(data.size());
        throw std::length_error("Vector data has incompatable length. Should be " +
                                expected + ", but received " + actual + ".");
    }
    for (size_t i = 0; i < (end - start) + 1; i++)
        data_.at(start + i) = data[i];
}

std::vector<double> Variable::Segment(const size_t &start, const size_t &end) const
{
    if (start > end)
        throw std::invalid_argument("Start index greater than end index in Variable::Segment getter");
    if (end >= data_.size())
        throw std::out_of_range("End index out of range in Variable::Segment getter");
    std::vector<double> data(end - start + 1);
    for (size_t i = 0; i < (end - start) + 1; i++)
        data[i] = data_.at(start + i);
    return data;
}

std::vector<size_t> Variable::Shape() const noexcept
{
    return shape_;
}

size_t Variable::Size() const noexcept
{
    return data_.size();
}

double Variable::operator()(const size_t &i) const
{
    if (i >= data_.size())
        throw std::out_of_range("Index out of range in Variable::operator() const");
    return data_.at(i);
}

double &Variable::operator()(const size_t &i)
{
    if (i >= data_.size())
        throw std::out_of_range("Index out of range in Variable::operator() non-const");
    return data_.at(i);
}

Array Variable::CreateChunk(const size_t &start, const size_t &end) const
{
    philote::Array out;

    out.set_start(start);
    out.set_end(end);
    out.set_type(type_);  // Set the variable type

    // set the data
    const vector<double> segment = Segment(start, end);
    for (const double value : segment)
        out.add_data(value);

    return out;
}

void Variable::Send(string name,
                    string subname,
                    ClientReaderWriter<Array, Array> *stream,
                    const size_t &chunk_size) const
{
    Array array;

    size_t start, end;
    size_t n = Size();

    // find the chunk indices and create the chunk
    size_t num_chunks = n / chunk_size;
    if (num_chunks == 0)
        num_chunks = 1;

    for (size_t i = 0; i < num_chunks; i++)
    {
        start = i * chunk_size;
        end = start + chunk_size - 1;  // end is inclusive
        if (end >= n)
            end = n - 1;

        array = CreateChunk(start, end);
        array.set_name(name);
        array.set_subname(subname);
        if (!stream->Write(array))
        {
            throw std::runtime_error(
                "Failed to write variable '" + name +
                "' to stream (chunk " + std::to_string(i + 1) +
                " of " + std::to_string(num_chunks) + ")");
        }
    }
}

void philote::Variable::Send(std::string name,
                             std::string subname,
                             grpc::ServerReaderWriterInterface<::philote::Array, ::philote::Array> *stream,
                             const size_t &chunk_size,
                             grpc::ServerContext* context) const
{
    Array array;
    size_t start = 0, end;
    size_t n = Size();
    size_t num_chunks = n / chunk_size;
    if (num_chunks == 0)
        num_chunks = 1;
    for (size_t i = 0; i < num_chunks; i++)
    {
        // Check for cancellation before processing each chunk
        if (context != nullptr && context->IsCancelled())
        {
            throw std::runtime_error(
                "Operation cancelled while sending variable '" + name +
                "' (chunk " + std::to_string(i + 1) +
                " of " + std::to_string(num_chunks) + ")");
        }

        start = i * chunk_size;
        end = start + chunk_size - 1;  // end is inclusive
        if (end >= n)
            end = n - 1;
        array = CreateChunk(start, end);
        array.set_name(name);
        array.set_subname(subname);
        if (!stream->Write(array))
        {
            throw std::runtime_error(
                "Failed to write variable '" + name +
                "' to stream (chunk " + std::to_string(i + 1) +
                " of " + std::to_string(num_chunks) + ")");
        }
    }
}

void philote::Variable::Send(std::string name,
                             std::string subname,
                             grpc::ClientReaderWriterInterface<::philote::Array, ::philote::Array> *stream,
                             const size_t &chunk_size) const
{
    Array array;
    size_t start = 0, end;
    size_t n = Size();
    size_t num_chunks = n / chunk_size;
    if (num_chunks == 0)
        num_chunks = 1;
    for (size_t i = 0; i < num_chunks; i++)
    {
        start = i * chunk_size;
        end = start + chunk_size - 1;  // end is inclusive
        if (end >= n)
            end = n - 1;
        array = CreateChunk(start, end);
        array.set_name(name);
        array.set_subname(subname);
        if (!stream->Write(array))
        {
            throw std::runtime_error(
                "Failed to write variable '" + name +
                "' to stream (chunk " + std::to_string(i + 1) +
                " of " + std::to_string(num_chunks) + ")");
        }
    }
}

void Variable::AssignChunk(const Array &data)
{
    // Validate indices are non-negative before casting to size_t
    // This prevents integer overflow attacks where negative values wrap to SIZE_MAX
    if (data.start() < 0)
        throw std::invalid_argument("Start index cannot be negative in Variable::AssignChunk");
    if (data.end() < 0)
        throw std::invalid_argument("End index cannot be negative in Variable::AssignChunk");

    size_t start = static_cast<size_t>(data.start());
    size_t end = static_cast<size_t>(data.end());

    if (start > end)
        throw std::invalid_argument("Start index greater than end index in Variable::AssignChunk");
    if (end >= data_.size())
        throw std::out_of_range("End index out of range in Variable::AssignChunk");
    if (data.data_size() != static_cast<int>((end - start + 1)))
        throw std::length_error("Chunk data size does not match the specified range in Variable::AssignChunk");

    for (size_t i = 0; i < end - start + 1; i++)
        data_[start + i] = data.data(i);
}