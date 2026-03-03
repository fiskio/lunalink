/**
 * @file example_core.cpp
 * @brief Implementation of example C++ functions.
 */

#include "example.hpp"

bool add(int32_t a, int32_t b, int32_t& result) noexcept {
    // __builtin_add_overflow returns true on overflow and is compiled to a
    // single hardware instruction (e.g. ADDO on SPARC, ADD + JO on x86).
    return !__builtin_add_overflow(a, b, &result);
}
