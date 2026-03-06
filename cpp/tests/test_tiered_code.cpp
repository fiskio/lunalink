#include "lunalink/signal/prn.hpp"
#include "lunalink/signal/tiered_code.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <span>

using namespace lunalink::signal;

TEST_CASE("secondary_code_index cycles through S0-S3") {
  uint8_t idx = 0;
  REQUIRE(secondary_code_index_checked(PrnId{1}, &idx) == TieredCodeStatus::kOk);
  REQUIRE(idx == 0);  // S0
  REQUIRE(secondary_code_index_checked(PrnId{2}, &idx) == TieredCodeStatus::kOk);
  REQUIRE(idx == 1);  // S1
  REQUIRE(secondary_code_index_checked(PrnId{3}, &idx) == TieredCodeStatus::kOk);
  REQUIRE(idx == 2);  // S2
  REQUIRE(secondary_code_index_checked(PrnId{4}, &idx) == TieredCodeStatus::kOk);
  REQUIRE(idx == 3);  // S3
  REQUIRE(secondary_code_index_checked(PrnId{5}, &idx) == TieredCodeStatus::kOk);
  REQUIRE(idx == 0);  // S0 again
  REQUIRE(secondary_code_index_checked(PrnId{12}, &idx) == TieredCodeStatus::kOk);
  REQUIRE(idx == 3); // last in Table 11
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
  REQUIRE(tiered_code_epoch(PrnId{1}, 0, out) == TieredCodeStatus::kOk);
  for (auto v : out)
    REQUIRE(v <= 1u);
}

TEST_CASE("tiered_code_epoch equals primary XOR secondary XOR tertiary") {
  // Manually compute expected result for PRN 1, epoch 0.
  // PRN 1 → S0 = {1, 1, 1, 0}; epoch 0 → secondary chip = S0[0] = 1
  // epoch 0 → tertiary chip index = 0/4 = 0
  PrnCode primary_code;
  PrnCode tertiary_code;
  REQUIRE(weil10230_prn_packed(PrnId{1}, primary_code) == PrnStatus::kOk);
  REQUIRE(weil1500_prn_packed(PrnId{1}, tertiary_code) == PrnStatus::kOk);

  const uint8_t sec_chip = kSecondaryCodes[0][0]; // S0[0] = 1
  uint8_t tert_chip = 0;
  REQUIRE(unpack_chip(tertiary_code, 0, &tert_chip) ==
          PrnStatus::kOk);

  std::array<uint8_t, kWeil10230ChipLength> expected{};
  for (uint16_t i = 0; i < kWeil10230ChipLength; ++i) {
    uint8_t primary_chip = 0;
    REQUIRE(unpack_chip(primary_code, i, &primary_chip) ==
            PrnStatus::kOk);
    expected[i] =
        static_cast<uint8_t>(primary_chip ^ sec_chip ^ tert_chip);
  }

  std::array<uint8_t, kWeil10230ChipLength> out{};
  REQUIRE(tiered_code_epoch(PrnId{1}, 0, out) == TieredCodeStatus::kOk);
  REQUIRE(out == expected);
}

TEST_CASE("tiered_code_epoch differs across secondary code boundary") {
  std::array<uint8_t, kWeil10230ChipLength> e0{};
  std::array<uint8_t, kWeil10230ChipLength> e3{};
  REQUIRE(tiered_code_epoch(PrnId{1}, 0, e0) == TieredCodeStatus::kOk);
  REQUIRE(tiered_code_epoch(PrnId{1}, 3, e3) == TieredCodeStatus::kOk);

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
  std::array<uint8_t, kWeil10230ChipLength> e0{};
  std::array<uint8_t, kWeil10230ChipLength> e4{};
  REQUIRE(tiered_code_epoch(PrnId{1}, 0, e0) == TieredCodeStatus::kOk);
  REQUIRE(tiered_code_epoch(PrnId{1}, 4, e4) == TieredCodeStatus::kOk);

  PrnCode tert;
  REQUIRE(weil1500_prn_packed(PrnId{1}, tert) == PrnStatus::kOk);
  uint8_t chip0 = 0;
  uint8_t chip1 = 0;
  REQUIRE(unpack_chip(tert, 0, &chip0) == PrnStatus::kOk);
  REQUIRE(unpack_chip(tert, 1, &chip1) == PrnStatus::kOk);
  if (chip0 != chip1) {
    REQUIRE(e0 != e4);
  }
}

