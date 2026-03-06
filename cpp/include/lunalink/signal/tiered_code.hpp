#pragma once
#include <cstdint>

#include "lunalink/signal/prn.hpp"

namespace lunalink::signal {

enum class TieredCodeStatus : uint8_t {
  kOk = 0,
  kInvalidPrn,
  kInvalidEpoch,
  kNullOutput,
  kInvalidAssignment,
};

/// Secondary code definitions per LSIS V1.0 §2.3.5.3.2, Table 10.
inline constexpr uint8_t kSecondaryCodeLength = 4;
inline constexpr uint8_t kSecondaryCodeCount  = 4;
inline constexpr uint8_t kInterimAssignmentMaxPrn = 12;

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
[[nodiscard]] inline constexpr TieredCodeStatus secondary_code_index_checked(
    uint8_t  prn_id,
    uint8_t* out_idx) noexcept {
  if (out_idx == nullptr) {
    return TieredCodeStatus::kNullOutput;
  }
  if (prn_id < 1U) {
    return TieredCodeStatus::kInvalidPrn;
  }
  *out_idx = static_cast<uint8_t>((prn_id - 1U) % kSecondaryCodeCount);
  return TieredCodeStatus::kOk;
}

/// True when PRN is covered by LSIS V1.0 interim Table 11 mapping.
[[nodiscard]] inline constexpr bool is_interim_prn(uint8_t prn_id) noexcept {
  return prn_id >= 1U && prn_id <= kInterimAssignmentMaxPrn;
}

/// Tiered code component assignment for one LNSP node.
struct TieredCodeAssignment {
  uint8_t  primary_prn;
  uint8_t  secondary_code_idx;
  uint8_t  tertiary_prn;
  uint16_t tertiary_phase_offset;
};

/// Build the interim test assignment from LSIS V1.0 Table 11.
[[nodiscard]] inline constexpr TieredCodeStatus default_tiered_assignment_checked(
    uint8_t                prn_id,
    TieredCodeAssignment*  out_assignment) noexcept {
  if (out_assignment == nullptr) {
    return TieredCodeStatus::kNullOutput;
  }
  if (!is_interim_prn(prn_id)) {
    return TieredCodeStatus::kInvalidPrn;
  }
  uint8_t secondary_idx = 0;
  const auto secondary_status = secondary_code_index_checked(
      prn_id,
      &secondary_idx);
  if (secondary_status != TieredCodeStatus::kOk) {
    return secondary_status;
  }
  *out_assignment = TieredCodeAssignment{
      prn_id,
      secondary_idx,
      prn_id,
      0U,
  };
  return TieredCodeStatus::kOk;
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
///   prn_id    – LNSP node identifier (1–12 for default interim mapping)
///   epoch_idx – primary code epoch within the 12 s frame [0, 5999]
///   out       – caller-allocated buffer, length ≥ kWeil10230ChipLength
[[nodiscard]] TieredCodeStatus tiered_code_epoch(
    uint8_t  prn_id,
    uint16_t epoch_idx,
    uint8_t* out) noexcept;

/// Checked variant with explicit assignment and phase offset.
/// Returns explicit status and leaves output unchanged on invalid input.
[[nodiscard]] TieredCodeStatus tiered_code_epoch_checked(
    const TieredCodeAssignment& assignment,
    uint16_t                    epoch_idx,
    uint8_t*                    out) noexcept;

} // namespace lunalink::signal
