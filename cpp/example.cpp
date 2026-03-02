/**
 * @file example.cpp
 * @brief pybind11 bindings for the example module.
 */

#include <pybind11/pybind11.h>

#include "include/example.hpp"

namespace py = pybind11;

PYBIND11_MODULE(_example, m) {
    m.doc() = "Example C++ extension module.";
    m.def("add", &add, "Add two integers.", py::arg("a"), py::arg("b"));
}
