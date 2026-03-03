/**
 * @file example.hpp
 * @brief Example C++ function declarations.
 *
 * All arithmetic uses explicit fixed-width types and checked overflow
 * detection to eliminate undefined behaviour on space-grade targets.
 */

#pragma once

#include <climits>
#include <cstdint>

static_assert(CHAR_BIT == 8, "char must be 8 bits");
static_assert(sizeof(int32_t) == 4, "int32_t must be exactly 32 bits");

/**
 * Add two 32-bit integers with overflow detection.
 *
 * Uses __builtin_add_overflow for zero-cost hardware overflow detection
 * on GCC and Clang targets (maps to a single ADC/ADDO instruction).
 * The function is noexcept and free of side effects.
 *
 * @param a      First operand.
 * @param b      Second operand.
 * @param result Output parameter. Set to the exact sum when the function
 *               returns true. Value is unspecified when the function
 *               returns false.
 * @return true  if the addition succeeded without overflow.
 *         false if signed 32-bit overflow was detected.
 */
[[nodiscard]] bool add(int32_t a, int32_t b, int32_t& result) noexcept;
