FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-22.04

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="3.22.2"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
    chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh


# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends meson ninja-build clang libclang-dev pkg-config

WORKDIR /tmp
RUN git clone --recurse-submodules -b v1.56.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
RUN git clone https://github.com/google/googletest.git

WORKDIR /tmp/grpc/cmake/build
RUN cmake -DgRPC_INSTALL=ON \
    -DgRPC_BUILD_TESTS=OFF \
    -DCMAKE_INSTALL_PREFIX=/grpc \
    ../..
RUN make -j `nproc` install

WORKDIR /tmp/googletest/build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/gtest ..
RUN make -j `nproc` install

WORKDIR /grpc/bin
RUN ln -s grpc_cpp_plugin protoc-gen-grpc
ENV PATH "$PATH:/grpc:/grpc/bin:/gtest"

WORKDIR /tmp
RUN git clone https://github.com/hdoc/hdoc.git
WORKDIR /tmp/hdoc
RUN meson --prefix /hdoc build; ninja -C build
RUN mkdir /hdoc; cp build/hdoc /hdoc/hdoc; cp build/hdoc /hdoc/hdoc

# remove the temporary files
WORKDIR /
RUN rm -rf /tmp