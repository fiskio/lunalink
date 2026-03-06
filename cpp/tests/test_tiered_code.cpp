#include "lunalink/signal/prn.hpp"
#include "lunalink/signal/tiered_code.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>

using namespace lunalink::signal;

TEST_CASE("secondary_code_index cycles through S0-S3") {
  REQUIRE(secondary_code_index(1) == 0);  // S0
  REQUIRE(secondary_code_index(2) == 1);  // S1
  REQUIRE(secondary_code_index(3) == 2);  // S2
  REQUIRE(secondary_code_index(4) == 3);  // S3
  REQUIRE(secondary_code_index(5) == 0);  // S0 again
  REQUIRE(secondary_code_index(12) == 3); // last in Table 11
}

TEST_CASE("secondary codes match spec Table 10") {
  // S0 = 1110, S1 = 0111, S2 = 1011, S3 = 1101
  const uint8_t s0[4] = {1, 1, 1, 0};
  const uint8_t s1[4] = {0, 1, 1, 1};
  const uint8_t s2[4] = {1, 0, 1, 1};
  const uint8_t s3[4] = {1, 1, 0, 1};
  for (int i = 0; i < 4; ++i) {
    REQUIRE(kSecondaryCodes[0][i] == s0[i]);
    REQUIRE(kSecondaryCodes[1][i] == s1[i]);
    REQUIRE(kSecondaryCodes[2][i] == s2[i]);
    REQUIRE(kSecondaryCodes[3][i] == s3[i]);
  }
}

TEST_CASE("tiered_code_epoch produces binary chip values") {
  std::array<uint8_t, kWeil10230ChipLength> out{};
  tiered_code_epoch(1, 0, out.data());
  for (auto v : out)
    REQUIRE(v <= 1u);
}

TEST_CASE("tiered_code_epoch equals primary XOR secondary XOR tertiary") {
  // Manually compute expected result for PRN 1, epoch 0.
  // PRN 1 → S0 = {1, 1, 1, 0}; epoch 0 → secondary chip = S0[0] = 1
  // epoch 0 → tertiary chip index = 0/4 = 0
  const auto *primary_packed = weil10230_prn_packed(1);
  const auto *tertiary_packed = weil1500_prn_packed(1);

  const uint8_t sec_chip = kSecondaryCodes[0][0]; // S0[0] = 1
  const uint8_t tert_chip = unpack_chip(tertiary_packed, 0);

  std::array<uint8_t, kWeil10230ChipLength> expected{};
  for (uint16_t i = 0; i < kWeil10230ChipLength; ++i)
    expected[i] =
        static_cast<uint8_t>(unpack_chip(primary_packed, i) ^ sec_chip ^ tert_chip);

  std::array<uint8_t, kWeil10230ChipLength> out{};
  tiered_code_epoch(1, 0, out.data());
  REQUIRE(out == expected);
}

TEST_CASE("tiered_code_epoch differs across secondary code boundary") {
  // Epoch 0 and epoch 1 have different secondary chips (S0[0]=1, S0[1]=1)
  // unless tertiary also flips. But epoch 3 has S0[3]=0, so should differ.
  std::array<uint8_t, kWeil10230ChipLength> e0{};
  std::array<uint8_t, kWeil10230ChipLength> e3{};
  tiered_code_epoch(1, 0, e0.data());
  tiered_code_epoch(1, 3, e3.data());

  // S0 = {1,1,1,0}: epoch 0 sec=1, epoch 3 sec=0 → modifier differs
  // (assuming same tertiary chip since both are in tertiary chip 0)
  // so the two epochs should be bitwise complements
  bool all_flipped = true;
  for (uint16_t i = 0; i < kWeil10230ChipLength; ++i) {
    if (e0[i] == e3[i]) {
      all_flipped = false;
      break;
    }
  }
  REQUIRE(all_flipped);
}

TEST_CASE("tiered_code_epoch changes across tertiary chip boundary") {
  // Epochs 0-3 share tertiary chip 0; epoch 4 uses tertiary chip 1.
  // If tertiary chips 0 and 1 differ, epoch 3 and 4 will differ even
  // though their secondary chips are the same (S0[3]=0, S0[0]=1 — wait,
  // those differ too). Use epochs 0 and 4 which have the same secondary
  // chip (S0[0]=1) but different tertiary chips.
  std::array<uint8_t, kWeil10230ChipLength> e0{};
  std::array<uint8_t, kWeil10230ChipLength> e4{};
  tiered_code_epoch(1, 0, e0.data());
  tiered_code_epoch(1, 4, e4.data());

  // Same secondary chip (S0[0]=1 for both), so any difference must come
  // from the tertiary code (chip 0 vs chip 1).
  const auto *tert = weil1500_prn_packed(1);
  if (unpack_chip(tert, 0) != unpack_chip(tert, 1)) {
    // Tertiary chips differ → outputs must differ
    REQUIRE(e0 != e4);
  }
  // If tertiary chips happen to be equal, outputs would match — still valid.
}

TEST_CASE("tiered_code_epoch different PRNs produce different sequences") {
  std::array<uint8_t, kWeil10230ChipLength> out1{};
  std::array<uint8_t, kWeil10230ChipLength> out2{};
  tiered_code_epoch(1, 0, out1.data());
  tiered_code_epoch(2, 0, out2.data());
  REQUIRE(out1 != out2);
}

TEST_CASE("kEpochsPerFrame is 6000") {
  REQUIRE(kEpochsPerFrame == 6000);
}

TEST_CASE("tiered_code_epoch_checked rejects invalid inputs") {
  std::array<uint8_t, kWeil10230ChipLength> out{};
  auto a = default_tiered_assignment(1);

  REQUIRE_FALSE(tiered_code_epoch_checked(a, kEpochsPerFrame, out.data()));
  REQUIRE_FALSE(tiered_code_epoch_checked(a, 0, nullptr));

  a.secondary_code_idx = 4;
  REQUIRE_FALSE(tiered_code_epoch_checked(a, 0, out.data()));
}

TEST_CASE("tiered_code_epoch_checked supports tertiary phase offset") {
  TieredCodeAssignment a0 = default_tiered_assignment(1);
  TieredCodeAssignment a1 = a0;
  a1.tertiary_phase_offset = 1;

  std::array<uint8_t, kWeil10230ChipLength> out0{};
  std::array<uint8_t, kWeil10230ChipLength> out1{};
  REQUIRE(tiered_code_epoch_checked(a0, 0, out0.data()));
  REQUIRE(tiered_code_epoch_checked(a1, 0, out1.data()));

  const auto *tert = weil1500_prn_packed(1);
  if (unpack_chip(tert, 0) != unpack_chip(tert, 1)) {
    REQUIRE(out0 != out1);
  }
}
