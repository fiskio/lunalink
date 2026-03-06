#pragma once
#include <cassert>
#include <cstdint>

#include "lunalink/signal/prn.hpp"

namespace lunalink::signal {

/// Secondary code definitions per LSIS V1.0 §2.3.5.3.2, Table 10.
inline constexpr uint8_t kSecondaryCodeLength = 4;
inline constexpr uint8_t kSecondaryCodeCount  = 4;

// NOLINTNEXTLINE(hicpp-avoid-c-arrays)
inline constexpr uint8_t kSecondaryCodes[4][4] = {
    {1, 1, 1, 0},  // S0
    {0, 1, 1, 1},  // S1
    {1, 0, 1, 1},  // S2
    {1, 1, 0, 1},  // S3
};

/// Total primary epochs per 12 s frame: Ns × NT = 4 × 1500 = 6000.
inline constexpr uint16_t kEpochsPerFrame =
    static_cast<uint16_t>(static_cast<unsigned>(kSecondaryCodeLength) *
                          kWeil1500ChipLength);

/// Return the secondary code index for a given PRN ID (1-indexed).
/// Per Table 11 interim assignments: PRN i → S_{(i-1) mod 4}.
[[nodiscard]] inline constexpr uint8_t secondary_code_index(
    uint8_t prn_id) noexcept {
  assert(prn_id >= 1);
  return static_cast<uint8_t>((prn_id - 1U) % kSecondaryCodeCount);
}

/// Tiered code component assignment for one LNSP node.
struct TieredCodeAssignment {
  uint8_t  primary_prn;
  uint8_t  secondary_code_idx;
  uint8_t  tertiary_prn;
  uint16_t tertiary_phase_offset;
};

/// Build the interim test assignment from LSIS V1.0 Table 11.
[[nodiscard]] inline constexpr TieredCodeAssignment
default_tiered_assignment(uint8_t prn_id) noexcept {
  return TieredCodeAssignment{
      prn_id,
      secondary_code_index(prn_id),
      prn_id,
      0U,
  };
}

/// Validate an assignment.
[[nodiscard]] inline constexpr bool valid_tiered_assignment(
    const TieredCodeAssignment& a) noexcept {
  return a.primary_prn >= 1U && a.primary_prn <= kPrnCount &&
         a.secondary_code_idx < kSecondaryCodeCount &&
         a.tertiary_prn >= 1U && a.tertiary_prn <= kPrnCount &&
         a.tertiary_phase_offset < kWeil1500ChipLength;
}

/// Generate one primary epoch (10 230 chips) of the tiered AFS-Q code.
///
/// For the given PRN and epoch index, produces:
///   out[i] = primary[i] XOR secondary[epoch % 4] XOR tertiary[epoch / 4]
///
/// per LSIS V1.0 §2.3.5.2 (C_Tiered = Cp ⊕ Cs ⊕ CT).
///
/// Note: tertiary PRN phase offset is assumed to be 0 for all PRNs,
/// matching the interim test assignments in Table 11 (LSIS-TBD-2001).
///
/// Parameters:
///   prn_id    – LNSP node identifier (1–210)
///   epoch_idx – primary code epoch within the 12 s frame [0, 5999]
///   out       – caller-allocated buffer, length ≥ kWeil10230ChipLength
void tiered_code_epoch(
    uint8_t  prn_id,
    uint16_t epoch_idx,
    uint8_t* out) noexcept;

/// Checked variant with explicit assignment and phase offset.
/// Returns false if inputs are invalid and leaves output unchanged.
[[nodiscard]] bool tiered_code_epoch_checked(
    const TieredCodeAssignment& assignment,
    uint16_t                    epoch_idx,
    uint8_t*                    out) noexcept;

} // namespace lunalink::signal
