/**
 * @file test_example.cpp
 * @brief Catch2 unit tests for example C++ functions.
 *
 * Tests cover normal operation and both overflow boundaries to ensure
 * the checked arithmetic implementation is correct.
 */

#include <climits>
#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "example.hpp"

TEST_CASE("add positive numbers", "[add]") {
  std::int32_t result{};
  REQUIRE(add(2, 3, result));
  REQUIRE(result == 5);
}

TEST_CASE("add negative numbers", "[add]") {
  std::int32_t result{};
  REQUIRE(add(-1, -2, result));
  REQUIRE(result == -3);
}

TEST_CASE("add zeros", "[add]") {
  std::int32_t result{};
  REQUIRE(add(0, 0, result));
  REQUIRE(result == 0);
}

TEST_CASE("add mixed sign numbers", "[add]") {
  std::int32_t result{};
  REQUIRE(add(5, -3, result));
  REQUIRE(result == 2);
}

TEST_CASE("add detects positive overflow", "[add][overflow]") {
  std::int32_t result{};
  REQUIRE_FALSE(add(INT32_MAX, 1, result));
  REQUIRE_FALSE(add(INT32_MAX, INT32_MAX, result));
}

TEST_CASE("add detects negative overflow", "[add][overflow]") {
  std::int32_t result{};
  REQUIRE_FALSE(add(INT32_MIN, -1, result));
  REQUIRE_FALSE(add(INT32_MIN, INT32_MIN, result));
}

TEST_CASE("add at exact boundaries does not overflow", "[add][overflow]") {
  std::int32_t result{};
  REQUIRE(add(INT32_MAX, 0, result));
  REQUIRE(result == INT32_MAX);
  REQUIRE(add(INT32_MIN, 0, result));
  REQUIRE(result == INT32_MIN);
  REQUIRE(add(INT32_MAX, -1, result));
  REQUIRE(result == INT32_MAX - 1);
  REQUIRE(add(INT32_MIN, 1, result));
  REQUIRE(result == INT32_MIN + 1);
}
