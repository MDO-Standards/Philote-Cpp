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
#pragma once

#include <string>
#include <map>
#include <vector>

#include <data.pb.h>
#include <disciplines.grpc.pb.h>

namespace philote
{
    /**
     * @brief A class for storing continuous and discrete variables
     *
     * The Variable class provides a container for multi-dimensional arrays used
     * in Philote disciplines. Variables can represent inputs, outputs, or residuals
     * and support operations like segmentation, streaming over gRPC, and element access.
     *
     * @par Example: Creating and Using Variables
     * @code
     * // Create a variable with shape
     * philote::Variable pressure(philote::kInput, {10, 10});
     *
     * // Set values
     * for (size_t i = 0; i < 100; ++i) {
     *     pressure(i) = 101325.0 + i * 10.0;
     * }
     *
     * // Get shape information
     * std::vector<size_t> shape = pressure.Shape();  // Returns {10, 10}
     * size_t total = pressure.Size();                // Returns 100
     *
     * // Access individual elements
     * double val = pressure(0);  // Get first element
     * @endcode
     *
     * @par Example: Creating from Metadata
     * @code
     * philote::VariableMetaData meta;
     * meta.set_name("temperature");
     * meta.add_shape(5);
     * meta.set_units("K");
     * meta.set_type(philote::kOutput);
     *
     * philote::Variable temp(meta);
     * temp(0) = 300.0;
     * @endcode
     *
     * @note Thread Safety: This class is NOT thread-safe. Concurrent reads and writes
     * to the same Variable instance will cause data races. If multiple threads need to
     * access the same Variable, external synchronization is required. Each thread should
     * preferably have its own Variable instances.
     */
    class Variable
    {
    public:
        /**
         * @brief Construct a new Variables object
         *
         */
        Variable() = default;

        /**
         * @brief Construct a new Variable object
         *
         * @param type
         * @param shape
         */
        Variable(const philote::VariableType &type,
                 const std::vector<size_t> &shape);

        /**
         * @brief Construct a new Variable object
         *
         * @param meta
         */
        explicit Variable(const philote::VariableMetaData &meta);

        /**
         * @brief Construct a new Variable object
         *
         * @param meta
         */
        explicit Variable(const philote::PartialsMetaData &meta);

        /**
         * @brief Destroy the Variables object
         *
         */
        ~Variable() = default;

        /**
         * @brief Assigns a segment of the array given a subvector
         *
         * @param start starting index of the segment
         * @param end ending index of the segment
         * @param data data to be assigned to the segment
         *
         * @par Example
         * @code
         * philote::Variable vec(philote::kOutput, {100});
         *
         * // Set a segment from indices 10 to 14 (inclusive)
         * std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};
         * vec.Segment(10, 15, data);
         * @endcode
         */
        void Segment(const size_t &start, const size_t &end,
                     const std::vector<double> &data);

        /**
         * @brief Retrieves a reference to a segment of the array given a
         * subvector
         *
         * @param start
         * @param end
         * @return std::vector<Type>&
         */
        std::vector<double> Segment(const size_t &start, const size_t &end) const;

        /**
         * @brief Returns the shape of the array
         *
         * @return std::vector<size_t> vector containing the length of the
         * individual dimensions
         */
        std::vector<size_t> Shape() const;

        /**
         * @brief Returns the size of the array.
         *
         * @return size_t size of the array
         */
        size_t Size() const;

        /**
         * @brief Returns the value of the array at a given index
         *
         * @param indices indices at which the array should be accessed
         * @return double value of the array at the given indices
         */
        double operator()(const size_t &i) const;

        /**
         * @brief Returns the value of the array at a given index
         *
         * @param indices indices at which the array should be accessed
         * @return double value of the array at the given indices
         */
        double &operator()(const size_t &i);

        /**
         * @brief Create a Chunk of the variable
         *
         * @param start
         * @param end
         * @return philote::Array
         */
        philote::Array CreateChunk(const size_t &start, const size_t &end) const;

        /**
         * @brief Sends the variable from the client to the server
         *
         * @param stream
         */
        void Send(std::string name,
                  std::string subname,
                  grpc::ClientReaderWriter<::philote::Array, ::philote::Array> *stream,
                  const size_t &chunk_size) const;

