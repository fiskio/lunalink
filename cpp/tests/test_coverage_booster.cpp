#include "lunalink/signal/bch.hpp"
#include "lunalink/signal/frame.hpp"
#include "lunalink/signal/modulator.hpp"
#include "lunalink/signal/prn.hpp"
#include "lunalink/signal/iq_mux.hpp"
#include "lunalink/signal/matched_code.hpp"
#include "lunalink/signal/ldpc.hpp"
#include "lunalink/signal/ldpc_tables.hpp"
#include "lunalink/signal/safety.hpp" // Added include
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cstring>

using namespace lunalink::signal;

namespace {
MatchedCodeAssignment make_assignment(PrnId p) {
    MatchedCodeAssignment a;
    a.primary_prn = p;
    a.tertiary_prn = PrnId(1);
    a.secondary_code_idx = CheckedRange<uint8_t, 0, 3>(0);
    a.tertiary_phase_offset = CheckedRange<uint16_t, 0, 1499>(0);
    return a;
}
}

TEST_CASE("Coverage Booster: TmrValue repair paths") {
    // 1. v2 == v3 (Repair v1)
    TmrValue<uint8_t> t1(10);
    t1.inject_fault(20); // Set all to 20
    t1.v2 = 10;
    t1.v3 = 10;
    // v1=20, v2=10, v3=10 -> vote should return 10 and repair v1
    CHECK(t1.vote() == 10);
    CHECK(t1.v1 == 10);

    // 2. All different (Critical Failure -> return v1)
    TmrValue<uint8_t> t2(10);
    t2.v1 = 10;
    t2.v2 = 20;
    t2.v3 = 30;
    CHECK(t2.vote() == 10);
}

TEST_CASE("Coverage Booster: PRN invalid paths") {
    PrnCode p;
    PrnId bad_prn(1);
    uint8_t over_prn = 250;
    
    // Force invalid value via memcpy into all TMR copies
    std::memcpy(&bad_prn.storage.v1, &over_prn, 1);
    std::memcpy(&bad_prn.storage.v2, &over_prn, 1);
    std::memcpy(&bad_prn.storage.v3, &over_prn, 1);

    CHECK(static_cast<uint8_t>(gold_prn_packed(bad_prn, p)) == static_cast<uint8_t>(PrnStatus::kInvalidPrn));
    CHECK(static_cast<uint8_t>(weil10230_prn_packed(bad_prn, p)) == static_cast<uint8_t>(PrnStatus::kInvalidPrn));
    CHECK(static_cast<uint8_t>(weil1500_prn_packed(bad_prn, p)) == static_cast<uint8_t>(PrnStatus::kInvalidPrn));
}

TEST_CASE("Coverage Booster: Matched Code error paths") {
    std::array<uint8_t, kWeil10230ChipLength> out_chips{};
    
    // 1. Output too small
    std::span<uint8_t> small_out(out_chips.data(), 100);
    CHECK(static_cast<uint8_t>(matched_code_epoch_checked(make_assignment(PrnId(1)), 0, small_out)) == static_cast<uint8_t>(MatchedCodeStatus::kOutputTooSmall));

    // 2. Invalid Epoch
    CHECK(static_cast<uint8_t>(matched_code_epoch_checked(make_assignment(PrnId(1)), kEpochsPerFrame, out_chips)) == static_cast<uint8_t>(MatchedCodeStatus::kInvalidEpoch));

    // 3. Invalid PRN
    PrnId bad_prn(1);
    uint8_t over_prn = 250;
    // Set all copies to invalid so repair() returns an invalid value
    bad_prn.storage.v1 = CheckedRange<uint8_t, 1, 210>(1); // Initialize
    std::memcpy(&bad_prn.storage.v1, &over_prn, 1);
    std::memcpy(&bad_prn.storage.v2, &over_prn, 1);
    std::memcpy(&bad_prn.storage.v3, &over_prn, 1);
    
    auto assignment = make_assignment(bad_prn);
    // Expect kInvalidAssignment (0x0F) because matched_code_epoch_checked collapses kInvalidPrn
    CHECK(static_cast<uint8_t>(matched_code_epoch_checked(assignment, 0, out_chips)) == static_cast<uint8_t>(MatchedCodeStatus::kInvalidAssignment));
}

TEST_CASE("Coverage Booster: Modulator error paths") {
    std::array<uint8_t, 10> chips = {0};
    std::array<int8_t, 10> out = {0};
    
    // Invalid Chip Value
    chips[0] = 2;
    CHECK(static_cast<uint8_t>(modulate_bpsk_any(chips, 1, out)) == static_cast<uint8_t>(ModulationStatus::kInvalidChipValue));
}

TEST_CASE("Coverage Booster: LDPC error paths") {
    std::array<uint8_t, 200> msg = {0};
    std::array<uint8_t, 2400> out = {0};
    
    // Invalid Input Size
    std::span<uint8_t> short_msg(msg.data(), 199);
    CHECK(static_cast<uint8_t>(ldpc_encode(LdpcSubframe::kSF2, short_msg, out)) == static_cast<uint8_t>(LdpcStatus::kInvalidInput));

    // Invalid Bits
    msg[0] = 2;
    CHECK(static_cast<uint8_t>(ldpc_encode(LdpcSubframe::kSF2, msg, out)) == static_cast<uint8_t>(LdpcStatus::kInvalidInput));
    msg[0] = 0;

    // Output too small
    std::span<uint8_t> small_out(out.data(), 100);
    CHECK(static_cast<uint8_t>(ldpc_encode(LdpcSubframe::kSF2, msg, small_out)) == static_cast<uint8_t>(LdpcStatus::kOutputTooSmall));
}

TEST_CASE("Coverage Booster: TmrValue fault injection") {
    TmrValue<uint8_t> v(10);
    v.inject_fault(20);
    CHECK(v.peek() == 20);
    CHECK(v.vote() == 20);
}

TEST_CASE("Coverage Booster: bch_codebook_checksum stable") {
    uint64_t c1 = bch_codebook_checksum();
    uint64_t c2 = bch_codebook_checksum();
    CHECK(c1 == c2);
    CHECK(c1 != 0);
}

TEST_CASE("Status codes: exhaustive coverage") {
    CHECK(static_cast<uint8_t>(BchStatus::kAmbiguousMatch) == 0xF0U);
    CHECK(static_cast<uint8_t>(BchStatus::kNullOutput) == 0x99U);
    CHECK(static_cast<uint8_t>(BchStatus::kFaultDetected) == 0x0FU);
    CHECK(static_cast<uint8_t>(ModulationStatus::kFaultDetected) == 0x99U);
    CHECK(static_cast<uint8_t>(MatchedCodeStatus::kFaultDetected) == 0x99U);
    CHECK(static_cast<uint8_t>(LdpcStatus::kFaultDetected) == 0x99U);
}
