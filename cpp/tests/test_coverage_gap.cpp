#include "lunalink/signal/bch.hpp"
#include "lunalink/signal/frame.hpp"
#include "lunalink/signal/modulator.hpp"
#include "lunalink/signal/prn.hpp"
#include "lunalink/signal/iq_mux.hpp"
#include "lunalink/signal/matched_code.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>

using namespace lunalink::signal;

TEST_CASE("BCH encode/decode error paths") {
  std::array<uint8_t, 52> out{};
  // Invalid FID
  CHECK(bch_encode(static_cast<Fid>(4), Toi(0), out) == BchStatus::kInvalidFid);
  // Invalid TOI
  CHECK(bch_encode(Fid::kNode1, Toi(100), out) == BchStatus::kInvalidToi);
  
  // Ergonomic overload test
  uint8_t arr[52] = {0};
  CHECK(bch_encode(Fid::kNode1, Toi(0), arr) == BchStatus::kOk);
}

TEST_CASE("Frame build error paths") {
  std::array<uint8_t, 6000> frame{};
  // Trigger failure via invalid BCH params
  CHECK(frame_build_partial(static_cast<Fid>(4), Toi(0), frame) == FrameStatus::kInvalidFid);
  CHECK(frame_build_partial(Fid::kNode1, Toi(100), frame) == FrameStatus::kInvalidToi);
  
  // Ergonomic overload test
  uint8_t arr[6000] = {0};
  CHECK(frame_build_partial(Fid::kNode1, Toi(0), arr) == FrameStatus::kOk);
}

TEST_CASE("PRN packing error paths") {
  PrnCode p;
  // Invalid PRN 0
  CHECK(gold_prn_packed(PrnId{0}, p) == PrnStatus::kInvalidPrn);
  CHECK(weil10230_prn_packed(PrnId{0}, p) == PrnStatus::kInvalidPrn);
  CHECK(weil1500_prn_packed(PrnId{0}, p) == PrnStatus::kInvalidPrn);
  
  // Invalid PRN 211
  CHECK(gold_prn_packed(PrnId{211}, p) == PrnStatus::kInvalidPrn);
  CHECK(weil10230_prn_packed(PrnId{211}, p) == PrnStatus::kInvalidPrn);
  CHECK(weil1500_prn_packed(PrnId{211}, p) == PrnStatus::kInvalidPrn);
}

TEST_CASE("Unpack chip error paths") {
  PrnCode p;
  REQUIRE(gold_prn_packed(PrnId{1}, p) == PrnStatus::kOk);
  uint8_t chip = 0;
  
  // Invalid chip index
  CHECK(unpack_chip(p, kGoldChipLength, &chip) == PrnStatus::kInvalidChipIndex);
  
  // Logical index within chip_length but physically outside span
  p.data = p.data.subspan(0, 10); // Shrink span
  CHECK(unpack_chip(p, 100, &chip) == PrnStatus::kInvalidChipIndex);
}

TEST_CASE("Modulator error paths") {
  std::array<uint8_t, 10> chips{};
  std::array<int8_t, 10> out{};
  chips.fill(0);
  
  // Invalid data symbol
  CHECK(modulate_bpsk_any(chips, 0, out) == ModulationStatus::kInvalidSymbol);
  
  // Invalid chip value
  chips[0] = 2;
  CHECK(modulate_bpsk_any(chips, 1, out) == ModulationStatus::kInvalidChipValue);

  // Empty output
  chips[0] = 0;
  std::span<int8_t> empty_out{};
  CHECK(modulate_bpsk_any(chips, 1, empty_out) == ModulationStatus::kOutputTooSmall);
}

TEST_CASE("IQ Mux error paths") {
  std::array<int8_t, 2046> i_ok{};
  std::array<int8_t, 10230> q_ok{};
  std::array<int16_t, 20460> out_ok{};
  i_ok.fill(1);
  q_ok.fill(1);

  // Input Q too small
  std::array<int8_t, 10229> q_small{};
  CHECK(multiplex_iq(i_ok, q_small, out_ok) == IqMuxStatus::kInputTooSmall);

  // Input I too small
  std::array<int8_t, 2045> i_small{};
  CHECK(multiplex_iq(i_small, q_ok, out_ok) == IqMuxStatus::kInputTooSmall);

  // Empty output
  std::span<int16_t> empty_out{};
  CHECK(multiplex_iq(i_ok, q_ok, empty_out) == IqMuxStatus::kOutputTooSmall);
}

TEST_CASE("Matched code error paths") {
  std::array<uint8_t, kWeil10230ChipLength> out{};
  
  // Invalid PRN ID in default mapping
  CHECK(matched_code_epoch(PrnId{0}, 0, out) == MatchedCodeStatus::kInvalidPrn);
  
  // Empty output in default mapping
  std::span<uint8_t> empty_out{};
  CHECK(matched_code_epoch(PrnId{1}, 0, empty_out) == MatchedCodeStatus::kOutputTooSmall);

  // Invalid assignment in checked API
  MatchedCodeAssignment a{};
  a.primary_prn = PrnId{0};
  CHECK(matched_code_epoch_checked(a, 0, out) == MatchedCodeStatus::kInvalidAssignment);
  
  // Invalid primary PRN ID (internal check in matched_code_epoch_checked)
  a.primary_prn = PrnId{211};
  CHECK(matched_code_epoch_checked(a, 0, out) == MatchedCodeStatus::kInvalidAssignment);

  // Invalid tertiary PRN ID
  a.primary_prn = PrnId{1};
  a.tertiary_prn = PrnId{0};
  CHECK(matched_code_epoch_checked(a, 0, out) == MatchedCodeStatus::kInvalidAssignment);

  // Invalid epoch
  a.tertiary_prn = PrnId{1};
  CHECK(matched_code_epoch_checked(a, kEpochsPerFrame, out) == MatchedCodeStatus::kInvalidEpoch);
  
  // Ergonomic overload test
  uint8_t arr[kWeil10230ChipLength] = {0};
  CHECK(matched_code_epoch(PrnId{1}, 0, arr) == MatchedCodeStatus::kOk);
  CHECK(matched_code_epoch_checked(a, 0, arr) == MatchedCodeStatus::kOk);
}
