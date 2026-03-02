/**
 * @file test_example.cpp
 * @brief Catch2 unit tests for example C++ functions.
 */

#include <catch2/catch_test_macros.hpp>

#include "../include/example.hpp"

TEST_CASE("add positive numbers", "[add]") {
    REQUIRE(add(2, 3) == 5);
}

TEST_CASE("add negative numbers", "[add]") {
    REQUIRE(add(-1, -2) == -3);
}

TEST_CASE("add zeros", "[add]") {
    REQUIRE(add(0, 0) == 0);
}

TEST_CASE("add mixed sign numbers", "[add]") {
    REQUIRE(add(5, -3) == 2);
}