        /**
         * @brief Sends the variable from the server to the client using the interface
         *
         * @param stream
         */
        void Send(std::string name,
                  std::string subname,
                  grpc::ServerReaderWriterInterface<::philote::Array, ::philote::Array> *stream,
                  const size_t &chunk_size) const;

        /**
         * @brief Sends the variable from the client to the server using the interface
         *
         * @param stream
         */
        void Send(std::string name,
                  std::string subname,
                  grpc::ClientReaderWriterInterface<::philote::Array, ::philote::Array> *stream,
                  const size_t &chunk_size) const;

        /**
         * @brief Assigns a chunk to the variable
         *
         * @param stream
         * @param chunk_size
         */
        void AssignChunk(const Array &data);

    private:
        //! variable type
        philote::VariableType type_;

        //! array shape
        std::vector<size_t> shape_;

        //! raw data (serialized, row major)
        std::vector<double> data_;

        //! raw discrete data (serialized, row major)
        std::vector<int64_t> discrete_data_;
    };

    /**
     * @brief Dictionary for storing values with respect to two keys
     *
     * Provides a cleaner interface for accessing values using two string keys,
     * similar to Philote-Python's PairDict class. This is particularly useful for
     * storing Jacobian/partial derivative values.
     *
     * @tparam T The value type to store
     *
     * @par Example: Using PairDict for Gradients
     * @code
     * philote::PairDict<philote::Variable> gradients;
     *
     * // Create gradient variables
     * gradients("f", "x") = philote::Variable(philote::kOutput, {1});
     * gradients("f", "y") = philote::Variable(philote::kOutput, {1});
     *
     * // Set gradient values
     * gradients("f", "x")(0) = 2.5;
     * gradients("f", "y")(0) = 3.7;
     *
     * // Check if a gradient exists
     * if (gradients.contains("f", "x")) {
     *     double df_dx = gradients("f", "x")(0);
     * }
     * @endcode
     *
     * @par Example: Traditional Partials vs PairDict
     * @code
     * // Traditional approach with std::map
     * philote::Partials partials;
     * partials[std::make_pair("output", "input")](0) = 1.5;
     *
     * // Cleaner PairDict approach
     * philote::PartialsPairDict pair_partials;
     * pair_partials("output", "input")(0) = 1.5;
     * @endcode
     *
     * @note Thread Safety: This class is NOT thread-safe. The underlying std::map
     * does not provide thread safety for concurrent modifications. If multiple threads
     * need to access the same PairDict, external synchronization is required.
     */
    template<typename T>
    class PairDict
    {
    public:
        /**
         * @brief Default constructor
         */
        PairDict() = default;

        /**
         * @brief Access/modify value using two keys
         *
         * @param key1 First key
         * @param key2 Second key
         * @return Reference to the value
         */
        T& operator()(const std::string& key1, const std::string& key2)
        {
            return data_[std::make_pair(key1, key2)];
        }

        /**
         * @brief Access value using two keys (const version)
         *
         * @param key1 First key
         * @param key2 Second key
         * @return Const reference to the value
         */
        const T& operator()(const std::string& key1, const std::string& key2) const
        {
            return data_.at(std::make_pair(key1, key2));
        }

        /**
         * @brief Check if a key pair exists
         *
         * @param key1 First key
         * @param key2 Second key
         * @return true if the key pair exists
         */
        bool contains(const std::string& key1, const std::string& key2) const
        {
            return data_.find(std::make_pair(key1, key2)) != data_.end();
        }

        /**
         * @brief Get the size of the dictionary
         *
         * @return Number of key-value pairs
         */
        size_t size() const
        {
            return data_.size();
        }

        /**
         * @brief Check if the dictionary is empty
         *
         * @return true if empty
         */
        bool empty() const
        {
            return data_.empty();
        }

        /**
         * @brief Clear all entries
         */
        void clear()
        {
            data_.clear();
        }

        /**
         * @brief Get iterator to beginning
         */
        auto begin() { return data_.begin(); }
        auto begin() const { return data_.begin(); }

        /**
         * @brief Get iterator to end
         */
        auto end() { return data_.end(); }
        auto end() const { return data_.end(); }

    private:
        std::map<std::pair<std::string, std::string>, T> data_;
    };

    typedef std::map<std::string, philote::Variable> Variables;
    typedef std::map<std::pair<std::string, std::string>, philote::Variable> Partials;
    typedef PairDict<philote::Variable> PartialsPairDict;
}