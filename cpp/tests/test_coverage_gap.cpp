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
  // FID 4 saturates to 3
  CHECK(bch_encode(static_cast<Fid>(static_cast<uint8_t>(4)), Toi(0), out) == BchStatus::kOk);
  // Invalid TOI (still raw uint8_t in Toi struct)
  CHECK(bch_encode(Fid::kNode1(), Toi(100), out) == BchStatus::kInvalidToi);
  
  // Ergonomic overload test
  uint8_t arr[52] = {0};
  CHECK(bch_encode(Fid::kNode1(), Toi(0), arr) == BchStatus::kOk);
}

TEST_CASE("Frame build error paths") {
  std::array<uint8_t, 6000> frame{};
  // FID 4 saturates to 3
  CHECK(frame_build_partial(static_cast<Fid>(static_cast<uint8_t>(4)), Toi(0), frame) == FrameStatus::kOk);
  CHECK(frame_build_partial(Fid::kNode1(), Toi(100), frame) == FrameStatus::kInvalidToi);
  
  // Ergonomic overload test
  uint8_t arr_frame[6000] = {0};
  CHECK(frame_build_partial(Fid::kNode1(), Toi(0), arr_frame) == FrameStatus::kOk);
}

TEST_CASE("PRN packing: saturating logic") {
  PrnCode p;
  // PRN 0 saturates to 1
  CHECK(gold_prn_packed(PrnId{0}, p) == PrnStatus::kOk);
  CHECK(weil10230_prn_packed(PrnId{0}, p) == PrnStatus::kOk);
  CHECK(weil1500_prn_packed(PrnId{0}, p) == PrnStatus::kOk);
  
  // PRN 211 saturates to 210
  CHECK(gold_prn_packed(PrnId{211}, p) == PrnStatus::kOk);
  CHECK(weil10230_prn_packed(PrnId{211}, p) == PrnStatus::kOk);
  CHECK(weil1500_prn_packed(PrnId{211}, p) == PrnStatus::kOk);
}

TEST_CASE("Unpack chip error paths") {
  PrnCode p;
  REQUIRE(gold_prn_packed(PrnId{1}, p) == PrnStatus::kOk);
  uint8_t chip = 0;
  
  // Invalid chip index
  CHECK(unpack_chip(p, kGoldChipLength, chip) == PrnStatus::kInvalidChipIndex);
  
  // Logical index within chip_length but physically outside span
  p.data = p.data.subspan(0, 10); // Shrink span
  CHECK(unpack_chip(p, 100, chip) == PrnStatus::kInvalidChipIndex);
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

  // Output too small
  chips[0] = 0;
  std::span<int8_t> small_out(out.data(), 5);
  CHECK(modulate_bpsk_any(chips, 1, small_out) == ModulationStatus::kOutputTooSmall);
}

TEST_CASE("IQ Mux error paths") {
  std::array<int8_t, 2046> i_ok{};
  std::array<int8_t, 10230> q_ok{};
  std::array<int16_t, 20460> out_ok{};
  i_ok.fill(1);
  q_ok.fill(1);

  // Input Q too small
  std::span<int8_t> q_small(q_ok.data(), 10229);
  CHECK(multiplex_iq(i_ok, q_small, out_ok) == IqMuxStatus::kInputTooSmall);

  // Input I too small
  std::span<int8_t> i_small(i_ok.data(), 2045);
  CHECK(multiplex_iq(i_small, q_ok, out_ok) == IqMuxStatus::kInputTooSmall);

  // Output too small
  std::span<int16_t> out_small(out_ok.data(), 10);
  CHECK(multiplex_iq(i_ok, q_ok, out_small) == IqMuxStatus::kOutputTooSmall);
  
  // Invalid samples
  i_ok[0] = 0;
  CHECK(multiplex_iq(i_ok, q_ok, out_ok) == IqMuxStatus::kInvalidISample);
  i_ok[0] = 1;
  q_ok[0] = 2;
  CHECK(multiplex_iq(i_ok, q_ok, out_ok) == IqMuxStatus::kInvalidQSample);
}

TEST_CASE("Matched code error paths") {
  MatchedCodeAssignment a;
  a.primary_prn = PrnId{1};
  a.tertiary_prn = PrnId{1};
  a.secondary_code_idx = CheckedRange<uint8_t, 0, 3>{0};
  a.tertiary_phase_offset = CheckedRange<uint16_t, 0, 1499>{0};

  std::array<uint8_t, kWeil10230ChipLength> out = {};
  
  // Output too small
  std::span<uint8_t> out_small(out.data(), 10);
  CHECK(matched_code_epoch_checked(a, 0, out_small) == MatchedCodeStatus::kOutputTooSmall);
  
  // Invalid epoch
  CHECK(matched_code_epoch_checked(a, kEpochsPerFrame, out) == MatchedCodeStatus::kInvalidEpoch);
}
