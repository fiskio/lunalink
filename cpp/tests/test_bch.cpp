#include "lunalink/signal/bch.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

using namespace lunalink::signal;

// ---------------------------------------------------------------------------
// Spec test vector — Figure 8 of LSIS V1.0 §2.4.2.1
// SB1 data = 0x045 (FID=0, TOI=69), encoded field = 0x229f61dbb84a0
// ---------------------------------------------------------------------------
TEST_CASE("bch_encode matches spec Figure 8 test vector") {
  // 0x229f61dbb84a0 as 52 individual bits (MSB first)
  constexpr std::array<uint8_t, kBchCodewordLength> expected = {
      0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1,
      1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1,
      1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
  };

  std::array<uint8_t, kBchCodewordLength> out{};
  const auto status = bch_encode(0, 69, out.data());

  REQUIRE(status == BchStatus::kOk);
  REQUIRE(out == expected);
}

TEST_CASE("bch_encode with bit0=1 XORs all LFSR outputs") {
  // Use FID=2 (bit0=1), TOI=69 — same 8 data bits as Figure 8 but bit0 flips.
  std::array<uint8_t, kBchCodewordLength> out_bit0_0{};
  std::array<uint8_t, kBchCodewordLength> out_bit0_1{};

  REQUIRE(bch_encode(0, 69, out_bit0_0.data()) == BchStatus::kOk);
  REQUIRE(bch_encode(2, 69, out_bit0_1.data()) == BchStatus::kOk);

  // Bit 0 should be raw MSB of FID.
  REQUIRE(out_bit0_0[0] == 0);
  REQUIRE(out_bit0_1[0] == 1);

  // All remaining 51 symbols should be flipped.
  for (uint8_t i = 1; i < kBchCodewordLength; ++i) {
    REQUIRE(out_bit0_1[i] == static_cast<uint8_t>(out_bit0_0[i] ^ 1U));
  }
}

TEST_CASE("bch_encode output values are binary") {
  std::array<uint8_t, kBchCodewordLength> out{};
  REQUIRE(bch_encode(0, 0, out.data()) == BchStatus::kOk);
  for (auto v : out) {
    REQUIRE(v <= 1U);
  }
}

TEST_CASE("bch_encode all-zero SB1") {
  // FID=0, TOI=0 → SB1 = 000000000, bit0=0, data=00000000
  // All-zero LFSR state → all-zero output.
  std::array<uint8_t, kBchCodewordLength> out{};
  REQUIRE(bch_encode(0, 0, out.data()) == BchStatus::kOk);
  for (auto v : out) {
    REQUIRE(v == 0U);
  }
}

TEST_CASE("bch_encode rejects null output") {
  REQUIRE(bch_encode(0, 0, nullptr) == BchStatus::kNullOutput);
}

TEST_CASE("bch_encode rejects invalid FID") {
  std::array<uint8_t, kBchCodewordLength> out{};
  REQUIRE(bch_encode(4, 0, out.data()) == BchStatus::kInvalidFid);
}

TEST_CASE("bch_encode rejects invalid TOI") {
  std::array<uint8_t, kBchCodewordLength> out{};
  REQUIRE(bch_encode(0, 100, out.data()) == BchStatus::kInvalidToi);
  REQUIRE(bch_encode(0, 127, out.data()) == BchStatus::kInvalidToi);
}

TEST_CASE("bch_encode boundary TOI=99 succeeds") {
  std::array<uint8_t, kBchCodewordLength> out{};
  REQUIRE(bch_encode(0, 99, out.data()) == BchStatus::kOk);
}

TEST_CASE("bch_encode different inputs produce different codewords") {
  std::array<uint8_t, kBchCodewordLength> out_a{};
  std::array<uint8_t, kBchCodewordLength> out_b{};
  REQUIRE(bch_encode(0, 1, out_a.data()) == BchStatus::kOk);
  REQUIRE(bch_encode(0, 2, out_b.data()) == BchStatus::kOk);
  REQUIRE(out_a != out_b);
}
