/**
 * @file example.cpp
 * @brief pybind11 bindings for the example module.
 *
 * The binding layer translates the C++ overflow-safe API (bool + out-param)
 * into a conventional Python function that raises OverflowError on failure.
 * pybind11 automatically converts std::overflow_error to Python OverflowError.
 */

#include <pybind11/pybind11.h>
#include <stdexcept>

#include "example.hpp"

namespace py = pybind11;

PYBIND11_MODULE(_afs, m) {
    m.doc() = "Example C++ extension module.";
    m.def(
        "add",
        [](int32_t a, int32_t b) -> int32_t {
            int32_t result{};
            if (!add(a, b, result)) {
                throw std::overflow_error("integer overflow in add");
            }
            return result;
        },
        "Add two 32-bit integers. Raises OverflowError on overflow.",
        py::arg("a"),
        py::arg("b")
    );
}
