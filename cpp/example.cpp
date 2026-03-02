/**
 * @file example.cpp
 * @brief Example C++ implementation with pybind11 bindings.
 */

#include <pybind11/pybind11.h>

namespace py = pybind11;

/**
 * Add two integers.
 *
 * @param a First integer.
 * @param b Second integer.
 * @return Sum of a and b.
 */
int add(int a, int b) {
    return a + b;
}

PYBIND11_MODULE(_example, m) {
    m.doc() = "Example C++ extension module.";
    m.def("add", &add, "Add two integers.", py::arg("a"), py::arg("b"));
}