TEST_CASE("tiered_code_epoch different PRNs produce different sequences") {
  std::array<uint8_t, kWeil10230ChipLength> out1{};
  std::array<uint8_t, kWeil10230ChipLength> out2{};
  REQUIRE(tiered_code_epoch(PrnId{1}, 0, out1) == TieredCodeStatus::kOk);
  REQUIRE(tiered_code_epoch(PrnId{2}, 0, out2) == TieredCodeStatus::kOk);
  REQUIRE(out1 != out2);
}

TEST_CASE("kEpochsPerFrame is 6000") {
  REQUIRE(kEpochsPerFrame == 6000);
}

TEST_CASE("default interim mapping only applies to PRN 1-12") {
  std::array<uint8_t, kWeil10230ChipLength> out{};
  REQUIRE(tiered_code_epoch(PrnId{13}, 0, out) == TieredCodeStatus::kInvalidPrn);
}

TEST_CASE("tiered_code_epoch_checked rejects invalid inputs") {
  std::array<uint8_t, kWeil10230ChipLength> out{};
  TieredCodeAssignment a{};
  REQUIRE(default_tiered_assignment_checked(PrnId{1}, &a) == TieredCodeStatus::kOk);

  REQUIRE(tiered_code_epoch_checked(a, kEpochsPerFrame, out) ==
          TieredCodeStatus::kInvalidEpoch);

  std::array<uint8_t, 2> short_out{};
  REQUIRE(tiered_code_epoch_checked(a, 0, short_out) ==
          TieredCodeStatus::kOutputTooSmall);

  a.secondary_code_idx = 4;
  REQUIRE(tiered_code_epoch_checked(a, 0, out) ==
          TieredCodeStatus::kInvalidAssignment);
}

TEST_CASE("tiered_code_epoch_checked supports tertiary phase offset") {
  TieredCodeAssignment a0{};
  REQUIRE(default_tiered_assignment_checked(PrnId{1}, &a0) == TieredCodeStatus::kOk);
  TieredCodeAssignment a1 = a0;
  a1.tertiary_phase_offset = 1;

  std::array<uint8_t, kWeil10230ChipLength> out0{};
  std::array<uint8_t, kWeil10230ChipLength> out1{};
  REQUIRE(tiered_code_epoch_checked(a0, 0, out0) == TieredCodeStatus::kOk);
  REQUIRE(tiered_code_epoch_checked(a1, 0, out1) == TieredCodeStatus::kOk);

  PrnCode tert;
  REQUIRE(weil1500_prn_packed(PrnId{1}, tert) == PrnStatus::kOk);
  uint8_t chip0 = 0;
  uint8_t chip1 = 0;
  REQUIRE(unpack_chip(tert, 0, &chip0) == PrnStatus::kOk);
  REQUIRE(unpack_chip(tert, 1, &chip1) == PrnStatus::kOk);
  if (chip0 != chip1) {
    REQUIRE(out0 != out1);
  }
}

TEST_CASE("checked tiered helper APIs reject invalid inputs") {
  uint8_t idx = 0;
  REQUIRE(secondary_code_index_checked(PrnId{0}, &idx) == TieredCodeStatus::kInvalidPrn);
  REQUIRE(secondary_code_index_checked(PrnId{1}, nullptr) ==
          TieredCodeStatus::kInvalidAssignment);

  TieredCodeAssignment a{};
  REQUIRE(default_tiered_assignment_checked(PrnId{13}, &a) ==
          TieredCodeStatus::kInvalidPrn);
  REQUIRE(default_tiered_assignment_checked(PrnId{1}, nullptr) ==
          TieredCodeStatus::kInvalidAssignment);
}
