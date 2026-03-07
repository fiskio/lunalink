#include "lunalink/signal/bch.hpp"
#include "lunalink/signal/frame.hpp"
#include "lunalink/signal/modulator.hpp"
#include "lunalink/signal/prn.hpp"
#include "lunalink/signal/iq_mux.hpp"
#include "lunalink/signal/matched_code.hpp"
#include "lunalink/signal/ldpc.hpp"
#include "lunalink/signal/ldpc_tables.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cstring>

using namespace lunalink::signal;

TEST_CASE("PRN packing: error path boosters") {
    PrnCode p;
    PrnId bad_prn(1);
    uint8_t over_prn = 250;
    std::memcpy(&bad_prn.storage.v1, &over_prn, 1);
    std::memcpy(&bad_prn.storage.v2, &over_prn, 1);
    std::memcpy(&bad_prn.storage.v3, &over_prn, 1);

    CHECK(gold_prn_packed(bad_prn, p) == PrnStatus::kInvalidPrn);
    CHECK(weil10230_prn_packed(bad_prn, p) == PrnStatus::kInvalidPrn);
    CHECK(weil1500_prn_packed(bad_prn, p) == PrnStatus::kInvalidPrn);
}

TEST_CASE("BCH/Frame: error path coverage booster") {
    std::array<uint8_t, 52> out_bch{};
    std::array<uint8_t, 6000> out_frame{};
    
    // 1. Force Invalid Fid in bch_encode using Fault Injection
    Fid bad_fid(0);
    bad_fid.storage.inject_fault(CheckedRange<uint8_t, 0, 3>(4)); // This still saturates to 3...
    
    // Wait, CheckedRange constructor saturates. 
    // To hit > kBchFidMax (3), we need to bypass CheckedRange too.
    // Let's use a raw uint8_t in Fid if we want to hit the path, or just use CheckedRange<u8, 0, 255>.
    // Actually, I can just use CheckedRange with a large max for the injection.
    
    // Simplest: use a local Fid-like struct that mimics the layout but has a wider range.
    struct EvilRange { uint8_t val; };
    struct EvilFid { TmrValue<EvilRange> storage; };
    
    Fid fid_container(0);
    uint8_t over_fid = 10;
    // Corrupt all three copies to bypass TMR
    std::memcpy(&fid_container.storage.v1, &over_fid, 1);
    std::memcpy(&fid_container.storage.v2, &over_fid, 1);
    std::memcpy(&fid_container.storage.v3, &over_fid, 1);
    
    CHECK(bch_encode(fid_container, Toi(0), out_bch) == BchStatus::kInvalidFid);
    CHECK(frame_build_partial(fid_container, Toi(0), out_frame) == FrameStatus::kInvalidFid);

    // 2. Force Invalid Toi in bch_encode
    Toi toi_container(0);
    uint8_t over_toi = 200;
    std::memcpy(&toi_container.storage.v1, &over_toi, 1);
    std::memcpy(&toi_container.storage.v2, &over_toi, 1);
    std::memcpy(&toi_container.storage.v3, &over_toi, 1);
    
    CHECK(bch_encode(Fid::kNode1(), toi_container, out_bch) == BchStatus::kInvalidToi);
    CHECK(frame_build_partial(Fid::kNode1(), toi_container, out_frame) == FrameStatus::kInvalidToi);
}

TEST_CASE("LDPC: additional error path boosters") {
    std::array<uint8_t, 200> msg{};
    std::array<uint8_t, 2400> out{};
    
    // Invalid input size
    std::span<uint8_t> short_msg(msg.data(), 199);
    CHECK(ldpc_encode(LdpcSubframe::kSF2, short_msg, out) == LdpcStatus::kInvalidInput);

    // Invalid bits
    msg[0] = 2;
    CHECK(ldpc_encode(LdpcSubframe::kSF2, msg, out) == LdpcStatus::kInvalidInput);
    msg[0] = 0;

    // Use a tampered local copy of the matrix to avoid RO memory bus errors
    LdpcCsrMatrix tampered = kLdpc_sf2_a;
    tampered.crc32 ^= 0xFFFFFFFFU;
    
    // Since ldpc_encode uses global matrices, I can't easily swap them.
    // I already covered verify_integrity() failure branch in test_ldpc.cpp.
}

TEST_CASE("Modulator error paths booster") {
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

TEST_CASE("Matched code error paths booster") {
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
