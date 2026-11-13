"""Bidirectional interoperability test between Philote-Cpp and Philote-Python."""

from __future__ import annotations

import argparse
from concurrent import futures
import os
import select
import socket
import subprocess
import sys
import time


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def wait_for_cpp_server(process: subprocess.Popen[str]) -> None:
    ready, _, _ = select.select([process.stdout], [], [], 10.0)
    if not ready or process.stdout.readline().strip() != "READY":
        raise RuntimeError("C++ interoperability server did not become ready")


def wait_for_channel(grpc, channel) -> None:
    grpc.channel_ready_future(channel).result(timeout=10.0)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--python-source", required=True)
    parser.add_argument("--cpp-server", required=True)
    parser.add_argument("--cpp-client", required=True)
    args = parser.parse_args()

    sys.path.insert(0, os.path.abspath(args.python_source))
    try:
        import grpc
        import numpy as np
        from philote_mdo.general import ExplicitClient, ExplicitDiscipline, ExplicitServer
        import philote_mdo.generated.data_pb2 as data
    except ImportError as exc:
        print(f"SKIP: Philote-Python dependencies unavailable: {exc}")
        return 77

    # Python client -> C++ server.
    cpp_port = free_port()
    cpp_server = subprocess.Popen(
        [args.cpp_server, str(cpp_port)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    try:
        wait_for_cpp_server(cpp_server)
        channel = grpc.insecure_channel(f"127.0.0.1:{cpp_port}")
        wait_for_channel(grpc, channel)
        client = ExplicitClient(channel=channel)
        client.get_available_options()
        assert client.options_list == {"config": "dict", "label": "str"}
        client.run_setup()
        client.get_variable_definitions()
        client.send_variable_shapes(
            [
                client.set_variable_shape("x", (3,), data.kInput),
                client.set_variable_shape("y", (3,), data.kOutput),
            ]
        )
        client.get_partials_definitions()
        outputs, discrete = client.run_compute(
            {"x": np.array([1.0, 2.0, 4.0])},
            discrete_inputs={"mode": "triple"},
        )
        np.testing.assert_allclose(outputs["y"], [3.0, 6.0, 12.0])
        assert discrete["status"] == {"ok": True, "mode": "triple"}
        jac = client.run_compute_partials(
            {"x": np.array([1.0, 2.0, 4.0])},
            discrete_inputs={"mode": "triple"},
        )
        np.testing.assert_allclose(jac["y", "x"], 3.0 * np.eye(3))
    finally:
        cpp_server.terminate()
        try:
            cpp_server.wait(timeout=5.0)
        except subprocess.TimeoutExpired:
            cpp_server.kill()

    # C++ client -> Python server.
    class PythonInteropDiscipline(ExplicitDiscipline):
        def initialize(self):
            self.add_option("config", "dict")
            self.add_option("label", "str")

        def setup(self):
            self.add_input("x", dynamic_shape=True)
            self.add_output("y", dynamic_shape=True)
            self.add_discrete_input("mode")
            self.add_discrete_output("status")

        def setup_partials(self):
            self.declare_partials("y", "x")

        def compute(self, inputs, outputs, discrete_inputs, discrete_outputs):
            scale = 3.0 if discrete_inputs["mode"] == "triple" else 2.0
            outputs["y"] = scale * inputs["x"]
            discrete_outputs["status"] = {"ok": True, "mode": discrete_inputs["mode"]}

        def compute_partials(self, inputs, partials, discrete_inputs):
            scale = 3.0 if discrete_inputs["mode"] == "triple" else 2.0
            partials["y", "x"] = scale * np.eye(inputs["x"].size)

    py_port = free_port()
    py_server = grpc.server(futures.ThreadPoolExecutor(max_workers=4))
    service = ExplicitServer(discipline=PythonInteropDiscipline())
    service.attach_to_server(py_server)
    py_server.add_insecure_port(f"127.0.0.1:{py_port}")
    py_server.start()
    try:
        result = subprocess.run(
            [args.cpp_client, str(py_port)], capture_output=True, text=True, timeout=20.0
        )
        if result.returncode != 0:
            raise RuntimeError(
                f"C++ client failed ({result.returncode}): {result.stdout}\n{result.stderr}"
            )
    finally:
        py_server.stop(0).wait()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

