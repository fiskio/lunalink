#include "lunalink/signal/safety.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <vector>

using namespace lunalink::signal;

TEST_CASE("CheckedRange: signed types") {
    CheckedRange<int, -10, 10> r(0);
    CHECK(r.value() == 0);

    r = -20;
    CHECK(r.value() == -10);

    r = 20;
    CHECK(r.value() == 10);
    
    r = -10;
    CHECK(r.value() == -10);
    r = 10;
    CHECK(r.value() == 10);
}

TEST_CASE("CheckedRange: assignment saturation branches") {
    CheckedRange<int, 10, 20> r(15);
    r = 5;
    CHECK(r.value() == 10);
    r = 25;
    CHECK(r.value() == 20);
    r = 15;
    CHECK(r.value() == 15);
}

TEST_CASE("secure_scrub: template variants") {
    std::array<uint32_t, 4> data32 = {1, 2, 3, 4};
    secure_scrub(data32);
    for (auto v : data32) CHECK(v == 0);
    
    std::array<int64_t, 2> data64 = {1, 2};
    secure_scrub(data64);
    for (auto v : data64) CHECK(v == 0);

    std::vector<float> data_flt = {1.0f, 2.0f};
    secure_scrub(std::span(data_flt));
    for (auto v : data_flt) CHECK(v == 0.0f);
    
    std::span<uint8_t> empty;
    secure_scrub(empty);
    SUCCEED("Empty scrub handled");
}

struct CustomType {
    int a;
    int b;
    bool operator==(const CustomType& other) const = default;
};

TEST_CASE("TmrValue: complex types") {
    CustomType val{1, 2};
    TmrValue<CustomType> tmr(val);
    CHECK(tmr.vote() == val);
    tmr.v1 = {9, 9};
    CHECK(tmr.vote() == val);
    CHECK(tmr.v1 == val);
}

TEST_CASE("CheckedRange: boundary constructors") {
    CheckedRange<uint8_t, 10, 10> r_min(5);
    CHECK(r_min.value() == 10);
    CheckedRange<uint8_t, 10, 10> r_max(15);
    CHECK(r_max.value() == 10);
}
