# generate gRPC stubs
#--------------------

# Set the path to the directory where you want to generate the C++ headers
# Generated files go in the build directory, not the source directory
set(GENERATED_DIR ${CMAKE_BINARY_DIR}/src/generated)

# Create the generated directory if it doesn't exist
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Set the path to the directory containing the .proto files
set(PROTO_DIR ${CMAKE_SOURCE_DIR}/proto)

# Set the paths to your .proto files
set(PROTO_FILES data.proto
                disciplines.proto)


# Combine the two lists element-wise
set(PROTO_FILES_DEPEND "")

foreach(elem ${PROTO_FILES})
    string(REPLACE ".proto" "" file_no_ext ${elem})
    list(APPEND PROTO_FILES_DEPEND ${CMAKE_SOURCE_DIR}/proto/${elem})
    list(APPEND PROTO_HEADERS ${GENERATED_DIR}/${file_no_ext}.pb.h)
    list(APPEND GRPC_HEADERS ${GENERATED_DIR}/${file_no_ext}.grpc.pb.h)
    list(APPEND GRPC_MOCK_HEADERS ${GENERATED_DIR}/${file_no_ext}_mock.grpc.pb.h)
    list(APPEND PROTO_SRC ${GENERATED_DIR}/${file_no_ext}.pb.cc)
    list(APPEND GRPC_SRC ${GENERATED_DIR}/${file_no_ext}.grpc.pb.cc)
endforeach()

# Add a custom command to generate the C++ headers and sources from the .proto files
# This command only runs when the proto files are modified (incremental builds)
add_custom_command(
    OUTPUT ${PROTO_HEADERS} ${GRPC_HEADERS} ${GRPC_MOCK_HEADERS} ${PROTO_SRC} ${GRPC_SRC}
    COMMAND protobuf::protoc
        --proto_path=${PROTO_DIR}
        --grpc_out=generate_mock_code=true:${GENERATED_DIR}
        --cpp_out=${GENERATED_DIR}
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
        ${PROTO_FILES}
    DEPENDS ${PROTO_FILES_DEPEND}
    COMMENT "Generating C++ stubs from protobuf files"
)

# Create a custom target that depends on all generated files
# This ensures the generation happens before any target that depends on GrpcGenerated
add_custom_target(GenerateGrpcFiles
    DEPENDS ${PROTO_HEADERS} ${GRPC_HEADERS} ${GRPC_MOCK_HEADERS} ${PROTO_SRC} ${GRPC_SRC}
)

install(FILES ${PROTO_FILES_DEPEND}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/philote/proto
)
install(FILES ${PROTO_HEADERS} ${GRPC_HEADERS} ${GRPC_MOCK_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/philote
)

# Create the GrpcGenerated library using the generated sources
add_library(GrpcGenerated OBJECT ${PROTO_SRC} ${GRPC_SRC})

# Ensure the library depends on the generation target
add_dependencies(GrpcGenerated GenerateGrpcFiles)

target_include_directories(GrpcGenerated
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${GENERATED_DIR}
)
target_link_libraries(GrpcGenerated PRIVATE protobuf::libprotobuf gRPC::grpc++)
enable_coverage(GrpcGenerated)
